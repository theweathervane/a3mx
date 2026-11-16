#!/usr/bin/env bash
# a3mx test runner (the container's entry point; see README.md).
#
#   1. clone the code (or fetch and check out the newest A3MX_GIT_REF if it is already there)
#   2. extract the disc from /iso into work/ and assets/us (only the first time, or for another disc)
#   3. build the headless preset and run ctest (and/or the disc-free ci preset)
#   4. write the log, the JUnit XML and a summary to /tests/results/<date-time>/ (and results/latest)
#
# Mounts: /iso (your ISO, read-only) and /tests (the clone and build in /tests/a3mx, results in
# /tests/results).
#
# Settings (environment variables, all optional):
#   A3MX_GIT_URL     where to clone from     (default: the project's GitLab; any mirror works)
#   A3MX_GIT_REF     branch, tag or commit   (default: main)
#   A3MX_GIT_TOKEN   an access token, if the repository is private (GitLab or Gitea)
#   A3MX_GIT_USER    the user name sent with it (default: oauth2, which GitLab accepts; your user name on Gitea)
#   A3MX_ISO         one ISO's path inside the container (default: every *.iso in /iso is offered to
#                    extract_assets.py, which picks ULUS-10062 by its program's checksum)
#   A3MX_SUITE       disc (default: every test of the headless build) | ci (disc-free only) | both
#   A3MX_TESTS       ctest -R regex: only these tests, e.g. "boot_to_select|arcade_fight"
#   A3MX_EXCLUDE     ctest -E regex: leave these out
#   A3MX_JOBS        parallel build and test jobs (default: every core)
#   A3MX_REEXTRACT=1 extract the disc again (after changing the analysis tools)
#   A3MX_CLEAN=1     delete the build folders first
set -uo pipefail

GIT_URL=${A3MX_GIT_URL:-https://gitlab.com/theweathervane-group/a3mx.git}
GIT_REF=${A3MX_GIT_REF:-main}
GIT_TOKEN=${A3MX_GIT_TOKEN:-}
GIT_USER=${A3MX_GIT_USER:-oauth2}
TESTS_ROOT=${A3MX_TESTS_ROOT:-/tests}
SRC=${A3MX_SRC:-$TESTS_ROOT/a3mx}
ISO_DIR=${A3MX_ISO_DIR:-/iso}
ISO=${A3MX_ISO:-}
RESULTS=${A3MX_RESULTS:-$TESTS_ROOT/results}
SUITE=${A3MX_SUITE:-disc}
TESTS=${A3MX_TESTS:-}
EXCLUDE=${A3MX_EXCLUDE:-}
JOBS=${A3MX_JOBS:-}
[ -n "$JOBS" ] || JOBS=$(nproc)
REEXTRACT=${A3MX_REEXTRACT:-0}
CLEAN=${A3MX_CLEAN:-0}

run_id=$(date -u +%Y%m%d-%H%M%S)
out="$RESULTS/$run_id"
mkdir -p "$out"
ln -sfn "$run_id" "$RESULTS/latest"
exec > >(tee -a "$out/run.log") 2>&1
tee_pid=$!

say()  { stage=$*; printf '\n=== %s  %s\n' "$(date -u +%H:%M:%S)" "$*"; }
die()  { printf '\nERROR: %s\n' "$*"; echo "ERROR: $*" > "$out/summary.txt"; exit 2; }

# --- stopping cleanly ----------------------------------------------------------------------------
# Exit codes: 0 every test passed, 1 a test failed, 2 a setup step failed, 143 / 130 stopped
# (docker stop / Ctrl-C). The long steps run in their own process group (step), so a stop ends the
# whole step at once (ctest and the game it is running included) instead of Docker killing
# everything after its grace period; the summary then says the run was stopped, not that it failed.
stage=starting
child=
finish() {
    # Let tee write the last lines to run.log and the container's log before the container stops.
    exec 1>&- 2>&-
    wait "$tee_pid" 2>/dev/null
}
trap finish EXIT
stop() {
    trap - TERM INT
    [ -n "$child" ] && kill -TERM -- "-$child" 2>/dev/null
    printf '\n=== %s  Stopped by SIG%s during: %s\n' "$(date -u +%H:%M:%S)" "$1" "$stage"
    [ -n "$child" ] && wait "$child" 2>/dev/null
    {
        echo "a3mx ${version:-?} at ${commit:-?}"
        echo "suite $SUITE, run $run_id, stopped $(date -u '+%Y-%m-%d %H:%M:%S') UTC during: $stage"
        echo "RESULT: STOPPED (SIG$1) before the end: not a test result"
    } > "$out/summary.txt"
    cat "$out/summary.txt"
    [ "$1" = INT ] && exit 130
    exit 143
}
trap 'stop TERM' TERM
trap 'stop INT' INT
# Run a long step in its own process group and wait for it: unlike a foreground command, a wait
# lets the trap above run as soon as the signal comes.
step() {
    setsid "$@" &
    child=$!
    wait "$child"
    local rc=$?
    child=
    return $rc
}

say "a3mx tests, run $run_id: $GIT_URL ($GIT_REF), suite $SUITE, $JOBS jobs"

# --- 1. the code -------------------------------------------------------------------------------
# The token goes in a header for each git command only, never into .git/config (or the log).
git_auth=()
if [ -n "$GIT_TOKEN" ]; then
    basic=$(printf '%s:%s' "$GIT_USER" "$GIT_TOKEN" | base64 -w0)
    git_auth=(-c "http.extraHeader=Authorization: Basic $basic")
fi

if [ ! -d "$SRC/.git" ]; then
    say "Cloning"
    mkdir -p "$(dirname "$SRC")"
    git "${git_auth[@]}" clone "$GIT_URL" "$SRC" || die "could not clone $GIT_URL"
fi
cd "$SRC" || die "no $SRC"
git remote set-url origin "$GIT_URL"
say "Fetching"
git "${git_auth[@]}" fetch --prune --tags origin || die "could not fetch from $GIT_URL"
if git rev-parse -q --verify "origin/$GIT_REF^{commit}" >/dev/null; then
    git checkout -q -f --detach "origin/$GIT_REF"
else
    git checkout -q -f --detach "$GIT_REF" || die "no branch, tag or commit $GIT_REF"
fi
# Untracked files go; git-ignored ones (work/, assets/, build-*/, .tools/) stay, so nothing is redone.
git clean -fdq
if command -v git-lfs >/dev/null && grep -qs "filter=lfs" .gitattributes; then
    git lfs install --local >/dev/null && git "${git_auth[@]}" lfs pull || echo "WARN: git lfs pull failed"
fi
commit=$(git log -1 --format='%h %s')
version=$(tr -d ' \r\n' < VERSION 2>/dev/null || echo '?')
say "At $commit (VERSION $version)"
git log -1 --format='%H%n%an <%ae>%n%ad%n%n%B' > "$out/commit.txt"

# Python packages, if the project ever lists any.
for req in requirements.txt tools/requirements.txt; do
    [ -f "$req" ] && { say "pip install -r $req"; pip3 install -q --break-system-packages -r "$req" || die "pip install failed"; }
done

if [ "$CLEAN" = 1 ]; then
    say "Deleting the build folders (A3MX_CLEAN=1)"
    rm -rf build-headless build-ci
fi

status=0

# --- 2 and 3. the disc and the headless build ---------------------------------------------------
if [ "$SUITE" = disc ] || [ "$SUITE" = both ]; then
    # Every ISO in the folder is linked into the clone's imports/psp/ (git-ignored) and extract_assets.py
    # picks the disc by its program's checksum, as on a player's PC: other discs there (EU, JP, PS2...)
    # are passed over, and if none is the right one it lists what it found. A3MX_ISO links only that one.
    mkdir -p imports/psp
    find imports/psp -maxdepth 1 -type l -delete
    if [ -n "$ISO" ]; then
        [ -f "$ISO" ] || die "A3MX_ISO: no file $ISO"
        ln -sfn "$ISO" "imports/psp/$(basename "$ISO")"
    else
        while IFS= read -r -d '' f; do
            ln -sfn "$f" "imports/psp/$(basename "$f")"
        done < <(find "$ISO_DIR" -maxdepth 2 -type f -iname '*.iso' -print0 2>/dev/null)
    fi
    n_isos=$(find imports/psp -maxdepth 1 -type l | wc -l)
    [ "$n_isos" -gt 0 ] || die "no ISO found: mount the folder with your ULUS-10062 disc image at $ISO_DIR (or set A3MX_ISO)"
    echo "ISOs to choose from ($n_isos):"; find imports/psp -maxdepth 1 -type l -printf '  %f\n' | sort

    # The extraction is kept and redone only when the disc it came from has changed or gone (adding
    # other ISOs to the folder doesn't), when A3MX_ISO names another one, or with A3MX_REEXTRACT=1.
    stamp=work/.docker-extracted          # line 1: the ISO used; line 2: its size and date
    prev_iso=; prev_id=
    [ -f "$stamp" ] && { prev_iso=$(sed -n 1p "$stamp"); prev_id=$(sed -n 2p "$stamp"); }
    extract=1
    if [ "$REEXTRACT" != 1 ] && [ -n "$prev_iso" ] && [ -f "$prev_iso" ] \
       && [ "$(stat -L -c '%s %Y' "$prev_iso")" = "$prev_id" ] \
       && { [ -z "$ISO" ] || [ "$(readlink -f "$ISO")" = "$prev_iso" ]; }; then
        extract=0
    fi
    if [ $extract = 1 ]; then
        say "Extracting the disc (extract_assets.py picks it by checksum)"
        step bash -c 'python3 tools/extract_assets.py 2>&1 | tee "$1"; exit "${PIPESTATUS[0]}"' extract "$out/extract.log" \
            || die "extract_assets.py failed (see extract.log): is your ULUS-10062 disc in $ISO_DIR?"
        used=$(sed -n 's/^disc //p' "$out/extract.log" | head -n 1)
        used=$(readlink -f "$used")
        mkdir -p work && printf '%s\n%s\n' "$used" "$(stat -L -c '%s %Y' "$used")" > "$stamp"
        echo "Extracted from $used"
    else
        say "Disc already extracted from $prev_iso"
    fi

    say "Configuring (preset headless)"
    step cmake --preset headless || die "cmake --preset headless failed"
    say "Building"
    step cmake --build --preset headless -j "$JOBS" || die "the build failed"

    say "Testing (build-headless)"
    ctest_args=(--test-dir build-headless -j "$JOBS" --output-on-failure --output-junit "$out/junit-headless.xml")
    [ -n "$TESTS" ]   && ctest_args+=(-R "$TESTS")
    [ -n "$EXCLUDE" ] && ctest_args+=(-E "$EXCLUDE")
    step ctest "${ctest_args[@]}"
    rc=$?
    [ $rc -ne 0 ] && status=1
    cp -f build-headless/Testing/Temporary/LastTest.log "$out/LastTest-headless.log" 2>/dev/null
    cp -f build-headless/Testing/Temporary/LastTestsFailed.log "$out/failed-headless.txt" 2>/dev/null
fi

# --- the disc-free ci preset (what GitLab CI runs) ----------------------------------------------
if [ "$SUITE" = ci ] || [ "$SUITE" = both ]; then
    say "Configuring, building and testing (preset ci, no disc)"
    if step cmake --preset ci && step cmake --build --preset ci -j "$JOBS"; then
        step ctest --preset ci -j "$JOBS" --output-junit "$out/junit-ci.xml" || status=1
        cp -f build-ci/Testing/Temporary/LastTest.log "$out/LastTest-ci.log" 2>/dev/null
    else
        echo "ERROR: the ci preset did not build"; status=1
    fi
fi

# --- 4. summary ----------------------------------------------------------------------------------
{
    echo "a3mx $version at $commit"
    echo "suite $SUITE, run $run_id, finished $(date -u '+%Y-%m-%d %H:%M:%S') UTC"
    if [ $status -eq 0 ]; then echo "RESULT: all tests passed"; else echo "RESULT: FAILED"; fi
    for f in "$out"/failed-*.txt; do [ -f "$f" ] && { echo "failed tests:"; cat "$f"; }; done
} > "$out/summary.txt"
say "Done"
cat "$out/summary.txt"
exit $status

#!/bin/sh
# Build the test image and run every test, from a shell on the Docker host.
#   ./run.sh ISO_FOLDER [RESULTS_FOLDER] [-e A3MX_SETTING=VALUE ...]
# e.g. ./run.sh /srv/a3mx/iso
#      ./run.sh /srv/a3mx/iso /srv/a3mx/results -e A3MX_TESTS="boot_to_select|arcade_fight"
# Settings in a .env file beside this script (git-ignored) are passed too, e.g. A3MX_GIT_URL=<your mirror>.
set -e
[ -n "$1" ] || { echo "usage: $0 ISO_FOLDER [RESULTS_FOLDER] [-e A3MX_SETTING=VALUE ...]"; exit 2; }
iso_dir=$(cd "$1" && pwd); shift
results_dir=${1:-$PWD/results}
case "$results_dir" in -*) results_dir=$PWD/results ;; *) [ $# -gt 0 ] && shift ;; esac
mkdir -p "$results_dir"
results_dir=$(cd "$results_dir" && pwd)

cd "$(dirname "$0")"
docker build -t a3mx-tests:latest .
env_file=
if [ -f .env ]; then env_file="--env-file .env"; fi
# shellcheck disable=SC2086
exec docker run --rm --name a3mx-tests $env_file \
    -v "$iso_dir":/iso:ro \
    -v "$results_dir":/results \
    -v a3mx-src:/a3mx \
    "$@" a3mx-tests:latest

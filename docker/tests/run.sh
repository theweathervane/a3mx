#!/bin/sh
# Build the test image and run every test, from a shell on the Docker host.
#   ./run.sh ISO_FOLDER TESTS_FOLDER [-e A3MX_SETTING=VALUE ...]
# ISO_FOLDER holds your ISO (mounted read-only at /iso); TESTS_FOLDER keeps the clone and the build
# (a3mx/) and each run's results (results/), mounted at /tests.
# e.g. ./run.sh /mnt/Docker/A3MX/ISO /mnt/Docker/A3MX/Tests
#      ./run.sh /mnt/Docker/A3MX/ISO /mnt/Docker/A3MX/Tests -e A3MX_TESTS="boot_to_select|arcade_fight"
# Settings in a .env file beside this script (git-ignored) are passed too, e.g. A3MX_GIT_URL=<your mirror>.
set -e
[ $# -ge 2 ] || { echo "usage: $0 ISO_FOLDER TESTS_FOLDER [-e A3MX_SETTING=VALUE ...]"; exit 2; }
iso_dir=$(cd "$1" && pwd)
mkdir -p "$2"
tests_dir=$(cd "$2" && pwd)
shift 2

cd "$(dirname "$0")"
docker build -t a3mx-tests:latest .
env_file=
if [ -f .env ]; then env_file="--env-file .env"; fi
# shellcheck disable=SC2086
exec docker run --rm --init --stop-timeout 30 --name a3mx-tests $env_file \
    -v "$iso_dir":/iso:ro \
    -v "$tests_dir":/tests \
    "$@" a3mx-tests:latest

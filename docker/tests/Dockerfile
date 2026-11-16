# a3mx test runner: a Linux machine that pulls the code from a git server (the project's GitLab
# by default; set A3MX_GIT_URL for a mirror), extracts your disc once, builds the headless preset and runs every ctest test.
#
# No code and no game data are baked into the image: the code is cloned when the container starts
# (into the /a3mx volume, kept between runs so rebuilds are incremental), and the ISO is mounted
# read-only at /iso. Results go to /results/<date-time>/. See README.md beside this file.

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# The same packages as .gitlab-ci.yml's build:linux-x64 and docs/BUILDING.md, plus git-lfs and pip.
# (The headless preset needs none of SDL's libraries, but tools/build.py checks for them and the
# ci preset's launcher uses them, so they are here too.)
RUN apt-get update -q \
 && apt-get install -y -q --no-install-recommends \
        ca-certificates git git-lfs curl tzdata \
        python3 python3-pip python3-venv \
        cmake ninja-build clang pkg-config \
        libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev libxkbcommon-dev \
        libwayland-dev wayland-protocols libegl-dev libgl-dev \
        libasound2-dev libpulse-dev libudev-dev libxtst-dev libxfixes-dev \
 && rm -rf /var/lib/apt/lists/* \
 && git config --system --add safe.directory '*'

COPY run-tests.sh /usr/local/bin/run-tests
# A Windows checkout may have given the script CRLF line endings: strip them.
RUN sed -i 's/\r$//' /usr/local/bin/run-tests && chmod +x /usr/local/bin/run-tests

ENV A3MX_GIT_URL=https://gitlab.com/theweathervane-group/a3mx.git \
    A3MX_GIT_REF=main

VOLUME ["/a3mx", "/results"]
WORKDIR /a3mx
ENTRYPOINT ["run-tests"]

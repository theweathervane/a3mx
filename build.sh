#!/bin/sh
# Linux and macOS: build the game from your disc (put the ISO in imports/psp/ first).
# Options: ./build.sh --help   (details: docs/BUILDING.md)
cd "$(dirname "$0")" || exit 1
if ! command -v python3 >/dev/null 2>&1; then
    echo "Python 3.9 or later is needed: sudo apt install python3 (Linux), or xcode-select --install (macOS: Apple's command line tools include it)"
    exit 1
fi
exec python3 tools/build.py "$@"

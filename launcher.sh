#!/bin/sh
# Linux and macOS: build the launcher if needed (no disc needed for that) and open it.
# ./launcher.sh --clean: start from nothing first (keeps your saves; docs/BUILDING.md).
# The launcher picks your ISO and the options, then Runs or Builds the game (docs/BUILDING.md).
cd "$(dirname "$0")" || exit 1
exec ./build.sh --launcher "$@"

#!/usr/bin/bash
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_ROOT=$(dirname "$SCRIPT_DIR")
cd "$PROJECT_ROOT"

INPUT=$1
ABS_FILE="$PROJECT_ROOT/$INPUT"

REL_PATH=$(realpath --relative-to="build" "$ABS_FILE")

cd build && make && ./proglang $REL_PATH

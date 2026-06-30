#!/usr/bin/bash
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_ROOT=$(dirname "$SCRIPT_DIR")
cd "$PROJECT_ROOT"

if [ $# -ne 1 ]; then 
	echo "invalid arguments"
	echo "usage: ./run_examples.sh <path to .proglang program>"
	exit 1
fi

INPUT=$1
ABS_FILE="$PROJECT_ROOT/$INPUT"

REL_PATH=$(realpath --relative-to="build" "$ABS_FILE")

cd build && make && ./proglang $REL_PATH

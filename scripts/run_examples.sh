#!/usr/bin/bash
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_ROOT=$(dirname "$SCRIPT_DIR")
cd "$PROJECT_ROOT"
cd build && make && ./proglang ../examples/helloworld.proglang

#../build/proglang ../examples/variables.proglang
#../build/proglang ../examples/fibonacci.proglang

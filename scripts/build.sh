#!/usr/bin/bash


if [ -z "$1" ]; then
    echo "Usage: $0 {debug|release}"
    exit 1
fi

build_type=$(echo "$1" | tr '[:upper:]' '[:lower:]')

if [ "$build_type" = "debug" ] || [ "$build_type" = "release" ]; then
	cmake -S . -B build -DCMAKE_BUILD_TYPE="$build_type"
	cmake --build build/
else
    echo "Usage: $0 {debug|release}"
fi

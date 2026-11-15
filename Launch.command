#!/bin/zsh
set -eu
project_dir="${0:A:h}"
export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"
cd "$project_dir"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
exec ./build/SolarSystem

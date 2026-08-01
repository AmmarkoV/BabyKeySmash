#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$DIR"

if [ ! -e build/babykeysmash ]
then
  echo "BabyKeySmash is not built yet , building it now.."
  cmake -S . -B build && cmake --build build -j$(nproc)
fi

./build/babykeysmash "$@"

exit 0

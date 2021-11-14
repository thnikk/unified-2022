#!/usr/bin/env sh

mkdir firmware
cd .pio/build || exit
for f in *; do
    if [ -d "$f" ]; then
        cp "$f/firmware.bin" "../../firmware/$f.bin"
    fi
done

#!/bin/bash
v="`cat bin/Release/avr-pcm-out.elf.hex | sed '{s/:........\(.*\).[a-fA-F0-9][^a-fA-F0-9]*/\1/}' | grep "..*" | tr -d $'\n' | wc -c`"; echo $((v / 2))

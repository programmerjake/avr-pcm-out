#!/bin/bash
current_byte_value=0
current_bit_count=0
function write_bit()
{
    local bit=$(($1 & 1))
    #echo "write_bit $bit"
    current_byte_value=$((current_byte_value / 2 + bit * 0x80))
    current_bit_count=$((current_bit_count + 1))
    if [ $current_bit_count -ge 8 ]; then
        printf "    0x%02X,\n" $current_byte_value
        current_byte_value=0
        current_bit_count=0
    fi
}

function finish_off_byte()
{
    while [ $current_bit_count -gt 0 ]; do
        write_bit 0
    done
}

function write_bits()
{
    local v=$(($1))
    #echo "write_bits $1 $2"
    local i
    for((i=0;i<$2;i++)); do
        write_bit $(((v >> i) & 1))
    done
}

function encode_hex()
{
    local define_name="$1"
    local last_v
    local v
    local d
    local nibble_count=0
    printf "#ifndef %s\n#define %s\n\n#include <stdint.h>\n#ifdef __AVR__\n#include <avr/pgmspace.h>\n#endif\n\nuint8_t sound_data[] PROGMEM =\n{\n" "$define_name" "$define_name"
    sample_count=0
    read last_v
    #echo "last_v: $last_v"
    if [ -z "$last_v" ]; then
        printf "    0x7F,\n};\n\n#warning no audio samples\nconst unsigned sample_count = 1;\n\n#endif\n"
        return
    fi
    sample_count=$((sample_count + 1))
    write_bits $((last_v)) 8
    nibble_count=$((nibble_count + 2))
    read v
    while [ "a$v" != "a" ]; do
        sample_count=$((sample_count + 1))
        #echo "v: $v"
        d=$((v - last_v))
        #echo "d: $d"
        if [ $d -lt -7 -o $d -gt 7 ]; then
            write_bits 0x8 4
            write_bits $((d & 0xFF)) 8
            nibble_count=$((nibble_count + 3))
        else
            write_bits $((d & 0xF)) 4
            nibble_count=$((nibble_count + 1))
        fi
        last_v=$v
        read v
    done
    finish_off_byte
    printf "};\n\nconst unsigned sample_count = %d;\n// byte count : %d (%d nibbles)\n\n#endif\n" "$sample_count" $(((nibble_count + 1) / 2)) "$nibble_count"
}

if [ "a$1" == "a" -o "a$1" == "a-h" -o "a$1" == "a--help" ]; then
    echo usage: "$0" "<raw-input-file.raw>"
    echo converts from a .raw to a .h file
    exit
elif [ ! -r "$1" ]; then
    echo "'$1' doesn't exist"
    exit 1
elif [ ! -s "$1" ]; then
    echo "'$1' is empty"
    exit 1
fi
output_file="${1%.raw}.h"
define_name="`echo "$1" | tr -c '[:alnum:]_' '_' | sed '{s/^[0-9_]/F&/}' | tr 'a-z' 'A-Z'`"
#var_name="`echo "$define_name" | tr 'A-Z' 'a-z'`"
echo "$output_file"
hexdump -v -e '1/1 "0x%02X\n"' < "$1" | encode_hex "$define_name" > "$output_file"

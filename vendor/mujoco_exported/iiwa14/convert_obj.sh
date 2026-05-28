#!/bin/bash
set -euo pipefail

BLENDER="/Applications/Blender.app/Contents/MacOS/Blender"

# Directory to search. Defaults to current directory.
ROOT_DIR="${1:-.}"

find "$ROOT_DIR" -type f -iname "*.obj" -print0 | while IFS= read -r -d '' dae_file; do
    obj_file="${dae_file%.*}.obj"

    echo "Converting:"
    echo "  Input : $dae_file"
    echo "  Output: $obj_file"

    "$BLENDER" --background --python-expr "
import bpy
import sys

args = sys.argv[sys.argv.index('--') + 1:]
input_file = args[0]
output_file = f'output/{args[1]}'

bpy.ops.wm.obj_import(filepath=input_file)
bpy.ops.wm.obj_export(filepath=output_file)
" -- "$dae_file" "$obj_file"

    echo "Done: $obj_file"
    echo
done

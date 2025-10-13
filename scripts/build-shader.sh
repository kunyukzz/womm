#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SHADER_DIR="$SCRIPT_DIR/../assets/shaders"

for file in "$SHADER_DIR"/*.glsl; do
	base="${file%.glsl}"
	out="${base}.spv"

	case "$file" in
		*.vert.glsl) stage=vert ;;
		*.frag.glsl) stage=frag ;;
		*) echo "Skip: $(basename "$file") (unknown stage)"; continue ;;
	esac

	glslc -fshader-stage=$stage "$file" -o "$out"
	echo "Compiled: $(basename "$file") -> ${out##*/}"
done

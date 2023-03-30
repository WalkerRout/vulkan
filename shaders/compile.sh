#!/bin/bash

echo "[+] compile.sh running glslc..."

# flag to check if this script is being executed from cmake in /build, or manually in shaders/
if [ $1 == "__cmake" ]
then
	glslc -fshader-stage=vertex ../shaders/vert.glsl -o ../shaders/vert.spv
	glslc -fshader-stage=fragment ../shaders/frag.glsl -o ../shaders/frag.spv
else
  glslc -fshader-stage=vertex vert.glsl -o vert.spv
  glslc -fshader-stage=fragment frag.glsl -o frag.spv
fi
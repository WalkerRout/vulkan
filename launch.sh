#!/bin/bash

PROJ_NAME=vulkan

cd build/

# hot-reload for shader compile target
make comp_shaders

if [ "$1" == "run" ]
then
  make "$PROJ_NAME"_run; src/"$PROJ_NAME"_run
else
  make "$PROJ_NAME"_test; test/"$PROJ_NAME"_test
fi

cd ..

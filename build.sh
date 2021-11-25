#!/bin/bash

mkdir -p build
cd build
cmake -DUSE_SHARED_MEMORY=ON ..
make -j4

cd ..
cp build/libgstlookoutvision.so \
    greengrass-build/artifacts/aws.greengrass.labs.lookoutvision.GStreamer/NEXT_PATCH/libgstlookoutvision.so
rm -rf build

cp recipe.json greengrass-build/recipes/recipe.json

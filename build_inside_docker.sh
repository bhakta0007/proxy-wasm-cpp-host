#!/bin/bash

chown -R root ~/.cache/bazel
bazel build //src/proxy-wasm-cli:proxy-wasm-cli
RET=$?
if [ $RET -ne 0 ];
then
    echo "Error building proxy-wasm-cli"
    exit $RET
fi
cp -f bazel-bin/src/proxy-wasm-cli/proxy-wasm-cli src/proxy-wasm-cli/dist/
echo "Build successful. Binary can be found in the dist folder"

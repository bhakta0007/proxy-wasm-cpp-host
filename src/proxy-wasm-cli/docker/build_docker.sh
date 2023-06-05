#!/bin/bash

# script to copy the dependent files and build a docker

FILE=$(readlink -f ${BASH_SOURCE[0]})
BASE=$(dirname ${FILE})
FILE1="/../example/myproject.wasm"
FILE2="/../dist/proxy-wasm-cli"
FILE3="/../proxy_wasm_cli_pb2_grpc.py"
FILE4="/../proxy_wasm_cli_pb2.py"
FILE5="/../proxy_wasm_cli.py"
cp -f $BASE$FILE1 .
cp -f $BASE$FILE2 .

mkdir -p python-client
cp -f $BASE$FILE3 $BASE$FILE4 $BASE$FILE5 python-client
docker build -t proxy-wasm-cli:1.0 .

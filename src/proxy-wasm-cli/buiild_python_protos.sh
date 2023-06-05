#!/bin/bash
python3 -m grpc_tools.protoc --proto_path=./proto/ ./proto/proxy_wasm_cli.proto --python_out=./ --grpc_python_out=./

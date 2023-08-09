#!/bin/bash

bazel build --jobs 12  --cxxopt=-std=c++17 --subcommands --verbose_failures //src/proxy-wasm-cli:proxy_wasm_bridge
bazel build --subcommands --verbose_failures //:wasmedge_lib
bazel build --subcommands --verbose_failures //:wasmedge_lib
bazel build --subcommands --verbose_failures //:wasmtime_lib
bazel build --subcommands --verbose_failures //:wamr_lib 
bazel build --subcommands --verbose_failures //external:wasmedge
bazel build --subcommands --verbose_failures //:base_lib
bazel build --subcommands --verbose_failures @com_github_bytecodealliance_wasmtime//:rust_c_api


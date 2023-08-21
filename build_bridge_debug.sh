#!/bin/bash

#bazel build --jobs 12  --cxxopt=-std=c++17 --subcommands --verbose_failures //src/proxy-wasm-cli:proxy_wasm_bridge --compilation_mode=dbg --strip=never
bazel build --compilation_mode=dbg --subcommands --verbose_failures //:wasmedge_lib  --strip=never
bazel build --compilation_mode=dbg --subcommands --verbose_failures //:wasmedge_lib  --strip=never
bazel build --compilation_mode=dbg --subcommands --verbose_failures //:wasmtime_lib  --strip=never
bazel build --compilation_mode=dbg --subcommands --verbose_failures //:wamr_lib --strip=never
bazel build --compilation_mode=dbg --subcommands --verbose_failures //external:wasmedge  --strip=never
bazel build --compilation_mode=dbg --subcommands --verbose_failures //:base_lib  --strip=never
bazel build --compilation_mode=dbg --subcommands --verbose_failures @com_github_bytecodealliance_wasmtime//:rust_c_api  --strip=never
bazel build --subcommands --verbose_failures wasm_vm_headers --strip=never
g++ -g -fPIC -c -I. -Ibazel-proxy-wasm-cpp-host/external/proxy_wasm_cpp_sdk  -L. src/proxy-wasm-cli/proxy_wasm_bridge.cc -lcombined -lcrypto
gcc -shared -o libproxy_wasm_bridge.so proxy_wasm_bridge.o


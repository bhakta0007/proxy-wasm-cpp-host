#!/bin/bash

#bazel build --jobs 12  --cxxopt=-std=c++17 --subcommands --verbose_failures //src/proxy-wasm-cli:proxy_wasm_bridge
bazel build --subcommands --verbose_failures //:wasmedge_lib
bazel build --subcommands --verbose_failures //:wasmedge_lib
bazel build --subcommands --verbose_failures //:wasmtime_lib
bazel build --subcommands --verbose_failures //:wamr_lib 
bazel build --subcommands --verbose_failures //external:wasmedge
bazel build --subcommands --verbose_failures //:base_lib
bazel build --subcommands --verbose_failures @com_github_bytecodealliance_wasmtime//:rust_c_api
bazel build --subcommands --verbose_failures wasm_vm_headers
gcc -shared -o libcombined.so -Wl,--whole-archive bazel-bin/libwamr_lib.a bazel-bin/libwasmedge_lib.a bazel-bin/libwasmtime_lib.a bazel-bin/libbase_lib.lo bazel-bin/external/com_github_bytecodealliance_wasmtime/librust_c_api.a bazel-bin/external/com_github_wasmedge_wasmedge/wasmedge_lib/lib/libwasmedge.a -Wl,--no-whole-archive
g++ -s -fPIC -c -I. -Ibazel-proxy-wasm-cpp-host/external/proxy_wasm_cpp_sdk  -L. src/proxy-wasm-cli/proxy_wasm_bridge.cc -lcombined -lcrypto
gcc -shared -o libproxy_wasm_bridge.so proxy_wasm_bridge.o

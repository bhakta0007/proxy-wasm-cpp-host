load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_proto_library")
load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")

proto_library(
  name = "proxy_wasm_cli_proto",
  srcs = ["proto/proxy_wasm_cli.proto"],
  deps = ["@com_google_protobuf//:any_proto", "@com_google_protobuf//:timestamp_proto"],
)

cc_proto_library(
  name = "proxy_wasm_cli_proto_cc",
  deps = [":proxy_wasm_cli_proto"],
  visibility = ["//visibility:public"],
)

cc_grpc_library(
    name = "proxy_wasm_cli_cc_grpc",
    srcs = [":proxy_wasm_cli_proto"],
    grpc_only = True,
    deps = [":proxy_wasm_cli_proto_cc"],
)

cc_binary(
    name = "proxy-wasm-cli",
    srcs = ["proxy_wasm_cli.cc", "proxy_wasm_includes.h"],
    deps = [
        "//:base_lib",
        "//:wamr_lib",
        "//:wasmtime_lib",
        "//:wasmedge_lib",
        ":proxy_wasm_cli_proto_cc",
        ":proxy_wasm_cli_cc_grpc",
        "@bazel_remote//:yaml-cpp",
        "@boost//:system",
        "@boost//:asio",
        "@boost//:date_time",
        "@served_remote//:served",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_library(
    name = "proxy_wasm_bridge1",
    hdrs = [
    ],
    srcs = [
        "proxy_wasm_bridge.cc",
        "proxy_wasm_includes.h"
    ],
    deps = [
        "//:base_lib",
        "//:wamr_lib",
        "//:wasmtime_lib",
        "//:wasmedge_lib",
        ":proxy_wasm_cli_proto_cc",
        ":proxy_wasm_cli_cc_grpc",
        "@bazel_remote//:yaml-cpp",
        "@boost//:system",
        "@boost//:asio",
        "@boost//:date_time",
        "@served_remote//:served",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/strings:str_format",
    ],
)



cc_library(
    name = "proxy_wasm_bridge",
    hdrs = [
    ],
    srcs = [
        "proxy_wasm_bridge.cc",
        "proxy_wasm_includes.h",
        "proxy_wasm_bridge.h",
    ],
    deps = [
        "//:base_lib",
        "//:wamr_lib",
        "//:wasmtime_lib",
        "//:wasmedge_lib",
        ":proxy_wasm_cli_proto_cc",
        ":proxy_wasm_cli_cc_grpc",
        "@bazel_remote//:yaml-cpp",
        "@boost//:system",
        "@boost//:asio",
        "@boost//:date_time",
    ],
)

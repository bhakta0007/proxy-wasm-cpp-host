workspace(name = "proxy_wasm_cpp_host")

load("@proxy_wasm_cpp_host//bazel:repositories.bzl", "proxy_wasm_cpp_host_repositories")

proxy_wasm_cpp_host_repositories()

load("@proxy_wasm_cpp_host//bazel:dependencies.bzl", "proxy_wasm_cpp_host_dependencies")

proxy_wasm_cpp_host_dependencies()

load("@proxy_wasm_cpp_sdk//bazel:repositories.bzl", "proxy_wasm_cpp_sdk_repositories")

proxy_wasm_cpp_sdk_repositories()

load("@proxy_wasm_cpp_sdk//bazel:dependencies.bzl", "proxy_wasm_cpp_sdk_dependencies")

proxy_wasm_cpp_sdk_dependencies()

load("@proxy_wasm_cpp_sdk//bazel:dependencies_extra.bzl", "proxy_wasm_cpp_sdk_dependencies_extra")

proxy_wasm_cpp_sdk_dependencies_extra()


load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
git_repository(
    name = "com_github_nelhage_rules_boost",
    commit = "552baa34c17984ca873cd0c4fdd010c177737b6f",
    remote = "https://github.com/nelhage/rules_boost",
)
load("@com_github_nelhage_rules_boost//:boost/boost.bzl", "boost_deps")
boost_deps()

git_repository(
  name = "bazel_remote",
  remote = "https://github.com/jbeder/yaml-cpp.git",
  commit = "0e6e28d",
)

git_repository(
  name = "served_remote",
  remote = "https://github.com/meltwater/served.git",
  commit = "2eb36b8",
)

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "rules_proto",
    sha256 = "66bfdf8782796239d3875d37e7de19b1d94301e8972b3cbd2446b332429b4df1",
    strip_prefix = "rules_proto-4.0.0",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_proto/archive/refs/tags/4.0.0.tar.gz",
        "https://github.com/bazelbuild/rules_proto/archive/refs/tags/4.0.0.tar.gz",
    ],
)
load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")
rules_proto_dependencies()
rules_proto_toolchains()




http_archive(
    name = "com_github_grpc_grpc",
    sha256 = "de2d3168e77e5ffb27758b07e87f6066fd0d8087fe272f278771e7780e6aaacb",
    strip_prefix = "grpc-1.44.0",
    urls = [
        "https://github.com/grpc/grpc/archive/v1.44.0.zip",
    ],
)

load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")
grpc_deps()
# Not mentioned in official docs... mentioned here https://github.com/grpc/grpc/issues/20511
load("@com_github_grpc_grpc//bazel:grpc_extra_deps.bzl", "grpc_extra_deps")
grpc_extra_deps()
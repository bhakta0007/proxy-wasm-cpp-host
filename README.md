# WebAssembly for Proxies (C++ host implementation)

# Proxy-Wasm-Cli : Stand alone utility that demonstrates capabilities of this project

Thanks to the Apache Traffic Server (https://github.com/apache/trafficserver) implementation - i took most of the code from there!

This project (proxy-wasm-cpp-host) provides the host utilities to allow working with runtimes to implement the proxy wasm ABI. The only way to see this code working is integrate it with a proxy (or run the tests). The idea behind writing the proxy-wasm-cli is to create
a simple stand-alone utility that can demonstrate the usage of the proxm-wasm-cpp-host code. This was done as an exercise to understand the functionality. The source code for this stand-alone is under src/proxy-wasm-cli and specifically in src/proxy-wasm-cli/proxy_wasm_cli.cc

![proxy-wasm-cli](https://github.com/bhakta0007/proxy-wasm-cpp-host/assets/18511513/f764b2f6-dee7-4407-a2af-6b398837ba62)

A real proxy server would be serving HTTP protocol. Adding HTTP support would be a lot more work - so the proxy-wasm-cli utility listens on GRPC (see src/proxy-wasm-cli/proto/proxy_wasm_cli.proto). A client can send GRPC request to create, destroy, modify-settings and also a simple emulation of sending traffic to the VM(s). The proxy_wasm_cli.py client provides a menu driven interface to talk to the GRPC server and demonstrate simple functions. An example application .wasm file is also provided.

The example wasm cpp program is in [src/proxy-wasm-cli/example/myproject.cc](src/proxy-wasm-cli/example/myproject.cc) and the compiled wasm in [src/proxy-wasm-cli/example/myproject.wasm](src/proxy-wasm-cli/example/myproject.wasm)

The myproject.wasm was built using these steps:

```
# First build the builder Docker (one time)
git clone https://github.com/proxy-wasm/proxy-wasm-cpp-sdk.git
cd /path/to/proxy-wasm-cpp-sdk

root@bhakta-ngx-build-vm:/home/bhakta/dev/wasm/proxy-wasm-cpp-sdk# docker build -t wasmsdk:v2 -f Dockerfile-sdk .

then build the example (copied from trafficserver repo with some changes):

cd src/proxy-wasm-cli/example/
docker run -v $PWD:/work -w /work  wasmsdk:v2 /build_wasm.sh
```

# Building and Running the stand-alone proxy-wasm-cli:

Here are the top-level steps:

1. Build the proxy-wasm-cli stand-alone utility. You can build on the localhost or use Docker (recommended).
2. Run the proxy-wasm-cli
3. Run the python client to talk to proxy-wasm-cli via GRPC


## Build on localhost

Note: You will need to setup the necessary dependencies

run: bazel build //src/proxy-wasm-cli:proxy-wasm-cli

## Build using Docker

First build the builder image (the docker image that has the necessary dependencies to build the proxy-wasm-cli)

### build the proxy-wasm-cli-builder (once)

```
docker build -t proxy-wasm-cli-builder:1.0 .
```

The above step should build the docker image : 'proxy-wasm-cli-builder:1.0'


### Using the builder, build the proxy-wasm-cli package

Ensure you are in the current folder and the run. The idea of mounting the .bazel-cache is to speed up subsequent builds:

```
docker run --name proxy-wasm-cli-builder -v $(pwd):/opt/proxy-wasm-cli  -v $(pwd)/.bazel-cache:/opt/bazel-cache proxy-wasm-cli-builder:1.0
```

This should produce the binary in src/proxy-wasm-cli/dist/

The binary can be run on the localhost, or better use the other docker that bundles the proxm-wasm-cli + the necessary python client along with its dependencies.

## Run the binary (proxy-wasm-cli and python client to test)
Follow the steps in [src/proxy-wasm-cli/README.md](src/proxy-wasm-cli/README.md)


# proxy-wasm-cli Code walkthrough

## proto definition

1. [src/proxy-wasm-cli/proto/proxy_wasm_cli.proto](src/proxy-wasm-cli/proto/proxy_wasm_cli.proto) defines the RPCs that are being served via GRPC
2. There are 4 RPC's being exposed
```
service WasmHost {
  rpc ListInstances (WasmListInstanceRequest) returns (WasmListInstanceReply) {}
  rpc LaunchInstance (WasmLaunchInstanceRequest) returns (WasmLaunchInstanceReply) {}
  rpc SendTraffic (WasmSendTrafficRequest) returns (WasmSendTrafficReply) {}
  rpc WasmSettings (WasmSettingsRequest) returns (WasmSettingsReply) {}
}
```
3. The bazel build for proxy-wasm-cli cpp utility will compile the proto and generate the cpp stubs. Note that the python stubs for the client needs the proto to be manually built. Run buiild_python_protos.sh from within the proxy-wasm-cpp-host/src/proxy-wasm-cli/ folder to build the python proto.
4. The Dockerfile for proxy-wasm-cli expects the protos and the proxy-wasm-cli binary to be built.


## proxy_wasm_cli.cc

1. Main function launches the GRPC server and waits for request.
2. The handler functiosn in src/proxy-wasm-cli/proxy_wasm_cli.cc [class WasmHostServiceImpl] handle the incoming requests. (e.g. LaunchInstance, ListInstances, SendTraffic and WasmSettings)


### LaunchInstance

1. The input will provide the project name, vm-name, wasm file, trace settings, runtime. etc (See the LaunchInstance proto definition)
2. First check if the VM with the name exists (inside NewWasmVM), if not create a new WasmVmInfo() [the data-structure that stores the information specific to each VM that is launched] and then call LaunchVm to do the actual runtime creation.
3. LaunchVm will see the runtime and instantiate the corresponding runtime proxy_wasm::createWasmEdgeVm() / proxy_wasm::createWasmtimeVm() / proxy_wasm::createWamrVm() and create "class Wasm" instance.
4. The WASM code is then read, loaded, initialized
5. A root context is created for every VM once launched [see proxy_wasm_cli.cc->Wasm::start] and the VM is configured.

![Proxy-WASM-Setup](https://github.com/bhakta0007/proxy-wasm-cpp-host/assets/18511513/8970de85-1a1f-4006-be0c-d11d9e9dec7c)

### Sending traffic to the VM

1. A real proxy server would handle a HTTP request and have logic to decipher the various header and body fields and do upstream processing / caching etc. We need an easy way to send traffic to our VM and emulate various stages (request headers, response headers etc..). This is achieved by calling SendTraffic which accepts a list of packets. See src/proxy-wasm-cli/docker/data/http_4pkts.json for example (also pasted below)
```
[
  {"sip": "10.0.0.2", "sp": "50124", "dip": "10.0.0.1", "dp": "443", "scheme": "https", "uri_path": "/content/web/index.html", "method": "GET", "query_params": {"a": "1"}, "headers": {"h1": "v1", "h2": "v2", "D": "5"}, "forward_direction": true, "delta_ms": 0},
  {"sip": "10.0.0.1", "sp": "443", "dip": "10.0.0.2", "dp": "50124", "scheme": "https", "uri_path": "/content/web/index.html", "headers": {"rh1": "rv1"}, "forward_direction": false, "delta_ms": 5, "body": "Hello World!"},
  {"sip": "10.0.0.2", "sp": "50125", "dip": "10.0.0.1", "dp": "443", "scheme": "https", "uri_path": "/content/web/10bytes.txt", "method": "GET", "headers": {"h11": "v11", "h21": "v21"}, "forward_direction": true, "delta_ms": 10},
  {"sip": "10.0.0.1", "sp": "443", "dip": "10.0.0.2", "dp": "50125", "scheme": "https", "uri_path": "/content/web/index.html", "headers": {"rh1": "rv1"}, "forward_direction": false, "delta_ms": 15, "body": "ABCDEFGHI"}
]

```

2. The idea is to define the typical fields that are useful for our processing. The first packet is https from 10.0.0.2:51024 to GET https://10.0.0.1:443/content/web/index.html?a=1 with headers h1: v1, h2: v2, D: 5 and we mark it forward direction (client -> server). The second packet is response from the server to client. The delta_ms indicates the delay before processing the next packet (not yet implemented) - I guess you get the idea.
3. On receiving this traffic payload, the SendTraffic handler walks through the packets and calls ProcessTraffic().
4. ProcessTraffic will create a request contex (one per request), save the data (headers, body, etc) in the context and call the corresponding handler. As of now, only onRequestHeaders and onResponseHeaders are implemented. Note that the functions into the VM do not actually pass the data (i.e. onRequestHeaders will not receive the list of request headers) - instead the VM must call back the host to get what it wants. When the VM Calls back, the src/proxy-wasm-cli/proxy_wasm_includes.h::getHeaderMapValue is called with the context and it must return what the VM wants.
5. Similarly, proxy-wasm-cli/proxy_wasm_includes.h::addHeaderMapValue() sets the headers (VM code can call this to set any header values)

![Proxy-WASM-Request-Flow](https://github.com/bhakta0007/proxy-wasm-cpp-host/assets/18511513/e8aa731b-0119-40fc-9ce6-441538c2d8a4)

6. If a VM does not implement a specific call, then the reqeust flow just continues.

![Proxy-WASM-Request-Flow-Not-Implemented](https://github.com/bhakta0007/proxy-wasm-cpp-host/assets/18511513/a788a8a5-9d73-4aac-b670-1250d45003cd)



# C Library using proxy-wasm-cpp-host

This section describes the details of the C library. The C Library is provided as a wrapper on top of the CPP implementation.


## Building the library


-  Build the individual wasm runtime libraries/dependencies
```
 bazel build --compilation_mode=dbg --subcommands --verbose_failures //:wasmedge_lib  --strip=never
 bazel build --compilation_mode=dbg --subcommands --verbose_failures //:wasmedge_lib  --strip=never
 bazel build --compilation_mode=dbg --subcommands --verbose_failures //:wasmtime_lib  --strip=never
 bazel build --compilation_mode=dbg --subcommands --verbose_failures //external:wasmedge  --strip=never
 bazel build --compilation_mode=dbg --subcommands --verbose_failures //:base_lib  --strip=never
 bazel build --compilation_mode=dbg --subcommands --verbose_failures @com_github_bytecodealliance_wasmtime//:rust_c_api  --strip=never
```

- Build the bridge

```
bazel build --cxxopt=-std=c++17 --subcommands --verbose_failures //src/proxy-wasm-cli:proxy_wasm_bridge
```

update: Use the ./build_bridge.sh which has the above commands in it


- And finally run make to build the c test program that to demonstrate the functionality

```
make
```


## Proxy WASM - CPP Data Structures

This section summarizes the existing data structures in the CPP module

### Wasm VM

Defined in: include/proxy-wasm/wasm_vm.h "class WasmVm {"

Description: This class defines the interface that a runtime needs to implement. Each runtime has a createVM function that returns a WasnVn object.


### WasmBase and Wasm

Defined in: include/proxy-wasm/wasm.h "class WasmBase"  / src/proxy-wasm-cli/proxy_wasm_includes.h "class Wasm : public WasmBase"
Description: Manages the Host Side of the WASM interface


### Context

Defined in: proxy_wasm_includes.h:: class Context : public ContextBase

Description: Context is the data structure that holds all the information between the Host and the WASM VM. It houses a bunch of function vectors, that are used as call-ins and call-outs between the Host and the WASM-VM. There are two types of contexts:

- Root Context: Root Context is created at the start of the WASM-VM life-cycle. The root has the same life-span as that of the VM. All the initial Host<>VM interactions are performed through the root context. The root context outlives a request.
- Stream Context: Is created when a new stream comes into the VM. The life-span of the stream context matches the life-span of the stream. All the HOST<>VM interactions during the life-cycle of the stream are done using the Stream context. The stream context is destroyed when the stream ends. Every stream results in creation of a corresponding unique stream context.

Both the Root and Stream context data-structures have function pointers that are called as part of the interaction sequence.

Context stores all the VM to Host function pointers - that must be supplied by the host layer


Here is a relevant comment from the code:

```
/**
 * ContextBase is the interface between the VM host and the VM. It has several uses:
 *
 * 1) To provide host-specific implementations of ABI calls out of the VM. For example, a proxy
 * which wants to provide the ability to make an HTTP call must implement the
 * ContextBase::httpCall() method.
 *
 * 2) To call into the VM. For example, when the above mentioned httpCall() completes, the host must
 * call ContextBase::onHttpCallResponse(). Similarly, when a new HTTP request arrives and the
 * headers are available, the host must create a new ContextBase object to manage the new stream and
 * call onRequestHeaders() on that object which will cause a corresponding Context to be allocated
 * in the VM which will receive the proxy_on_context_create and proxy_on_request_headers calls.
 *
 * 3) For testing and instrumentation the methods of ContextBase can be replaces or augmented.
```

The implementation has a single ContextBase (include/proxy-wasm/context.h) class that is used to create a Context (src/proxy-wasm-cli/proxy_wasm_includes.h) class - which is created for both Root and Stream context. There are flags in the Context that indicate if the context is a Root or Stream.



## Proxy WASM Bridge

This is the main cpp library that is built out of the CPP source. This library has dependency on the underlying runtime libraries that are also available as bazel targets. Two new files are introduced for this library:

proxy_wasm_bridge.cc - The bridge between C++ and C worlds
proxy_wasm_bridge.h - Header files


## C Sample Host implementation

The C implementation uses the proxm_wasm_bridge - which is a CPP library exposing C callable functions

To build:

```
./build_bridge.sh
```


### C Data structures

#### wasm_integration_fns_t

integration config is global for all WASM functionality

1. trace: Function to emit a trace
2. error: Function to emit a level
3. get_log_level: Gets the integration log level
4. set_log_level: Sets the integration log level

#### wasm_vm_to_host_fns_t

All these functions are at the context level (either root or stream)


  1. log_str: Logs a string
  2. get_property: Get the value of a property:
  3. error: Log error
  4. add_header_map_value: For a given header type (request/response/etc), set a key/value pair
  5. get_header_map_value: For a given header type (request/response/etc), get a key/value pair
  6. get_header_map_pairs: For a given header type (request/response/etc), get all key/value pairs
  7. get_current_time_ns: Get current time in ns
  8. get_monitonic_time_ns: Get the monitonic time in ns


Note: Other vectors in src/proxy-wasm-cli/proxy_wasm_includes.h::Context are not yet implemented. They can be added on a need-basis

#### wasm_vm_config_t

Holds the Configurations for the VM

1. project_name: Name of the project (Also used as root_id inside the lib)
2. vm_name: Name of the VM
3. code: Union of a file-name / bytes
4. code_type: Indicates the type of the code
5. log_level: Log level for the VM
6. runtime: Runtime used by the VM
7. host_functions: Set of host functions that VM can call
8. integration_functions: Set of integration functions that the VM can call


#### wasm_c_vm_t

wasm_c_vm_t holds all the information related to a WASM VM.

It has the following fields:

1. magic: Set to 0xc001d00d
2. context: Void pointer, holds cpp lib context. Not to be touched by C layer.
3. wasm_vm_config_t: Pointer to config structure



##### wasm_c_vm_t * wasm_launch_instance(wasm_vm_config_t *config)
Launch a WASM VM using the provided configuration and returns the pointer to the wasm_vm_t.


1. Calls the Specific runtime code to create the VM Data structure
2. Loads the WASM Code - using the VM code in the config
3. Configures the log level in the VM - based on the config

##### wasm_err_t * wasm_destroy_vm(wasm_c_vm_t *)

Note: This is yet to be implemented


##### wasm_stream_t *wasm_create_stream(wasm_vm_t *cVm, void *host_context);

Host must call this funciton on arrival of a new stream. The bridge will create a VM context
(based on the VM root context) and call onCreate vector which will land inside the WASM VM.

Host can provide a context that will be returned back to the host during callbacks.

The bridge will return back a wasm_stream_t pointer


#####   wasm_bool_t (*on_done)(struct wasm_stream_t *stream);
Host must call this function once the stream ends (or is reset). At this point, the stream information
is not expected to change.

#####   void (*on_log)(struct wasm_stream_t *stream);

Host must call this function after on_done. This is the place to do any logging

#####   void (*on_delete)(struct wasm_stream_t *stream);

Host must call this function after on_log. This is the last handler to be called for the stream. All
data-structures related to the stream will be destroyed/freed at this point.

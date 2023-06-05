# Steps to build the docker

## Build the docker image

```
cd src/proxy-wasm-cli/docker
./build_docker.sh
```
The above step will build 'proxy-wasm-cli:1.0'

## Run the docker

```
docker run --rm --name proxy-wasm-cli proxy-wasm-cli:1.0
```


## Using proxy-wasm-cli

In two separate windows, get into the docker:


### from one of the windows, start the proxy-wasm-cli

```
docker exec -it proxy-wasm-cli  bash
root@0ba62b62557b:/# /opt/wasm/proxy-wasm-cli
Server listening on 0.0.0.0:50051
```

### from the other window, start the python client

#### Launch a VM
Note: the wasm file is coded with project name "p1". The same needs to be specified if using the myproject.wasm. VM name can be anything

```
➜ docker exec -it proxy-wasm-cli  bash
root@137be394c2e5:/# cd /opt/wasm/
root@137be394c2e5:/# python3 proxy_wasm_cli.py
Using Server localhost:50051
Enter value for "Project Name" [] : p1
Enter value for "Vm Name" [] : v1
Enter value for "WASM File" [/opt/wasm/myproject.wasm] :
Enter value for "Log Level" [debug] : trace
Enter value for "Runtime [wasmedge]" [wasmedge] :
Inputs {'root_id': 'p1', 'name': 'v1', 'wasm_file': '/opt/wasm/myproject.wasm', 'log_level': 'debug', 'runtime': 'wasmedge'}
> Create VM
  List VM's
  Vm Settings
  Send Traffic
  q/Q - Quit Menu
```


#### Send traffic on the VM

```
root@137be394c2e5:/# cd /opt/wasm/
root@137be394c2e5:/opt/wasm#
root@137be394c2e5:/opt/wasm#
root@137be394c2e5:/opt/wasm# python3 proxy_wasm_cli.py
Using Server localhost:50051
Enter value for "Project Name" [] : p1
Enter value for "Vm Name [leave it empty to send to all VMs in the project]" [] :
Enter value for "Packets Data Json File" [data/http_4pkts.json] :
Added packet 1
Added packet 2
Added packet 3
Added packet 4
Send traffic resp =
> Create VM
  List VM's
  Vm Settings
  Send Traffic
  q/Q - Quit Menu
```

When you do the above, watch the proxy-wasm-cli window

```
wasm[getProperty] looking for plugin_root_id: p1
 [:] called WASM plugin onStart
wasmWasm /opt/wasm/myproject.wasm configured
Vm with key p1:v1 not found. launched with root_id p1 vm name v1 wasm file /opt/wasm/myproject.wasmvm info name v1 proj = p1
Launch Instance0x7f2d7c00b990
Send Traffic to VM p1:
 [p1:v1] called WASM plugin onCreate 2 from client 100.0.0.1:31337
ProcessTraffic header count3 forward = 1
 [p1:v1] called WASM plugin onRequestHeaders2 cnt 1
 [p1:v1] UA
 [p1:v1] #headers 3
 [p1:v1] headers: 3
 [p1:v1] h2 -> v2
 [p1:v1] D -> 5
 [p1:v1] h1 -> v1
 [p1:v1] time 1685961682392665505
addHeaderMapValue A = B2
 [p1:v1] onRequestHeaders is finished
wasmcalled onRequestHeaders with ret = 0
 [p1:v1] called WASM plugin onCreate 3 from client 100.0.0.1:31337
ProcessTraffic header count1 forward = 0
 [p1:v1] [myproject.cc:128]::onResponseHeaders() called WASM plugin onResponseHeaders 3
 [p1:v1] [myproject.cc:131]::onResponseHeaders() headers: 1
 [p1:v1] [myproject.cc:133]::onResponseHeaders() Response header rh1 -> rv1
addHeaderMapValue X-Wasm-custom = FOO
wasmcalled onResponseHeaders with ret = 0
 [p1:v1] called WASM plugin onCreate 4 from client 100.0.0.1:31337
ProcessTraffic header count2 forward = 1
 [p1:v1] called WASM plugin onRequestHeaders4 cnt 2
 [p1:v1] UA
 [p1:v1] #headers 2
 [p1:v1] headers: 2
 [p1:v1] h21 -> v21
 [p1:v1] h11 -> v11
 [p1:v1] time 1685961682395123303
addHeaderMapValue A = B2
 [p1:v1] onRequestHeaders is finished
wasmcalled onRequestHeaders with ret = 0
 [p1:v1] called WASM plugin onCreate 5 from client 100.0.0.1:31337
ProcessTraffic header count1 forward = 0
 [p1:v1] [myproject.cc:128]::onResponseHeaders() called WASM plugin onResponseHeaders 5
 [p1:v1] [myproject.cc:131]::onResponseHeaders() headers: 1
 [p1:v1] [myproject.cc:133]::onResponseHeaders() Response header rh1 -> rv1
addHeaderMapValue X-Wasm-custom = FOO
wasmcalled onResponseHeaders with ret = 0
```
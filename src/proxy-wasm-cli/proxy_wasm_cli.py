import click
import json
import grpc
from simple_term_menu import TerminalMenu
import proxy_wasm_cli_pb2_grpc as pb2_grpc
import proxy_wasm_cli_pb2 as pb2
from datetime import datetime


def show_menu(options):
    # show = True
    menuOptions = options + ["q/Q - Quit Menu"]
    while True:
        terminal_menu = TerminalMenu(menuOptions, quit_keys="q")
        idx = terminal_menu.show()
        if idx is None:
            return None, None
            # continue
        # if idx is not None:
        #     choice = options[idx]
        if idx == len(menuOptions) - 1:
            idx = None
            choice = None
            break
        choice = menuOptions[idx]
        break
    return idx, choice


def getCliOptions(options=[]):
    resp = {}
    while True:
        for op in options:
            key = op["key"]
            title = op["title"]
            nullable = op.get("nullable", True)
            vType = op["type"]
            default = op.get("default", "")
            defStr = f" [{default}]"
            valid = op.get("valid", [])
            retry = True
            while retry:
                retry = False
                x = input(f"Enter value for \"{title}\"{defStr} : ")
                try:
                    if not x:
                        x = default
                    if not nullable and not x:
                        print("value cannot be null, try again")
                        retry = True
                        continue
                    if vType == "int":
                        x = int(x)
                    if valid and x not in valid:
                        print(f"Error - value \"{x}\" is not among valid values \"{valid}\"")
                        print("Please try again")
                        retry = True
                        continue
                    resp[key] = x
                except Exception as ec:
                    print(f"Error: type of value is expected to be {vType} - {str(ec)}")
                    print("Please try again")
            if len(resp) == len(options):
                return resp


def toWasmLogLevel(logLevel):
    ll = pb2.WASM_LOGLEVEL_WARNING
    if logLevel == "trace":
        ll = pb2.WASM_LOGLEVEL_TRACE
    elif logLevel == "debug":
        ll = pb2.WASM_LOGLEVEL_DEBUG
    elif logLevel == "info":
        ll = pb2.WASM_LOGLEVEL_INFO
    elif logLevel == "error":
        ll = pb2.WASM_LOGLEVEL_ERROR
    elif logLevel == "critical":
        ll = pb2.WASM_LOGLEVEL_CRITICAL
    return ll

class WasmHostClient(object):
    """
    Client for gRPC functionality
    """

    def __init__(self, server='localhost:50051'):
        self.server = server
        # instantiate a channel
        self.channel = grpc.insecure_channel(self.server)
        # bind the client and the server
        self.stub = pb2_grpc.WasmHostStub(self.channel)

    def _listInstances(self, filter=""):
        """
        Client function to call the rpc for GetServerResponse
        """
        req = pb2.WasmListInstanceRequest(filter=filter)
        resp = self.stub.ListInstances(req)
        return resp

    def _createVm(self, inputs={}):
        rt = pb2.WasmLaunchInstanceRequest.WASM_RUNTIME_WASMEDGE
        if inputs["runtime"] == "wasmedge":
            rt = pb2.WasmLaunchInstanceRequest.WASM_RUNTIME_WASMEDGE
        elif inputs["runtime"] == "wamr":
            rt = pb2.WasmLaunchInstanceRequest.WASM_RUNTIME_WAMR
        elif inputs["runtime"] == "wasmtime":
            rt = pb2.WasmLaunchInstanceRequest.WASM_RUNTIME_WASMTIME
        ll = toWasmLogLevel(inputs["log_level"])
        req = pb2.WasmLaunchInstanceRequest(name=inputs["name"], root_id=inputs["root_id"], runtime=rt, wasm_file=inputs["wasm_file"], log_level=ll)
        resp = self.stub.LaunchInstance(req)
        return resp

    def _editVmSettings(self, project=None, vm=None, logLevel=None):
        req = pb2.WasmSettingsRequest(project=project, vm_name=vm)
        if logLevel:
            req.log_level = toWasmLogLevel(logLevel)
        resp = self.stub.WasmSettings(req)
        print(resp)
        return resp

    def _listVms(self):
        req = pb2.WasmListInstanceRequest()
        resp = self.stub.ListInstances(req)
        return resp

    def listVms(self):
        vms = self._listVms()
        print("List of active VMs: ")
        for idx, vm in enumerate(vms.vms):
            cts = datetime.fromtimestamp(vm.create_ts.seconds + vm.create_ts.nanos/1e9)
            print(f"{idx+1}.    Key = \"{vm.key}\" : project \"{vm.project}\", created \"{cts}\"")
        print("\n")

    def createmVm(self):
        inputOptions = [
            {"key": "root_id", "title": "Project Name", "type": "string", "nullable": False},
            {"key": "name", "title": "Vm Name", "type": "string", "nullable": False},
            {"key": "wasm_file", "title": "WASM File", "type": "string", "default": "/home/bhakta/dev/proxy-wasm-cpp-host/src/proxy-wasm-cli/example/myproject.wasm"},
            {"key": "log_level", "title": "Log Level", "type": "string", "default": "debug", "valid": ["debug", "trace", "info", "warn", "error", "critical"]},
            {"key": "runtime", "title": "Runtime", "type": "string", "valid": ["wasmedge", "wasmtime", "wamr"], "default": "wasmtime"},
        ]
        inputs = getCliOptions(options=inputOptions)
        print(f"Inputs {inputs}")
        self._createVm(inputs)

    def sendTraffic(self):
        inputOptions = [
            {"key": "project", "title": "Project Name", "type": "string"},
            {"key": "vm_name", "title": "Vm Name [leave it empty to send to all VMs in the project]", "type": "string", "default": ""},
            {"key": "data_file", "title": "Packets Data Json File", "type": "string", "default": "data/http_4pkts.json", "nullable": False},
        ]
        inputs = getCliOptions(options=inputOptions)
        dataFile = inputs["data_file"]
        packets = []
        try:
            with open(dataFile) as fp:
                packets = json.loads(fp.read())
        except Exception as ec:
            input(f"Error opening/loading {dataFile} - {str(ec)}. Press Enter to continue!")
            return
        req = pb2.WasmSendTrafficRequest()
        for i, packetData in enumerate(packets):
            pkt = req.packets.add()
            try:
                pkt.sip = packetData["sip"]
                pkt.sp = int(packetData["sp"])
                pkt.dip = packetData["dip"]
                pkt.dp = int(packetData["dp"])
                scheme = packetData["scheme"].lower()
                if scheme == "http":
                    scheme = pb2.HTTPScheme.SCHEME_HTTP
                elif scheme == "https":
                    scheme = pb2.HTTPScheme.SCHEME_HTTPS
                else:
                    raise Exception(f"Unsupported http scheme {scheme}")
                pkt.scheme = scheme
                pkt.uri_path = packetData["uri_path"]
                pkt.forward_direction = packetData["forward_direction"]
                method = packetData.get("method", "").lower()
                methodEnum = pb2.HTTPMethod.HTTP_GET
                if pkt.forward_direction:
                    if method == "get":
                        methodEnum = pb2.HTTPMethod.HTTP_GET
                    elif method == "put":
                        methodEnum = pb2.HTTPMethod.HTTP_GET
                    elif method == "post":
                        methodEnum = pb2.HTTPMethod.HTTP_GET
                    elif method == "delete":
                        methodEnum = pb2.HTTPMethod.HTTP_GET
                    else:
                        raise Exception(f"Unsupported http method {method}")
                pkt.method = methodEnum
                query_params = packetData.get("query_params", {})
                for k, v in query_params.items():
                    qp = pkt.query_params.add()
                    qp.key = k
                    qp.value = v
                headers = packetData.get("headers", {})
                for k, v in headers.items():
                    hdr = pkt.headers.add()
                    hdr.key = k
                    hdr.value = v
                pkt.delta_ms = packetData.get("delta_ms", 0)
            except Exception as ec:
                input(f"Error loading packet number {i+1}. {str(ec)}. Press Enter to continue!")
                return
            print(f"Added packet {i+1}")
        req.project = inputs["project"]
        req.vm_name = inputs["vm_name"]
        resp = self.stub.SendTraffic(req)
        print(f"Send traffic resp = {resp}")


    def setVmLogLevel(self, project=None, vm=None):
        options = ["trace", "debug", "info", "warn", "error", "critical"]
        idx, choice = show_menu(options)
        if idx is not None:
            self._editVmSettings(project=project, vm=vm, logLevel=choice)

    def vmSettingsMenu(self, project=None, vm=None):
        print(f"Settings for vm \"{project}:{vm}\"")
        options = ["Show Log Level", "Set Log Level"]
        while True:
            idx, choice = show_menu(options)
            if idx is None:
                break
            if idx == 0:
                print(f"Show log level for {project}:{vm}")
            elif idx == 1:
                self.setVmLogLevel(project=project, vm=vm)

    def vmSettings(self):
        vms = self._listVms()
        options = []
        vmKeys = []
        for idx, vm in enumerate(vms.vms):
            cts = datetime.fromtimestamp(vm.create_ts.seconds + vm.create_ts.nanos/1e9)
            options.append(f"{idx+1}.    Key = \"{vm.key}\" : project \"{vm.project}\", created \"{cts}\"")
            vmKeys.append(vm.key)
        while True:
            idx, choice = show_menu(options)
            if idx is None:
                break
            sp = vmKeys[idx].split(":")
            self.vmSettingsMenu(sp[0], sp[1])


@click.command()
@click.option('--server', '-s', default='localhost:50051', help='Server Address:Port')
def main(server):
    client = WasmHostClient(server)
    print(f"Using Server {server}")
    options = ["Create VM", "List VM's", "Vm Settings", "Send Traffic"]
    while True:
        idx, choice = show_menu(options)
        if idx is None:
            break
        if idx == 0:
            client.createmVm()
        elif idx == 1:
            client.listVms()
        elif idx == 2:
            client.vmSettings()
        elif idx == 3:
            client.sendTraffic()

if __name__ == '__main__':
    main()

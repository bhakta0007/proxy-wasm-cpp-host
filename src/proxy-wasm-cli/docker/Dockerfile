FROM ubuntu:20.04

# install dependencies
RUN apt-get update && \
    apt-get install -y python3-pip protobuf-compiler python-dev python3-dev python3-protobuf
RUN mkdir -p /opt/wasm/data
COPY requirements.txt /opt/wasm/
RUN python3 -m pip install --upgrade pip && pip3 install -r /opt/wasm/requirements.txt

#Example WASM plugin
COPY wasm/myproject.wasm /opt/wasm/

# proxy_wasm_cli: python client
COPY python-client /opt/wasm/
COPY proxy-wasm-cli /opt/wasm/
COPY data /opt/wasm/data/

COPY wait.sh /
CMD ["/wait.sh"]

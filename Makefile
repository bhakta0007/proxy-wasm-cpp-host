CC = gcc
CPP = g++
CFLAGS += -Wall -g -fPIC -Isrc/proxy-wasm-cli/ -L.
#CFLAGS += -O0 -Lbazel-bin/src/proxy-wasm-cli/ -Lbazel-bin/ -Lbazel-bin/external/com_github_bytecodealliance_wasmtime -Lbazel-bin/external/com_github_wasmedge_wasmedge/wasmedge_lib/lib/
#LDLIBS = -lproxy_wasm_bridge -lwamr_lib -lwasmedge_lib -lwasmedge  -lwasmtime_lib  -lrust_c_api -ldl -lrt -lpthread -lbase_lib -lcrypto
LDLIBS = -lproxy_wasm_bridge -lcombined -ldl -lrt -lpthread -lcrypto

all: proxy-wasm-c-cli

proxy_wasm_c_cli.o: proxy_wasm_c_cli.c
	$(CC) -c $^ $(CFLAGS) -o $@

proxy-wasm-c-cli: proxy_wasm_c_cli.o
	$(CPP) $(CFLAGS) $^ $(LOADLIBS) $(LDLIBS) -DLIB_MODE -o $@

clean:
	rm -rf proxy-wasm-c-cli

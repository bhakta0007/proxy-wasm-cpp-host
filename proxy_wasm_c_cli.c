#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include "proxy_wasm_bridge.h"


// Example stream info structure
typedef struct stream_info_t_ {
    int stream_id;
} stream_info_t;

/*
 * Host functions that are called back by WASM
 */

/*
 * Integration (VM level) callback for error
 */
void do_integration_error(const char * message)  {
    printf("C-INTEGRATION-ERROR %s\n", message);
}

/*
 * Integration (VM level) callback for trace messages
 */
void do_integration_trace(const char * message)  {
    printf("C-INTEGRATION-TRACE %s\n", message);
}

/*
 * Integration (VM level) get log level.
 * The host is expected to maintain a global log level
 * for proxy-wasm layer
 */
wasm_log_level_e do_integration_get_log_level(void)  {
    wasm_log_level_e log_level = WASM_LOG_LEVEL_DEBUG;
    return log_level;
}

/*
 * Integration (VM level) set log level.
 */
void do_integration_set_log_level(wasm_log_level_e log_level)  {
    printf("%s: set level to  %d\n", __FUNCTION__, log_level);
}

/*
 * FIXME: Fix this comment
 * Below are the Stream specific handlers. Each handler gets a context
 * which can be typecasted to wasm_vm_t
 */


/*
 * error
 */
void error(void *host_ctx, wasm_bool_t is_stream, const char *message)
{
    printf("VM-Error: %s\n", message);
    abort();
}

void get_vm_stream_prefix(void *host_ctx, wasm_bool_t is_stream, char *str, int len)
{
    wasm_vm_t *vm;
    char stream_str[32];
    bzero(str, len);
    if (is_stream) {
        wasm_stream_t *stream = (wasm_stream_t *) host_ctx;
        vm = host_ctx ? stream->vm : NULL;
        stream_info_t *si = (stream_info_t *) stream->host_context;
        snprintf(stream_str, sizeof(stream_str), ",stream:%d", si->stream_id);
    } else {
        vm = (wasm_vm_t *) host_ctx;
    }
    if (vm) {
        snprintf(str, len, "%s[proj:%s,vm:%s]:%s %s", ANSI_COLOR_RED, vm->config->project_name, vm->config->vm_name, (char *)stream_str, ANSI_COLOR_RESET);
    }
}

// get_real_time_ns
uint64_t get_real_time_ns(void)
{
    struct timespec tms;
    uint64_t ts;
    if (clock_gettime(CLOCK_REALTIME, &tms)) {
        return 0ULL;
    }
    ts = tms.tv_sec * 1000000000;
    ts += tms.tv_nsec;
    // printf("tS = %lu\n", ts);
    return ts;
}

// get_monotonic_time_ns
uint64_t get_monotonic_time_ns(void)
{
    struct timespec tms;
    uint64_t ts;
    if (clock_gettime(CLOCK_MONOTONIC, &tms)) {
        return 0ULL;
    }
    ts = tms.tv_sec * 1000000000;
    ts += tms.tv_nsec;
    // printf("tS = %lu\n", ts);
    return ts;
}

/*
 * get_header_map_value: Get the value for the key for a given map type
 */
wasm_result_e get_header_map_value(void *host_ctx, wasm_bool_t is_stream, wasm_header_map_type_e type,
                                    const char * key, int key_len, char **value)
{
    char prefix_str[WASM_STR_MAX];
    char *tmpStr = NULL;
    get_vm_stream_prefix(host_ctx, is_stream, prefix_str, WASM_STR_MAX);
    tmpStr = (char *)malloc(16);
    if (!tmpStr) {
        return InternalFailure;
    }
    strcpy(tmpStr, "DUMMY-VAL");
    *value = tmpStr;
    printf("%s: Get header %s -> %s for %s\n", prefix_str, key, tmpStr, WASM_HEADER_MAP_STR[type]);
    return Ok;
}

/*
 * get_header_map_pairs: Get all the key/value pairs for the given type
 */
wasm_result_e get_header_map_pairs(void *host_ctx, wasm_bool_t is_stream, wasm_header_map_type_e type,
                                    wasm_kv_list_t **kvList)
{
    int i = 0;
    char buf[16];
    wasm_kv_list_t *head = NULL, *prev = NULL, *ptr = NULL;
    char prefix_str[WASM_STR_MAX];
    get_vm_stream_prefix(host_ctx, is_stream, prefix_str, WASM_STR_MAX);
    for (i = 0; i < 5; i++) {
        ptr = (wasm_kv_list_t *) malloc(sizeof(wasm_kv_list_t));
        if (!ptr) {
            return InternalFailure;
        }
        if (head == NULL) {
            head = prev = ptr;
        }
        // Skiping malloc check..
        ptr->key = (char *)malloc(sizeof(buf));
        ptr->value = (char *)malloc(sizeof(buf));
        snprintf(buf, sizeof(buf), "key-%d", i+1);
        strncpy(ptr->key, buf, 16);
        snprintf(buf, sizeof(buf), "val-%d", i+1);
        strncpy(ptr->value, buf, 16);
        ptr->next = NULL;
        prev->next = ptr;
        prev = ptr;
    }
    *kvList = head;
    printf("%s: Get header pairs for %s\n", prefix_str,  WASM_HEADER_MAP_STR[type]);
    return Ok;
}

/*
 * add_header_map_value: Add a header key/value pair to the requested type
 */
wasm_result_e add_header_map_value(void *host_ctx, wasm_bool_t is_stream, wasm_header_map_type_e type,
                                    const char * key, int key_len, const char *value)
{
    char prefix_str[WASM_STR_MAX];
    get_vm_stream_prefix(host_ctx, is_stream, prefix_str, WASM_STR_MAX);
    printf("%s: Add header %s -> %s to %s\n", prefix_str, key, value, WASM_HEADER_MAP_STR[type]);
    return Ok;
}

/*
 * get_property: Given a path and length, get the property value
 * properties are defined in proxy_wasm_bridge.h
 */
wasm_result_e get_property(void *host_ctx, wasm_bool_t is_stream, const char *path, int path_len, char **result)
{
    wasm_vm_t *vm = NULL;
    char *tmpStr;
    printf("%s: path %s\n", __FUNCTION__, path);
    // 16 for now - just for this example
    tmpStr = (char *)malloc(16);
    if (!tmpStr) {
        return InternalFailure;
    }
    strcpy(tmpStr, "");
    if (is_stream) {
        wasm_stream_t *stream = (wasm_stream_t *) host_ctx;
        vm = host_ctx ? stream->vm : NULL;
    } else {
        vm = (wasm_vm_t *) host_ctx;
    }
    *result = tmpStr;
    printf("%s: stream:%d, path %s [%d]\n", __FUNCTION__, is_stream, path, path_len);
    if (memcmp(path, P_PLUGIN_ROOT_ID, path_len) == 0) {
        strncpy(tmpStr, vm->config->project_name, 16);
        return Ok;
    } else if (memcmp(path, P_SOURCE_ADDRESS, path_len) == 0) {
        strncpy(tmpStr, "100.0.0.1", 16);
        return Ok;
    } else if (memcmp(path, P_SOURCE_PORT, path_len) == 0) {
        strncpy(tmpStr, "31337", 16);
        return Ok;
    } else {
        printf("%s: FIXME: Add support for property %s\n", __FUNCTION__, path);
        return NotFound;
    }
    return NotFound;
}
/*
 * Integration (VM level) set log level.
 */
wasm_result_e do_log(void *host_ctx, wasm_bool_t is_stream, uint32_t log_level, const char * message)  {
    wasm_vm_t *vm = NULL;
    char stream_str[16];

    bzero(stream_str, sizeof(stream_str));
    if (is_stream) {
        wasm_stream_t *stream = (wasm_stream_t *) host_ctx;
        vm = host_ctx ? stream->vm : NULL;
        stream_info_t *si = (stream_info_t *) stream->host_context;
        snprintf(stream_str, sizeof(stream_str), ",stream:%d", si->stream_id);
    } else {
        vm = (wasm_vm_t *) host_ctx;
    }
    if (vm) {
        printf("%s[lvl:%d,proj:%s,vm:%s%s]:%s %s\n", ANSI_COLOR_RED, log_level, vm->config->project_name, vm->config->vm_name, stream_str, ANSI_COLOR_RESET, message);
    } else {
        printf("Missing VM context-FIXME: [lvl:%d]: %s\n", log_level, message);
    }
    return Ok;
}


int main(int argc, char **argv) {
    wasm_vm_t *cVm;
    wasm_stream_t *stream;
    wasm_vm_config_t *cfg;
    wasm_vm_to_host_fns_t host_fns;
    wasm_integration_fns_t integration_fns;

    if (argc < 2) {
        printf("require runtime name [wasmedge|wamr|wasmtime]\n");
        exit(1);
    }
    cfg = malloc(sizeof(wasm_vm_config_t));
    if (cfg == NULL) {
        printf("Error allocating memory for wasm vm config\n");
        exit(2);
    }
    integration_fns.error = do_integration_error;
    integration_fns.trace = do_integration_trace;
    integration_fns.get_log_level = do_integration_get_log_level;
    integration_fns.set_log_level = do_integration_set_log_level;
    host_fns.log_str = do_log;
    host_fns.get_property = get_property;
    host_fns.add_header_map_value = add_header_map_value;
    host_fns.get_header_map_value = get_header_map_value;
    host_fns.get_header_map_pairs = get_header_map_pairs;
    host_fns.get_current_time_ns = get_real_time_ns;
    host_fns.get_monitonic_time_ns = get_monotonic_time_ns;
    cfg->host_functions = host_fns;
    cfg->integration_functions = integration_fns;
    printf("Using runtime: %s\n", argv[1]);
    wasm_runtime_e runtime = WASM_RUNTIME_INVALID;

    if (strcmp(argv[1], "wasmtime") == 0) {
        runtime = WASM_RUNTIME_WASMTIME;
    } else if (strcmp(argv[1], "wasmedge") == 0) {
        runtime = WASM_RUNTIME_WASMEDGE;
    } else if (strcmp(argv[1], "wamr") == 0) {
        runtime = WASM_RUNTIME_WAMR;
    }

    cfg->runtime = runtime;
    cfg->code_type = WASM_CODE_TYPE_FILE;
    strncpy(cfg->project_name, "p1", WASM_STR_MAX);
    strncpy(cfg->vm_name, "v1", WASM_STR_MAX);
    cfg->code.file = "/home/bhakta/dev/proxy-wasm-cpp-host/src/proxy-wasm-cli/example/myproject.wasm";
    cVm = wasm_launch_instance(cfg, NULL);
    if (cVm == NULL) {
        printf("Error launching instance\n");
        return 1;
    }
    stream_info_t *si = (stream_info_t *) malloc(sizeof(stream_info_t));
    si->stream_id = 1;
    if (si == NULL) {
        printf("Error allocating stream_info_t memory\n");
        exit(4);
    }

    stream = wasm_create_stream(cVm, (void *)si);
    process_traffic(stream);

    // Example: calling on_request_headers
    stream->vm_functions.on_request_headers(stream, 1, 1);
    stream->vm_functions.on_response_headers(stream, 1, 1);
    stream->vm_functions.on_done(stream);
    stream->vm_functions.on_log(stream);
    stream->vm_functions.on_delete(stream);
    printf("Delete stream\n");
    wasm_delete_stream(stream);
    return 0;
}
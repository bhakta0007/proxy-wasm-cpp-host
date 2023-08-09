#ifndef __PROXY_WASM_BRIDGE_H__
#define __PROXY_WASM_BRIDGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define WASM_STR_MAX 128
typedef int wasm_bool_t;
#define WASM_FALSE 0
#define WASM_TRUE 1

/* Data structures */

typedef enum wasm_err_t_ {
  WASM_ERR_NONE         = 0,
  WASM_ERR_INVALID_ARGS = 1,
  WASM_ERR_NOMEM        = 2,
  WASM_ERR_INVALID_CODE = 3,
} wasm_err_t;

typedef enum wasm_runtime_e_ {
  WASM_RUNTIME_INVALID  = 0,
  WASM_RUNTIME_WASMTIME = 1,
  WASM_RUNTIME_WASMEDGE = 2,
  WASM_RUNTIME_WAMR     = 3,
} wasm_runtime_e;

typedef enum wasm_code_type_e_ {
  WASM_CODE_TYPE_INVALID  = 0,
  WASM_CODE_TYPE_FILE = 1,
  WASM_CODE_TYPE_DATA = 2,
} wasm_code_type_e;

typedef enum wasm_log_level_e_ {
  WASM_LOG_LEVEL_INVALID = 0,
  WASM_LOG_LEVEL_WARNING = 1,
  WASM_LOG_LEVEL_TRACE = 2,
  WASM_LOG_LEVEL_DEBUG = 3,
  WASM_LOG_LEVEL_INFO = 4,
  WASM_LOG_LEVEL_ERROR = 5,
  WASM_LOG_LEVEL_CRITICAL = 6,
} wasm_log_level_e;

typedef enum wasm_result_e_ {
  Ok = 0,
  // The result could not be found, e.g. a provided key did not appear in a
  // table.
  NotFound = 1,
  // An argument was bad, e.g. did not not conform to the required range.
  BadArgument = 2,
  // A protobuf could not be serialized.
  SerializationFailure = 3,
  // A protobuf could not be parsed.
  ParseFailure = 4,
  // A provided expression (e.g. "foo.bar") was illegal or unrecognized.
  BadExpression = 5,
  // A provided memory range was not legal.
  InvalidMemoryAccess = 6,
  // Data was requested from an empty container.
  Empty = 7,
  // The provided CAS did not match that of the stored data.
  CasMismatch = 8,
  // Returned result was unexpected, e.g. of the incorrect size.
  ResultMismatch = 9,
  // Internal failure: trying check logs of the surrounding system.
  InternalFailure = 10,
  // The connection/stream/pipe was broken/closed unexpectedly.
  BrokenConnection = 11,
  // Feature not implemented.
  Unimplemented = 12,
} wasm_result_e;

// constants for property names
const char *P_REQUEST_PATH                         = "request\0path";
const char *P_REQUEST_URL_PATH                     = "request\0url_path";
const char *P_REQUEST_HOST                         = "request\0host";
const char *P_REQUEST_SCHEME                       = "request\0scheme";
const char *P_REQUEST_METHOD                       = "request\0method";
const char *P_REQUEST_HEADERS                      = "request\0headers";
const char *P_REQUEST_REFERER                      = "request\0referer";
const char *P_REQUEST_USERAGENT                    = "request\0useragent";
const char *P_REQUEST_TIME                         = "request\0time";
const char *P_REQUEST_ID                           = "request\0id";
const char *P_REQUEST_PROTOCOL                     = "request\0protocol";
const char *P_REQUEST_QUERY                        = "request\0query";
const char *P_REQUEST_DURATION                     = "request\0duration";
const char *P_REQUEST_SIZE                         = "request\0size";
const char *P_REQUEST_TOTAL_SIZE                   = "request\0total_size";
const char *P_RESPONSE_CODE                        = "response\0code";
const char *P_RESPONSE_CODE_DETAILS                = "response\0code_details";
const char *P_RESPONSE_HEADERS                     = "response\0headers";
const char *P_RESPONSE_SIZE                        = "response\0size";
const char *P_RESPONSE_TOTAL_SIZE                  = "response\0total_size";
const char *P_NODE                                 = "node";
const char *P_PLUGIN_NAME                          = "plugin_name";
const char *P_PLUGIN_ROOT_ID                       = "plugin_root_id";
const char *P_PLUGIN_VM_ID                         = "plugin_vm_id";
const char *P_SOURCE_ADDRESS                       = "source\0address";
const char *P_SOURCE_PORT                          = "source\0port";
const char *P_DESTINATION_ADDRESS                  = "destination\0address";
const char *P_DESTINATION_PORT                     = "destination\0port";
const char *P_CONNECTION_MTLS                      = "connection\0mtls";
const char *P_CONNECTION_REQUESTED_SERVER_NAME     = "connection\0requested_server_name";
const char *P_CONNECTION_TLS_VERSION               = "connection\0tls_version";
const char *P_CONNECTION_SUBJECT_LOCAL_CERTIFICATE = "connection\0subject_local_certificate";
const char *P_CONNECTION_SUBJECT_PEER_CERTIFICATE  = "connection\0subject_peer_certificate";
const char *P_CONNECTION_DNS_SAN_LOCAL_CERTIFICATE = "connection\0dns_san_local_certificate";
const char *P_CONNECTION_DNS_SAN_PEER_CERTIFICATE  = "connection\0dns_san_peer_certificate";
const char *P_CONNECTION_URI_SAN_LOCAL_CERTIFICATE = "connection\0uri_san_local_certificate";
const char *P_CONNECTION_URI_SAN_PEER_CERTIFICATE  = "connection\0uri_san_peer_certificate";
const char *P_UPSTREAM_ADDRESS                     = "upstream\0address";
const char *P_UPSTREAM_PORT                        = "upstream\0port";
const char *P_UPSTREAM_LOCAL_ADDRESS               = "upstream\0local_address";
const char *P_UPSTREAM_LOCAL_PORT                  = "upstream\0local_port";
const char *P_UPSTREAM_TLS_VERSION                 = "upstream\0tls_version";
const char *P_UPSTREAM_SUBJECT_LOCAL_CERTIFICATE   = "upstream\0subject_local_certificate";
const char *P_UPSTREAM_SUBJECT_PEER_CERTIFICATE    = "upstream\0subject_peer_certificate";
const char *P_UPSTREAM_DNS_SAN_LOCAL_CERTIFICATE   = "upstream\0dns_san_local_certificate";
const char *P_UPSTREAM_DNS_SAN_PEER_CERTIFICATE    = "upstream\0dns_san_peer_certificate";
const char *P_UPSTREAM_URI_SAN_LOCAL_CERTIFICATE   = "upstream\0uri_san_local_certificate";
const char *P_UPSTREAM_URI_SAN_PEER_CERTIFICATE    = "upstream\0uri_san_peer_certificate";


typedef enum wasm_header_map_type_e_ {
  WASM_HEADER_MAP_TYPE_REQUEST_HEADERS = 0,              // During the onLog callback these are immutable
  WASM_HEADER_MAP_TYPE_REQUEST_TRAILERS = 1, // During the onLog callback these are immutable
  WASM_HEADER_MAP_TYPE_RESPONSE_HEADERS = 2, // During the onLog callback these are immutable
  WASM_HEADER_MAP_TYPE_RESPONSE_TRAILERS = 3,// During the onLog callback these are immutable
  WASM_HEADER_MAP_TYPE_GRPC_RECEIVE_INITIAL_METADATA = 4,// Immutable
  WASM_HEADER_MAP_TYPE_GRPC_RECEIVE_TRAILING_METADATA = 5, // Immutable
  WASM_HEADER_MAP_TYPE_HTTP_CALL_RESPONSE_HEADERS = 6, // Immutable
  WASM_HEADER_MAP_TYPE_HTTP_CALL_RESPONSE_TRAILERS = 7,// Immutable
  WASM_HEADER_MAP_TYPE_MAX = 7,
} wasm_header_map_type_e;

const char *WASM_HEADER_MAP_STR[] = {
  "WASM_HEADER_MAP_TYPE_REQUEST_HEADERS",
  "WASM_HEADER_MAP_TYPE_REQUEST_TRAILERS",
  "WASM_HEADER_MAP_TYPE_RESPONSE_HEADERS",
  "WASM_HEADER_MAP_TYPE_RESPONSE_TRAILERS",
  "WASM_HEADER_MAP_TYPE_GRPC_RECEIVE_INITIAL_METADATA",
  "WASM_HEADER_MAP_TYPE_GRPC_RECEIVE_TRAILING_METADATA",
  "WASM_HEADER_MAP_TYPE_HTTP_CALL_RESPONSE_HEADERS",
  "WASM_HEADER_MAP_TYPE_HTTP_CALL_RESPONSE_TRAILERS",
  "WASM_HEADER_MAP_TYPE_MAX",
};


typedef enum wasm_filter_status_e_ {
  WASM_FILTER_STATUS_CONTINUE       = 0,
  WASM_FILTER_STATUS_STOP_ITERATION = 1
} wasm_filter_status_e;

typedef enum wasm_filter_header_status_e_ {
  WASM_FILTER_HEADER_STATUS_CONTINUE                         = 0,
  WASM_FILTER_HEADER_STATUS_STOP_ITERATION                   = 1,
  WASM_FILTER_HEADER_STATUS_CONTINUE_AND_END_STREAM          = 2,
  WASM_FILTER_HEADER_STATUS_STOP_ALL_ITERATION_AND_BUFFER    = 3,
  WASM_FILTER_HEADER_STATUS_STOP_ALL_ITERATION_AND_WATERMARK = 4,
} wasm_filter_header_status_e;

typedef struct wasm_kv_list_t_ {
  char *key;
  char *value;
  struct wasm_kv_list_t_ *next;
} wasm_kv_list_t;

typedef struct wasm_integration_fns_t_ {
  void (*trace)(const char *message);
  void (*error)(const char *message);
  wasm_log_level_e (*get_log_level)();
  void (*set_log_level)(wasm_log_level_e log_level);
} wasm_integration_fns_t;

typedef struct wasm_vm_to_host_fns_t_ {
  wasm_result_e (*log_str)(void *host_ctx, wasm_bool_t is_stream, uint32_t level, const char *message);
  wasm_result_e (*get_property)(void *host_ctx, wasm_bool_t is_stream, const char *path, int path_len, char **result);
  wasm_result_e (*error)(void *host_ctx, wasm_bool_t is_stream, const char *message);
  wasm_result_e (*add_header_map_value)(void *host_ctx, wasm_bool_t is_stream, wasm_header_map_type_e type, const char * key, int key_len, const char * value);
  wasm_result_e (*get_header_map_value)(void *host_ctx, wasm_bool_t is_stream, wasm_header_map_type_e type, const char * key, int key_len, char **value);
  wasm_result_e (*get_header_map_pairs)(void *host_ctx, wasm_bool_t is_stream, wasm_header_map_type_e type, wasm_kv_list_t **kvList);
  uint64_t      (*get_current_time_ns)(void);
  uint64_t      (*get_monitonic_time_ns)(void);
  // Note: Other vectors in src/proxy-wasm-cli/proxy_wasm_includes.h::Context are not yet implemented. They can be added on a need-basis
} wasm_vm_to_host_fns_t;

typedef struct wasm_vm_config_t_ {
  char project_name[WASM_STR_MAX];
  char vm_name[WASM_STR_MAX];
  union code_ {
    char *file;
    uint8_t *data;
  } code;
  wasm_code_type_e code_type;
  wasm_log_level_e log_level;
  wasm_runtime_e runtime;
  wasm_vm_to_host_fns_t host_functions;
  wasm_integration_fns_t integration_functions;
} wasm_vm_config_t;


struct wasm_stream_t;
typedef struct wasm_host_to_vm_fns_t_ {
  wasm_filter_header_status_e (*on_request_headers)(struct wasm_stream_t *stream, uint32_t num_headers, int end_of_stream);
  wasm_filter_header_status_e (*on_response_headers)(struct wasm_stream_t *stream, uint32_t num_headers, int end_of_stream);
  wasm_bool_t (*on_done)(struct wasm_stream_t *stream);
  void (*on_log)(struct wasm_stream_t *stream);
  void (*on_delete)(struct wasm_stream_t *stream);
  // Note: Other vectors in src/proxy-wasm-cli/proxy_wasm_includes.h::Context are not yet implemented. They can be added on a need-basis
} wasm_host_to_vm_fns_t;


#define WASM_VM_CTX_MAGIC 0xc001d00d
typedef struct wasm_vm_t_ {
  uint32_t magic;
  void *context;
  void *host_context;
  wasm_vm_config_t *config;
} wasm_vm_t;


#define WASM_STREAM_CTX_MAGIC 0xc001babe
typedef struct wasm_stream_t {
  uint32_t magic;
  void *context;
  void *host_context;
  wasm_host_to_vm_fns_t vm_functions;   // Filled by Bridge
  wasm_vm_t *vm;
} wasm_stream_t;


/* Function definitions */
wasm_vm_t *wasm_launch_instance(wasm_vm_config_t *vm, void *host_context);
wasm_stream_t *wasm_create_stream(wasm_vm_t *vm, void *host_context);
wasm_stream_t *wasm_delete_stream(wasm_stream_t *stream);
wasm_err_t process_traffic(wasm_stream_t *stream);

#ifdef __cplusplus
}
#endif


#endif
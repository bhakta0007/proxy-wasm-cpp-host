#include "include/proxy-wasm/wasm_vm.h"

#include <iostream>
#include <string>
#include <errno.h>
#include <stdarg.h>

#include <yaml-cpp/yaml.h>
#include <sys/types.h>
#include "proxy_wasm_includes.h"
#include "proxy_wasm_bridge.h"
#include <unistd.h>
#include <fcntl.h>
#include <chrono>
#include <ctime>
#include <sys/time.h>
#include <cstdint>

namespace proxy_wasm_host
{


void
LogDebug(const char *tag, const char *fmt, ...)
{
  va_list args;

  std::printf("%s", tag);
  va_start(args, fmt);
  std::vprintf(fmt, args);
  va_end(args);
  std::printf("\n");
}

void
LogError(const char *fmt, ...)
{
  va_list args;

  std::printf("LogError:\n");
  va_start(args, fmt);
  std::vprintf(fmt, args);
  va_end(args);
  std::printf("\n");
}

std::string_view toWasmRuntineString(wasm_runtime_e runtime)
{
  switch (runtime) {
    case WASM_RUNTIME_WAMR:
      return "wamr";
    case WASM_RUNTIME_WASMEDGE:
      return "wasmedge";
    case WASM_RUNTIME_WASMTIME:
      return "wasmtime";
    default:
      break;
  }
  return "<invalid>";
}

proxy_wasm::WasmResult toWasmResult(wasm_result_e result)
{
  switch (result) {
  case Ok:
    return proxy_wasm::WasmResult::Ok;
  case NotFound:
    return proxy_wasm::WasmResult::NotFound;
  case BadArgument:
    return proxy_wasm::WasmResult::BadArgument;
  case SerializationFailure:
    return proxy_wasm::WasmResult::SerializationFailure;
  case ParseFailure:
    return proxy_wasm::WasmResult::ParseFailure;
  case BadExpression:
    return proxy_wasm::WasmResult::BadExpression;
  case InvalidMemoryAccess:
    return proxy_wasm::WasmResult::InvalidMemoryAccess;
  case Empty:
    return proxy_wasm::WasmResult::Empty;
  case CasMismatch:
    return proxy_wasm::WasmResult::CasMismatch;
  case ResultMismatch:
    return proxy_wasm::WasmResult::ResultMismatch;
  case InternalFailure:
    return proxy_wasm::WasmResult::InternalFailure;
  case BrokenConnection:
    return proxy_wasm::WasmResult::BrokenConnection;
  case Unimplemented:
    return proxy_wasm::WasmResult::Unimplemented;
  default:
    break;
  }
  return proxy_wasm::WasmResult::Unimplemented;

}

wasm_log_level_e toHostLogLevel(proxy_wasm::LogLevel log_level)
{
  auto ll = WASM_LOG_LEVEL_DEBUG;
  switch (log_level) {
    case proxy_wasm::LogLevel::warn:
      ll = WASM_LOG_LEVEL_WARNING;
      break;
    case proxy_wasm::LogLevel::trace:
      ll = WASM_LOG_LEVEL_TRACE;
      break;
    case proxy_wasm::LogLevel::debug:
      ll = WASM_LOG_LEVEL_DEBUG;
      break;
    case proxy_wasm::LogLevel::info:
      ll = WASM_LOG_LEVEL_INFO;
      break;
    case proxy_wasm::LogLevel::error:
      ll = WASM_LOG_LEVEL_ERROR;
      break;
    case proxy_wasm::LogLevel::critical:
      ll = WASM_LOG_LEVEL_CRITICAL;
      break;
    default:
      break;
  }
  return ll;
}

proxy_wasm::LogLevel toIntegrationLogLevel(wasm_log_level_e log_level)
{
  auto ll = proxy_wasm::LogLevel::debug;
  switch (log_level) {
    case WASM_LOG_LEVEL_WARNING:
      ll = proxy_wasm::LogLevel::warn;
      break;
    case WASM_LOG_LEVEL_TRACE:
      ll = proxy_wasm::LogLevel::trace;
      break;
    case WASM_LOG_LEVEL_DEBUG:
      ll = proxy_wasm::LogLevel::debug;
      break;
    case WASM_LOG_LEVEL_INFO:
      ll = proxy_wasm::LogLevel::info;
      break;
    case WASM_LOG_LEVEL_ERROR:
      ll = proxy_wasm::LogLevel::error;
      break;
    case WASM_LOG_LEVEL_CRITICAL:
      ll = proxy_wasm::LogLevel::critical;
      break;
    default:
      break;
  }
  return ll;
}

// toHostHeaderMapType
wasm_header_map_type_e toHostHeaderMapType(WasmHeaderMapType t)
{
  switch (t) {
    case WasmHeaderMapType::RequestHeaders:
      return WASM_HEADER_MAP_TYPE_REQUEST_HEADERS;
    case WasmHeaderMapType::RequestTrailers:
      return WASM_HEADER_MAP_TYPE_REQUEST_TRAILERS;
    case WasmHeaderMapType::ResponseHeaders:
      return WASM_HEADER_MAP_TYPE_RESPONSE_HEADERS;
    case WasmHeaderMapType::ResponseTrailers:
      return WASM_HEADER_MAP_TYPE_RESPONSE_TRAILERS;
    case WasmHeaderMapType::GrpcReceiveInitialMetadata:
      return WASM_HEADER_MAP_TYPE_GRPC_RECEIVE_INITIAL_METADATA;
    case WasmHeaderMapType::GrpcReceiveTrailingMetadata:
      return WASM_HEADER_MAP_TYPE_GRPC_RECEIVE_TRAILING_METADATA;
    case WasmHeaderMapType::HttpCallResponseHeaders:
      return WASM_HEADER_MAP_TYPE_HTTP_CALL_RESPONSE_HEADERS;
    case WasmHeaderMapType::HttpCallResponseTrailers:
      return WASM_HEADER_MAP_TYPE_HTTP_CALL_RESPONSE_TRAILERS;
    default:
      break;
  }
  return WASM_HEADER_MAP_TYPE_MAX;
}

// toWasmHeaderMapType
WasmHeaderMapType toWasmHeaderMapType(wasm_header_map_type_e t)
{
  switch (t) {
    case WASM_HEADER_MAP_TYPE_REQUEST_HEADERS:
      return WasmHeaderMapType::RequestHeaders;
    case WASM_HEADER_MAP_TYPE_REQUEST_TRAILERS:
      return WasmHeaderMapType::RequestTrailers;
    case WASM_HEADER_MAP_TYPE_RESPONSE_HEADERS:
      return WasmHeaderMapType::ResponseHeaders;
    case WASM_HEADER_MAP_TYPE_RESPONSE_TRAILERS:
      return WasmHeaderMapType::ResponseTrailers;
    case WASM_HEADER_MAP_TYPE_GRPC_RECEIVE_INITIAL_METADATA:
      return WasmHeaderMapType::GrpcReceiveInitialMetadata;
    case WASM_HEADER_MAP_TYPE_GRPC_RECEIVE_TRAILING_METADATA:
      return WasmHeaderMapType::GrpcReceiveTrailingMetadata;
    case WASM_HEADER_MAP_TYPE_HTTP_CALL_RESPONSE_HEADERS:
      return WasmHeaderMapType::HttpCallResponseHeaders;
    case WASM_HEADER_MAP_TYPE_HTTP_CALL_RESPONSE_TRAILERS:
      return WasmHeaderMapType::HttpCallResponseTrailers;
    default:
      break;
  }
  return WasmHeaderMapType::MAX;
}


proxy_wasm::FilterHeadersStatus toWasmFilterHeaderStatus(wasm_filter_header_status_e s)
{
  switch (s) {
    case WASM_FILTER_HEADER_STATUS_CONTINUE:
      return proxy_wasm::FilterHeadersStatus::Continue;
    case WASM_FILTER_HEADER_STATUS_CONTINUE_AND_END_STREAM:
      return proxy_wasm::FilterHeadersStatus::ContinueAndEndStream;
    case WASM_FILTER_HEADER_STATUS_STOP_ALL_ITERATION_AND_BUFFER:
      return proxy_wasm::FilterHeadersStatus::StopAllIterationAndBuffer;
    case WASM_FILTER_HEADER_STATUS_STOP_ALL_ITERATION_AND_WATERMARK:
      return proxy_wasm::FilterHeadersStatus::StopAllIterationAndWatermark;
    case WASM_FILTER_HEADER_STATUS_STOP_ITERATION:
    default:
      return proxy_wasm::FilterHeadersStatus::StopIteration;
  }
}

wasm_filter_header_status_e toHostFilterHeaderStatus(proxy_wasm::FilterHeadersStatus f)
{
  switch (f) {
    case proxy_wasm::FilterHeadersStatus::Continue:
      return WASM_FILTER_HEADER_STATUS_CONTINUE;
    case proxy_wasm::FilterHeadersStatus::ContinueAndEndStream:
      return WASM_FILTER_HEADER_STATUS_CONTINUE_AND_END_STREAM;
    case proxy_wasm::FilterHeadersStatus::StopAllIterationAndBuffer:
      return WASM_FILTER_HEADER_STATUS_STOP_ALL_ITERATION_AND_BUFFER;
    case proxy_wasm::FilterHeadersStatus::StopAllIterationAndWatermark:
      return WASM_FILTER_HEADER_STATUS_STOP_ALL_ITERATION_AND_WATERMARK;
    case proxy_wasm::FilterHeadersStatus::StopIteration:
    default:
      return WASM_FILTER_HEADER_STATUS_STOP_ITERATION;
  }
}



proxy_wasm::WasmResult Context::log(uint32_t log_level, std::string_view message)  {
  wasm_result_e result = Ok;
  if (this->host_functions.log_str) {
    result = this->host_functions.log_str(this->host_ctx, this->isStream, log_level, std::string(message).c_str());
  }
  // auto new_log = std::string(message);
  // std::cout << ANSI_COLOR_RED << " [" << project << ":" << vm_name << "] " << ANSI_COLOR_RESET << new_log << std::endl;
  return toWasmResult(result);
}

// extended constructors to initialize mutex
Wasm::Wasm(const std::shared_ptr<WasmHandleBase> &base_wasm_handle, const WasmVmFactory &factory)
  : WasmBase(base_wasm_handle, factory)
{
}

Wasm::Wasm(std::unique_ptr<WasmVm> wasm_vm, std::string_view vm_id, std::string_view vm_configuration, std::string_view vm_key,
           std::unordered_map<std::string, std::string> envs, AllowedCapabilitiesMap allowed_capabilities)
  : WasmBase(std::move(wasm_vm), vm_id, vm_configuration, vm_key, std::move(envs), std::move(allowed_capabilities))
{
}


// functions to create contexts
ContextBase *
Wasm::createVmContext()
{
  ContextBase *ctx = new Context(this);
  return ctx;
}

ContextBase *
Wasm::createRootContext(const std::shared_ptr<PluginBase> &plugin)
{
  proxy_wasm_host::LogDebug(WASM_DEBUG_TAG, " Create root context.");
  ContextBase *ctx = new Context(this, plugin);
  return ctx;
}

ContextBase *
Wasm::createContext(const std::shared_ptr<PluginBase> &plugin)
{
  ContextBase *ctx = new Context(this, plugin);
  return ctx;
}

void
Wasm::configureHostFunctions(Context *context, wasm_vm_to_host_fns_t *host_fns, void *host_ctx)
{
  context->host_functions.log_str = host_fns->log_str;
  context->host_functions.get_property = host_fns->get_property;
  context->host_functions.error = host_fns->error;
  context->host_functions.add_header_map_value = host_fns->add_header_map_value;
  context->host_functions.get_header_map_value = host_fns->get_header_map_value;
  context->host_functions.get_header_map_pairs = host_fns->get_header_map_pairs;
  context->host_functions.get_current_time_ns = host_fns->get_current_time_ns;
  context->host_functions.get_monitonic_time_ns = host_fns->get_monitonic_time_ns;
  context->host_ctx = host_ctx;
}

// Function to start a new root context
Context *
Wasm::start(const std::shared_ptr<PluginBase> &plugin, wasm_vm_to_host_fns_t *host_fns, void *host_ctx)
{
  auto it = root_contexts_.find(plugin->key());
  proxy_wasm_host::LogDebug(WASM_DEBUG_TAG, "[%s] starting root ctx \"%s\"", __FUNCTION__, plugin->key().c_str());
  if (it != root_contexts_.end()) {
    auto *c = static_cast<Context *>(it->second.get());
    c->onStart(plugin);
    return c;
  }
  auto context      = std::unique_ptr<Context>(static_cast<Context *>(createRootContext(plugin)));
  auto *context_ptr = context.get();
  configureHostFunctions(context_ptr, host_fns, host_ctx);
  root_contexts_[plugin->key()] = std::move(context);
  if (!context_ptr->onStart(plugin)) {
    proxy_wasm_host::LogDebug(WASM_DEBUG_TAG, "[%s] Error calling onStart", __FUNCTION__);
    return nullptr;
  }
  return context_ptr;
}

// functions to manage lifecycle of VM
bool
Wasm::readyShutdown()
{
  // if there is a non-root context, there is an unfinished transaction
  for (const auto &n : contexts_) {
    if (!n.second->isRootContext()) {
      return false;
    }
  }
  // if there is an entry in timer_period_, there is a continuation still running for that root context
  return timer_period_.empty();
}

bool
Wasm::readyDelete()
{
  return (root_contexts_.empty() && pending_done_.empty() && pending_delete_.empty());
}

// functions to manage timer
bool
Wasm::existsTimerPeriod(uint32_t root_context_id)
{
  return timer_period_.find(root_context_id) != timer_period_.end();
}

std::chrono::milliseconds
Wasm::getTimerPeriod(uint32_t root_context_id)
{
  return timer_period_[root_context_id];
}

void
Wasm::removeTimerPeriod(uint32_t root_context_id)
{
  auto it = timer_period_.find(root_context_id);
  if (it != timer_period_.end()) {
    timer_period_.erase(it);
  }
}

// function to override error report
void
Wasm::error(std::string_view message)
{
  proxy_wasm_host::LogError("%.*s", static_cast<int>(message.size()), message.data());
}

void
Context::InitializeHostFunctions() {
  this->host_functions.log_str = NULL;
  this->host_functions.get_property = NULL;
  this->host_functions.error = NULL;
  this->host_functions.add_header_map_value = NULL;
  this->host_functions.get_header_map_value = NULL;
  this->host_functions.get_header_map_pairs = NULL;
  this->host_functions.get_current_time_ns = NULL;
  this->host_functions.get_monitonic_time_ns = NULL;
}

Context::Context() : ContextBase() {
  InitializeHostFunctions();
}

Context::Context(Wasm *wasm) : ContextBase(wasm) {
  InitializeHostFunctions();
}

Context::Context(Wasm *wasm, const std::shared_ptr<PluginBase> &plugin) : ContextBase(wasm, plugin) {
  InitializeHostFunctions();
}

// NB: wasm can be nullptr if it failed to be created successfully.
Context::Context(Wasm *wasm, Context *root_context, const std::shared_ptr<PluginBase> &plugin,
                std::string project, std::string vm_name, void *host_ctx, bool isStream) : ContextBase(wasm)
{
  id_                = (wasm_ != nullptr) ? wasm_->allocContextId() : 0;
  parent_context_id_ = root_context->id();
  plugin_            = plugin;
  if (wasm_ != nullptr) {
    parent_context_ = wasm_->getContext(parent_context_id_);
  }
  this->project = project;
  this->vm_name = vm_name;
  InitializeHostFunctions();
  if (root_context != NULL) {
    this->host_functions.log_str = root_context->host_functions.log_str;
    this->host_functions.get_property = root_context->host_functions.get_property;
    this->host_functions.add_header_map_value = root_context->host_functions.add_header_map_value;
    this->host_functions.get_header_map_value = root_context->host_functions.get_header_map_value;
    this->host_functions.get_header_map_pairs = root_context->host_functions.get_header_map_pairs;
    this->host_functions.get_current_time_ns = root_context->host_functions.get_current_time_ns;
    this->host_functions.get_monitonic_time_ns = root_context->host_functions.get_monitonic_time_ns;
    this->host_functions.error = root_context->host_functions.error;
  }
  this->host_ctx = host_ctx;
  this->isStream = isStream;
}

void Context::setHeaders(std::unordered_map<std::string, std::string> headers,
                        bool forward_direction, bool in)
{
  if (in) {
    this->in_headers = headers;
  } else {
    this->out_headers = headers;
  }
  this->forward_direction = forward_direction;
}

// utility functions for the extended class
Wasm *
Context::wasm() const
{
  return static_cast<Wasm *>(wasm_);
}

Context *
Context::parent_context() const
{
  return static_cast<Context *>(parent_context_);
}

Context *
Context::root_context() const
{
  const ContextBase *previous = this;
  ContextBase *parent         = parent_context_;
  while (parent != previous) {
    previous = parent;
    parent   = parent->parent_context();
  }
  return static_cast<Context *>(parent);
}

void
Context::error(std::string_view message)
{
  LogError("%.*s", static_cast<int>(message.size()), message.data());
  // abort();
  if (this->host_functions.error) {
    this->host_functions.error(this->host_ctx, this->isStream,
                              std::string(message).c_str());
  }
}

// local reply handler
void
Context::onLocalReply() {}


// Properties
WasmResult
Context::getProperty(std::string_view path, std::string *result)
{
#if 1
  char *tmpStr = NULL;
  wasm_result_e res;
  if (this->host_functions.get_property) {
    res = this->host_functions.get_property(this->host_ctx, this->isStream,
                                            std::string(path).c_str(),
                                            path.size(), &tmpStr);
    if (res == Ok && tmpStr != NULL) {
      *result = std::string(tmpStr);
    }
    return toWasmResult(res);
  }
  return unimplemented();
#else
  if (path.substr(0, p_plugin_root_id.size()) == p_plugin_root_id) {
    *result = this->plugin_->root_id_;
    LogDebug(WASM_DEBUG_TAG, "[%s] looking for plugin_root_id: %.*s", __FUNCTION__,
            static_cast<int>((*result).size()), (*result).data());
    return WasmResult::Ok;
  } else if (path.substr(0, p_source_address.size()) == p_source_address) {
    *result = "100.0.0.1";
    return WasmResult::Ok;
  } else if (path.substr(0, p_source_port.size()) == p_source_port) {
    *result = "31337";
    return WasmResult::Ok;
  } else {
    std::cout << "GetProperty FIXME: " << path << std::endl;
    return WasmResult::NotFound;
  }
#endif
}

// addHeaderMapValue
WasmResult
Context::addHeaderMapValue(WasmHeaderMapType type, std::string_view key, std::string_view value)
{
#if 1

  wasm_result_e res;
  std::cout << "Add header map values for " << (int)type << " key " << key << " val " << value << std::endl;
  if (this->host_functions.add_header_map_value) {
    res = this->host_functions.add_header_map_value(this->host_ctx, this->isStream,
                                            toHostHeaderMapType(type),
                                            std::string(key).c_str(),
                                            key.size(),
                                            std::string(value).c_str());
    return toWasmResult(res);
  }
  return unimplemented();
#else
  std::unordered_map<std::string, std::string> headers;
  std::cout << "addHeaderMapValue " << key.data() << " = " << value.data() << std::endl;
  headers[key.data()] = value.data();
  this->setHeaders(headers, false, false);
  return WasmResult::Ok;
#endif
}

uint64_t
Context::getCurrentTimeNanoseconds()
{
#if 1
  return this->host_functions.get_current_time_ns();
#else
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
              std::chrono::system_clock::now().time_since_epoch())
      .count();
#endif
}

uint64_t
Context::getMonotonicTimeNanoseconds()
{
#if 1
  return this->host_functions.get_monitonic_time_ns();
#else
  return std::chrono::duration_cast<std::chrono::nanoseconds>(
              std::chrono::system_clock::now().time_since_epoch())
      .count();
#endif
}

WasmResult
Context::getHeaderMapValue(WasmHeaderMapType type, std::string_view key, std::string_view *result)
{
#if 1
  char *tmpStr = NULL;
  wasm_result_e res;
  std::cout << "Get header map values for " << (int)type << " key " << key << std::endl;
  if (this->host_functions.get_header_map_value) {
    res = this->host_functions.get_header_map_value(this->host_ctx, this->isStream,
                                            toHostHeaderMapType(type),
                                            std::string(key).c_str(),
                                            key.size(),
                                            &tmpStr);
    if (res == Ok && tmpStr != NULL) {
      *result = std::string_view(tmpStr);
      std::cout << "Got header = " << key << " value => " << *result << std::endl;
    }
    if (1) {
      Pairs pairs;
      std::cout << "Call getHeader Map Pairs from getHeaderMapValue" << std::endl;
      getHeaderMapPairs(type, &pairs);
    }
    return toWasmResult(res);
  }
  return unimplemented();
#else
  for (const auto& x: this->in_headers){
    std::cout << "Walk key " << x.first << " val " << x.second << std::endl;
    if (x.first == key.data()) {
      *result = x.second;
      break;
    }
  }
  std::cout << "getHeaderValue" << static_cast<int>(type) << " key " << key.data() << " = " << *result << std::endl;
  return WasmResult::Ok;
#endif
}


WasmResult
Context::getHeaderMapPairs(WasmHeaderMapType type, Pairs *result)
{
#if 1
  wasm_kv_list_t *kvList = NULL, *ptr = NULL;
  int kvSize;
  wasm_result_e res;
  std::cout << "Get header map values for " << (int)type << std::endl;
  if (this->host_functions.get_header_map_pairs) {
    res = this->host_functions.get_header_map_pairs(this->host_ctx, this->isStream,
                                            toHostHeaderMapType(type),
                                            &kvList);
    if (res == Ok && kvList != NULL) {
      ptr = kvList;
      if (ptr != NULL) {
        // Need to reserve the length of the list
        kvSize = 0;
        while (ptr) {
          kvSize++;
          ptr = ptr->next;
        }
        ptr = kvList;
        result->reserve(kvSize);
        while (ptr != NULL) {
          // printf("push kv pair %s : %s\n", ptr->key, ptr->value);
          result->push_back(std::make_pair(std::string_view(ptr->key),  std::string_view(ptr->value)));
          ptr = ptr->next;
        }
      }
    }
    return toWasmResult(res);
  }
  return unimplemented();
#else
  bool valid = false;
  switch (type) {
  case WasmHeaderMapType::RequestHeaders:
  case WasmHeaderMapType::ResponseHeaders:
    if (this->in_headers.size()) {
      if (type == WasmHeaderMapType::RequestHeaders && this->forward_direction) {
        valid = true;
      }
      if (type == WasmHeaderMapType::ResponseHeaders && !this->forward_direction) {
        valid = true;
      }
      if (valid) {
        result->reserve(this->in_headers.size());
        for (const auto &x : this->in_headers) {
          result->push_back(std::make_pair(std::string_view(x.first),  std::string_view(x.second)));
        }
      }
    }
    break;
  case WasmHeaderMapType::RequestTrailers:
  case WasmHeaderMapType::ResponseTrailers:
  case WasmHeaderMapType::GrpcReceiveTrailingMetadata:
  case WasmHeaderMapType::GrpcReceiveInitialMetadata:
  case WasmHeaderMapType::HttpCallResponseHeaders:
  case WasmHeaderMapType::HttpCallResponseTrailers:
    return {};
  }
  return WasmResult::Ok;
#endif
}

// BHAKTA: FIXME:
// ProxyWasmVmIntegration is suposed to hook to the host leve. For e.g. when getLogLevel is
// called, this is supposed to check the host configuration for the WASM VM and return the
// corresponding log Level.
// TODO: Need to have Host provide function vectors that will get called


// extended Wasm VM Integration object
proxy_wasm::LogLevel
ProxyWasmVmIntegration::getLogLevel()
{
  return toIntegrationLogLevel(integration_fns.get_log_level());
}

void
ProxyWasmVmIntegration::setLogLevel(proxy_wasm::LogLevel level)
{
  log_level = level;
  integration_fns.set_log_level(toHostLogLevel(level));
}

void
ProxyWasmVmIntegration::set_integration_functions(wasm_integration_fns_t *fns)
{
  if (fns->get_log_level != NULL) {
    this->integration_fns.get_log_level = fns->get_log_level;
  } else {
    this->integration_fns.get_log_level = NULL;
  }

  if (fns->set_log_level != NULL) {
    this->integration_fns.set_log_level = fns->set_log_level;
  } else {
    this->integration_fns.set_log_level = NULL;
  }

  if (fns->error != NULL) {
    this->integration_fns.error = fns->error;
  } else {
    this->integration_fns.error = NULL;
  }

  if (fns->trace != NULL) {
    this->integration_fns.trace = fns->trace;
  } else {
    this->integration_fns.trace = NULL;
  }
}

void
ProxyWasmVmIntegration::error(std::string_view message)
{
  if (integration_fns.error) {
    integration_fns.error(std::string(message).c_str());
  }
  // LogError("%.*s", static_cast<int>(message.size()), message.data());
}

void
ProxyWasmVmIntegration::trace(std::string_view message)
{
  if (integration_fns.trace) {
    integration_fns.trace(std::string(message).c_str());
  }
  // LogDebug(WASM_DEBUG_TAG, "trace -> %.*s", static_cast<int>(message.size()), message.data());
  // std::cout << "trace - " << message.data() << std::endl;
  // std::printf("trace -> %.*s\n", static_cast<int>(message.size()), message.data());
}

bool
ProxyWasmVmIntegration::getNullVmFunction(std::string_view function_name, bool returns_word,
                                          int number_of_arguments, proxy_wasm::NullPlugin *plugin,
                                          void *ptr_to_function_return)
{
  return false;
}

} // namespace proxy_wasm_host


// function to read a file
static inline int
read_file(const std::string &fn, std::string *s)
{
  auto fd = open(fn.c_str(), O_RDONLY);
  if (fd < 0) {
    char *errmsg = strerror(errno);
    proxy_wasm_host::LogError("[wasm][%s] wasm unable to open: %s", __FUNCTION__, errmsg);
    return -1;
  }
  auto n = ::lseek(fd, 0, SEEK_END);
  if (n < 0) {
    char *errmsg = strerror(errno);
    proxy_wasm_host::LogError("[wasm][%s] wasm unable to lseek: %s", __FUNCTION__, errmsg);
    return -1;
  }
  ::lseek(fd, 0, SEEK_SET);
  s->resize(n);
  auto nn = ::read(fd, const_cast<char *>(&*s->begin()), n);
  if (nn < 0) {
    char *errmsg = strerror(errno);
    proxy_wasm_host::LogError("[wasm][%s] wasm unable to read: %s", __FUNCTION__, errmsg);
    return -1;
  }
  if (nn != static_cast<ssize_t>(n)) {
    proxy_wasm_host::LogError("[wasm][%s] wasm unable to read: size different from buffer",
                              __FUNCTION__);
    return -1;
  }
  return 0;
}

class WasmManager;

class WasmVmInfo
{
  public:
    WasmVmInfo() {}
    WasmVmInfo(std::string project, std::string vmName, std::shared_ptr<proxy_wasm_host::Wasm> wasm,
              uint64_t ts, std::shared_ptr<proxy_wasm::PluginBase> plugin, WasmManager *manager)
    {
      this->project = project;
      this->name = vmName;
      this->wasm = wasm;
      this->ts = ts;
      this->plugin = plugin;
      this->manager = manager;
    }

    void SetProject(std::string project) { this->project = project;}
    void SetName(std::string name) { this->name = name;}
    void SetWasm(std::shared_ptr<proxy_wasm_host::Wasm> wasm) { this->wasm = wasm;}
    // void SetTs(::google::protobuf::Timestamp *ts) { this->ts = ts;}
    void SetTs(uint64_t ts) { this->ts = ts;}
    void SetPlugin(std::shared_ptr<proxy_wasm::PluginBase> plugin) { this->plugin = plugin;}
    void SetRootContext(proxy_wasm_host::Context *context) { this->rootContext = context;}

    // ::google::protobuf::Timestamp *ts;
    uint64_t ts;
    std::string project;
    std::string name;
    std::shared_ptr<proxy_wasm_host::Wasm> wasm;
    std::shared_ptr<proxy_wasm::PluginBase> plugin;
    proxy_wasm_host::Context *rootContext;
    WasmManager *manager;
};

class WasmManager
{
    /**
     * The WasmManager's constructor/destructor should always be private to
     * prevent direct construction/desctruction calls with the `new`/`delete`
     * operator.
     */
private:
    static WasmManager * pinstance_;
    static std::mutex mutex_;
    std::unordered_map<std::string, WasmVmInfo*> _wasmVms;

protected:
    WasmManager() {}
    ~WasmManager() {}
    std::shared_ptr<proxy_wasm_host::Wasm> LaunchVm(WasmVmInfo *vm,
                                                    wasm_vm_config_t *cfg,
                                                    void *host_ctx,
                                                    WasmManager *manager);

public:
    /**
     * WasmManagers should not be cloneable.
     */
    WasmManager(WasmManager &other) = delete;
    /**
     * WasmManagers should not be assignable.
     */
    void operator=(const WasmManager &) = delete;
    /**
     * This is the static method that controls the access to the WasmManager
     * instance. On the first run, it creates a WasmManager object and places it
     * into the static field. On subsequent runs, it returns the client existing
     * object stored in the static field.
     */


//  WasmVmInfo* NewWasmVM(wasm_runtime_e runtime, std::string root_id, std::string vm_name,
//                           std::string wasmFile, proxy_wasm::LogLevel logLevel)
    static WasmManager *GetInstance();
    /**
     * Finally, any WasmManager should define some business logic, which can be
     * executed on its instance.
     */
    WasmVmInfo* NewWasmVM(wasm_vm_config_t *cfg, void *host_ctx)
    {
        std::shared_ptr<proxy_wasm_host::Wasm> wasm;
        std::string key = std::string(cfg->project_name) + ":" + std::string(cfg->vm_name);
        bool found = false;
        for (const auto& x:_wasmVms){
          std::cout << x.first << std::endl;
          if (x.first == key) {
            found = true;
            break;
          }
        }
        if (!found) {
          // time_t current_time = time(NULL);
          auto vm = new WasmVmInfo();

          wasm = LaunchVm(vm, cfg, host_ctx, this);
          std::cout << "Vm with key " << key << " not found. launched with root_id " << cfg->project_name <<
                      " vm name " << cfg->vm_name << " wasm file " << cfg->code.file << "vm info name "
                      << vm->name << " proj = " << vm->project << std::endl;
          _wasmVms[key] = vm;
        } else {
          std::cout << "Vm with key " << key << " found!" << std::endl;
        }
        return _wasmVms[key];
    }
    std::unordered_map<std::string, WasmVmInfo*> ListVms() {
      return _wasmVms;
    }
    std::vector<WasmVmInfo*> FilterVms(std::string projectName, std::string vmName) {
      std::vector<WasmVmInfo*> ret;
      for (const auto& x:_wasmVms){
        if (vmName.length()) {
            if (vmName == x.second->name) {
              ret.push_back(x.second);
            }
        } else {
          ret.push_back(x.second);
        }
      }
      return ret;
    }

};

/**
 * Static methods should be defined outside the class.
 */

WasmManager* WasmManager::pinstance_{nullptr};
std::mutex WasmManager::mutex_;
WasmManager *WasmManager::GetInstance()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (pinstance_ == nullptr)
    {
        pinstance_ = new WasmManager();
    }
    return pinstance_;
}

wasm_err_t get_code(wasm_vm_config_t *cfg, std::string *code)
{
  if (cfg->code_type == WASM_CODE_TYPE_FILE) {
    if (read_file(cfg->code.file, code) < 0) {
      proxy_wasm_host::LogError("[wasm][%s] wasm unable to read file '%s'", __FUNCTION__, cfg->code.file);
      return WASM_ERR_INVALID_CODE;
    }
  } else if (cfg->code_type == WASM_CODE_TYPE_DATA) {
    *code = std::string((char *)cfg->code.data);
    return WASM_ERR_INVALID_CODE;
  }
  return WASM_ERR_NONE;
}

std::shared_ptr<proxy_wasm_host::Wasm> WasmManager::LaunchVm(WasmVmInfo *vm,
                                                            wasm_vm_config_t *cfg,
                                                            void *host_ctx,
                                                            WasmManager *manager)
{
  std::string name = "test";
  std::string configuration = "";
  bool fail_open            = true;
  std::string wasm_filename = "/opt/lilac/wasm/myproject.wasm";
  std::string vm_configuration = "";
  std::unordered_map<std::string, std::string> envs;
  bool allow_precompiled       = true;
  proxy_wasm::AllowedCapabilitiesMap cap_maps;
  std::string vm_id = "test-vm";
  std::shared_ptr<proxy_wasm_host::Wasm> wasm = nullptr;
  auto runtime_str = proxy_wasm_host::toWasmRuntineString(cfg->runtime);
  std::cout << "Launching with " << runtime_str << std::endl;
  if (cfg->runtime == WASM_RUNTIME_WASMEDGE) {
    std::cout << "Created wasm Edge VM" << std::endl;
    wasm = std::make_shared<proxy_wasm_host::Wasm>(proxy_wasm::createWasmEdgeVm(), // VM
                                              cfg->vm_name,                          // vm_id
                                              vm_configuration,               // vm_configuration
                                              "",                             // vm_key,
                                              envs,                           // envs
                                              cap_maps                        // allowed capabilities
      );
  } else if (cfg->runtime == WASM_RUNTIME_WASMTIME) {
    std::cout << "Created wasmtime VM" << std::endl;
    wasm = std::make_shared<proxy_wasm_host::Wasm>(proxy_wasm::createWasmtimeVm(), // VM
                                              cfg->vm_name,                          // vm_id
                                              vm_configuration,               // vm_configuration
                                              "",                             // vm_key,
                                              envs,                           // envs
                                              cap_maps                        // allowed capabilities
      );
  } else if (cfg->runtime == WASM_RUNTIME_WAMR) {
    std::cout << "Created wamr VM" << std::endl;
    wasm = std::make_shared<proxy_wasm_host::Wasm>(proxy_wasm::createWamrVm(), // VM
                                              cfg->vm_name,                          // vm_id
                                              vm_configuration,               // vm_configuration
                                              "",                             // vm_key,
                                              envs,                           // envs
                                              cap_maps                        // allowed capabilities
      );
  } else {
    std::cout << "Unsupported runtime " << cfg->runtime << std::endl;
    return wasm;
  }
  wasm->wasm_vm()->integration() = std::make_unique<proxy_wasm_host::ProxyWasmVmIntegration>();
  auto *pvm = dynamic_cast<proxy_wasm_host::ProxyWasmVmIntegration *>(wasm->wasm_vm()->integration().get());
  wasm_integration_fns_t fns;
  fns.set_log_level = cfg->integration_functions.set_log_level;
  fns.get_log_level = cfg->integration_functions.get_log_level;
  pvm->set_integration_functions(&fns);
  proxy_wasm::LogLevel logLevel = proxy_wasm_host::toIntegrationLogLevel(cfg->log_level);
  pvm->setLogLevel(logLevel);
  auto plugin = std::make_shared<proxy_wasm::PluginBase>(cfg->vm_name,          // name
                                                        cfg->project_name,       // root_id
                                                        cfg->vm_name,         // vm_id
                                                        runtime_str,       // engine
                                                        configuration, // plugin_configuration
                                                        fail_open,     // failopen
                                                        ""             // TODO: plugin key from where ?
  );

  std::string code;

  if (get_code(cfg, &code) != WASM_ERR_NONE) {
    proxy_wasm_host::LogError("[wasm][%s] code could not be read", __FUNCTION__);
    return wasm;
  }

  if (code.empty()) {
    proxy_wasm_host::LogError("[wasm][%s] code is empty", __FUNCTION__);
    return wasm;
  }

  if (!wasm->load(code, allow_precompiled)) {
    proxy_wasm_host::LogError("[wasm][%s] Failed to load Wasm code", __FUNCTION__);
    return wasm;
  }

  if (!wasm->initialize()) {
    proxy_wasm_host::LogError("[wasm][%s] Failed to initialize Wasm code", __FUNCTION__);
    return wasm;
  }
  proxy_wasm_host::LogDebug(WASM_DEBUG_TAG, "Wasm Code %s loaded", cfg->code.file);

  // wasm_vm_to_host_fns_t host_fns;

  // host_fns.log = cfg->host_functions.log;
  // host_fns.log_str = cfg->host_functions.log_str;
  // printf("Got log_str %#x\n", host_fns.log_str);
  auto *rootContext = wasm->start(plugin,  &cfg->host_functions, host_ctx);

  if (!wasm->configure(rootContext, plugin)) {
    proxy_wasm_host::LogError("[wasm][%s] Failed to configure Wasm", __FUNCTION__);
    return wasm;
  }
  proxy_wasm_host::LogDebug(WASM_DEBUG_TAG, "Wasm %s configured", cfg->code.file);

  vm->SetProject(cfg->project_name);
  vm->SetName(cfg->vm_name);
  vm->SetWasm(wasm);
  vm->SetRootContext(rootContext);
  // ::google::protobuf::Timestamp* ts = new ::google::protobuf::Timestamp();
  // gettimeofday(&tv, NULL);
  // ts->set_seconds(tv.tv_sec);
  // ts->set_nanos(tv.tv_usec * 1000);
  vm->SetTs(0);
  vm->SetPlugin(plugin);
  return wasm;
}

bool ProcessTraffic(WasmVmInfo* vm, wasm_stream_t *stream) {
  int count = 1;
  bool forward_direction = true;
  proxy_wasm_host::Context *context = (proxy_wasm_host::Context *)stream->context;
  // auto *context = new proxy_wasm_host::Context(vm->wasm.get(), vm->rootContext, pl,
  //                                             vm->project, vm->name, static_cast<void *>(stream), true);
  // bool forward_direction = true;
  // context->onCreate();
  std::cout << "ProcessTraffic header count" << count << " forward = " << forward_direction << std::endl;
  if (count) {
    std::unordered_map<std::string, std::string> headers;
    headers["k1"] = "v1";
    headers["k2"] = "v2";
    headers["User-Agent"] = "Shell";
    context->setHeaders(headers, forward_direction, true);
    proxy_wasm::FilterHeadersStatus status;
    if (forward_direction) {
      status = context->onRequestHeaders(count, false);
    } else {
      status = context->onResponseHeaders(count, false);
    }
    if (static_cast<uint64_t>(status) != static_cast<uint64_t>(proxy_wasm::FilterHeadersStatus::Continue)) {
      proxy_wasm_host::LogDebug(WASM_DEBUG_TAG, "onRequestHeaders returned with = %d. Skip the request", status);
      return false;
    }
    proxy_wasm_host::LogDebug(WASM_DEBUG_TAG, "called %s with ret = %d", forward_direction ? \
                              "onRequestHeaders" : "onResponseHeaders", uint64_t(status));
  }
  return true;
}

extern "C" {

wasm_filter_header_status_e handle_on_request_headers (wasm_stream_t *stream, uint32_t num_headers, int end_of_stream)
{
  std::cout << "handle_on_request_headers : " << num_headers << std::endl;
  proxy_wasm_host::Context *context = reinterpret_cast<proxy_wasm_host::Context *> (stream->context);
  auto status = context->onRequestHeaders(num_headers, end_of_stream);
  return proxy_wasm_host::toHostFilterHeaderStatus(status);
}

wasm_filter_header_status_e handle_on_response_headers (wasm_stream_t *stream, uint32_t num_headers, int end_of_stream)
{
  std::cout << "handle_on_response_headers : " << num_headers << std::endl;
  proxy_wasm_host::Context *context = reinterpret_cast<proxy_wasm_host::Context *> (stream->context);
  auto status = context->onResponseHeaders(num_headers, end_of_stream);
  return proxy_wasm_host::toHostFilterHeaderStatus(status);
}

wasm_bool_t handle_on_done (wasm_stream_t *stream)
{
  std::cout << "handle_on_done : " << std::endl;
  proxy_wasm_host::Context *context = reinterpret_cast<proxy_wasm_host::Context *> (stream->context);
  auto status = context->onDone();
  return status ? WASM_TRUE : WASM_FALSE;
}

void handle_on_log (wasm_stream_t *stream)
{
  std::cout << "handle_on_log : " << std::endl;
  proxy_wasm_host::Context *context = reinterpret_cast<proxy_wasm_host::Context *> (stream->context);
  context->onLog();
}

void handle_on_delete(wasm_stream_t *stream)
{
  std::cout << "handle_on_delete : " << std::endl;
  proxy_wasm_host::Context *context = reinterpret_cast<proxy_wasm_host::Context *> (stream->context);
  context->onDelete();
}

// setup_stream_vm_functions
void setup_stream_vm_functions(wasm_stream_t *stream)
{
  stream->vm_functions.on_request_headers = handle_on_request_headers;
  stream->vm_functions.on_response_headers = handle_on_response_headers;
  stream->vm_functions.on_done = handle_on_done;
  stream->vm_functions.on_log = handle_on_log;
  stream->vm_functions.on_delete = handle_on_delete;
}

// wasm_launch_instance: Launch a VM instance using the provided configuration
// Return: Pointer to wasm_vm_t on success, NULL on failure.
wasm_vm_t *wasm_launch_instance(wasm_vm_config_t *cfg, void *host_context)
{
  // validate the config
  if (cfg == NULL) {
    std::cout << "Invalid argument - Missing config" << std::endl;
    return NULL;
  }
  if (strlen(cfg->project_name) == 0) {
    std::cout << "Invalid argument - project_name" << std::endl;
    return NULL;
  }
  if (strlen(cfg->vm_name) == 0) {
    std::cout << "Invalid argument - vm_name" << std::endl;
    return NULL;
  }
  if (cfg->runtime == WASM_RUNTIME_INVALID) {
    std::cout << "Invalid argument - runtime " << cfg->runtime << std::endl;
    return NULL;
  }
  if (cfg->code_type != WASM_CODE_TYPE_DATA && cfg->code_type != WASM_CODE_TYPE_FILE) {
    std::cout << "Invalid argument - code_type" << std::endl;
    return NULL;
  }
  wasm_vm_t *vm = new wasm_vm_t;
  vm->magic = WASM_VM_CTX_MAGIC;
  vm->config = cfg;
  WasmManager* wm = WasmManager::GetInstance();
  WasmVmInfo *vmInfo = wm->NewWasmVM(cfg, static_cast<void *>(vm));
  vm->context = reinterpret_cast<void *> (vmInfo);
  vm->host_context = host_context;
  std::cout << "Launch Instance success" << std::endl;
  return vm;
}

/*
 * wasm_create_stream
 * Create a stream structure: Should be invoked when a stream arrives
 */
wasm_stream_t *wasm_create_stream(wasm_vm_t *vm, void *host_ctx)
{
  wasm_stream_t *stream = new wasm_stream_t;
  stream->magic = WASM_STREAM_CTX_MAGIC;
  stream->vm = vm;
  stream->host_context = host_ctx;
  setup_stream_vm_functions(stream);
  WasmVmInfo *vmInfo = reinterpret_cast<WasmVmInfo *> (vm->context);
  std::shared_ptr<proxy_wasm::PluginBase> &pl = vmInfo->plugin;
  auto *context = new proxy_wasm_host::Context(vmInfo->wasm.get(), vmInfo->rootContext, pl,
                                              vmInfo->project, vmInfo->name, static_cast<void *>(stream),
                                              true);
  stream->context = (void *)context;
  printf("create stream %p, vm %p magic %#x\n", stream, stream->vm, stream->vm->magic);
  context->onCreate();
  return stream;
}

/*
 * wasm_delete_stream
 * Delete a stream from the bridge - and cleans up all data structures
 */
wasm_stream_t *wasm_delete_stream(wasm_stream_t *stream)
{
  proxy_wasm_host::Context *context = (proxy_wasm_host::Context *)stream->context;
  delete context;
  delete stream;
}

wasm_err_t process_traffic(wasm_stream_t *stream)
{
  if (stream == NULL) {
    return WASM_ERR_INVALID_ARGS;
  }
  WasmVmInfo *vmInfo = reinterpret_cast<WasmVmInfo *> (stream->vm->context);
  auto proceed = ProcessTraffic(vmInfo, stream);
  if (!proceed) {
    std::cout << "VM - " << vmInfo->project << ":" <<vmInfo->name << \
                 " did not return Ok. Stop further processing" << std::endl;
  }
  return WASM_ERR_NONE;
}

}
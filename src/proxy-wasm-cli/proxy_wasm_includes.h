/*
  Licensed to the Apache Software Foundation (ASF) under one
  or more contributor license agreements.  See the NOTICE file
  distributed with this work for additional information
  regarding copyright ownership.  The ASF licenses this file
  to you under the Apache License, Version 2.0 (the
  "License"); you may not use this file except in compliance
  with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#pragma once

#include <tuple>
#include "include/proxy-wasm/wasm.h"
#include "include/proxy-wasm/wasmedge.h"
#include "include/proxy-wasm/wasmtime.h"
#include "include/proxy-wasm/wamr.h"
#include "src/shared_data.h"
#include "src/shared_queue.h"
namespace proxy_wasm_host
{
using proxy_wasm::ContextBase;
using proxy_wasm::PluginBase;

using proxy_wasm::WasmBase;
using proxy_wasm::WasmVm;
using proxy_wasm::WasmHandleBase;
using proxy_wasm::WasmVmFactory;
using proxy_wasm::AllowedCapabilitiesMap;
using proxy_wasm::WasmVmIntegration;
using proxy_wasm::WasmResult;
using proxy_wasm::BufferInterface;
using proxy_wasm::BufferBase;
using proxy_wasm::WasmHeaderMapType;
using proxy_wasm::Pairs;
using proxy_wasm::GrpcStatusCode;
using proxy_wasm::WasmBufferType;
using proxy_wasm::LogLevel;
using proxy_wasm::MetricType;

class Wasm;

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
// auto colorT = std::make_tuple(ANSI_COLOR_RED, ANSI_COLOR_GREEN, ANSI_COLOR_YELLOW, ANSI_COLOR_BLUE, ANSI_COLOR_MAGENTA, ANSI_COLOR_CYAN);


// constants for property names
constexpr std::string_view p_request_path                         = {"request\0path", 12};
constexpr std::string_view p_request_url_path                     = {"request\0url_path", 16};
constexpr std::string_view p_request_host                         = {"request\0host", 12};
constexpr std::string_view p_request_scheme                       = {"request\0scheme", 14};
constexpr std::string_view p_request_method                       = {"request\0method", 14};
constexpr std::string_view p_request_headers                      = {"request\0headers", 15};
constexpr std::string_view p_request_referer                      = {"request\0referer", 15};
constexpr std::string_view p_request_useragent                    = {"request\0useragent", 17};
constexpr std::string_view p_request_time                         = {"request\0time", 12};
constexpr std::string_view p_request_id                           = {"request\0id", 10};
constexpr std::string_view p_request_protocol                     = {"request\0protocol", 16};
constexpr std::string_view p_request_query                        = {"request\0query", 13};
constexpr std::string_view p_request_duration                     = {"request\0duration", 16};
constexpr std::string_view p_request_size                         = {"request\0size", 12};
constexpr std::string_view p_request_total_size                   = {"request\0total_size", 18};
constexpr std::string_view p_response_code                        = {"response\0code", 13};
constexpr std::string_view p_response_code_details                = {"response\0code_details", 21};
constexpr std::string_view p_response_headers                     = {"response\0headers", 16};
constexpr std::string_view p_response_size                        = {"response\0size", 13};
constexpr std::string_view p_response_total_size                  = {"response\0total_size", 19};
constexpr std::string_view p_node                                 = {"node", 4};
constexpr std::string_view p_plugin_name                          = {"plugin_name", 11};
constexpr std::string_view p_plugin_root_id                       = {"plugin_root_id", 14};
constexpr std::string_view p_plugin_vm_id                         = {"plugin_vm_id", 12};
constexpr std::string_view p_source_address                       = {"source\0address", 14};
constexpr std::string_view p_source_port                          = {"source\0port", 11};
constexpr std::string_view p_destination_address                  = {"destination\0address", 19};
constexpr std::string_view p_destination_port                     = {"destination\0port", 16};
constexpr std::string_view p_connection_mtls                      = {"connection\0mtls", 15};
constexpr std::string_view p_connection_requested_server_name     = {"connection\0requested_server_name", 32};
constexpr std::string_view p_connection_tls_version               = {"connection\0tls_version", 22};
constexpr std::string_view p_connection_subject_local_certificate = {"connection\0subject_local_certificate", 36};
constexpr std::string_view p_connection_subject_peer_certificate  = {"connection\0subject_peer_certificate", 35};
constexpr std::string_view p_connection_dns_san_local_certificate = {"connection\0dns_san_local_certificate", 36};
constexpr std::string_view p_connection_dns_san_peer_certificate  = {"connection\0dns_san_peer_certificate", 35};
constexpr std::string_view p_connection_uri_san_local_certificate = {"connection\0uri_san_local_certificate", 36};
constexpr std::string_view p_connection_uri_san_peer_certificate  = {"connection\0uri_san_peer_certificate", 35};
constexpr std::string_view p_upstream_address                     = {"upstream\0address", 16};
constexpr std::string_view p_upstream_port                        = {"upstream\0port", 13};
constexpr std::string_view p_upstream_local_address               = {"upstream\0local_address", 22};
constexpr std::string_view p_upstream_local_port                  = {"upstream\0local_port", 19};
constexpr std::string_view p_upstream_tls_version                 = {"upstream\0tls_version", 20};
constexpr std::string_view p_upstream_subject_local_certificate   = {"upstream\0subject_local_certificate", 35};
constexpr std::string_view p_upstream_subject_peer_certificate    = {"upstream\0subject_peer_certificate", 34};
constexpr std::string_view p_upstream_dns_san_local_certificate   = {"upstream\0dns_san_local_certificate", 35};
constexpr std::string_view p_upstream_dns_san_peer_certificate    = {"upstream\0dns_san_peer_certificate", 34};
constexpr std::string_view p_upstream_uri_san_local_certificate   = {"upstream\0uri_san_local_certificate", 35};
constexpr std::string_view p_upstream_uri_san_peer_certificate    = {"upstream\0uri_san_peer_certificate", 34};

// constants for property values
constexpr std::string_view pv_http2  = {"HTTP/2", 6};
constexpr std::string_view pv_http10 = {"HTTP/1.0", 8};
constexpr std::string_view pv_http11 = {"HTTP/1.1", 8};
constexpr std::string_view pv_empty  = {"", 0};

// local struct representing the transaction header
struct HeaderMap {

  ~HeaderMap()
  {
  }

  int
  size() const
  {
    return 0;
  }
};

class Context : public ContextBase
{
public:
  // constructors for the extend class
  Context();
  Context(Wasm *wasm);
  Context(Wasm *wasm, const std::shared_ptr<PluginBase> &plugin);
  Context(Wasm *wasm, uint32_t parent_context_id, const std::shared_ptr<PluginBase> &plugin, std::string project, std::string vm_name);

  // extend class utility functions
  Wasm *wasm() const;
  Context *parent_context() const;
  Context *root_context() const;

  void error(std::string_view message) override;

  // local reply handler
  void onLocalReply();

  //
  // General Callbacks.
  //
  proxy_wasm::WasmResult log(uint32_t /*log_level*/, std::string_view message) override {
    auto new_log = std::string(message);
    std::cout << ANSI_COLOR_RED << " [" << project << ":" << vm_name << "] " << ANSI_COLOR_RESET << new_log << std::endl;
    return WasmResult::Ok;
  }

  uint64_t getCurrentTimeNanoseconds() override {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }
  uint64_t getMonotonicTimeNanoseconds() override {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
  }

  // std::string_view getConfiguration() override;

  // proxy_wasm::WasmResult setTimerPeriod(std::chrono::milliseconds period, uint32_t *timer_token_ptr) override;

  // BufferInterface *getBuffer(WasmBufferType type) override;

  // // Metrics
  // proxy_wasm::WasmResult defineMetric(uint32_t metric_type, std::string_view name, uint32_t *metric_id_ptr) override;
  // proxy_wasm::WasmResult incrementMetric(uint32_t metric_id, int64_t offset) override;
  // proxy_wasm::WasmResult recordMetric(uint32_t metric_id, uint64_t value) override;
  // proxy_wasm::WasmResult getMetric(uint32_t metric_id, uint64_t *value_ptr) override;

  // Properties
  proxy_wasm::WasmResult getProperty(std::string_view path, std::string *result) override;

  // proxy_wasm::WasmResult setProperty(std::string_view key, std::string_view serialized_value) override;

  // // send a premade response
  // proxy_wasm::WasmResult sendLocalResponse(uint32_t response_code, std::string_view body_text, Pairs additional_headers,
  //                              GrpcStatusCode /* grpc_status */, std::string_view details) override;

  // proxy_wasm::WasmResult getSharedData(std::string_view key, std::pair<std::string, uint32_t /* cas */> *data) override;

  // // Header/Trailer/Metadata Maps
  proxy_wasm::WasmResult addHeaderMapValue(WasmHeaderMapType type, std::string_view key, std::string_view value) override {
    std::unordered_map<std::string, std::string> headers;
    std::cout << "addHeaderMapValue " << key.data() << " = " << value.data() << std::endl;
    headers[key.data()] = value.data();
    this->setHeaders(headers, false, false);
    return WasmResult::Ok;
  }

  proxy_wasm::WasmResult getHeaderMapValue(WasmHeaderMapType type, std::string_view key, std::string_view *result) override {
    for (const auto& x: this->in_headers){
      if (x.first == key.data()) {
        *result = x.second;
        break;
      }
    }
    // std::cout << "getHeaderValue" << static_cast<int>(type) << " key " << key.data() << " = " << *result << std::endl;
    return WasmResult::Ok;
  }
  proxy_wasm::WasmResult getHeaderMapPairs(WasmHeaderMapType type, Pairs *result) override;
  // proxy_wasm::WasmResult setHeaderMapPairs(WasmHeaderMapType type, const Pairs &pairs) override;
  // proxy_wasm::WasmResult removeHeaderMapValue(WasmHeaderMapType type, std::string_view key) override;
  // proxy_wasm::WasmResult replaceHeaderMapValue(WasmHeaderMapType type, std::string_view key, std::string_view value) override;
  // proxy_wasm::WasmResult getHeaderMapSize(WasmHeaderMapType type, uint32_t *result) override;
  void setHeaders(std::unordered_map<std::string, std::string> headers, bool forward_direction, bool in);

protected:
  friend class Wasm;

private:
  HeaderMap getHeaderMap(WasmHeaderMapType type);

  Pairs local_reply_headers_{};
  std::string local_reply_details_ = "";
  bool local_reply_                = false;
  std::string project;
  std::string vm_name;
  BufferBase buffer_;
  std::unordered_map<std::string, std::string> in_headers;
  std::unordered_map<std::string, std::string> out_headers;
  bool forward_direction;
};

#define WASM_DEBUG_TAG "wasm"

class ProxyWasmVmIntegration : public WasmVmIntegration
{
public:
  //  proxy_wasm::WasmVmIntegration
  WasmVmIntegration *
  clone() override
  {
    return new ProxyWasmVmIntegration();
  }
  bool getNullVmFunction(std::string_view function_name, bool returns_word, int number_of_arguments, proxy_wasm::NullPlugin *plugin,
                         void *ptr_to_function_return) override;
  proxy_wasm::LogLevel getLogLevel() override;
  void setLogLevel(proxy_wasm::LogLevel);
  void error(std::string_view message) override;
  void trace(std::string_view message) override;
private:
  proxy_wasm::LogLevel log_level = proxy_wasm::LogLevel::debug;
};

class Wasm : public WasmBase
{
public:
  // new constructors
  Wasm(std::unique_ptr<WasmVm> wasm_vm, std::string_view vm_id, std::string_view vm_configuration, std::string_view vm_key,
       std::unordered_map<std::string, std::string> envs, AllowedCapabilitiesMap allowed_capabilities);
  Wasm(const std::shared_ptr<WasmHandleBase> &base_wasm_handle, const WasmVmFactory &factory);

  // start a new VM
  Context *start(const std::shared_ptr<PluginBase> &plugin);

  // provide access to VM mutex
//   TSMutex mutex() const;

  // functions to manage lifecycle of VM
  bool readyShutdown();
  bool readyDelete();

  // functions for creating contexts from the VM
  ContextBase *createVmContext() override;
  ContextBase *createRootContext(const std::shared_ptr<PluginBase> &plugin) override;
  ContextBase *createContext(const std::shared_ptr<PluginBase> &plugin) override;

  // functions managing timer
  bool existsTimerPeriod(uint32_t root_context_id);
  std::chrono::milliseconds getTimerPeriod(uint32_t root_context_id);
  void removeTimerPeriod(uint32_t root_context_id);

  // override function for reporting error
  void error(std::string_view message) override;

protected:
  friend class Context;

private:
//   TSMutex mutex_{nullptr};
};

} // namespace proxy_wasm_host

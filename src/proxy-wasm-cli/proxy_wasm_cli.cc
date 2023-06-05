#include "include/proxy-wasm/wasm_vm.h"

#include <iostream>
#include <string>
#include <stdarg.h>

#include <yaml-cpp/yaml.h>
#include <sys/types.h>
#include "proxy_wasm_includes.h"
#include <unistd.h>
#include <fcntl.h>
#include <chrono>
#include <ctime>
#include <sys/time.h>
#include <cstdint>
#include <served/served.hpp>
#include <google/protobuf/util/json_util.h>
#include "src/proxy-wasm-cli/proto/proxy_wasm_cli.pb.h"
#include "src/proxy-wasm-cli/proto/proxy_wasm_cli.grpc.pb.h"

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using proxy_wasm_cli::WasmHost;
using proxy_wasm_cli::WasmListInstanceRequest;
using proxy_wasm_cli::WasmListInstanceReply;
using proxy_wasm_cli::WasmLaunchInstanceRequest;
using proxy_wasm_cli::WasmLaunchInstanceReply;
using proxy_wasm_cli::WasmSendTrafficRequest;
using proxy_wasm_cli::WasmSendTrafficReply;
using proxy_wasm_cli::WasmSettingsRequest;
using proxy_wasm_cli::WasmSettingsReply;
using proxy_wasm_cli::WASM_LOG_LEVEL;
using proxy_wasm_cli::wasmHTTPPacket;


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

proxy_wasm::LogLevel toWasmLogLevel(WASM_LOG_LEVEL log_level)
{
  auto ll = proxy_wasm::LogLevel::debug;
  switch (log_level) {
    case WASM_LOG_LEVEL::WASM_LOGLEVEL_WARNING:
      ll = proxy_wasm::LogLevel::warn;
      break;
    case WASM_LOG_LEVEL::WASM_LOGLEVEL_TRACE:
      ll = proxy_wasm::LogLevel::trace;
      break;
    case WASM_LOG_LEVEL::WASM_LOGLEVEL_DEBUG:
      ll = proxy_wasm::LogLevel::debug;
      break;
    case WASM_LOG_LEVEL::WASM_LOGLEVEL_INFO:
      ll = proxy_wasm::LogLevel::info;
      break;
    case WASM_LOG_LEVEL::WASM_LOGLEVEL_ERROR:
      ll = proxy_wasm::LogLevel::error;
      break;
    case WASM_LOG_LEVEL::WASM_LOGLEVEL_CRITICAL:
      ll = proxy_wasm::LogLevel::critical;
      break;
    default:
      break;
  }
  return ll;
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
  return new Context(this);
}

ContextBase *
Wasm::createRootContext(const std::shared_ptr<PluginBase> &plugin)
{
  proxy_wasm_host::LogDebug(WASM_DEBUG_TAG, " Create root context.");
  return new Context(this, plugin);
}

ContextBase *
Wasm::createContext(const std::shared_ptr<PluginBase> &plugin)
{
  return new Context(this, plugin);
}

// Function to start a new root context
Context *
Wasm::start(const std::shared_ptr<PluginBase> &plugin)
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


Context::Context() : ContextBase() {}

Context::Context(Wasm *wasm) : ContextBase(wasm) {}

Context::Context(Wasm *wasm, const std::shared_ptr<PluginBase> &plugin) : ContextBase(wasm, plugin) {}

// NB: wasm can be nullptr if it failed to be created successfully.
Context::Context(Wasm *wasm, uint32_t parent_context_id, const std::shared_ptr<PluginBase> &plugin, std::string project, std::string vm_name) : ContextBase(wasm)
{
  id_                = (wasm_ != nullptr) ? wasm_->allocContextId() : 0;
  parent_context_id_ = parent_context_id;
  plugin_            = plugin;
  if (wasm_ != nullptr) {
    parent_context_ = wasm_->getContext(parent_context_id_);
  }
  this->project = project;
  this->vm_name = vm_name;
}

void Context::setHeaders(std::unordered_map<std::string, std::string> headers, bool forward_direction, bool in)
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
}

// local reply handler
void
Context::onLocalReply() {}


// Properties
WasmResult
Context::getProperty(std::string_view path, std::string *result)
{
  if (path.substr(0, p_plugin_root_id.size()) == p_plugin_root_id) {
    *result = this->plugin_->root_id_;
    LogDebug(WASM_DEBUG_TAG, "[%s] looking for plugin_root_id: %.*s", __FUNCTION__, static_cast<int>((*result).size()),
            (*result).data());
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
  return unimplemented();
}

WasmResult
Context::getHeaderMapPairs(WasmHeaderMapType type, Pairs *result)
{
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
}

// extended Wasm VM Integration object
proxy_wasm::LogLevel
ProxyWasmVmIntegration::getLogLevel()
{
  return log_level;
}

void
ProxyWasmVmIntegration::setLogLevel(proxy_wasm::LogLevel level)
{
  log_level = level;
}

void
ProxyWasmVmIntegration::error(std::string_view message)
{
  LogError("%.*s", static_cast<int>(message.size()), message.data());
}

void
ProxyWasmVmIntegration::trace(std::string_view message)
{
  LogDebug(WASM_DEBUG_TAG, "trace -> %.*s", static_cast<int>(message.size()), message.data());
  // std::cout << "trace - " << message.data() << std::endl;
  // std::printf("trace -> %.*s\n", static_cast<int>(message.size()), message.data());
}

bool
ProxyWasmVmIntegration::getNullVmFunction(std::string_view function_name, bool returns_word, int number_of_arguments,
                                        proxy_wasm::NullPlugin *plugin, void *ptr_to_function_return)
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
    proxy_wasm_host::LogError("[wasm][%s] wasm unable to read: size different from buffer", __FUNCTION__);
    return -1;
  }
  return 0;
}


class WasmVmInfo
{
  public:
    WasmVmInfo() {}
    WasmVmInfo(std::string project, std::string vmName, std::shared_ptr<proxy_wasm_host::Wasm> wasm, ::google::protobuf::Timestamp *ts, std::shared_ptr<proxy_wasm::PluginBase> plugin) {
      this->project = project;
      this->name = vmName;
      this->wasm = wasm;
      this->ts = ts;
      this->plugin = plugin;
    }

    void SetProject(std::string project) { this->project = project;}
    void SetName(std::string name) { this->name = name;}
    void SetWasm(std::shared_ptr<proxy_wasm_host::Wasm> wasm) { this->wasm = wasm;}
    void SetTs(::google::protobuf::Timestamp *ts) { this->ts = ts;}
    void SetPlugin(std::shared_ptr<proxy_wasm::PluginBase> plugin) { this->plugin = plugin;}
    void SetRootContext(proxy_wasm_host::Context *context) { this->rootContext = context;}

    ::google::protobuf::Timestamp *ts;
    std::string project;
    std::string name;
    std::shared_ptr<proxy_wasm_host::Wasm> wasm;
    std::shared_ptr<proxy_wasm::PluginBase> plugin;
    proxy_wasm_host::Context *rootContext;
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
    std::shared_ptr<proxy_wasm_host::Wasm> LaunchVm(WasmVmInfo *vm, std::string runTime, std::string projectName, std::string vmName, std::string wasmFile, WASM_LOG_LEVEL logLevel);

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

    static WasmManager *GetInstance();
    /**
     * Finally, any WasmManager should define some business logic, which can be
     * executed on its instance.
     */
    void NewWasmVM(std::string runtime, std::string root_id, std::string vm_name, std::string wasmFile, WASM_LOG_LEVEL logLevel)
    {
        std::shared_ptr<proxy_wasm_host::Wasm> wasm;
        std::string key = root_id + ":" + vm_name;
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
          wasm = LaunchVm(vm, runtime, root_id, vm_name, wasmFile, logLevel);
          std::cout << "Vm with key " << key << " not found. launched with root_id " << root_id << " vm name " << vm_name << " wasm file " << wasmFile << "vm info name " << vm->name << " proj = " << vm->project << std::endl;
          _wasmVms[key] = vm;
        } else {
          std::cout << "Vm with key " << key << " found!" << std::endl;
        }
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

std::shared_ptr<proxy_wasm_host::Wasm> WasmManager::LaunchVm(WasmVmInfo *vm, std::string runtime, std::string projectName, std::string vmName, std::string wasmFile, WASM_LOG_LEVEL logLevel)
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
  struct timeval tv;

  if (runtime == "wasmedge") {
    // std::unique_ptr<proxy_wasm::WasmVm> vm = proxy_wasm::createWasmEdgeVm();
    std::cout << "Created wasm Edge VM" << std::endl;
    wasm = std::make_shared<proxy_wasm_host::Wasm>(proxy_wasm::createWasmEdgeVm(), // VM
                                              vmName,                          // vm_id
                                              vm_configuration,               // vm_configuration
                                              "",                             // vm_key,
                                              envs,                           // envs
                                              cap_maps                        // allowed capabilities
      );
  } else if (runtime == "wasmtime") {
    std::cout << "Created wasmtime VM" << std::endl;
    wasm = std::make_shared<proxy_wasm_host::Wasm>(proxy_wasm::createWasmtimeVm(), // VM
                                              vmName,                          // vm_id
                                              vm_configuration,               // vm_configuration
                                              "",                             // vm_key,
                                              envs,                           // envs
                                              cap_maps                        // allowed capabilities
      );
  } else if (runtime == "wamr") {
    std::cout << "Created wamr VM" << std::endl;
    wasm = std::make_shared<proxy_wasm_host::Wasm>(proxy_wasm::createWamrVm(), // VM
                                              vmName,                          // vm_id
                                              vm_configuration,               // vm_configuration
                                              "",                             // vm_key,
                                              envs,                           // envs
                                              cap_maps                        // allowed capabilities
      );
  } else {
    std::cout << "Unsupported runtime " << runtime << std::endl;
    return wasm;
  }
  wasm->wasm_vm()->integration() = std::make_unique<proxy_wasm_host::ProxyWasmVmIntegration>();
  auto *pvm = dynamic_cast<proxy_wasm_host::ProxyWasmVmIntegration *>(wasm->wasm_vm()->integration().get());
  auto ll = proxy_wasm_host::toWasmLogLevel(logLevel);
  pvm->setLogLevel(ll);
  auto plugin = std::make_shared<proxy_wasm::PluginBase>(vmName,          // name
                                                          projectName,       // root_id
                                                          vmName,         // vm_id
                                                          runtime,       // engine
                                                          configuration, // plugin_configuration
                                                          fail_open,     // failopen
                                                          ""             // TODO: plugin key from where ?
  );

  std::string code;
  if (read_file(wasmFile, &code) < 0) {
    proxy_wasm_host::LogError("[wasm][%s] wasm unable to read file '%s'", __FUNCTION__, wasmFile.c_str());
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
  proxy_wasm_host::LogDebug(WASM_DEBUG_TAG, "Wasm Code %s loaded", wasmFile.c_str());

  auto *rootContext = wasm->start(plugin);

  if (!wasm->configure(rootContext, plugin)) {
    proxy_wasm_host::LogError("[wasm][%s] Failed to configure Wasm", __FUNCTION__);
    return wasm;
  }
  proxy_wasm_host::LogDebug(WASM_DEBUG_TAG, "Wasm %s configured", wasmFile.c_str());

  vm->SetProject(projectName);
  vm->SetName(vmName);
  vm->SetWasm(wasm);
  vm->SetRootContext(rootContext);
  ::google::protobuf::Timestamp* ts = new ::google::protobuf::Timestamp();
  gettimeofday(&tv, NULL);
  ts->set_seconds(tv.tv_sec);
  ts->set_nanos(tv.tv_usec * 1000);
  vm->SetTs(ts);
  vm->SetPlugin(plugin);
  return wasm;
}


// Logic and data behind the server's behavior.
class WasmHostServiceImpl final : public WasmHost::Service {
  Status ListInstances(ServerContext* context, const WasmListInstanceRequest* request,
                  WasmListInstanceReply* reply) override {
    WasmManager* wm = WasmManager::GetInstance();
    auto vms = wm->ListVms();
    for (const auto& x : vms){
      std::cout << "VM = " << x.first << " project " << x.second->project << std::endl;
      auto *vm = reply->add_vms();
      vm->set_key(x.first);
      vm->set_project(x.second->project);
      vm->set_name(x.second->name);
      auto ts = vm->mutable_create_ts();
      ts->set_seconds(x.second->ts->seconds());
      ts->set_nanos(x.second->ts->nanos());
    }
    reply->set_error("");
    return Status::OK;
  }

  Status LaunchInstance(ServerContext* context, const WasmLaunchInstanceRequest* request,
                  WasmLaunchInstanceReply* reply) override {
    std::string runtime = "wasmedge";
    WasmManager* wm = WasmManager::GetInstance();

    switch (request->runtime()) {
      case WasmLaunchInstanceRequest::WASM_RUNTIME_WASMTIME:
        runtime = "wasmtime";
        break;
      case WasmLaunchInstanceRequest::WASM_RUNTIME_WAMR:
        runtime = "wamr";
        break;
      default:
        break;
    }
    // std::string logLevel = "debug";
    WASM_LOG_LEVEL logLevel = WASM_LOG_LEVEL::WASM_LOGLEVEL_DEBUG;
    if (request->log_level() != WASM_LOG_LEVEL::WASM_LOGLEVEL_INVALID) {
      logLevel = request->log_level();
    }
    wm->NewWasmVM(runtime, request->root_id(), request->name(), request->wasm_file(), logLevel);
    std::cout << "Launch Instance" << request << std::endl;
    reply->set_error("");
    reply->set_success(1);
    return Status::OK;
  }

  bool ProcessTraffic(WasmVmInfo* vm, wasmHTTPPacket pkt) {
    std::shared_ptr<proxy_wasm::PluginBase> &pl = vm->plugin;
    auto *context = new proxy_wasm_host::Context(vm->wasm.get(), vm->rootContext->id(), pl, vm->project, vm->name);
    int count = pkt.headers_size();
    context->onCreate();
    std::cout << "ProcessTraffic header count" << count << " forward = " << pkt.forward_direction() << std::endl;
    if (count) {
      std::unordered_map<std::string, std::string> headers;
      for (int i = 0; i < pkt.headers_size(); i++) {
        headers[pkt.headers(i).key()] = pkt.headers(i).value();
      }
      context->setHeaders(headers, pkt.forward_direction(), true);
      proxy_wasm::FilterHeadersStatus status;
      if (pkt.forward_direction()) {
        status = context->onRequestHeaders(count, false);
      } else {
        status = context->onResponseHeaders(count, false);
      }
      if (static_cast<uint64_t>(status) != static_cast<uint64_t>(proxy_wasm::FilterHeadersStatus::Continue)) {
        proxy_wasm_host::LogDebug(WASM_DEBUG_TAG, "onRequestHeaders returned with = %d. Skip the request", status);
        return false;
      }
      proxy_wasm_host::LogDebug(WASM_DEBUG_TAG, "called %s with ret = %d", pkt.forward_direction() ? "onRequestHeaders" : "onResponseHeaders", uint64_t(status));
    }
    return true;
  }

  Status SendTraffic(ServerContext* context, const WasmSendTrafficRequest* request,
                  WasmSendTrafficReply* reply) override {
    WasmManager* wm = WasmManager::GetInstance();
    // wm->NewWasmVM(request->root_id(), request->name(), request->wasm_file());
    std::cout << "Send Traffic to VM " << request->project() << ":" << request->vm_name() << std::endl;
    auto vmList = wm->FilterVms(request->project(), request->vm_name());
    for (int j = 0; j < request->packets_size(); j++) {
      auto pkt = request->packets(j);
      for (int i = 0; i < (int)vmList.size(); i++) {
          // std::cout << "VM - " << vmList[i]->project << ":" << vmList[i]->name << " sip = " << pkt.sip() << " dip = " << pkt.dip() << std::endl;
          auto proceed = ProcessTraffic(vmList[i], pkt);
          if (!proceed) {
            std::cout << "VM - " << vmList[i]->project << ":" << vmList[i]->name << " did not return Ok. Stop further processing" << std::endl;
          }
      }
    }
    return Status::OK;
  }

  Status WasmSettings(ServerContext* context, const WasmSettingsRequest* request,
                  WasmSettingsReply* reply) override {
    WasmManager* wm = WasmManager::GetInstance();
    std::cout << "Edit settings for VM " << request->project() << ":" << request->vm_name() << std::endl;
    auto vmList = wm->FilterVms(request->project(), request->vm_name());
    for (int i = 0; i < (int)vmList.size(); i++) {
        std::cout << "settings for Vm - " << vmList[i]->project << ":" << vmList[i]->name << std::endl;
        if (request->log_level() != WASM_LOG_LEVEL::WASM_LOGLEVEL_INVALID) {
          auto ll = proxy_wasm_host::toWasmLogLevel(request->log_level());
          auto *pvm = dynamic_cast<proxy_wasm_host::ProxyWasmVmIntegration *>(vmList[i]->wasm->wasm_vm()->integration().get());
          pvm->setLogLevel(ll);
        }
    }
    // reply->set_error("Not Implemneted");
    // reply->set_success(0);
    return Status::OK;
  }
};

ABSL_FLAG(uint16_t, port, 50051, "Server port for the service");



// // serverHttp: Initially though of running HTTP but then
// // chose GRPC instead (so we can use structured messaging via protobufs)
// int serveHttp()
// {
// 	served::multiplexer mux;
// 	mux.handle("/hello")
// 		.get([](served::response & res, const served::request &) {
// 			res << "Hello world";
// 		});
// 	std::cout << "Try this example with:" << std::endl;
// 	std::cout << " curl http://localhost:8123/hello" << std::endl;
// 	served::net::server server("127.0.0.1", "8123", mux);
// 	server.run(10); // Run with a pool of 10 threads.
// 	return 0;
// }

int serveGRPC()
{

  std::string server_address = absl::StrFormat("0.0.0.0:%d", absl::GetFlag(FLAGS_port));
  WasmHostServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
  return 0;
}


int main(int argc, char** argv) {
  serveGRPC();
  return 0;
}


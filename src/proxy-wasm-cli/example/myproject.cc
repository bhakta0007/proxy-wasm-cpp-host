/**
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

#include <string>
#include <unordered_map>

#include "proxy_wasm_intrinsics.h"

static int _onReqHdrsCnt = 0;
class ExampleRootContext : public RootContext
{
public:
  explicit ExampleRootContext(uint32_t id, std::string_view root_id) : RootContext(id, root_id) {}

  bool onStart(size_t) override;
private:
  int _onReqHdrsCnt = 0;
};

class ExampleContext : public Context
{
public:
  explicit ExampleContext(uint32_t id, RootContext *root) : Context(id, root) {}

  FilterDataStatus onRequestBody(size_t body_buffer_length, bool end_of_stream) override;
  FilterDataStatus onResponseBody(size_t body_buffer_length, bool end_of_stream) override;
  FilterHeadersStatus onRequestHeaders(uint32_t headers, bool end_of_stream) override;
  FilterHeadersStatus onResponseHeaders(uint32_t headers, bool end_of_stream) override;
  void onCreate() override;
  void onLog() override;
  // void onTick() override;
  void onDone() override;

  void onDelete() override;
};
static RegisterContextFactory register_ExampleContext(CONTEXT_FACTORY(ExampleContext), ROOT_FACTORY(ExampleRootContext),
                                                      "p1");

bool
ExampleRootContext::onStart(size_t)
{
  logInfo(std::string("called WASM plugin onStart"));

  return true;
}

FilterDataStatus
ExampleContext::onRequestBody(size_t body_buffer_length, bool end_of_stream)
{
  logInfo(std::string("called WASM plugin onRequestBody") + std::to_string(id()) + std::string(" buf-len ") + std::to_string(body_buffer_length) + std::string(" eos = ") + std::to_string(end_of_stream));
  return FilterDataStatus::Continue;

}

FilterDataStatus
ExampleContext::onResponseBody(size_t body_buffer_length, bool end_of_stream)
{
  logInfo(std::string("called WASM plugin onResponseBody") + std::to_string(id()) + std::string(" buf-len ") + std::to_string(body_buffer_length) + std::string(" eos = ") + std::to_string(end_of_stream));
  return FilterDataStatus::Continue;
}

FilterHeadersStatus
ExampleContext::onRequestHeaders(uint32_t headers, bool end_of_stream)
{
  _onReqHdrsCnt++;
  // print request id
  logInfo(std::string("called WASM plugin onRequestHeaders") + std::to_string(id()) + std::string(" cnt ") + std::to_string(_onReqHdrsCnt));

  // print UA
  auto ua = getRequestHeader("User-Agent");
  logInfo(std::string("UA ") + std::string(ua->view()));

  // print # of headers
  logInfo(std::string("#headers ") + std::to_string(headers));

  // print the headers
  auto result = getRequestHeaderPairs();
  auto pairs = result->pairs();
  logInfo(std::string("headers: ") + std::to_string(pairs.size()));
  for (auto& p : pairs) {
    logInfo(std::string(p.first) + std::string(" -> ") + std::string(p.second));
  }

  // print current time
  logInfo(std::string("time ") + std::to_string(getCurrentTimeNanoseconds()));

  if (0) {
    auto ds = getRequestHeader("D");
    logInfo(std::string("ds size = ") + std::to_string(ds->size()));
    logInfo(std::string(" value = ") + std::string(ds->data()));
    if (ds->size()) {
      int d = std::stoi(ds->data());
      logInfo(std::string("onRequestHeaders d = ") + std::to_string(d));
      auto e = 3 / d;
      logInfo(std::string("onRequestHeaders e = ") + std::to_string(e));
    }
    // if (d == 3) {
    //   int x = std::system("ping -c1 -s1 8.8.8.8  > /dev/null 2>&1");
    //   if (x==0){
    //       logInfo(std::string("ping success, x = ") + std::to_string(x));
    //   }else{
    //       logInfo(std::string("ping failed, x = ") + std::to_string(x));
    //   }
    // }
  }

  // adda request header
  addRequestHeader("A", "B2");
  logInfo(std::string("onRequestHeaders is finished"));

  return FilterHeadersStatus::Continue;
}

FilterHeadersStatus ExampleContext::onResponseHeaders(uint32_t headers, bool end_of_stream) {
  LOG_DEBUG(std::string("called WASM plugin onResponseHeaders ") + std::to_string(id()));
  auto result = getResponseHeaderPairs();
  auto pairs = result->pairs();
  LOG_INFO(std::string("headers: ") + std::to_string(pairs.size()));
  for (auto &p : pairs) {
    LOG_INFO(std::string("Response header ") + std::string(p.first) + std::string(" -> ") + std::string(p.second));
  }
  addResponseHeader("X-Wasm-custom", "FOO");
  //replaceResponseHeader("content-type", "text/plain; charset=utf-8");
  //removeResponseHeader("content-length");
  return FilterHeadersStatus::Continue;
}

void
ExampleContext::onCreate()
{
  std::string res;
  auto sa = getProperty({"source", "address"});
  std::string sas = std::string({sa.value()->data(), sa.value()->size()});
  auto sap = getProperty({"source", "port"});
  std::string saps = std::string({sap.value()->data(), sap.value()->size()});
  // logInfo("type of var = " + std::string(typeid(sa).name()));
  logInfo("called WASM plugin onCreate " + std::to_string(id()) + " from client " + sas + ":" + saps);
  return;
}

void
ExampleContext::onDone()
{
  logInfo("called WASM plugin onDone " + std::to_string(id()));
  return;
}

void
ExampleContext::onDelete()
{
  logInfo("called WASM plugin onDelete");
  return;
}

void
ExampleContext::onLog()
{
  logInfo("called WASM plugin onLog");
  return;
}

// void
// ExampleContext::onTick()
// {
//   logInfo("called WASM plugin onTick");
//   return;
// }

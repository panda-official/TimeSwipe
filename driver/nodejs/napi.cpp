#include <napi.h>
#include "napi-thread-safe-callback.hpp"
#include "timeswipe.hpp"

class TimeSwipeNAPI: public TimeSwipe, public Napi::ObjectWrap<TimeSwipeNAPI>
{
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    TimeSwipeNAPI(const Napi::CallbackInfo& info);
private:
    static Napi::FunctionReference constructor;
    void SetBridgeWrap(const Napi::CallbackInfo& info);
    void SetSensorOffsetsWrap(const Napi::CallbackInfo& info);
    void SetSensorGainsWrap(const Napi::CallbackInfo& info);
    void SetSensorTransmissionsWrap(const Napi::CallbackInfo& info);
    void SetSecondaryWrap(const Napi::CallbackInfo& info);
    Napi::Value StartWrap(const Napi::CallbackInfo& info);
    Napi::Value SetSettingsWrap(const Napi::CallbackInfo& info);
    Napi::Value GetSettingsWrap(const Napi::CallbackInfo& info);
    Napi::Value onButtonWrap(const Napi::CallbackInfo& info);
    Napi::Value onErrorWrap(const Napi::CallbackInfo& info);
    Napi::Value StopWrap(const Napi::CallbackInfo& info);
};

Napi::FunctionReference TimeSwipeNAPI::constructor;

Napi::Object TimeSwipeNAPI::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func =
      DefineClass(env,
                  "TimeSwipeNAPI",
                  {
                  InstanceMethod("SetBridge", &TimeSwipeNAPI::SetBridgeWrap),
                  InstanceMethod("SetSensorOffsets", &TimeSwipeNAPI::SetSensorOffsetsWrap),
                  InstanceMethod("SetSensorGains", &TimeSwipeNAPI::SetSensorGainsWrap),
                  InstanceMethod("SetSensorTransmissions", &TimeSwipeNAPI::SetSensorTransmissionsWrap),
                  InstanceMethod("SetSecondary", &TimeSwipeNAPI::SetSecondaryWrap),
                  InstanceMethod("Start", &TimeSwipeNAPI::StartWrap),
                  InstanceMethod("SetSettings", &TimeSwipeNAPI::SetSecondaryWrap),
                  InstanceMethod("GetSettings", &TimeSwipeNAPI::GetSettingsWrap),
                  InstanceMethod("onButton", &TimeSwipeNAPI::onButtonWrap),
                  InstanceMethod("onError", &TimeSwipeNAPI::onErrorWrap),
                  InstanceMethod("Stop", &TimeSwipeNAPI::StopWrap),
                  });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("TimeSwipeNAPI", func);
  return exports;
}

TimeSwipeNAPI::TimeSwipeNAPI(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<TimeSwipeNAPI>(info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);
}

void TimeSwipeNAPI::SetBridgeWrap(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

  int length = info.Length();

  if (length <= 0 || !info[0].IsNumber()) {
    Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
  }

  Napi::Number value = info[0].As<Napi::Number>();
  this->SetBridge(value);
}

void TimeSwipeNAPI::SetSensorOffsetsWrap(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

  int length = info.Length();

  if (length <= 3 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsNumber() || !info[3].IsNumber()) {
    Napi::TypeError::New(env, "4 Numbers expected").ThrowAsJavaScriptException();
  }

  Napi::Number value0 = info[0].As<Napi::Number>();
  Napi::Number value1 = info[1].As<Napi::Number>();
  Napi::Number value2 = info[2].As<Napi::Number>();
  Napi::Number value3 = info[3].As<Napi::Number>();
  this->SetSensorOffsets(value0, value1, value2, value3);
}

void TimeSwipeNAPI::SetSensorGainsWrap(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

  int length = info.Length();

  if (length <= 3 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsNumber() || !info[3].IsNumber()) {
    Napi::TypeError::New(env, "4 Numbers expected").ThrowAsJavaScriptException();
  }

  Napi::Number value0 = info[0].As<Napi::Number>();
  Napi::Number value1 = info[1].As<Napi::Number>();
  Napi::Number value2 = info[2].As<Napi::Number>();
  Napi::Number value3 = info[3].As<Napi::Number>();
  this->SetSensorGains(value0, value1, value2, value3);
}

void TimeSwipeNAPI::SetSensorTransmissionsWrap(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

  int length = info.Length();

  if (length <= 3 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsNumber() || !info[3].IsNumber()) {
    Napi::TypeError::New(env, "4 Numbers expected").ThrowAsJavaScriptException();
  }

  Napi::Number value0 = info[0].As<Napi::Number>();
  Napi::Number value1 = info[1].As<Napi::Number>();
  Napi::Number value2 = info[2].As<Napi::Number>();
  Napi::Number value3 = info[3].As<Napi::Number>();
  this->SetSensorTransmissions(value0.DoubleValue(), value1.DoubleValue(), value2.DoubleValue(), value3.DoubleValue());
}

void TimeSwipeNAPI::SetSecondaryWrap(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

  int length = info.Length();

  if (length <= 0 || !info[0].IsNumber()) {
    Napi::TypeError::New(env, "Number expected").ThrowAsJavaScriptException();
  }

  Napi::Number value = info[0].As<Napi::Number>();
  this->SetSecondary(value);
}

Napi::Value TimeSwipeNAPI::StartWrap(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

  int length = info.Length();

  bool fail = false;
  if (length <= 0 || !info[0].IsFunction()) {
      //Napi::TypeError::New(env, "Function expected").ThrowAsJavaScriptException();
      fail = true;
  }
  auto callback = std::make_shared<ThreadSafeCallback>(info[0].As<Napi::Function>());

  auto res = this->Start([this, callback, fail](std::vector<Record> records, uint64_t errors) {
    try {
      if (fail) throw std::runtime_error("Function expected");
      callback->call([records, errors] (Napi::Env env, std::vector<napi_value>& args) {
          auto arr = Napi::Array::New(env, records.size());
          unsigned i = 0;
          for (const auto& r: records) {
              auto rec = Napi::Array::New(env, 4);
              for (unsigned j = 0; j < 4; j++) {
                rec[j] = Napi::Number::New(env, r.Sensors[j]);
              }
              arr[i++] = rec;
          }
          args = { arr, Napi::Number::New(env, errors) };
      });
    }
    catch (std::exception& e) {
        callback->callError(e.what());
    }
  });
  return Napi::Boolean::New(env, res);
}

Napi::Value TimeSwipeNAPI::SetSettingsWrap(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    int length = info.Length();

    if (length <= 0 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
    }

    auto value = info[0].As<Napi::String>();
    std::string err;
    auto resp = this->SetSettings(value, err);
    auto arr = Napi::Array::New(env, 2);
    arr[0u] = resp;
    arr[1u] = err;
    return arr;
}

Napi::Value TimeSwipeNAPI::GetSettingsWrap(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);

    int length = info.Length();

    if (length <= 0 || !info[0].IsString()) {
        Napi::TypeError::New(env, "String expected").ThrowAsJavaScriptException();
    }

    auto value = info[0].As<Napi::String>();
    std::string err;
    auto resp = this->GetSettings(value, err);
    auto arr = Napi::Array::New(env, 2);
    arr[0u] = resp;
    arr[1u] = err;
    return arr;
}

Napi::Value TimeSwipeNAPI::onButtonWrap(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

  int length = info.Length();

  bool fail = false;
  if (length <= 0 || !info[0].IsFunction()) {
    //Napi::TypeError::New(env, "Function expected").ThrowAsJavaScriptException();
    fail = true;
  }

  auto callback = std::make_shared<ThreadSafeCallback>(info[0].As<Napi::Function>());

  auto res = this->onButton([this, callback, fail](bool pressed, unsigned counter) {
    try {
      if (fail) throw std::runtime_error("Function expected");
      callback->call([pressed, counter] (Napi::Env env, std::vector<napi_value>& args) {
          args = { Napi::Boolean::New(env, pressed), Napi::Number::New(env, counter) };
      });
    }
    catch (std::exception& e) {
        callback->callError(e.what());
    }

  });
  return Napi::Boolean::New(env, res);
}

Napi::Value TimeSwipeNAPI::onErrorWrap(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

  int length = info.Length();

  bool fail = false;
  if (length <= 0 || !info[0].IsFunction()) {
    Napi::TypeError::New(env, "Function expected").ThrowAsJavaScriptException();
    fail = true;
  }

  auto callback = std::make_shared<ThreadSafeCallback>(info[0].As<Napi::Function>());

  auto res = this->onError([this, callback, fail](uint64_t errors) {
    try {
      if (fail) throw std::runtime_error("Function expected");
      callback->call([errors] (Napi::Env env, std::vector<napi_value>& args) {
          args = { Napi::Number::New(env, errors) };
      });
    }
    catch (std::exception& e) {
        callback->callError(e.what());
    }

  });
  return Napi::Boolean::New(env, res);
}

Napi::Value TimeSwipeNAPI::StopWrap(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

  auto res = this->Stop();
  return Napi::Boolean::New(env, res);
}


Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  return TimeSwipeNAPI::Init(env, exports);
}

NODE_API_MODULE(addon, InitAll)


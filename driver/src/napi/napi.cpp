
#include <napi.h>
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
    void StartWrap(const Napi::CallbackInfo& info); //TODO: bool
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
                  InstanceMethod("Start", &TimeSwipeNAPI::StartWrap)
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

void TimeSwipeNAPI::StartWrap(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

  int length = info.Length();

  if (length <= 0 || !info[0].IsFunction()) {
    Napi::TypeError::New(env, "Function expected").ThrowAsJavaScriptException();
  }

  Napi::Function cb = info[0].As<Napi::Function>();

  this->Start([this, cb, env](std::vector<Record> records, uint64_t errors) {
      auto arr = Napi::Array::New(env, records.size());
      unsigned i = 0;
      for (const auto& r: records) {
          auto rec = Napi::Array::New(env, 4);
          for (unsigned j = 0; j < 4; j++)
            rec[j] = Napi::Number::New(env, r.Sensors[j]);
          arr[i++] = rec;
      }
      printf("calling arr: %u errors: %lu\n", arr.Length(), errors);fflush(stdout);
      cb.Call(env.Global(), {arr, Napi::Number::New(env, errors)});
  });
}

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  return TimeSwipeNAPI::Init(env, exports);
}

NODE_API_MODULE(addon, InitAll)


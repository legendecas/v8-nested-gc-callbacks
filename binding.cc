#include <nan.h>

namespace {

v8::GCType current_type = v8::GCType::kGCTypeAll;
NAN_GC_CALLBACK(GCPrologueCallback) {
  printf("GCPrologueCallback: %d\n", type);
  assert(current_type == v8::GCType::kGCTypeAll);
  current_type = type;
}

NAN_GC_CALLBACK(GCEpilogueCallback) {
  printf("GCEpilogueCallback: %d\n", type);
  current_type = v8::GCType::kGCTypeAll;
}

class Wrapper {
 public:
  Wrapper(v8::Isolate* isolate, v8::Local<v8::Object> that) {
    that_.Reset(isolate, that);
    that_.SetWeak(this, WeakCallback, v8::WeakCallbackType::kParameter);
  }

  static void WeakCallback(const v8::WeakCallbackInfo<Wrapper>& data) {
    data.GetParameter()->that_.Reset();
    data.SetSecondPassCallback(SecondPassCallback);
  }

  static void SecondPassCallback(const v8::WeakCallbackInfo<Wrapper>& data) {
    delete data.GetParameter();
  }

 private:
  v8::Global<v8::Object> that_;
};

void Test(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();

  {
    v8::HandleScope scope(isolate);
    v8::Local<v8::Object> that = info[0].As<v8::Object>();
    new Wrapper(isolate, that);
  }
}

void RequestGarbageCollection(const Nan::FunctionCallbackInfo<v8::Value>& info) {
  v8::Isolate* isolate = info.GetIsolate();
  // Request a forced-gc to trigger the second pass callback synchronously.
  // https://source.chromium.org/chromium/chromium/src/+/main:v8/src/handles/global-handles.cc;l=1250?q=GlobalHandles::PostGarbageCollectionProcessing
  isolate->RequestGarbageCollectionForTesting(v8::Isolate::GarbageCollectionType::kFullGarbageCollection);
}

void Init(v8::Local<v8::Object> exports) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  isolate->AddGCPrologueCallback(GCPrologueCallback);
  isolate->AddGCEpilogueCallback(GCEpilogueCallback);

  v8::Local<v8::Context> context = exports->GetCreationContext().ToLocalChecked();
  exports->Set(context,
               Nan::New("test").ToLocalChecked(),
               Nan::New<v8::FunctionTemplate>(Test)
                   ->GetFunction(context)
                   .ToLocalChecked()).ToChecked();
  exports->Set(context,
               Nan::New("gc").ToLocalChecked(),
               Nan::New<v8::FunctionTemplate>(RequestGarbageCollection)
                   ->GetFunction(context)
                   .ToLocalChecked()).ToChecked();
}

NODE_MODULE(myaddon, Init)
}

// nodejs_api.cpp
#include <v8.h>
#include <libplatform/libplatform.h>
#include <fstream>
#include <sstream>
#include <iostream>

using namespace v8;

static std::unique_ptr<Platform> platform;
static Isolate* isolate = nullptr;
static Global<Context> global_context;

static ArrayBuffer::Allocator* allocator = nullptr;


// קרא קובץ JS
std::string ReadFile(const std::string& filename) {
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// אתחול סביבת Node/V8
void initialize_environment() {
    V8::InitializeICUDefaultLocation(".");
    V8::InitializeExternalStartupData(".");
    platform = platform::NewDefaultPlatform();
    V8::InitializePlatform(platform.get());
    V8::Initialize();

    allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
    Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = allocator;
    isolate = Isolate::New(create_params);

    std::cout << "✅ Environment initialized" << std::endl;
}

// טען קובץ JavaScript והרץ אותו
void load_runtime(const std::string& script_path) {
    if (!isolate) {
        std::cerr << "❌ Isolate is not initialized. Did you call initialize_environment()?" << std::endl;
        return;
    }

    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);
    Local<Context> context = Context::New(isolate);
    Context::Scope context_scope(context);

    global_context.Reset(isolate, context);  // שמור context ל־global

    std::string js_code = ReadFile(script_path);
    if (js_code.empty()) {
        std::cerr << "❌ Failed to read JS file: " << script_path << std::endl;
        return;
    }

    TryCatch try_catch(isolate);
    Local<String> source = String::NewFromUtf8(isolate, js_code.c_str()).ToLocalChecked();
    Local<Script> script;
    if (!Script::Compile(context, source).ToLocal(&script)) {
        String::Utf8Value err(isolate, try_catch.Exception());
        std::cerr << "❌ Compile error: " << *err << std::endl;
        return;
    }

    Local<Value> result;
    if (!script->Run(context).ToLocal(&result)) {
        String::Utf8Value err(isolate, try_catch.Exception());
        std::cerr << "❌ Runtime error: " << *err << std::endl;
        return;
    }

    std::cout << "📦 Runtime loaded: " << script_path << std::endl;
}

// שחרור משאבים
void free_runtime() {
    if (isolate) {
        global_context.Reset();
        isolate->Dispose();
        isolate = nullptr;
    }

    V8::Dispose();
    delete allocator;
    allocator=nullptr;


    std::cout << "🧹 Runtime freed" << std::endl;
}


v8::Local<v8::Value> call_js_function(const std::string& func_name, int argc, v8::Local<v8::Value>* argv) {
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);
    Local<Context> context = global_context.Get(isolate);
    Context::Scope context_scope(context);

    TryCatch try_catch(isolate);

    Local<Object> global = context->Global();
    Local<Value> val = global->Get(context, String::NewFromUtf8(isolate, func_name.c_str()).ToLocalChecked()).ToLocalChecked();

    if (!val->IsFunction()) {
        std::cerr << "❌ Function '" << func_name << "' not found" << std::endl;
        return Undefined(isolate);
    }

    Local<Function> func = Local<Function>::Cast(val);
    MaybeLocal<Value> maybe_result = func->Call(context, global, argc, argv);

    if (maybe_result.IsEmpty()) {
        String::Utf8Value err(isolate, try_catch.Exception());
        std::cerr << "❌ JS Exception: " << *err << std::endl;
        return Undefined(isolate);
    }

    return maybe_result.ToLocalChecked();
}

// nodejs_api_docker.cpp

#include <v8.h>
#include <libplatform/libplatform.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include "../plugin-sdk-main/runtime/xcall.h"
#include "../plugin-sdk-main/runtime/cdt.h"
#include <string.h>

#ifdef _MSC_VER
#define strdup _strdup
#endif

using namespace v8;
static std::unique_ptr<Platform> v8_platform;
static Isolate* isolate = nullptr;
static Global<Context> global_context;
static ArrayBuffer::Allocator* allocator = nullptr;

std::string ReadFile(const std::string& filename) {
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

struct NodeJSContext {
    Isolate* isolate;
    Global<Context>* context;
    Global<Function>* function;
};

void load_runtime_internal() {
    V8::InitializeICUDefaultLocation(".");
    V8::InitializeExternalStartupData(".");
    v8_platform = platform::NewDefaultPlatform();
    V8::InitializePlatform(v8_platform.get());
    V8::Initialize();

    allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
    Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = allocator;
    isolate = Isolate::New(create_params);

    std::cout << "✅ Environment initialized" << std::endl;
}

extern "C" void load_runtime() {
    load_runtime_internal();
}

extern "C" void free_runtime() {
    if (isolate) {
        global_context.Reset();
        isolate->Dispose();
        isolate = nullptr;
    }

    V8::Dispose();
    delete allocator;
    allocator = nullptr;

    std::cout << "🧹 Runtime freed" << std::endl;
}

void xcall_nodejs_no_params_no_ret(void* context_ptr, char** out_err) {
    if (!context_ptr) {
        if (out_err) *out_err = strdup("Context pointer is null");
        return;
    }

    NodeJSContext* ctx = static_cast<NodeJSContext*>(context_ptr);
    if (!ctx->function || !ctx->context) {
        if (out_err) *out_err = strdup("Invalid context: function or context is null");
        return;
    }

    Isolate* isolate = ctx->isolate;

    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);
    Local<Context> context = ctx->context->Get(isolate);
    Context::Scope context_scope(context);

    TryCatch try_catch(isolate);
    Local<Function> func = ctx->function->Get(isolate);

    MaybeLocal<Value> result = func->Call(context, context->Global(), 0, nullptr);
    if (result.IsEmpty() && out_err) {
        String::Utf8Value exception(isolate, try_catch.Exception());
        *out_err = strdup(*exception);
    }
}

extern "C" struct xcall* load_entity(
    const char* module_path,
    const char* entity_path,
    void*, int8_t,
    void*, int8_t,
    char** err
) {
    if (!isolate) {
        if (err) *err = strdup("V8 isolate not initialized. Call load_runtime first.");
        return nullptr;
    }

    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);
    Local<Context> context = Context::New(isolate);
    Context::Scope context_scope(context);

    std::string js_code = ReadFile(module_path);
    if (js_code.empty()) {
        if (err) *err = strdup("Failed to read JavaScript module");
        return nullptr;
    }

    TryCatch try_catch(isolate);
    Local<String> source = String::NewFromUtf8(isolate, js_code.c_str(), NewStringType::kNormal).ToLocalChecked();
    Local<Script> script;
    if (!Script::Compile(context, source).ToLocal(&script) || script->Run(context).IsEmpty()) {
        String::Utf8Value exception(isolate, try_catch.Exception());
        if (err) *err = strdup(*exception);
        return nullptr;
    }

        std::string entity_path_str(entity_path);
    std::string prefix = "callable=";
    std::string funcname = entity_path_str.rfind(prefix, 0) == 0 ? entity_path_str.substr(prefix.length()) : entity_path_str;
    Local<String> func_name = String::NewFromUtf8(isolate, funcname.c_str(), NewStringType::kNormal).ToLocalChecked();
    Local<Value> val = context->Global()->Get(context, func_name).ToLocalChecked();

    if (!val->IsFunction()) {
        if (err) *err = strdup("Function not found in JavaScript module");
        return nullptr;
    }

    std::unique_ptr<NodeJSContext> ctx = std::make_unique<NodeJSContext>(
        NodeJSContext{
            isolate,
            new Global<Context>(isolate, context),
            new Global<Function>(isolate, Local<Function>::Cast(val))
        }
    );

    void* xcall_func = (void*)((void (*)(void*, char**)) &xcall_nodejs_no_params_no_ret);
    struct xcall* pxcall = new xcall(xcall_func, ctx.release());
    return pxcall;
}

extern "C" void free_entity(void* context_ptr) {
    if (!context_ptr) return;
    NodeJSContext* ctx = static_cast<NodeJSContext*>(context_ptr);
    ctx->context->Reset();
    ctx->function->Reset();
    delete ctx->context;
    delete ctx->function;
    delete ctx;
}

extern "C" void* make_callable(void* callable_metadata, void* fp, char** out_err)
{
    if (out_err)
    {
        *out_err = strdup("make_callable is not supported in this xllr implementation");
    }
    return nullptr;
}

extern "C" void free_xcall(void* pxcall)
{
    delete static_cast<xcall*>(pxcall);
}

v8::Local<v8::Value> call_js_function(const std::string& func_name, int argc, v8::Local<v8::Value>* argv) {
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);
    Local<Context> context = global_context.Get(isolate);
    Context::Scope context_scope(context);

    TryCatch try_catch(isolate);

    Local<Object> global = context->Global();
    Local<String> name = String::NewFromUtf8(isolate, func_name.c_str(), NewStringType::kNormal).ToLocalChecked();
    Local<Value> val = global->Get(context, name).ToLocalChecked();

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

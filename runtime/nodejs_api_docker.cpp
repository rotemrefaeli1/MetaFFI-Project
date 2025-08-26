// nodejs_api_docker.cpp

#include <v8.h>
#include <libplatform/libplatform.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include "../plugin-sdk-main/runtime/xcall.h"
#include "../plugin-sdk-main/runtime/cdt.h"
#include <string.h>
#include <memory>     // חשוב: std::unique_ptr + make_unique
#include <string>     // std::string
#include <cstdint>    // int8_t

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
    //std::cout << "im here xcall 1" <<std::endl;
    if (!context_ptr) {
        if (out_err) *out_err = strdup("Context pointer is null");
        return;
    }
    //std::cout << "before static2" <<std::endl;
    NodeJSContext* ctx = static_cast<NodeJSContext*>(context_ptr);
    if (!ctx->function || !ctx->context) {
        if (out_err) *out_err = strdup("Invalid context: function or context is null");
        return;
    }
    //std::cout << "the static_cast is ok" <<std::endl;
    Isolate* isolate = ctx->isolate;
    //std::cout << "the isolate finished" <<std::endl;

    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);
    Local<Context> context = ctx->context->Get(isolate);
    Context::Scope context_scope(context);

    //std::cout << "get from isolate memory succedd" <<std::endl;

    TryCatch try_catch(isolate);
    Local<Function> func = ctx->function->Get(isolate);

   // std::cout << "function get the correct place from memory" <<std::endl;
    //std::cout << func->GetName() << std::endl;
    MaybeLocal<Value> result = func->Call(context, context->Global(), 0, nullptr);

   // std::cout << "the result is here" <<std::endl;

    //std::cout  << ", err: " << out_err << std::endl;

    if (result.IsEmpty() && out_err) {
        //std::cout << "inside the if" <<std::endl;
        String::Utf8Value exception(isolate, try_catch.Exception());
        //std::cout << "the result is here and the length is:" <<exception.length()<< std::endl;
        if(exception.length() > 0)
        {
            *out_err = strdup(exception.operator*());
        }
    }
    //std::cout << "got out of the function" <<std::endl;
}
//seg fault
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

    if (!module_path) {
        if (err) *err = strdup("Module path is null");
        return nullptr;
    }

    if (!entity_path) {
        if (err) *err = strdup("Entity path is null");
        return nullptr;
    }

    std::cout << "Successfully loaded JS file (module_path) = " << module_path << std::endl;
    std::cout << "entity_path = " << entity_path << std::endl;

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
    std::string funcname = entity_path_str.rfind(prefix, 0) == 0 ?
                           entity_path_str.substr(prefix.length()) :
                           entity_path_str;

    //std::cout << "Looking for function: " << funcname << std::endl;

    Local<String> func_name = String::NewFromUtf8(isolate, funcname.c_str(), NewStringType::kNormal).ToLocalChecked();

    TryCatch try_catch2(isolate);

    Local<Value> val;
    if (!context->Global()->Get(context, func_name).ToLocal(&val) || val.IsEmpty()) {
        String::Utf8Value exception(isolate, try_catch2.Exception());
        if (err) *err = strdup(*exception ? *exception : "Unknown error retrieving function");
        return nullptr;
    }
    std::cout << "val type: " << *String::Utf8Value(isolate, val->ToString(context).ToLocalChecked()) << std::endl;

    if (!val->IsFunction()) {
        if (err) *err = strdup("Function is not a JavaScript function");
        return nullptr;
    }


    // make the function safe object
    auto local_func = Local<Function>::Cast(val);
    auto* global_func = new Global<Function>();
    global_func->Reset(isolate, local_func);

    // make the context global
    auto* global_ctx = new Global<Context>();
    global_ctx->Reset(isolate, context);

    // make new struct
    NodeJSContext* raw_ctx = new NodeJSContext{
        isolate,
        global_ctx,
        global_func
    };

    // create the xcall function
    void* xcall_func = (void*)((void (*)(void*, char**)) &xcall_nodejs_no_params_no_ret);
    struct xcall* pxcall = new xcall(xcall_func, raw_ctx);
    //std::cout << "pxcall address: " << pxcall << std::endl;
    //std::cout << "context ptr: " << raw_ctx << ", func: " << global_func << ", ctx: " << global_ctx << std::endl;

    // save the global context
    global_context.Reset(isolate, context);

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


//with segmentation fault
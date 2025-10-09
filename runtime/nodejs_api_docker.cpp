// // nodejs_api_docker.cpp
// // ABI-safe: xcall is a C POD allocated with malloc; plugin frees it in free_xcall.
// // V8 handles are reset under v8::Locker + Isolate::Scope to be cross-thread safe.
//
// #include <v8.h>
// #include <libplatform/libplatform.h>
// #include <fstream>
// #include <sstream>
// #include <iostream>
// #include <memory>
// #include <string>
// #include <cstdint>
// #include <cstring>
// #include <cstdlib>   // malloc/free
//
// #include "../plugin-sdk-main/runtime/xcall.h"
// #include "../plugin-sdk-main/runtime/cdt.h"
//
// #ifdef _MSC_VER
// #define strdup _strdup
// #endif
//
// using namespace v8;
//
// static std::unique_ptr<Platform> v8_platform;
// static Isolate* isolate = nullptr;
// static Global<Context> global_context;
// static ArrayBuffer::Allocator* allocator = nullptr;
//
// struct NodeJSContext {
//     Isolate* isolate;
//     Global<Context>* context;
//     Global<Function>* function;
// };
//
// // ---------- Helpers ----------
//
// static std::string ReadFile(const std::string& filename) {
//     std::ifstream file(filename);
//     std::stringstream buffer;
//     buffer << file.rdbuf();
//     return buffer.str();
// }
//
// static void load_runtime_internal() {
//     std::cout << "[nodejs] load_runtime_internal" << std::endl;
//
//     V8::InitializeICUDefaultLocation(".");
//     V8::InitializeExternalStartupData(".");
//
//     v8_platform = platform::NewDefaultPlatform();
//     V8::InitializePlatform(v8_platform.get());
//     V8::Initialize();
//
//     allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
//     Isolate::CreateParams create_params;
//     create_params.array_buffer_allocator = allocator;
//     isolate = Isolate::New(create_params);
//
//     // Optional one-time build stamp for sanity
//     std::cout << "[nodejs] build " << __DATE__ << " " << __TIME__ << std::endl;
// }
//
// // ---------- Runtime API ----------
//
// extern "C" void load_runtime(char** err) {
//     if (err) *err = nullptr;
//     std::cout << "[nodejs] load_runtime" << std::endl;
//     load_runtime_internal();
// }
//
// extern "C" void free_runtime(char** err) {
//     if (err) *err = nullptr;
//     std::cout << "[nodejs] free_runtime" << std::endl;
//
//     if (isolate) {
//         global_context.Reset();
//         isolate->Dispose();
//         isolate = nullptr;
//     }
//
//     V8::Dispose();
//     v8_platform.reset();
//
//     delete allocator;
//     allocator = nullptr;
// }
//
// // ---------- XCall (no params, no return) ----------
//
// extern "C" void xcall_nodejs_no_params_no_ret(void* context_ptr, char** out_err) noexcept {
//     if (out_err) *out_err = nullptr;
//     std::cout << "[nodejs] xcall" << std::endl;
//
//     if (!context_ptr) { if (out_err) *out_err = strdup("Context pointer is null"); return; }
//     auto* ctx = static_cast<NodeJSContext*>(context_ptr);
//     if (!ctx->function || !ctx->context) { if (out_err) *out_err = strdup("Invalid context: function or context is null"); return; }
//     Isolate* iso = ctx->isolate;
//     if (!iso) { if (out_err) *out_err = strdup("Isolate is null"); return; }
//
//     v8::Locker locker(iso);
//     Isolate::Scope isolate_scope(iso);
//     HandleScope handle_scope(iso);
//     Local<Context> context = ctx->context->Get(iso);
//     Context::Scope context_scope(context);
//
//     TryCatch try_catch(iso);
//     Local<Function> func = ctx->function->Get(iso);
//     MaybeLocal<Value> result = func->Call(context, context->Global(), 0, nullptr);
//     if (result.IsEmpty() && out_err) {
//         String::Utf8Value ex(iso, try_catch.Exception());
//         *out_err = strdup(*ex ? *ex : "Unknown JS exception");
//     }
// }
//
// using xcall_fn_t = void(*)(void*, char**);
//
// // ---------- load_entity (no params, no return) ----------
//
// extern "C" struct xcall* load_entity(
//     const char* module_path,
//     const char* entity_path,
//     metaffi_type_info* /*param_types*/, int8_t params_count,
//     metaffi_type_info* /*ret_types*/,   int8_t retval_count,
//     char** err
// ) {
//     if (err) *err = nullptr;
//     std::cout << "[nodejs] load_entity" << std::endl;
//
//     if (params_count != 0) { if (err) *err = strdup("This Node.js entity currently supports zero params only"); return nullptr; }
//     if (retval_count != 0) { if (err) *err = strdup("This Node.js entity currently supports no return value"); return nullptr; }
//     if (!isolate) { if (err) *err = strdup("V8 isolate not initialized. Call load_runtime first."); return nullptr; }
//     if (!module_path) { if (err) *err = strdup("Module path is null"); return nullptr; }
//     if (!entity_path) { if (err) *err = strdup("Entity path is null"); return nullptr; }
//
//     v8::Locker locker(isolate);
//     Isolate::Scope isolate_scope(isolate);
//     HandleScope handle_scope(isolate);
//     Local<Context> context = Context::New(isolate);
//     Context::Scope context_scope(context);
//
//     std::string js_code = ReadFile(module_path);
//     if (js_code.empty()) { if (err) *err = strdup("Failed to read JavaScript module"); return nullptr; }
//
//     TryCatch try_catch(isolate);
//     Local<String> source = String::NewFromUtf8(isolate, js_code.c_str(), NewStringType::kNormal).ToLocalChecked();
//     Local<Script> script;
//     if (!Script::Compile(context, source).ToLocal(&script) || script->Run(context).IsEmpty()) {
//         String::Utf8Value exception(isolate, try_catch.Exception());
//         if (err) *err = strdup(*exception ? *exception : "Compile/Run failed");
//         return nullptr;
//     }
//
//     std::string entity_path_str(entity_path);
//     std::string prefix = "callable=";
//     std::string funcname = entity_path_str.rfind(prefix, 0) == 0
//                          ? entity_path_str.substr(prefix.length())
//                          : entity_path_str;
//
//     Local<String> func_name = String::NewFromUtf8(isolate, funcname.c_str(), NewStringType::kNormal).ToLocalChecked();
//     TryCatch try_catch2(isolate);
//     Local<Value> val;
//     if (!context->Global()->Get(context, func_name).ToLocal(&val) || val.IsEmpty()) {
//         String::Utf8Value ex(isolate, try_catch2.Exception());
//         if (err) *err = strdup(*ex ? *ex : "Unknown error retrieving function");
//         return nullptr;
//     }
//     if (!val->IsFunction()) { if (err) *err = strdup("Function is not a JavaScript function"); return nullptr; }
//
//     auto local_func = Local<Function>::Cast(val);
//     auto* global_func = new Global<Function>();
//     global_func->Reset(isolate, local_func);
//
//     auto* global_ctx = new Global<Context>();
//     global_ctx->Reset(isolate, context);
//
//     auto* raw_ctx = new NodeJSContext{ isolate, global_ctx, global_func };
//     xcall_fn_t f = &xcall_nodejs_no_params_no_ret;
//
//     struct xcall* pxcall = (xcall*)std::malloc(sizeof(xcall));
//     if (!pxcall) {
//         if (err) *err = strdup("malloc failed allocating xcall");
//         global_func->Reset();
//         global_ctx->Reset();
//         delete global_func;
//         delete global_ctx;
//         delete raw_ctx;
//         return nullptr;
//     }
//
//     pxcall->pxcall_and_context[0] = reinterpret_cast<void*>(f);
//     pxcall->pxcall_and_context[1] = raw_ctx;
//
//     global_context.Reset(isolate, context);
//     return pxcall;
// }
//
// // ---------- free_xcall ----------
// // Plugin frees both the V8 context (ours) and the xcall POD (allocated with malloc).
// extern "C" void free_xcall(xcall* pxcall, char** err) noexcept {
//     if (err) *err = nullptr;
//     std::cout << "[nodejs] free_xcall" << std::endl;
//     if (!pxcall) return;
//
//     void** arr = pxcall->pxcall_and_context;
//     NodeJSContext* ctx = (arr ? static_cast<NodeJSContext*>(arr[1]) : nullptr);
//
//     if (ctx) {
//         if (ctx->isolate) {
//             v8::Locker locker(ctx->isolate);
//             v8::Isolate::Scope isolate_scope(ctx->isolate);
//             v8::HandleScope handle_scope(ctx->isolate);
//             if (ctx->context)  ctx->context->Reset();
//             if (ctx->function) ctx->function->Reset();
//         } else {
//             if (ctx->context)  ctx->context->Reset();
//             if (ctx->function) ctx->function->Reset();
//         }
//
//         delete ctx->context;
//         delete ctx->function;
//         delete ctx;
//     }
//
//     if (arr) {
//         arr[0] = nullptr;
//         arr[1] = nullptr;
//     }
//
//     std::free(pxcall);
// }
//
// // ---------- free_entity (not used in the current flow) ----------
//
// extern "C" void free_entity(void* context_ptr, char** err) noexcept {
//     if (err) *err = nullptr;
//     std::cout << "[nodejs] free_entity" << std::endl;
//
//     if (!context_ptr) return;
//     auto* ctx = static_cast<NodeJSContext*>(context_ptr);
//
//     if (ctx->isolate) {
//         v8::Locker locker(ctx->isolate);
//         v8::Isolate::Scope isolate_scope(ctx->isolate);
//         v8::HandleScope handle_scope(ctx->isolate);
//         if (ctx->context)  ctx->context->Reset();
//         if (ctx->function) ctx->function->Reset();
//     } else {
//         if (ctx->context)  ctx->context->Reset();
//         if (ctx->function) ctx->function->Reset();
//     }
//
//     delete ctx->context;
//     delete ctx->function;
//     delete ctx;
// }
//
// // ---------- make_callable (not supported) ----------
//
// extern "C" xcall* make_callable(void*, metaffi_type_info*, int8_t, metaffi_type_info*, int8_t, char** out_err) {
//     if (out_err) *out_err = strdup("make_callable is not supported in this xllr implementation");
//     return nullptr;
// }
//----------------------------------------------------------------------------------------



// nodejs_api_docker.cpp
#include <v8.h>
#include <libplatform/libplatform.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <limits>
#include <cmath>

#include "../plugin-sdk-main/runtime/xcall.h"
#include "../plugin-sdk-main/runtime/cdt.h"
#include "cdts_nodejs.h"

#ifdef _MSC_VER
#define strdup _strdup
#endif

using namespace v8;

static std::unique_ptr<Platform> v8_platform;
static Isolate* isolate = nullptr;
static Global<Context> global_context;
static ArrayBuffer::Allocator* allocator = nullptr;

static std::string ReadFile(const std::string& filename){
    std::ifstream f(filename);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static inline void set_err(char** err, const char* msg){
    if(!err) return;
    *err = strdup(msg ? msg : "unknown error");
}

//-----------console

static void inject_console(v8::Isolate* isolate, v8::Local<v8::Context> context)
{
    using namespace v8;

    // create console.log
    Local<Object> console = Object::New(isolate);
    Local<Function> logFn =
        Function::New(context, [](const FunctionCallbackInfo<Value>& args){
            Isolate* iso = args.GetIsolate();
            std::ostringstream oss;
            for (int i = 0; i < args.Length(); ++i){
                String::Utf8Value s(iso, args[i]);
                if (i) oss << " ";
                oss << (*s ? *s : "");
            }
            std::string line = oss.str();

            // למסך (stdout של התהליך)
            std::cout << line << std::endl;

            // אופציונלי: גם לקובץ כדי שתוכל לקרוא מה-Java/Python
            try{
                std::ofstream out("/workspace/runtime/hello_output.txt", std::ios::app);
                out << line << "\n";
            } catch(...) {}
        }).ToLocalChecked();

    console->Set(context,
        String::NewFromUtf8(isolate, "log", NewStringType::kNormal).ToLocalChecked(),
        logFn
    ).Check();

    context->Global()->Set(
        context,
        String::NewFromUtf8(isolate, "console", NewStringType::kNormal).ToLocalChecked(),
        console
    ).Check();
}

// --------- Context ---------

struct NodeJSContext {
    Isolate* isolate;
    Global<Context>* context;
    Global<Function>* function;

    metaffi_type_info* param_types = nullptr;
    int8_t params_count = 0;
    metaffi_type_info* ret_types = nullptr;
    int8_t retval_count = 0;
};

// --------- Runtime ---------

static void load_runtime_internal(){
    V8::InitializeICUDefaultLocation(".");
    V8::InitializeExternalStartupData(".");

    v8_platform = platform::NewDefaultPlatform();
    V8::InitializePlatform(v8_platform.get());
    V8::Initialize();

    allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
    Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = allocator;
    isolate = Isolate::New(create_params);

    //std::cout << "[nodejs] build " << __DATE__ << " " << __TIME__ << std::endl;
}

extern "C" void load_runtime(char** err){
    if(err) *err = nullptr;
    std::cout << "[nodejs] load_runtime" << std::endl;
    load_runtime_internal();
}

extern "C" void free_runtime(char** err){
    if(err) *err = nullptr;
    std::cout << "[nodejs] free_runtime" << std::endl;

    if(isolate){
        global_context.Reset();
        isolate->Dispose();
        isolate = nullptr;
    }

    V8::Dispose();
    v8_platform.reset();

    delete allocator;
    allocator = nullptr;
}

// --------- XCall: no-params/no-ret ---------

extern "C" void xcall_nodejs_no_params_no_ret(void* context_ptr, char** out_err) noexcept {
    if(out_err) *out_err = nullptr;

    if(!context_ptr){ set_err(out_err,"null context"); return; }
    auto* ctx = static_cast<NodeJSContext*>(context_ptr);
    Isolate* iso = ctx->isolate;
    if(!iso){ set_err(out_err,"null isolate"); return; }

    v8::Locker locker(iso);
    Isolate::Scope iso_scope(iso);
    HandleScope hs(iso);
    Local<Context> context = ctx->context->Get(iso);
    Context::Scope cs(context);

    TryCatch tc(iso);
    Local<Function> func = ctx->function->Get(iso);
    if(func.IsEmpty()){ set_err(out_err,"empty function"); return; }

    if(func->Call(context, context->Global(), 0, nullptr).IsEmpty()){
        if(out_err){
            String::Utf8Value ex(iso, tc.Exception());
            *out_err = strdup(*ex ? *ex : "Unknown JS exception");
        }
    }
}

// --------- XCall: params + returns ---------

extern "C" void nodejs_xcall_params_ret(void* context_ptr, cdts params_ret[2], char** out_err) noexcept
{
    if(out_err) *out_err = nullptr;
    if(!context_ptr){ set_err(out_err,"null context"); return; }

    auto* ctx = static_cast<NodeJSContext*>(context_ptr);
    Isolate* iso = ctx->isolate;
    if(!iso){ set_err(out_err,"null isolate"); return; }

    v8::Locker locker(iso);
    Isolate::Scope iso_scope(iso);
    HandleScope hs(iso);
    Local<Context> context = ctx->context->Get(iso);
    Context::Scope cs(context);

    v8_conv_opts opts{ iso, context };

    // --- CDT params -> argv ---
    std::vector<Local<Value>> argv;
    if(!cdts_to_v8_argv(opts, params_ret[0], argv, out_err)) return;

    // --- call JS ---
    TryCatch tc(iso);
    Local<Function> func = ctx->function->Get(iso);
    if(func.IsEmpty()){ set_err(out_err,"empty function"); return; }

    Local<Value> js_ret;
    if(!func->Call(context, context->Global(), (int)argv.size(), argv.data()).ToLocal(&js_ret)){
        if(out_err){
            String::Utf8Value ex(iso, tc.Exception());
            *out_err = strdup(*ex ? *ex : "Unknown JS exception");
        }
        return;
    }

    // --- handle returns as declared in load_entity ---
    if (ctx->retval_count == 0) {
        // no return values declared
        return;
    }

    // ensure enough slots in params_ret[1]
    if (params_ret[1].length < ctx->retval_count) {
        new (&params_ret[1]) cdts((metaffi_size)ctx->retval_count, /*fixed_dimensions*/ 1);
    }

    if (ctx->retval_count == 1) {
        // single return value: convert according to declared type
        if (!v8_to_cdt_as_type(opts, js_ret, ctx->ret_types[0], &params_ret[1][0], out_err)) {
            return;
        }
    } else {
        // multiple returns expected → JS must return an Array
        if (!js_ret->IsArray()) { set_err(out_err, "JS did not return an Array for multiple return values"); return; }
        Local<Array> arr = js_ret.As<Array>();

        for (int i = 0; i < ctx->retval_count; ++i) {
            Local<Value> ri;
            if (!arr->Get(context, i).ToLocal(&ri)) {
                set_err(out_err, "Failed reading return array element");
                return;
            }
            if (!v8_to_cdt_as_type(opts, ri, ctx->ret_types[i], &params_ret[1][i], out_err)) {
                return;
            }
        }
    }
}

// --------- load_entity ---------

extern "C" struct xcall* load_entity(
    const char* module_path,
    const char* entity_path,
    metaffi_type_info* param_types, int8_t params_count,
    metaffi_type_info* ret_types,   int8_t retval_count,
    char** err
){
    if(err) *err = nullptr;
    std::cout << "[nodejs] load_entity"
              << " params_count=" << (int)params_count
              << " retval_count=" << (int)retval_count
              << std::endl;

    if(!isolate){ set_err(err,"V8 isolate not initialized. Call load_runtime first."); return nullptr; }
    if(!module_path){ set_err(err,"module_path is null"); return nullptr; }
    if(!entity_path){ set_err(err,"entity_path is null"); return nullptr; }

    v8::Locker locker(isolate);
    Isolate::Scope isolate_scope(isolate);
    HandleScope handle_scope(isolate);
    Local<Context> context = Context::New(isolate);
    Context::Scope context_scope(context);
    // inject console.log before running any JS
    inject_console(isolate, context);

    std::string js_code = ReadFile(module_path);
    if(js_code.empty()){ set_err(err,"Failed to read JavaScript module"); return nullptr; }

    TryCatch try_catch(isolate);
    Local<String> source = String::NewFromUtf8(isolate, js_code.c_str(), NewStringType::kNormal).ToLocalChecked();
    Local<Script> script;
    if(!Script::Compile(context, source).ToLocal(&script) || script->Run(context).IsEmpty()){
        String::Utf8Value ex(isolate, try_catch.Exception());
        set_err(err, *ex ? *ex : "Compile/Run failed");
        return nullptr;
    }

    std::string ep(entity_path);
    std::string prefix = "callable=";
    std::string funcname = (ep.rfind(prefix, 0) == 0) ? ep.substr(prefix.length()) : ep;

    Local<String> fn = String::NewFromUtf8(isolate, funcname.c_str(), NewStringType::kNormal).ToLocalChecked();
    TryCatch try_catch2(isolate);
    Local<Value> val;
    if(!context->Global()->Get(context, fn).ToLocal(&val) || val.IsEmpty()){
        String::Utf8Value ex(isolate, try_catch2.Exception());
        set_err(err, *ex ? *ex : "Failed retrieving function");
        return nullptr;
    }
    if(!val->IsFunction()){ set_err(err,"Resolved value is not a function"); return nullptr; }

    auto local_func = Local<Function>::Cast(val);
    auto* global_func = new Global<Function>();
    global_func->Reset(isolate, local_func);

    auto* global_ctx = new Global<Context>();
    global_ctx->Reset(isolate, context);

    auto* raw_ctx = new NodeJSContext{ isolate, global_ctx, global_func };

    if(params_count > 0 && param_types){
        raw_ctx->params_count = params_count;
        raw_ctx->param_types = new metaffi_type_info[params_count];
        std::memcpy(raw_ctx->param_types, param_types, sizeof(metaffi_type_info)*params_count);
    }
    if(retval_count > 0 && ret_types){
        raw_ctx->retval_count = retval_count;
        raw_ctx->ret_types = new metaffi_type_info[retval_count];
        std::memcpy(raw_ctx->ret_types, ret_types, sizeof(metaffi_type_info)*retval_count);
    }

    void* fptr = nullptr;
    if(params_count == 0 && retval_count == 0){
        fptr = reinterpret_cast<void*>(&xcall_nodejs_no_params_no_ret);
    }else{
        fptr = reinterpret_cast<void*>(&nodejs_xcall_params_ret);
    }

    struct xcall* pxcall = (xcall*)std::malloc(sizeof(xcall));
    if(!pxcall){
        set_err(err,"malloc failed allocating xcall");
        global_func->Reset(); global_ctx->Reset();
        delete global_func; delete global_ctx;
        delete[] raw_ctx->param_types; delete[] raw_ctx->ret_types;
        delete raw_ctx;
        return nullptr;
    }

    pxcall->pxcall_and_context[0] = fptr;
    pxcall->pxcall_and_context[1] = raw_ctx;

    global_context.Reset(isolate, context);
    return pxcall;
}

// --------- free_xcall ---------

extern "C" void free_xcall(xcall* pxcall, char** err) noexcept {
    if(err) *err = nullptr;
    std::cout << "[nodejs] free_xcall" << std::endl;
    if(!pxcall) return;

    void** arr = pxcall->pxcall_and_context;
    NodeJSContext* ctx = (arr ? static_cast<NodeJSContext*>(arr[1]) : nullptr);

    if(ctx){
        if(ctx->isolate){
            v8::Locker locker(ctx->isolate);
            v8::Isolate::Scope iso_scope(ctx->isolate);
            v8::HandleScope hs(ctx->isolate);
            if(ctx->context)  ctx->context->Reset();
            if(ctx->function) ctx->function->Reset();
        }else{
            if(ctx->context)  ctx->context->Reset();
            if(ctx->function) ctx->function->Reset();
        }

        delete ctx->context;
        delete ctx->function;
        delete[] ctx->param_types;
        delete[] ctx->ret_types;
        delete ctx;
    }

    if(arr){ arr[0] = nullptr; arr[1] = nullptr; }
    std::free(pxcall);
}

// --------- free_entity (optional) ---------

extern "C" void free_entity(void* context_ptr, char** err) noexcept {
    if(err) *err = nullptr;
    std::cout << "[nodejs] free_entity" << std::endl;

    if(!context_ptr) return;
    auto* ctx = static_cast<NodeJSContext*>(context_ptr);

    if(ctx->isolate){
        v8::Locker locker(ctx->isolate);
        v8::Isolate::Scope iso_scope(ctx->isolate);
        v8::HandleScope hs(ctx->isolate);
        if(ctx->context)  ctx->context->Reset();
        if(ctx->function) ctx->function->Reset();
    }else{
        if(ctx->context)  ctx->context->Reset();
        if(ctx->function) ctx->function->Reset();
    }

    delete ctx->context;
    delete ctx->function;
    delete[] ctx->param_types;
    delete[] ctx->ret_types;
    delete ctx;
}

// --------- make_callable (not supported) ---------

extern "C" xcall* make_callable(void*, metaffi_type_info*, int8_t, metaffi_type_info*, int8_t, char** out_err){
    if(out_err) *out_err = strdup("make_callable is not supported in this xllr implementation");
    return nullptr;
}


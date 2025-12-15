#include "nodejs_object.h"
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace v8;

#ifdef _MSC_VER
#define strdup _strdup
#endif

namespace
{
    // This is the internal "handle" stored on the C++ side
    struct nodejs_object_handle
    {
        uint64_t       id;      // debug only
        Isolate*       isolate; // isolate that created the Value
        Global<Value>  value;   // Global to prevent GC
    };

    static std::atomic<uint64_t> g_next_nodejs_handle_id{1};

    inline void set_err(char** err, const char* msg)
    {
        if(!err) return;
        if(*err) return;
        *err = strdup(msg ? msg : "nodejs_object: error");
    }

    inline nodejs_object_handle* make_nodejs_object_handle(Isolate* iso,
                                                           Local<Value> v)
    {
        auto* nh = new nodejs_object_handle;
        nh->id      = g_next_nodejs_handle_id++;
        nh->isolate = iso;
        nh->value.Reset(iso, v);
        return nh;
    }
} // anonymous namespace

namespace nodejs_object
{
    // Important: update if MetaFFI defines an official constant for the Node.js runtime id
    const metaffi_uint64 NODEJS_RUNTIME_ID = 2;

    void release_handle(cdt_metaffi_handle* h) noexcept
    {
        if(!h) return;

        // If this is not a Node.js runtime handle - ignore
        if(h->runtime_id != NODEJS_RUNTIME_ID){
            return;
        }

        auto* nh = static_cast<nodejs_object_handle*>(h->handle);
        if(nh){
            nh->value.Reset();
            delete nh;
        }

        h->handle     = nullptr;
        h->release    = nullptr;
        h->runtime_id = 0;
    }

    Local<Value> handle_to_v8(Isolate* iso,
                              Local<Context> ctx,
                              const cdt_metaffi_handle* h,
                              char** out_err)
    {
        (void)ctx; // currently unused, keep a generic signature

        if(!h){
            set_err(out_err, "handle_to_v8: null cdt_metaffi_handle");
            return Undefined(iso);
        }

        // std::cerr << "[nodejs] handle_to_v8: h=" << h
        //           << " handle=" << h->handle
        //           << " runtime_id=" << h->runtime_id
        //           << " release=" << (void*)h->release
        //           << std::endl;

        if(h->runtime_id != NODEJS_RUNTIME_ID){
            set_err(out_err, "handle_to_v8: handle belongs to different runtime");
            return Undefined(iso);
        }

        auto* nh = static_cast<nodejs_object_handle*>(h->handle);
        if(!nh){
            set_err(out_err, "handle_to_v8: internal handle pointer is null");
            return Undefined(iso);
        }

        // std::cerr << "[nodejs] handle_to_v8: nodejs_object_handle=" << nh
        //           << " id=" << nh->id
        //           << " isolate=" << nh->isolate
        //           << " value.IsEmpty=" << (nh->value.IsEmpty() ? "true" : "false")
        //           << std::endl;

        if(nh->isolate != iso){
            set_err(out_err, "handle_to_v8: isolate mismatch between handle and call site");
            return Undefined(iso);
        }
        if(nh->value.IsEmpty()){
            set_err(out_err, "handle_to_v8: internal V8 Global is empty");
            return Undefined(iso);
        }

        return nh->value.Get(iso);
    }

    bool js_value_to_handle(Isolate* iso,
                            Local<Context> ctx,
                            Local<Value> in,
                            cdt& out,
                            char** out_err) noexcept
    {
        (void)ctx; // not required currently

        if(!in->IsObject() && !in->IsFunction()){
            set_err(out_err, "js_value_to_handle: JS value is not an object/function");
            return false;
        }

        // If MetaFFI already provided a container - use it
        cdt_metaffi_handle* h = out.cdt_val.handle_val;
        if(!h){
            h = new cdt_metaffi_handle{};
            out.cdt_val.handle_val = h;
        }

        nodejs_object_handle* nh = make_nodejs_object_handle(iso, in);

        h->handle     = static_cast<void*>(nh);
        h->runtime_id = NODEJS_RUNTIME_ID;
        h->release    = &release_handle;

        out.type          = metaffi_handle_type;
        out.free_required = 0; // MetaFFI releases via h->release

        return true;
    }

} // namespace nodejs_object

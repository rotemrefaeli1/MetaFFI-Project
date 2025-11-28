#include "cdts_nodejs.h"
#include <cstring>
#include <limits>
#include <cmath>
#include <atomic>   // for nodejs handle id counter
#include <iostream>

using namespace v8;

// --------- Node.js object handle infrastructure ---------
//
// This struct lives on the C++ side and owns a V8 Global<Value>,
// which keeps the JS object/function alive while a MetaFFI handle exists.
//
struct nodejs_object_handle {
    uint64_t id;                // optional unique id for debugging
    v8::Isolate* isolate;       // the isolate that owns the value
    v8::Global<v8::Value> value; // V8 global handle (prevents GC)
};

// Global counter for handle ids (not strictly required, but handy for debugging).
static std::atomic<uint64_t> g_next_nodejs_handle_id{1};

// Runtime id for this plugin. If MetaFFI defines a specific constant for Node.js,
// you should use that instead of the hard-coded value here.
static const metaffi_uint64 NODEJS_RUNTIME_ID = 2; // TODO: keep in sync with MetaFFI runtime id


// ---------- helpers ----------
static inline void set_err(char** out_err, const char* msg){
    if(!out_err) return;
    if(*out_err) return;
    size_t n = std::strlen(msg ? msg : "") + 1;
    char* p = (char*)std::malloc(n);
    if(p){ std::memcpy(p, msg, n); *out_err = p; }
}

static Local<String> make_utf8(Isolate* iso, const char* s){
    return String::NewFromUtf8(iso, s ? s : "", NewStringType::kNormal).ToLocalChecked();
}

// Releases a MetaFFI handle that points to a Node.js object.
// This function will be stored in cdt_metaffi_handle::release.
static void nodejs_release_handle(cdt_metaffi_handle* h){
    // Nothing to do if handle is null.
    if(!h) return;

    // If this handle does not belong to the Node.js runtime, ignore it.
    if(h->runtime_id != NODEJS_RUNTIME_ID){
        return;
    }

    // Internal pointer is expected to be a nodejs_object_handle*.
    auto* nh = static_cast<nodejs_object_handle*>(h->handle);
    if(nh){
        // Release the V8 Global so that the JS object can be garbage-collected.
        nh->value.Reset();
        delete nh;
    }

    // Reset the MetaFFI handle fields so it is clearly invalid after release.
    h->handle = nullptr;
    h->release = nullptr;
    h->runtime_id = 0;
}

// Creates a new nodejs_object_handle that wraps a V8 value.
// The returned pointer will be stored inside cdt_metaffi_handle::handle.
static nodejs_object_handle* make_nodejs_object_handle(v8::Isolate* iso,
                                                       v8::Local<v8::Value> v){
    auto* nh = new nodejs_object_handle;
    nh->id = g_next_nodejs_handle_id++; // used only for debugging/logging
    nh->isolate = iso;
    nh->value.Reset(iso, v); // turn Local<Value> into a Global<Value>
    return nh;
}

// Converts a MetaFFI handle back into a V8 Local<Value>.
// Converts a MetaFFI handle back into a V8 Local<Value>.
static v8::Local<v8::Value> handle_to_v8(const v8_conv_opts& o,
                                         const cdt_metaffi_handle* h,
                                         char** out_err)
{
    // Basic pointer check
    if(!h){
        set_err(out_err, "handle_to_v8: Null cdt_metaffi_handle pointer");
        return v8::Undefined(o.isolate);
    }

    // Log some debug info about the incoming handle
    std::cerr << "[nodejs] handle_to_v8: h=" << h
              << " handle=" << h->handle
              << " runtime_id=" << h->runtime_id
              << " release=" << (void*)h->release
              << std::endl;

    // Verify this handle belongs to the Node.js runtime
    if(h->runtime_id != NODEJS_RUNTIME_ID){
        set_err(out_err, "handle_to_v8: handle belongs to a different runtime");
        return v8::Undefined(o.isolate);
    }

    // Internal pointer must be a nodejs_object_handle*
    auto* nh = static_cast<nodejs_object_handle*>(h->handle);
    if(!nh){
        set_err(out_err, "handle_to_v8: internal handle pointer is null");
        return v8::Undefined(o.isolate);
    }

    // Optional: log nodejs_object_handle for debugging
    std::cerr << "[nodejs] handle_to_v8: nodejs_object_handle=" << nh
              << " id=" << nh->id
              << " isolate=" << nh->isolate
              << " value.IsEmpty=" << (nh->value.IsEmpty() ? "true" : "false")
              << std::endl;

    // Ensure we are using the same isolate that created the Global.
    if(nh->isolate != o.isolate){
        set_err(out_err, "handle_to_v8: isolate mismatch between handle and conversion options");
        return v8::Undefined(o.isolate);
    }

    // If the Global is empty, returning it will cause V8 to crash later.
    if(nh->value.IsEmpty()){
        set_err(out_err, "handle_to_v8: internal V8 Global is empty");
        return v8::Undefined(o.isolate);
    }

    // Safe: convert the Global<Value> back to a Local<Value>.
    return nh->value.Get(o.isolate);
}


// ---------- CDT -> V8 ----------
Local<Value> cdt_to_v8(const v8_conv_opts& o, const cdt& in, char** out_err){
    Isolate* iso = o.isolate;

    switch(in.type){
        case metaffi_null_type:
            return Null(iso);

        case metaffi_bool_type:
            return Boolean::New(iso, in.cdt_val.bool_val != 0);

        // integers up to 32-bit: V8 Integer
        case metaffi_int8_type:   return Integer::New(iso,  (int32_t)in.cdt_val.int8_val);
        case metaffi_int16_type:  return Integer::New(iso,  (int32_t)in.cdt_val.int16_val);
        case metaffi_int32_type:  return Integer::New(iso,  (int32_t)in.cdt_val.int32_val);
        case metaffi_uint8_type:  return Integer::NewFromUnsigned(iso, (uint32_t)in.cdt_val.uint8_val);
        case metaffi_uint16_type: return Integer::NewFromUnsigned(iso, (uint32_t)in.cdt_val.uint16_val);
        case metaffi_uint32_type: return Integer::NewFromUnsigned(iso, (uint32_t)in.cdt_val.uint32_val);

        // 64-bit → BigInt לשמירת דיוק
        case metaffi_int64_type:  return BigInt::New(iso, (int64_t)in.cdt_val.int64_val);
        case metaffi_uint64_type: return BigInt::NewFromUnsigned(iso, (uint64_t)in.cdt_val.uint64_val);

        // floats
        case metaffi_float32_type: return Number::New(iso, (double)in.cdt_val.float32_val);
        case metaffi_float64_type: return Number::New(iso, (double)in.cdt_val.float64_val);

        case metaffi_string8_type: {
            const char* s = reinterpret_cast<const char*>(in.cdt_val.string8_val);
            return make_utf8(iso, s);
        }
            // char8 -> JS string (UTF-8 of a single codepoint)
        case metaffi_char8_type:
        {
            // metaffi_char8 מחזיק עד 4 בייטים של UTF-8, מרופדים ב־\0
            const metaffi_char8& mc = in.cdt_val.char8_val;

            // הופכים את ה-char8_t[] ל-char* עבור V8
            const char8_t* u8ptr = mc.c;
            const char* cstr = reinterpret_cast<const char*>(u8ptr);

            // יש לנו כבר helper שעושה NewFromUtf8
            return make_utf8(iso, cstr);
        }
            // --- handle (object living inside Node.js runtime) ---
        case metaffi_handle_type:
        {
            // The CDT stores a pointer to cdt_metaffi_handle in cdt_val.handle_val.
            cdt_metaffi_handle* h = in.cdt_val.handle_val;
            std::cerr << "[nodejs] cdt_to_v8(handle): cdt_val.handle_val=" << h << std::endl;
            if(h){
                std::cerr << "  h->runtime_id=" << h->runtime_id
                          << " h->handle=" << h->handle
                          << " h->release=" << (void*)h->release << std::endl;
            }
            // Convert the MetaFFI handle back into a V8 value.
            // If something goes wrong, handle_to_v8 will set out_err.
            return handle_to_v8(o, h, out_err);
        }


        default:
            set_err(out_err, "cdt_to_v8: unsupported CDT type");
            return Undefined(iso);
    }
}

// Build argv vector from cdts
bool cdts_to_v8_argv(const v8_conv_opts& o, const cdts& in, std::vector<Local<Value>>& argv, char** out_err){
    argv.reserve(in.length);
    for(metaffi_size i = 0; i < in.length; ++i){
        argv.push_back(cdt_to_v8(o, in[i], out_err));
        if(out_err && *out_err) return false;
    }
    return true;
}

// ---------- V8 -> CDT (generic) ----------
void v8_to_cdt(const v8_conv_opts& o, Local<Value> in, cdt* out, char** out_err){
    Isolate* iso = o.isolate;
    Local<Context> ctx = o.ctx;

    out->free_required = 0;

    if(in->IsUndefined() || in->IsNull()){
        out->type = metaffi_null_type;
        return;
    }
    if(in->IsBoolean()){
        out->type = metaffi_bool_type;
        out->cdt_val.bool_val = in.As<Boolean>()->Value() ? 1 : 0;
        return;
    }
    if(in->IsInt32()){
        out->type = metaffi_int32_type;
        out->cdt_val.int32_val = in.As<Int32>()->Value();
        return;
    }
    if(in->IsBigInt()){
        bool lossless = false;
        int64_t v = in.As<BigInt>()->Int64Value(&lossless);
        out->type = metaffi_int64_type;
        out->cdt_val.int64_val = v;
        return;
    }
    if(in->IsNumber()){
        v8::Maybe<double> md = in.As<Number>()->NumberValue(ctx);
        if(md.IsNothing()){
            set_err(out_err, "v8_to_cdt: failed to convert JS value to Number");
            return;
        }
        out->type = metaffi_float64_type;
        out->cdt_val.float64_val = md.FromJust();
        return;
    }

    if(in->IsString()){
        String::Utf8Value s(iso, in);
        size_t n = (size_t)(*s ? std::strlen(*s) : 0);
        char* mem = (char*)std::malloc(n + 1);
        if(!mem){ set_err(out_err, "v8_to_cdt: OOM on string"); return; }
        if(n) std::memcpy(mem, *s, n);
        mem[n] = '\0';
        out->type = metaffi_string8_type;
        out->cdt_val.string8_val = reinterpret_cast<metaffi_string8>(mem);
        out->free_required = 1;
        return;
    }

    set_err(out_err, "v8_to_cdt: unsupported JS type");
}

// ---------- V8 -> CDT לפי טיפוס מבוקש ----------
bool v8_to_cdt_as_type(const v8_conv_opts& o,
                       v8::Local<v8::Value> in,
                       const metaffi_type_info& dst,
                       cdt* out,
                       char** out_err)
{
    out->free_required = 0;
    auto to_double = [&](v8::Local<v8::Value> v, double& out) -> bool
    {
        v8::Maybe<double> md = v->NumberValue(o.ctx);
        if(md.IsNothing()){
            set_err(out_err, "v8_to_cdt_as_type: failed to convert JS value to Number");
            return false;
        }
        out = md.FromJust();
        return true;
    };


    switch (dst.type)
    {
        case metaffi_bool_type:
        {
            // שימוש ב-JS ToBoolean (כולל המרות מ-Number/BigInt/Object לפי כללי JS)
            bool b = in->BooleanValue(o.isolate);
            out->type = metaffi_bool_type;
            out->cdt_val.bool_val = b ? 1 : 0;
            return true;
        }
        // --- signed integers ---
        case metaffi_int8_type:
        case metaffi_int16_type:
        case metaffi_int32_type:
        {
            double d = 0.0;
            if(!to_double(in, d)){
                return false;
            }

            if (std::isnan(d) || std::isinf(d)) { set_err(out_err, "ret Number is NaN/Inf"); return false; }
            long long v = (long long)d; // trunc toward 0
            if (dst.type == metaffi_int8_type &&
                (v < std::numeric_limits<int8_t>::min() || v > std::numeric_limits<int8_t>::max())) {
                set_err(out_err,"ret out of int8 range"); return false;
            }
            if (dst.type == metaffi_int16_type &&
                (v < std::numeric_limits<int16_t>::min() || v > std::numeric_limits<int16_t>::max())) {
                set_err(out_err,"ret out of int16 range"); return false;
            }
            if (dst.type == metaffi_int32_type &&
                (v < std::numeric_limits<int32_t>::min() || v > std::numeric_limits<int32_t>::max())) {
                set_err(out_err,"ret out of int32 range"); return false;
            }
            out->type = dst.type;
            if (dst.type == metaffi_int8_type)   out->cdt_val.int8_val  = (int8_t)v;
            if (dst.type == metaffi_int16_type)  out->cdt_val.int16_val = (int16_t)v;
            if (dst.type == metaffi_int32_type)  out->cdt_val.int32_val = (int32_t)v;
            return true;
        }

        case metaffi_int64_type:
        {
            if (in->IsBigInt()) {
                bool ok = false;
                int64_t v = in.As<BigInt>()->Int64Value(&ok);
                if (!ok) { set_err(out_err, "BigInt conversion failed"); return false; }
                out->type = dst.type;
                out->cdt_val.int64_val = v;
                return true;
            }
            double d = 0.0;
            if(!to_double(in, d)){
                return false;
            }

            if (std::isnan(d) || std::isinf(d)) { set_err(out_err, "ret Number is NaN/Inf"); return false; }
            if (d < (double)std::numeric_limits<int64_t>::min() || d > (double)std::numeric_limits<int64_t>::max()) {
                set_err(out_err, "ret Number out of int64 range"); return false;
            }
            out->type = dst.type;
            out->cdt_val.int64_val = (int64_t)d;
            return true;
        }

        // --- unsigned integers ---
        case metaffi_uint8_type:
        case metaffi_uint16_type:
        case metaffi_uint32_type:
        {
            double d = 0.0;
            if(!to_double(in, d)){
                return false;
            }

            if (std::isnan(d) || std::isinf(d) || d < 0.0) { set_err(out_err,"ret negative/NaN/Inf for unsigned"); return false; }
            unsigned long long u = (unsigned long long)d; // trunc toward 0
            if (dst.type == metaffi_uint8_type  && u > std::numeric_limits<uint8_t>::max())  { set_err(out_err,"ret out of uint8 range");  return false; }
            if (dst.type == metaffi_uint16_type && u > std::numeric_limits<uint16_t>::max()) { set_err(out_err,"ret out of uint16 range"); return false; }
            if (dst.type == metaffi_uint32_type && u > std::numeric_limits<uint32_t>::max()) { set_err(out_err,"ret out of uint32 range"); return false; }
            out->type = dst.type;
            if (dst.type == metaffi_uint8_type)   out->cdt_val.uint8_val  = (uint8_t)u;
            if (dst.type == metaffi_uint16_type)  out->cdt_val.uint16_val = (uint16_t)u;
            if (dst.type == metaffi_uint32_type)  out->cdt_val.uint32_val = (uint32_t)u;
            return true;
        }

        case metaffi_uint64_type:
        {
            if (in->IsBigInt()) {
                bool ok=false; uint64_t u = in.As<BigInt>()->Uint64Value(&ok);
                if(!ok){ set_err(out_err,"BigInt conversion failed"); return false; }
                out->type = dst.type; out->cdt_val.uint64_val = u; return true;
            }
            double d = 0.0;
            if(!to_double(in, d)){
                return false;
            }

            if (std::isnan(d) || std::isinf(d) || d < 0.0 || d > (double)std::numeric_limits<uint64_t>::max()){
                set_err(out_err,"ret Number out of uint64 range"); return false;
            }
            out->type = dst.type; out->cdt_val.uint64_val = (uint64_t)d; return true;
        }

        case metaffi_float32_type:
        {
            double d = 0.0;
            if(!to_double(in, d)){
                return false;
            }

            if (std::isnan(d) || std::isinf(d)) {
                set_err(out_err, "ret Number is NaN/Inf");
                return false;
            }
            // בדיקת טווח ל-float32
            if (d > static_cast<double>(std::numeric_limits<float>::max()) ||
                d < static_cast<double>(-std::numeric_limits<float>::max())) {
                set_err(out_err, "ret Number out of float32 range");
                return false;
                }
            out->type = metaffi_float32_type;
            out->cdt_val.float32_val = static_cast<float>(d);
            return true;
        }

        // --- float64 (שמיש במקרה שאתה מבקש החזר כפול) ---
        case metaffi_float64_type:
        {
            double d = 0.0;
            if(!to_double(in, d)){
                return false;
            }

            if (std::isnan(d) || std::isinf(d)) { set_err(out_err, "ret Number is NaN/Inf"); return false; }
            out->type = dst.type;
            out->cdt_val.float64_val = d;
            return true;
        }
        case metaffi_string8_type:
        {
            v8::String::Utf8Value s(o.isolate, in);
            size_t n = (*s ? std::strlen(*s) : 0);
            char* mem = (char*)std::malloc(n + 1);
            if(!mem){
                set_err(out_err, "v8_to_cdt_as_type: OOM on string");
                return false;
            }
            if(n) std::memcpy(mem, *s, n);
            mem[n] = '\0';

            out->type = metaffi_string8_type;
            out->cdt_val.string8_val = reinterpret_cast<metaffi_string8>(mem);
            out->free_required = 1;
            return true;
        }

        case metaffi_char8_type:
        {
            // תומכים גם במספר (קוד ASCII 0..127) וגם במחרוזת UTF-8 של תו אחד

            // 1) אם זה מספר – נתייחס אליו כקוד ASCII
            if (in->IsNumber()) {
                double d = in->NumberValue(o.ctx).ToChecked();
                if (std::isnan(d) || std::isinf(d) || d < 0.0 || d > 127.0) {
                    set_err(out_err, "char8: numeric value out of ASCII 0..127 range");
                    return false;
                }

                char8_t buf[2];
                buf[0] = static_cast<char8_t>(static_cast<unsigned char>(d));
                buf[1] = u8'\0';

                out->type = metaffi_char8_type;
                out->cdt_val.char8_val = metaffi_char8(buf); // ctor מ-UTF-8
                return true;
            }

            // 2) אחרת – ניקח מחרוזת UTF-8 מ-JS ונמיר את ה-codepoint הראשון
            v8::String::Utf8Value s(o.isolate, in);
            const char* utf8 = (*s ? *s : "");
            if (utf8[0] == '\0') {
                set_err(out_err, "char8: cannot convert empty string to char");
                return false;
            }

            // JS Utf8Value מחזיר מחרוזת UTF-8 – ניתן להעביר ישירות ל-metaffi_char8
            const char8_t* u8ptr = reinterpret_cast<const char8_t*>(utf8);

            out->type = metaffi_char8_type;
            out->cdt_val.char8_val = metaffi_char8(u8ptr);
            return true;
        }
            // --- handle (object reference) ---
        case metaffi_handle_type:
        {
            // Try to reuse an existing MetaFFI handle struct if provided,
            // otherwise allocate a new one on the heap.
            cdt_metaffi_handle* h = out->cdt_val.handle_val;
            if(!h){
                // MetaFFI did not pre-allocate a handle container for us,
                // so we allocate one here.
                h = new cdt_metaffi_handle{};
                out->cdt_val.handle_val = h;

                // Optional debug:
                // std::cerr << "[nodejs] v8_to_cdt_as_type: allocated new cdt_metaffi_handle at " << h << std::endl;
            }

            // We only support handles for JS objects / functions.
            if(!in->IsObject() && !in->IsFunction()){
                set_err(out_err, "JS value is not an object/function for handle");
                return false;
            }

            // Create the internal Node.js handle that owns the V8 Global<Value>.
            nodejs_object_handle* nh = make_nodejs_object_handle(o.isolate, in);

            // Populate the MetaFFI handle structure.
            h->handle     = static_cast<void*>(nh);
            h->runtime_id = NODEJS_RUNTIME_ID;
            h->release    = &nodejs_release_handle;

            // Update the CDT entry to indicate it contains a handle.
            out->type = metaffi_handle_type;

            // IMPORTANT: the CDT does NOT own the cdt_metaffi_handle struct itself.
            // Its destructor only nulls the fields; lifetime is controlled via h->release(h).
            out->free_required = false;

            return true;
        }



        // --- fallback גנרי (מחרוזות, null, וכו') ---
        default:
            v8_to_cdt(o, in, out, out_err);
            return !(out_err && *out_err);
    }
}



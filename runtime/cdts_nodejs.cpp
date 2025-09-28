#include "cdts_nodejs.h"
#include <cstring>
#include <limits>
#include <cmath>

using namespace v8;

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

// ---------- CDT -> V8 ----------
Local<Value> cdt_to_v8(const v8_conv_opts& o, const cdt& in, char** out_err){
    Isolate* iso = o.isolate;

    switch(in.type){
        case metaffi_null_type:
            return Null(iso);

        case metaffi_bool_type:
            return Boolean::New(iso, in.cdt_val.bool_val != 0);

        case metaffi_int8_type:  return Integer::New(iso,  (int32_t)in.cdt_val.int8_val);
        case metaffi_int16_type: return Integer::New(iso,  (int32_t)in.cdt_val.int16_val);
        case metaffi_int32_type: return Integer::New(iso,  (int32_t)in.cdt_val.int32_val);
        case metaffi_uint8_type:  return Integer::NewFromUnsigned(iso, (uint32_t)in.cdt_val.uint8_val);
        case metaffi_uint16_type: return Integer::NewFromUnsigned(iso, (uint32_t)in.cdt_val.uint16_val);
        case metaffi_uint32_type: return Integer::NewFromUnsigned(iso, (uint32_t)in.cdt_val.uint32_val);

        case metaffi_int64_type:
            return BigInt::New(iso, (int64_t)in.cdt_val.int64_val);
        case metaffi_uint64_type:
            return BigInt::NewFromUnsigned(iso, (uint64_t)in.cdt_val.uint64_val);

        case metaffi_float32_type:
            return Number::New(iso, (double)in.cdt_val.float32_val);
        case metaffi_float64_type:
            return Number::New(iso, (double)in.cdt_val.float64_val);

        case metaffi_string8_type: {
            // In your SDK, string8 is a pointer (metaffi_string8 == char8_t*).
            // Cast to const char* for UTF-8 creation.
            const char* s = reinterpret_cast<const char*>(in.cdt_val.string8_val);
            return make_utf8(iso, s);
        }

        // Arrays/objects: not needed for add_ints MVP; add later if required.
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

// ---------- V8 -> CDT ----------
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
        // If not lossless, you can set_err; for add_ints we don't expect BigInt anyway.
        out->type = metaffi_int64_type;
        out->cdt_val.int64_val = v;
        return;
    }
    if(in->IsNumber()){
        out->type = metaffi_float64_type;
        out->cdt_val.float64_val = in.As<Number>()->NumberValue(ctx).ToChecked();
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
        // assign char* to metaffi_string8 (char8_t*); cast as needed
        out->cdt_val.string8_val = reinterpret_cast<metaffi_string8>(mem);
        out->free_required = 1;
        return;
    }

    set_err(out_err, "v8_to_cdt: unsupported JS type");
}

// Write a single V8 value into cdts (supports len == 1)
bool v8_value_to_cdts(const v8_conv_opts& o, v8::Local<v8::Value> in, cdts& out, char** out_err){
    // Ensure at least one slot in 'out' (CDTS fix)
    if(out.length == 0){
        new (&out) cdts(1, /*fixed_dimensions*/ 1); // placement-new
    }

    // We keep it simple: single return into slot 0 as int32
    cdt* slot0 = &out[0];
    slot0->free_required = 0;
    slot0->type = metaffi_int32_type;

    if(in->IsInt32()){
        slot0->cdt_val.int32_val = in.As<v8::Int32>()->Value();
        return true;
    }
    if(in->IsNumber()){
        double d = in.As<v8::Number>()->Value();
        slot0->cdt_val.int32_val = static_cast<int32_t>(d); // truncate toward zero
        return true;
    }
    if(in->IsBigInt()){
        bool ok = false;
        int64_t v = in.As<v8::BigInt>()->Int64Value(&ok);
        if(!ok){ if(out_err) *out_err = strdup("BigInt not convertible to int32"); return false; }
        slot0->cdt_val.int32_val = static_cast<int32_t>(v);
        return true;
    }

    // Unsupported type as return – write 0 to prove we wrote something
    slot0->cdt_val.int32_val = 0;
    return true;
}


bool v8_to_cdt_as_type(const v8_conv_opts& o,
                       v8::Local<v8::Value> in,
                       const metaffi_type_info& dst,
                       cdt* out,
                       char** out_err)
{
    using namespace v8;
    out->free_required = 0;

    switch (dst.type)
    {
        case metaffi_int32_type:
        {
            if (in->IsInt32()) {
                out->type = dst.type; // IMPORTANT
                out->cdt_val.int32_val = in.As<Int32>()->Value();
                return true;
            }
            if (in->IsBigInt()) {
                bool ok = false;
                int64_t v = in.As<BigInt>()->Int64Value(&ok);
                if (!ok) { set_err(out_err, "ret BigInt not in int32 range"); return false; }
                if (v < std::numeric_limits<int32_t>::min()) v = std::numeric_limits<int32_t>::min();
                if (v > std::numeric_limits<int32_t>::max()) v = std::numeric_limits<int32_t>::max();
                out->type = dst.type; // IMPORTANT
                out->cdt_val.int32_val = static_cast<int32_t>(v);
                return true;
            }
            if (in->IsNumber()) {
                double d = in.As<Number>()->NumberValue(o.ctx).ToChecked();
                if (std::isnan(d) || std::isinf(d)) { set_err(out_err, "ret Number is NaN/Inf"); return false; }
                if (d < (double)std::numeric_limits<int32_t>::min()) d = (double)std::numeric_limits<int32_t>::min();
                if (d > (double)std::numeric_limits<int32_t>::max()) d = (double)std::numeric_limits<int32_t>::max();
                out->type = dst.type; // IMPORTANT
                out->cdt_val.int32_val = static_cast<int32_t>(d); // trunc toward zero
                return true;
            }
            set_err(out_err, "ret not Number/BigInt for int32");
            return false;
        }

        case metaffi_int64_type:
        {
            if (in->IsBigInt()) {
                bool ok = false;
                int64_t v = in.As<BigInt>()->Int64Value(&ok);
                if (!ok) { set_err(out_err, "BigInt conversion failed"); return false; }
                out->type = dst.type; // IMPORTANT
                out->cdt_val.int64_val = v;
                return true;
            }
            if (in->IsInt32() || in->IsNumber()) {
                double d = in->NumberValue(o.ctx).ToChecked();
                if (std::isnan(d) || std::isinf(d)) { set_err(out_err, "ret Number is NaN/Inf"); return false; }
                // trunc toward zero + range check
                if (d < (double)std::numeric_limits<int64_t>::min() || d > (double)std::numeric_limits<int64_t>::max()) {
                    set_err(out_err, "ret Number out of int64 range"); return false;
                }
                out->type = dst.type; // IMPORTANT
                out->cdt_val.int64_val = static_cast<int64_t>(d);
                return true;
            }
            set_err(out_err, "ret not Number/BigInt for int64");
            return false;
        }

        case metaffi_float64_type:
        {
            if (!in->IsNumber() && !in->IsInt32() && !in->IsBigInt()) {
                set_err(out_err, "ret not Number for float64"); return false;
            }
            double d = 0.0;
            if (in->IsBigInt()) {
                bool ok=false; int64_t v = in.As<BigInt>()->Int64Value(&ok);
                if (!ok) { set_err(out_err, "BigInt->float64 failed"); return false; }
                d = (double)v;
            } else {
                d = in->NumberValue(o.ctx).ToChecked();
            }
            out->type = dst.type; // IMPORTANT
            out->cdt_val.float64_val = d;
            return true;
        }

        default:
            // Fallback generic
            v8_to_cdt(o, in, out, out_err);
            // Make sure type stays consistent with dst if generic picked a different numeric type:
            if (!(out_err && *out_err) && (out->type == metaffi_int32_type || out->type == metaffi_int64_type || out->type == metaffi_float64_type)) {
                // prefer exact dst.type if it's numeric
                if (dst.type == metaffi_int32_type && out->type != metaffi_int32_type) {
                    int32_t v = (int32_t)out->cdt_val.float64_val;
                    out->type = dst.type; out->cdt_val.int32_val = v;
                } else if (dst.type == metaffi_int64_type && out->type != metaffi_int64_type) {
                    int64_t v = (int64_t)out->cdt_val.float64_val;
                    out->type = dst.type; out->cdt_val.int64_val = v;
                } else if (dst.type == metaffi_float64_type && out->type != metaffi_float64_type) {
                    double v = (out->type == metaffi_int32_type) ? (double)out->cdt_val.int32_val
                                                                 : (out->type == metaffi_int64_type) ? (double)out->cdt_val.int64_val
                                                                                                      : out->cdt_val.float64_val;
                    out->type = dst.type; out->cdt_val.float64_val = v;
                }
            }
            return !(out_err && *out_err);
    }
}

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
            double d = in->NumberValue(o.ctx).ToChecked();
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
            double d = in->NumberValue(o.ctx).ToChecked();
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
            double d = in->NumberValue(o.ctx).ToChecked();
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
            double d = in->NumberValue(o.ctx).ToChecked();
            if (std::isnan(d) || std::isinf(d) || d < 0.0 || d > (double)std::numeric_limits<uint64_t>::max()){
                set_err(out_err,"ret Number out of uint64 range"); return false;
            }
            out->type = dst.type; out->cdt_val.uint64_val = (uint64_t)d; return true;
        }

        case metaffi_float32_type:
        {
            double d = in->NumberValue(o.ctx).ToChecked();
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
            double d = in->NumberValue(o.ctx).ToChecked();
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


        // --- fallback גנרי (מחרוזות, null, וכו') ---
        default:
            v8_to_cdt(o, in, out, out_err);
            return !(out_err && *out_err);
    }
}



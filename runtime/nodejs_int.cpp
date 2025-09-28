#include "nodejs_int.h"
#include <limits>
#include <cmath>

using namespace v8;

#ifdef _MSC_VER
#define strdup _strdup
#endif

static inline void set_err(char** err, const char* msg){ if(err) *err = strdup(msg); }

namespace nodejs_int {

// לשמירה על דיוק, מעבר ל־2^53-1 נעבוד עם BigInt
static constexpr int64_t JS_SAFE_MAX = (1LL<<53) - 1;
static constexpr int64_t JS_SAFE_MIN = -((1LL<<53) - 1);

static Local<Value> make_v8_int(Isolate* iso, int64_t v){
    if(v >= JS_SAFE_MIN && v <= JS_SAFE_MAX){
        return Number::New(iso, static_cast<double>(v));
    }
    return BigInt::New(iso, v);
}
static Local<Value> make_v8_uint(Isolate* iso, uint64_t v){
    if(v <= static_cast<uint64_t>(JS_SAFE_MAX)){
        return Number::New(iso, static_cast<double>(v));
    }
    return BigInt::NewFromUnsigned(iso, v);
}

bool to_v8(Isolate* iso, Local<Context> /*ctx*/, const cdt& in, Local<Value>& out, char** err)
{
    switch(in.type){
        case metaffi_int8_type:   out = make_v8_int (iso, static_cast<int64_t>(in.cdt_val.int8_val));  return true;
        case metaffi_int16_type:  out = make_v8_int (iso, static_cast<int64_t>(in.cdt_val.int16_val)); return true;
        case metaffi_int32_type:  out = make_v8_int (iso, static_cast<int64_t>(in.cdt_val.int32_val)); return true;
        case metaffi_int64_type:  out = make_v8_int (iso, static_cast<int64_t>(in.cdt_val.int64_val)); return true;

        case metaffi_uint8_type:  out = make_v8_uint(iso, static_cast<uint64_t>(in.cdt_val.uint8_val));  return true;
        case metaffi_uint16_type: out = make_v8_uint(iso, static_cast<uint64_t>(in.cdt_val.uint16_val)); return true;
        case metaffi_uint32_type: out = make_v8_uint(iso, static_cast<uint64_t>(in.cdt_val.uint32_val)); return true;
        case metaffi_uint64_type: out = make_v8_uint(iso, static_cast<uint64_t>(in.cdt_val.uint64_val)); return true;

        default:
            set_err(err, "nodejs_int::to_v8: unsupported CDT type (not integer)");
            return false;
    }
}

static bool read_bigint_to_int64(Local<BigInt> bi, int64_t& out, char** err){
    bool lossless = false;
    out = bi->Int64Value(&lossless);
    if(!lossless){ set_err(err, "BigInt does not fit in int64"); return false; }
    return true;
}
static bool read_bigint_to_uint64(Local<BigInt> bi, uint64_t& out, char** err){
    bool lossless = false;
    out = bi->Uint64Value(&lossless);
    if(!lossless){ set_err(err, "BigInt does not fit in uint64"); return false; }
    return true;
}

bool from_v8_to_int64(Isolate* iso, Local<Context> ctx, Local<Value> in, cdt& out, char** err)
{
    if(in->IsBigInt()){
        int64_t v = 0;
        if(!read_bigint_to_int64(in.As<BigInt>(), v, err)) return false;
        out.type = metaffi_int64_type;
        out.free_required = 0;
        out.cdt_val.int64_val = v;
        return true;
    }
    if(in->IsNumber()){
        double d = in->NumberValue(ctx).ToChecked();
        if(d < static_cast<double>(std::numeric_limits<int64_t>::min()) ||
           d > static_cast<double>(std::numeric_limits<int64_t>::max())){
            set_err(err, "Number out of int64 range");
            return false;
        }
        if(std::floor(d) != d){
            set_err(err, "Number is not an integer");
            return false;
        }
        out.type = metaffi_int64_type;
        out.free_required = 0;
        out.cdt_val.int64_val = static_cast<int64_t>(d);
        return true;
    }
    set_err(err, "Value is not Number/BigInt");
    return false;
}

bool from_v8_to_sized(Isolate* iso, Local<Context> ctx, Local<Value> in, int target_bits, bool is_signed, cdt& out, char** err)
{
    // קורא הערך ל־int64/uint64, ואז בודק טווחים לפי יעד
    int64_t  s_val = 0;
    uint64_t u_val = 0;

    if(in->IsBigInt()){
        if(is_signed){
            if(!read_bigint_to_int64(in.As<BigInt>(), s_val, err)) return false;
        }else{
            if(!read_bigint_to_uint64(in.As<BigInt>(), u_val, err)) return false;
        }
    }else if(in->IsNumber()){
        double d = in->NumberValue(ctx).ToChecked();
        if(std::floor(d) != d){ set_err(err, "Number is not an integer"); return false; }
        if(is_signed){
            if(d < static_cast<double>(std::numeric_limits<int64_t>::min()) ||
               d > static_cast<double>(std::numeric_limits<int64_t>::max())){
                set_err(err, "Number out of int64 range"); return false;
            }
            s_val = static_cast<int64_t>(d);
        }else{
            if(d < 0.0 || d > static_cast<double>(std::numeric_limits<uint64_t>::max())){
                set_err(err, "Number out of uint64 range"); return false;
            }
            u_val = static_cast<uint64_t>(d);
        }
    }else{
        set_err(err, "Value is not Number/BigInt");
        return false;
    }

    // כתיבה ל-CDT לפי יעד
    out.free_required = 0;
    if(is_signed){
        switch(target_bits){
            case 8:  out.type = metaffi_int8_type;  out.cdt_val.int8_val  = static_cast<int8_t>( s_val );  return true;
            case 16: out.type = metaffi_int16_type; out.cdt_val.int16_val = static_cast<int16_t>(s_val );   return true;
            case 32: out.type = metaffi_int32_type; out.cdt_val.int32_val = static_cast<int32_t>(s_val );   return true;
            case 64: out.type = metaffi_int64_type; out.cdt_val.int64_val = static_cast<int64_t>(s_val );   return true;
            default: set_err(err, "Unsupported signed width"); return false;
        }
    }else{
        switch(target_bits){
            case 8:  out.type = metaffi_uint8_type;  out.cdt_val.uint8_val  = static_cast<uint8_t>( u_val ); return true;
            case 16: out.type = metaffi_uint16_type; out.cdt_val.uint16_val = static_cast<uint16_t>(u_val ); return true;
            case 32: out.type = metaffi_uint32_type; out.cdt_val.uint32_val = static_cast<uint32_t>(u_val ); return true;
            case 64: out.type = metaffi_uint64_type; out.cdt_val.uint64_val = static_cast<uint64_t>(u_val ); return true;
            default: set_err(err, "Unsupported unsigned width"); return false;
        }
    }
}

} // namespace nodejs_int

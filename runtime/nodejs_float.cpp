#include "nodejs_float.h"
#include <limits>
#include <cmath>

using namespace v8;

#ifdef _MSC_VER
#define strdup _strdup
#endif

static inline void set_err(char** err, const char* msg){
    if(!err) return;
    if(*err) return;
    *err = strdup(msg ? msg : "nodejs_float: error");
}

namespace nodejs_float {

// CDT (float32/float64) -> V8 Number
bool to_v8(Isolate* iso, Local<Context> /*ctx*/, const cdt& in, Local<Value>& out, char** err) noexcept
{
    switch(in.type){
        case metaffi_float32_type:
            out = Number::New(iso, static_cast<double>(in.cdt_val.float32_val));
            return true;
        case metaffi_float64_type:
            out = Number::New(iso, static_cast<double>(in.cdt_val.float64_val));
            return true;
        default:
            set_err(err, "nodejs_float::to_v8: CDT is not a float type");
            out = Undefined(iso);
            return false;
    }
}

// Helper: convert Value to double (supports Number/Int32/BigInt)
static inline bool value_to_double(Isolate* iso, Local<Context> ctx, Local<Value> in, double& d, char** err) noexcept
{
    if (in->IsNumber() || in->IsInt32()){
        d = in->NumberValue(ctx).ToChecked();
        return true;
    }
    if (in->IsBigInt()){
        bool ok=false; int64_t v = in.As<BigInt>()->Int64Value(&ok);
        if(!ok){ set_err(err, "BigInt->double failed"); return false; }
        d = static_cast<double>(v);
        return true;
    }
    if (in->IsUndefined() || in->IsNull()){ d = 0.0; return true; }
    set_err(err, "Value is not Number/BigInt for float");
    return false;
}

// V8 Any -> CDT float64 (default)
bool from_v8_as_f64(Isolate* iso, Local<Context> ctx, Local<Value> in, cdt& out, char** err) noexcept
{
    double d;
    if(!value_to_double(iso, ctx, in, d, err)) return false;
    if (std::isnan(d) || std::isinf(d)){ set_err(err, "float64 is NaN/Inf"); return false; }

    out.type = metaffi_float64_type;
    out.free_required = 0;
    out.cdt_val.float64_val = d;
    return true;
}

// V8 -> CDT according to declared target type
bool from_v8_to_type(Isolate* iso, Local<Context> ctx, Local<Value> in,
                     const metaffi_type_info& dst, cdt& out, char** err) noexcept
{
    double d;
    if(!value_to_double(iso, ctx, in, d, err)) return false;
    if (std::isnan(d) || std::isinf(d)){ set_err(err, "float is NaN/Inf"); return false; }

    switch(dst.type){
        case metaffi_float32_type: {
            // Range check for float32
            if (d > static_cast<double>(std::numeric_limits<float>::max()) ||
                d < static_cast<double>(-std::numeric_limits<float>::max())) {
                set_err(err, "Number out of float32 range");
                return false;
            }
            out.type = metaffi_float32_type;
            out.free_required = 0;
            out.cdt_val.float32_val = static_cast<float>(d);
            return true;
        }
        case metaffi_float64_type: {
            out.type = metaffi_float64_type;
            out.free_required = 0;
            out.cdt_val.float64_val = d;
            return true;
        }
        default:
            set_err(err, "nodejs_float::from_v8_to_type: target type is not float");
            return false;
    }
}

} // namespace nodejs_float

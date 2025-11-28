#include "cdts_nodejs.h"

#include "nodejs_int.h"
#include "nodejs_bool.h"
#include "nodejs_float.h"
#include "nodejs_str.h"
#include "nodejs_char.h"
#include "nodejs_object.h"

#include <cstring>
#include <limits>
#include <cmath>
#include <iostream>

using namespace v8;

#ifdef _MSC_VER
#define strdup _strdup
#endif

// helper: set error only once
static inline void set_err(char** err, const char* msg)
{
    if (!err) return;
    if (*err) return;
    *err = strdup(msg ? msg : "cdts_nodejs: error");
}

// =====================================================
// CDT -> V8 (פריט בודד)
// =====================================================
Local<Value> cdt_to_v8(const v8_conv_opts& o, const cdt& in, char** out_err)
{
    if (out_err) *out_err = nullptr;

    Isolate* iso = o.isolate;
    Local<Context> ctx = o.ctx;
    Local<Value> out;

    switch (in.type)
    {
        case metaffi_null_type:
            return Null(iso);

        // --- bool ---
        case metaffi_bool_type:
            if (!nodejs_bool::to_v8(iso, ctx, in, out, out_err))
                return Undefined(iso);
            return out;

        // --- signed/unsigned integers ---
        case metaffi_int8_type:
        case metaffi_int16_type:
        case metaffi_int32_type:
        case metaffi_int64_type:
        case metaffi_uint8_type:
        case metaffi_uint16_type:
        case metaffi_uint32_type:
        case metaffi_uint64_type:
            if (!nodejs_int::to_v8(iso, ctx, in, out, out_err))
                return Undefined(iso);
            return out;

        // --- floats ---
        case metaffi_float32_type:
        case metaffi_float64_type:
            if (!nodejs_float::to_v8(iso, ctx, in, out, out_err))
                return Undefined(iso);
            return out;

        // --- string8 ---
        case metaffi_string8_type:
            if (!nodejs_str::to_v8(iso, ctx, in, out, out_err))
                return Undefined(iso);
            return out;

        // --- char8 ---
        case metaffi_char8_type:
            if (!nodejs_char::to_v8(iso, ctx, in, out, out_err))
                return Undefined(iso);
            return out;

        // --- handle (JS object/function) ---
        case metaffi_handle_type:
        {
            cdt_metaffi_handle* h = in.cdt_val.handle_val;
            return nodejs_object::handle_to_v8(iso, ctx, h, out_err);
        }

        default:
            set_err(out_err, "cdt_to_v8: unsupported CDT type");
            return Undefined(iso);
    }
}

// =====================================================
// CDTS -> argv
// =====================================================
bool cdts_to_v8_argv(const v8_conv_opts& o,
                     const cdts& in,
                     std::vector<Local<Value>>& argv,
                     char** out_err)
{
    if (out_err) *out_err = nullptr;
    argv.reserve(in.length);

    for (metaffi_size i = 0; i < in.length; ++i)
    {
        Local<Value> v = cdt_to_v8(o, in[i], out_err);
        if (out_err && *out_err)
            return false;
        argv.push_back(v);
    }
    return true;
}

// =====================================================
// V8 -> CDT (הסקה גנרית בלי טיפוס יעד)
// =====================================================
void v8_to_cdt(const v8_conv_opts& o, Local<Value> in, cdt* out, char** out_err)
{
    if (out_err) *out_err = nullptr;

    Isolate* iso = o.isolate;
    Local<Context> ctx = o.ctx;

    out->free_required = 0;

    // null / undefined
    if (in->IsUndefined() || in->IsNull())
    {
        out->type = metaffi_null_type;
        return;
    }

    // bool
    if (in->IsBoolean())
    {
        // לא אמור להיכשל
        (void)nodejs_bool::from_v8(iso, ctx, in, *out, out_err);
        return;
    }

    // BigInt -> int64
    if (in->IsBigInt())
    {
        (void)nodejs_int::from_v8_to_int64(iso, ctx, in, *out, out_err);
        return;
    }

    // Number -> float64
    if (in->IsNumber())
    {
        (void)nodejs_float::from_v8_as_f64(iso, ctx, in, *out, out_err);
        return;
    }

    // String -> string8
    if (in->IsString())
    {
        (void)nodejs_str::from_v8(iso, ctx, in, *out, out_err);
        return;
    }

    // Object / Function -> handle
    if (in->IsObject() || in->IsFunction())
    {
        (void)nodejs_object::js_value_to_handle(iso, ctx, in, *out, out_err);
        return;
    }

    // אחרת – unsupported
    set_err(out_err, "v8_to_cdt: unsupported JS type");
}

// =====================================================
// V8 -> CDT לפי טיפוס יעד (metaffi_type_info)
// =====================================================
bool v8_to_cdt_as_type(const v8_conv_opts& o,
                       Local<Value> in,
                       const metaffi_type_info& dst,
                       cdt* out,
                       char** out_err)
{
    if (out_err) *out_err = nullptr;

    Isolate* iso = o.isolate;
    Local<Context> ctx = o.ctx;

    out->free_required = 0;

    switch (dst.type)
    {
        // --- bool ---
        case metaffi_bool_type:
            return nodejs_bool::from_v8_to_type(iso, ctx, in, dst, *out, out_err);

        // --- signed integers ---
        case metaffi_int8_type:
        case metaffi_int16_type:
        case metaffi_int32_type:
        case metaffi_int64_type:
        {
            int bits = 0;
            switch (dst.type)
            {
                case metaffi_int8_type:  bits = 8;  break;
                case metaffi_int16_type: bits = 16; break;
                case metaffi_int32_type: bits = 32; break;
                case metaffi_int64_type: bits = 64; break;
                default: break;
            }
            if (bits == 0)
            {
                set_err(out_err, "v8_to_cdt_as_type: invalid signed int width");
                return false;
            }
            return nodejs_int::from_v8_to_sized(iso, ctx, in, bits, /*is_signed=*/true, *out, out_err);
        }

        // --- unsigned integers ---
        case metaffi_uint8_type:
        case metaffi_uint16_type:
        case metaffi_uint32_type:
        case metaffi_uint64_type:
        {
            int bits = 0;
            switch (dst.type)
            {
                case metaffi_uint8_type:  bits = 8;  break;
                case metaffi_uint16_type: bits = 16; break;
                case metaffi_uint32_type: bits = 32; break;
                case metaffi_uint64_type: bits = 64; break;
                default: break;
            }
            if (bits == 0)
            {
                set_err(out_err, "v8_to_cdt_as_type: invalid unsigned int width");
                return false;
            }
            return nodejs_int::from_v8_to_sized(iso, ctx, in, bits, /*is_signed=*/false, *out, out_err);
        }

        // --- floats ---
        case metaffi_float32_type:
        case metaffi_float64_type:
            return nodejs_float::from_v8_to_type(iso, ctx, in, dst, *out, out_err);

        // --- string8 ---
        case metaffi_string8_type:
            return nodejs_str::from_v8_to_type(iso, ctx, in, dst, *out, out_err);

        // --- char8 ---
        case metaffi_char8_type:
            return nodejs_char::from_v8_to_type(iso, ctx, in, dst, *out, out_err);

        // --- handle (JS object/function) ---
        case metaffi_handle_type:
            return nodejs_object::js_value_to_handle(iso, ctx, in, *out, out_err);

        // explicit null
        case metaffi_null_type:
            out->type = metaffi_null_type;
            out->free_required = 0;
            return true;

        // fallback: השתמש בהסקה הגנרית
        default:
            v8_to_cdt(o, in, out, out_err);
            return !(out_err && *out_err);
    }
}

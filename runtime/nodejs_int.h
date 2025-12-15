#pragma once
#include <v8.h>
#include <cstdint>
#include "../plugin-sdk-main/runtime/cdt.h"

namespace nodejs_int {

    // CDT (signed/unsigned integers) -> V8 Number/BigInt (without loss of precision)
    bool to_v8(v8::Isolate* iso,
               v8::Local<v8::Context> ctx,
               const cdt& in,
               v8::Local<v8::Value>& out,
               char** err);

    // V8 Number/BigInt -> CDT preferred integer type: int64 (or later: according to ret_type)
    bool from_v8_to_int64(v8::Isolate* iso,
                          v8::Local<v8::Context> ctx,
                          v8::Local<v8::Value> in,
                          cdt& out,
                          char** err);

    // Optional: V8 -> CDT according to a “narrow” target (int8/16/32/64 or unsigned)
    bool from_v8_to_sized(v8::Isolate* iso,
                          v8::Local<v8::Context> ctx,
                          v8::Local<v8::Value> in,
                          /* target */ int target_bits,
                          /* signed? */ bool is_signed,
                          cdt& out,
                          char** err);

} // namespace nodejs_int

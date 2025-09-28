#pragma once
#include <v8.h>
#include <cstdint>
#include "../plugin-sdk-main/runtime/cdt.h"

namespace nodejs_int {

    // CDT (signed/unsigned integers) -> V8 Number/BigInt (ללא אובדן דיוק)
    bool to_v8(v8::Isolate* iso,
               v8::Local<v8::Context> ctx,
               const cdt& in,
               v8::Local<v8::Value>& out,
               char** err);

    // V8 Number/BigInt -> CDT מסוג שלם *מועדף*: int64 (או בהמשך: לפי ret_type)
    bool from_v8_to_int64(v8::Isolate* iso,
                          v8::Local<v8::Context> ctx,
                          v8::Local<v8::Value> in,
                          cdt& out,
                          char** err);

    // אופציונלי: V8 -> CDT לפי יעד “דק” (int8/16/32/64 או unsigned)
    bool from_v8_to_sized(v8::Isolate* iso,
                          v8::Local<v8::Context> ctx,
                          v8::Local<v8::Value> in,
                          /* יעד */ int target_bits,
                          /* signed? */ bool is_signed,
                          cdt& out,
                          char** err);

} // namespace nodejs_int

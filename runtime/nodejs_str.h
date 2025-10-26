#pragma once
#include <v8.h>
#include "../plugin-sdk-main/runtime/cdt.h"

namespace nodejs_str {

    // CDT (string8) → V8 String
    bool to_v8(v8::Isolate* iso,
               v8::Local<v8::Context> ctx,
               const cdt& in,
               v8::Local<v8::Value>& out,
               char** err) noexcept;

    // V8 String → CDT string8
    bool from_v8(v8::Isolate* iso,
                 v8::Local<v8::Context> ctx,
                 v8::Local<v8::Value> in,
                 cdt& out,
                 char** err) noexcept;

    // V8 → CDT לפי טיפוס יעד (בדר"כ metaffi_string8_type)
    bool from_v8_to_type(v8::Isolate* iso,
                         v8::Local<v8::Context> ctx,
                         v8::Local<v8::Value> in,
                         const metaffi_type_info& dst,
                         cdt& out,
                         char** err) noexcept;
}

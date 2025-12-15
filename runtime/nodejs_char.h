#pragma once
#include <v8.h>
#include "../plugin-sdk-main/runtime/cdt.h"

namespace nodejs_char {

    // CDT (metaffi_char8_type) -> V8 String (UTF-8)
    bool to_v8(v8::Isolate* iso,
               v8::Local<v8::Context> ctx,
               const cdt& in,
               v8::Local<v8::Value>& out,
               char** err) noexcept;

    // V8 Any -> CDT metaffi_char8_type
    // - Number 0..127 → ASCII
    // - UTF-8 string → first character (up to 4 bytes)
    bool from_v8_to_type(v8::Isolate* iso,
                         v8::Local<v8::Context> ctx,
                         v8::Local<v8::Value> in,
                         const metaffi_type_info& dst,
                         cdt& out,
                         char** err) noexcept;
}

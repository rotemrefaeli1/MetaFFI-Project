#pragma once
#include <v8.h>
#include <cstdint>
#include "../plugin-sdk-main/runtime/cdt.h"

namespace nodejs_bool {

    // CDT (bool) -> V8 Boolean
    bool to_v8(v8::Isolate* iso,
               v8::Local<v8::Context> ctx,
               const cdt& in,
               v8::Local<v8::Value>& out,
               char** err) noexcept;

    // V8 Any -> CDT bool (according to JS ToBoolean semantics)
    bool from_v8(v8::Isolate* iso,
                 v8::Local<v8::Context> ctx,
                 v8::Local<v8::Value> in,
                 cdt& out,
                 char** err) noexcept;

    // Variant according to declared target type (must be metaffi_bool_type)
    bool from_v8_to_type(v8::Isolate* iso,
                         v8::Local<v8::Context> ctx,
                         v8::Local<v8::Value> in,
                         const metaffi_type_info& dst,
                         cdt& out,
                         char** err) noexcept;
}

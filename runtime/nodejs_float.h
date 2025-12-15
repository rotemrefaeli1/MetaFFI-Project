#pragma once
#include <v8.h>
#include <cstdint>
#include "../plugin-sdk-main/runtime/cdt.h"

namespace nodejs_float {

    // CDT (float32/float64) -> V8 Number
    bool to_v8(v8::Isolate* iso,
               v8::Local<v8::Context> ctx,
               const cdt& in,
               v8::Local<v8::Value>& out,
               char** err) noexcept;

    // V8 Any -> CDT float64 (a useful default when no target type is declared)
    bool from_v8_as_f64(v8::Isolate* iso,
                        v8::Local<v8::Context> ctx,
                        v8::Local<v8::Value> in,
                        cdt& out,
                        char** err) noexcept;

    // V8 -> CDT according to declared target type (metaffi_float32_type / metaffi_float64_type)
    bool from_v8_to_type(v8::Isolate* iso,
                         v8::Local<v8::Context> ctx,
                         v8::Local<v8::Value> in,
                         const metaffi_type_info& dst,
                         cdt& out,
                         char** err) noexcept;
}

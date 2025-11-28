#pragma once
#include <v8.h>
#include <cstdint>
#include "../plugin-sdk-main/runtime/cdt.h"

namespace nodejs_object
{
    // חייב להיות מסונכרן עם ה-runtime id של Node.js ב-MetaFFI
    extern const metaffi_uint64 NODEJS_RUNTIME_ID;

    // פונקציית release שתשב בתוך cdt_metaffi_handle::release
    void release_handle(cdt_metaffi_handle* h) noexcept;

    // המרת MetaFFI handle חזרה ל־V8 Value
    v8::Local<v8::Value> handle_to_v8(v8::Isolate* iso,
                                      v8::Local<v8::Context> ctx,
                                      const cdt_metaffi_handle* h,
                                      char** out_err);

    // המרת ערך JS (אובייקט/פונקציה) ל־MetaFFI handle בתוך CDT
    bool js_value_to_handle(v8::Isolate* iso,
                            v8::Local<v8::Context> ctx,
                            v8::Local<v8::Value> in,
                            cdt& out,
                            char** out_err) noexcept;
} // namespace nodejs_object

#pragma once
#include <v8.h>
#include <cstdint>
#include "../plugin-sdk-main/runtime/cdt.h"

namespace nodejs_object
{
    // Must be kept in sync with the Node.js runtime id in MetaFFI
    extern const metaffi_uint64 NODEJS_RUNTIME_ID;

    // Release function stored in cdt_metaffi_handle::release
    void release_handle(cdt_metaffi_handle* h) noexcept;

    // Convert a MetaFFI handle back to a V8 Value
    v8::Local<v8::Value> handle_to_v8(v8::Isolate* iso,
                                      v8::Local<v8::Context> ctx,
                                      const cdt_metaffi_handle* h,
                                      char** out_err);

    // Convert a JS value (object/function) into a MetaFFI handle stored in a CDT
    bool js_value_to_handle(v8::Isolate* iso,
                            v8::Local<v8::Context> ctx,
                            v8::Local<v8::Value> in,
                            cdt& out,
                            char** out_err) noexcept;
} // namespace nodejs_object

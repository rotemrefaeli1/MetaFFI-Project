#pragma once
#include <v8.h>
#include <vector>
#include "../plugin-sdk-main/runtime/cdt.h"

// Basic conversion options: isolate and context (must be the current context)
struct v8_conv_opts {
    v8::Isolate* isolate;
    v8::Local<v8::Context> ctx;
};

// CDT -> V8 (single item)
v8::Local<v8::Value> cdt_to_v8(const v8_conv_opts& o, const cdt& in, char** out_err);

// CDTS -> argv (parameter vector for invocation)
bool cdts_to_v8_argv(const v8_conv_opts& o,
                     const cdts& in,
                     std::vector<v8::Local<v8::Value>>& argv,
                     char** out_err);

// V8 -> CDT (generic; infers type based on the received value)
void v8_to_cdt(const v8_conv_opts& o,
               v8::Local<v8::Value> in,
               cdt* out,
               char** out_err);

// V8 -> CDT according to the requested type (metaffi_type_info)
// Used for declared parameters / return values
bool v8_to_cdt_as_type(const v8_conv_opts& o,
                       v8::Local<v8::Value> in,
                       const metaffi_type_info& dst,
                       cdt* out,
                       char** out_err);

#pragma once
#include <v8.h>
#include <vector>
#include <cstdlib>
#include "../plugin-sdk-main/runtime/cdt.h"

// Conversion options: isolate + context
struct v8_conv_opts {
    v8::Isolate* isolate;
    v8::Local<v8::Context> ctx;
};

// Single value conversions
v8::Local<v8::Value> cdt_to_v8(const v8_conv_opts& o, const cdt& in, char** out_err);
void v8_to_cdt(const v8_conv_opts& o, v8::Local<v8::Value> in, cdt* out, char** out_err);

// Helpers for argv and returns
bool cdts_to_v8_argv(const v8_conv_opts& o, const cdts& in, std::vector<v8::Local<v8::Value>>& argv, char** out_err);
bool v8_value_to_cdts(const v8_conv_opts& o, v8::Local<v8::Value> in, cdts& out, char** out_err); // supports len==1
bool v8_to_cdt_as_type(const v8_conv_opts& o, v8::Local<v8::Value> in, const metaffi_type_info& dst, cdt* out, char** out_err);





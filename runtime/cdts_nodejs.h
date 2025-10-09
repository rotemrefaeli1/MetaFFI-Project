#pragma once
#include <v8.h>
#include <vector>
#include "../plugin-sdk-main/runtime/cdt.h"

// אופציות המרה בסיסיות: איזולט וקונטקסט (חייב להיות current context)
struct v8_conv_opts {
    v8::Isolate* isolate;
    v8::Local<v8::Context> ctx;
};

// CDT -> V8 (פריט בודד)
v8::Local<v8::Value> cdt_to_v8(const v8_conv_opts& o, const cdt& in, char** out_err);

// CDTS -> argv (וקטור פרמטרים לקריאה)
bool cdts_to_v8_argv(const v8_conv_opts& o, const cdts& in, std::vector<v8::Local<v8::Value>>& argv, char** out_err);

// V8 -> CDT (גנרי; בוחר טיפוס לפי מה שמתקבל)
void v8_to_cdt(const v8_conv_opts& o, v8::Local<v8::Value> in, cdt* out, char** out_err);

// V8 -> CDT לפי הטיפוס המבוקש (metaffi_type_info) — לשימוש בהחזר/פרמטרים מוכרזים
bool v8_to_cdt_as_type(const v8_conv_opts& o,
                       v8::Local<v8::Value> in,
                       const metaffi_type_info& dst,
                       cdt* out,
                       char** out_err);


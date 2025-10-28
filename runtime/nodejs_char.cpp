#include <v8.h>
#include "cdts_nodejs.h"
#include "nodejs_str.h"

// CDT "char" (תו יחיד) -> v8::String
v8::Local<v8::String> v8_from_char(v8::Isolate* iso, const char* bytes, size_t len, char** out_err)
{
    if (!is_single_unicode_scalar_utf8(bytes, len)) {
        if (out_err) *out_err = strdup("char must contain exactly one Unicode scalar");
        return v8::Local<v8::String>();
    }
    return v8_from_utf8(iso, bytes, len, out_err);
}

// JS Value -> CDT "char" (תו יחיד)
bool v8_to_cdt_char(v8::Isolate* iso, v8::Local<v8::Value> v, cdts& out, char** out_err)
{
    if (!v->IsString()) {
        if (out_err) *out_err = strdup("expected JS string for char");
        return false;
    }
    v8::String::Utf8Value s(iso, v);
    const char* bytes = *s ? *s : "";
    size_t len = s.length();
    if (!is_single_unicode_scalar_utf8(bytes, len)) {
        if (out_err) *out_err = strdup("char result must contain exactly one Unicode scalar");
        return false;
    }
    cdt_set_char8(out, bytes, len);
    return true;
}

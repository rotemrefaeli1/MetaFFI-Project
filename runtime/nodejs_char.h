#pragma once
#include <v8.h>
#include "cdts_nodejs.h"

// CDT "char" (תו יוניקוד יחיד ב-UTF-8) -> v8::String
// אם הקלט איננו קוד-פוינט יחיד, תמולא out_err ויוחזר handle ריק.
v8::Local<v8::String>
v8_from_char(v8::Isolate* iso, const char* bytes, size_t len, char** out_err);

// JS Value -> CDT "char" (מחרוזת UTF-8 של קוד-פוינט יחיד)
// אם v אינו מחרוזת או שאינו תו יחיד, תמולא out_err ותחזור false.
bool
v8_to_cdt_char(v8::Isolate* iso, v8::Local<v8::Value> v, cdts& out, char** out_err);

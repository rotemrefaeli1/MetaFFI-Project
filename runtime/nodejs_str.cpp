#include "nodejs_str.h"
#include <cstring>

using namespace v8;

#ifdef _MSC_VER
#define strdup _strdup
#endif

static inline void set_err(char** err, const char* msg){
    if(!err) return;
    if(*err) return;
    *err = strdup(msg ? msg : "nodejs_str: error");
}

namespace nodejs_str {

    // CDT → V8 String
    bool to_v8(Isolate* iso, Local<Context> ctx, const cdt& in, Local<Value>& out, char** err) noexcept
    {
        if(in.type != metaffi_string8_type){
            set_err(err, "nodejs_str::to_v8: CDT is not metaffi_string8_type");
            out = Undefined(iso);
            return false;
        }

        const char* s = reinterpret_cast<const char*>(in.cdt_val.string8_val);
        out = String::NewFromUtf8(iso, s ? s : "", NewStringType::kNormal).ToLocalChecked();
        return true;
    }

    // V8 String → CDT
    bool from_v8(Isolate* iso, Local<Context> ctx, Local<Value> in, cdt& out, char** err) noexcept
    {
        if(!in->IsString()){
            set_err(err, "nodejs_str::from_v8: JS value is not a string");
            return false;
        }

        String::Utf8Value s(iso, in);
        size_t len = std::strlen(*s);
        char* mem = (char*)std::malloc(len + 1);
        if(!mem){ set_err(err, "nodejs_str::from_v8: OOM allocating string"); return false; }

        std::memcpy(mem, *s, len);
        mem[len] = '\0';

        out.type = metaffi_string8_type;
        out.cdt_val.string8_val = reinterpret_cast<metaffi_string8>(mem);
        out.free_required = 1;
        return true;
    }

    // לפי טיפוס יעד
    bool from_v8_to_type(Isolate* iso, Local<Context> ctx, Local<Value> in, const metaffi_type_info& dst, cdt& out, char** err) noexcept
    {
        if(dst.type != metaffi_string8_type){
            set_err(err, "nodejs_str::from_v8_to_type: target type is not metaffi_string8_type");
            return false;
        }
        return from_v8(iso, ctx, in, out, err);
    }

} // namespace nodejs_str

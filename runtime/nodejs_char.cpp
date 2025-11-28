#include "nodejs_char.h"
#include <cstring>
#include <cmath>

using namespace v8;

#ifdef _MSC_VER
#define strdup _strdup
#endif

static inline void set_err(char** err, const char* msg){
    if(!err) return;
    if(*err) return;
    *err = strdup(msg ? msg : "nodejs_char: error");
}

namespace nodejs_char {

    bool to_v8(Isolate* iso,
               Local<Context> ctx,
               const cdt& in,
               Local<Value>& out,
               char** err) noexcept
    {
        (void)ctx;

        if(in.type != metaffi_char8_type){
            set_err(err, "nodejs_char::to_v8: CDT is not metaffi_char8_type");
            out = Undefined(iso);
            return false;
        }

        const metaffi_char8& mc = in.cdt_val.char8_val;
        const char8_t* u8ptr    = mc.c;
        const char*   cstr      = reinterpret_cast<const char*>(u8ptr);

        out = String::NewFromUtf8(iso, cstr, NewStringType::kNormal).ToLocalChecked();
        return true;
    }

    bool from_v8_to_type(Isolate* iso,
                         Local<Context> ctx,
                         Local<Value> in,
                         const metaffi_type_info& dst,
                         cdt& out,
                         char** err) noexcept
    {
        if(dst.type != metaffi_char8_type){
            set_err(err, "nodejs_char::from_v8_to_type: target type is not metaffi_char8_type");
            return false;
        }

        // 1) מספר → ASCII 0..127
        if(in->IsNumber()){
            double d = in->NumberValue(ctx).ToChecked();
            if (std::isnan(d) || std::isinf(d) || d < 0.0 || d > 127.0) {
                set_err(err, "nodejs_char: numeric value out of ASCII 0..127 range");
                return false;
            }

            char8_t buf[2];
            buf[0] = static_cast<char8_t>(static_cast<unsigned char>(d));
            buf[1] = u8'\0';

            out.type          = metaffi_char8_type;
            out.free_required = 0;
            out.cdt_val.char8_val = metaffi_char8(buf);
            return true;
        }

        // 2) מחרוזת UTF-8: נשתמש בתו הראשון (עד 4 בייטים קידוד UTF-8)
        String::Utf8Value s(iso, in);
        const char* utf8 = (*s ? *s : "");
        if (utf8[0] == '\0') {
            set_err(err, "nodejs_char: cannot convert empty string to char8");
            return false;
        }

        const char8_t* u8ptr = reinterpret_cast<const char8_t*>(utf8);

        out.type          = metaffi_char8_type;
        out.free_required = 0;
        out.cdt_val.char8_val = metaffi_char8(u8ptr);
        return true;
    }

} // namespace nodejs_char

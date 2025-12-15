#include "nodejs_bool.h"

using namespace v8;

#ifdef _MSC_VER
#define strdup _strdup
#endif

static inline void set_err(char** err, const char* msg){
    if(!err) return;
    if(*err) return;
    *err = strdup(msg ? msg : "nodejs_bool: error");
}

namespace nodejs_bool {

    // CDT (bool) -> V8 Boolean
    bool to_v8(Isolate* iso, Local<Context> /*ctx*/, const cdt& in, Local<Value>& out, char** err) noexcept
    {
        if(in.type != metaffi_bool_type){
            set_err(err, "nodejs_bool::to_v8: CDT is not metaffi_bool_type");
            out = Undefined(iso);
            return false;
        }
        out = Boolean::New(iso, in.cdt_val.bool_val != 0);
        return true;
    }

    // V8 Any -> CDT bool (according to JS ToBoolean semantics)
    bool from_v8(Isolate* iso, Local<Context> /*ctx*/, Local<Value> in, cdt& out, char** /*err*/) noexcept
    {
        out.type = metaffi_bool_type;
        out.free_required = 0;
        out.cdt_val.bool_val = in->BooleanValue(iso) ? 1 : 0; // JS ToBoolean semantics
        return true;
    }

    // Variant according to declared target type
    bool from_v8_to_type(Isolate* iso, Local<Context> ctx, Local<Value> in,
                         const metaffi_type_info& dst, cdt& out, char** err) noexcept
    {
        if(dst.type != metaffi_bool_type){
            set_err(err, "nodejs_bool::from_v8_to_type: target type is not metaffi_bool_type");
            return false;
        }
        return from_v8(iso, ctx, in, out, err);
    }

} // namespace nodejs_bool

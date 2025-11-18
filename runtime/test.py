import metaffi
import metaffi.metaffi_runtime as mr
import metaffi.metaffi_types as mt

print("Python: Starting MetaFFI test")
rt = mr.MetaFFIRuntime('nodejs')
rt.load_runtime_plugin()

# Load the unified JS file
mod = rt.load_module('TestFunctions.js')
print("Python: module loaded")

# -------------------------------------------------
# helloMetaFFI (no params, no return)
# -------------------------------------------------
hello = mod.load_entity('callable=helloMetaFFI', None, None)
print("====helloMetaFFI====")
hello()
print("helloMetaFFI done")

# --- int32 ---
params_i32 = [
    mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int32_type),
    mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int32_type)
]
ret_i32 = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int32_type)]
add_ints = mod.load_entity('callable=add_ints', params_i32, ret_i32)
print("====add_ints====")
print("add_ints(7, 2) =", add_ints(7, 2))

# --- int64 ---
params_i64 = [
    mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int64_type),
    mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int64_type)
]
ret_i64 = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int64_type)]
add_ints64 = mod.load_entity('callable=add_ints64', params_i64, ret_i64)
print("====add_ints64====")
print("add_ints64(2**40, 2**40) =", add_ints64(2**40, 2**40))

# --- float32 ---
params_f32 = [
    mt.metaffi_type_info(mt.MetaFFITypes.metaffi_float32_type),
    mt.metaffi_type_info(mt.MetaFFITypes.metaffi_float32_type)
]
ret_f32 = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_float32_type)]
add_f32 = mod.load_entity('callable=add_f32', params_f32, ret_f32)
print("====add_f32====")
print("add_f32(1.5, 2.25) =", add_f32(1.5, 2.25))

# --- float64 ---
params_f64 = [
    mt.metaffi_type_info(mt.MetaFFITypes.metaffi_float64_type),
    mt.metaffi_type_info(mt.MetaFFITypes.metaffi_float64_type)
]
ret_f64 = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_float64_type)]
add_f64 = mod.load_entity('callable=add_f64', params_f64, ret_f64)
print("====add_f64====")
print("add_f64(1.5, 2.25) =", add_f64(1.5, 2.25))

# --- div_i64 (int64) ---
i64 = mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int64_type)
div_i64 = mod.load_entity('callable=div_i64', [i64, i64], [i64])
print("====div_i64====")
print("div_i64(6, 3) ->", div_i64(6, 3))
print("div_i64(5, 0) ->", div_i64(5, 0))

# --- booleans ---
params_b = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_bool_type)]
ret_b = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_bool_type)]
invert_bool = mod.load_entity('callable=invert_bool', params_b, ret_b)
print("====invert_bool====")
print("invert_bool(True)  =", invert_bool(True))
print("invert_bool(False) =", invert_bool(False))

params_bb = [
    mt.metaffi_type_info(mt.MetaFFITypes.metaffi_bool_type),
    mt.metaffi_type_info(mt.MetaFFITypes.metaffi_bool_type)
]
ret_bb = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_bool_type)]
and_bool = mod.load_entity('callable=and_bool', params_bb, ret_bb)
print("====and_bool====")
print("and_bool(True, False) =", and_bool(True, False))

# --- strings ---
str_t = mt.metaffi_type_info(mt.MetaFFITypes.metaffi_string8_type)

echo = mod.load_entity('callable=echo', [str_t], [str_t])
print("====echo====")
print("echo('hello metaffi') =", echo("hello metaffi"))
print("echo('metaffi ') =", echo("metaffi "))

greet = mod.load_entity('callable=greet', [str_t], [str_t])
print("====greet====")
print("greet('Sagi') =", greet("Sagi"))


# --- char8: to_upper_char ---
char_t = mt.metaffi_type_info(mt.MetaFFITypes.metaffi_char8_type)

to_upper_char = mod.load_entity(
    'callable=to_upper_char',
    [char_t],      # פרמטר אחד מסוג char8
    [char_t]       # החזרה char8
)

print("====to_upper_char====")
print("to_upper_char('a') =", to_upper_char('a'))

# --- cleanup ---
del hello, add_ints, add_ints64, add_f32, add_f64, div_i64, invert_bool, and_bool, echo, greet, to_upper_char
del mod

rt.release_runtime_plugin()
del rt
print("Python: Test complete")

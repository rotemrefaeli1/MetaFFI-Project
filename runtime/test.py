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
    [char_t],
    [char_t]
)

print("====to_upper_char====")
print("to_upper_char('a') =", to_upper_char('a'))

# ---------------- OBJECT / HANDLE TEST ----------------
# (יישור קו עם Java: int64)
print("==== OBJECT / HANDLE TEST ====")

handle_t = mt.metaffi_type_info(mt.MetaFFITypes.metaffi_handle_type)
i64_t = mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int64_type)

create_counter = mod.load_entity('callable=create_counter', [i64_t], [handle_t])
counter_get    = mod.load_entity('callable=counter_get',    [handle_t], [i64_t])
counter_inc    = mod.load_entity('callable=counter_inc',    [handle_t, i64_t], [i64_t])

c = create_counter(10)
print("handle returned from create_counter(10) =", c)
print("counter_get(c) ->", counter_get(c))
print("counter_inc(c, 5) ->", counter_inc(c, 5))
print("counter_get(c) ->", counter_get(c))

# ==================== TYPED ARRAY TESTS ====================

print("\n==== TYPED ARRAY TESTS (Python) ====")

# --- string8[] + join ---
print("\n-- string8[] + join --")
str8_arr_t = mt.metaffi_type_info(mt.MetaFFITypes.metaffi_string8_array_type)

make_string_array = mod.load_entity('callable=make_string_array', None, [str8_arr_t])
join_string_array = mod.load_entity('callable=join_string_array', [str8_arr_t], [str_t])

sarr = make_string_array()
print("make_string_array ->", sarr, "(type:", type(sarr), ")")
print("join_string_array(sarr) ->", join_string_array(sarr))

# --- int32[] + sum ---
print("\n-- int32[] + sum --")
i32_t = mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int32_type)
i32_arr_t = mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int32_array_type)

make_int32_array = mod.load_entity('callable=make_int32_array', None, [i32_arr_t])
sum_int32_array  = mod.load_entity('callable=sum_int32_array',  [i32_arr_t], [i32_t])

iarr = make_int32_array()
print("make_int32_array ->", iarr, "(type:", type(iarr), ")")
print("sum_int32_array(iarr) ->", sum_int32_array(iarr))

# --- handle[] (objects) + sum_ids ---
print("\n-- handle[] (objects) + sum_ids --")
handle_arr_t = mt.metaffi_type_info(mt.MetaFFITypes.metaffi_handle_array_type)

make_object_array = mod.load_entity('callable=make_object_array', None, [handle_arr_t])
sum_ids           = mod.load_entity('callable=sum_ids', [handle_arr_t], [i32_t])

obj_arr = make_object_array()
print("make_object_array ->", obj_arr, "(type:", type(obj_arr), ")")
print("sum_ids(obj_arr) ->", sum_ids(obj_arr), "(expected 6)")

# ==================== ANY[] BASIC TEST ====================

print("\n==== ANY[] BASIC TEST (Python) ====")
any_arr_t = mt.metaffi_type_info(mt.MetaFFITypes.metaffi_any_array_type)

get_any_array_basic = mod.load_entity('callable=get_any_array_basic', None, [any_arr_t])
any_arr = get_any_array_basic()

print("get_any_array_basic ->", any_arr, "(type:", type(any_arr), ")")
if isinstance(any_arr, (list, tuple)):
    print("length =", len(any_arr))
    for i, el in enumerate(any_arr):
        print(f"  a[{i}] = {el} (py type: {type(el)})")


# --- cleanup ---
del hello, add_ints, add_ints64, add_f32, add_f64, div_i64
del invert_bool, and_bool, echo, greet, to_upper_char
del create_counter, counter_get, counter_inc
del make_string_array, join_string_array, make_int32_array, sum_int32_array, make_object_array, sum_ids
del get_any_array_basic
del mod

rt.release_runtime_plugin()
del rt
print("Python: Test complete")

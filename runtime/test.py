# ---------------------------------------------------------------------------------------
# working:

# import metaffi
# import metaffi.metaffi_runtime
# import metaffi.metaffi_module
#
# runtime = metaffi.metaffi_runtime.MetaFFIRuntime('nodejs')
# runtime.load_runtime_plugin()
#
# mod = runtime.load_module('hello.js')
# func = mod.load_entity('callable=helloMetaFFI', None, None)
#
# try:
#     func()  # הקריאה עצמה
# finally:
#     # שחרור מפורש של ה-entity לפני runtime
#     del func
#     del mod
#     runtime.release_runtime_plugin()  # משחרר את ה-isolate בסוף
#     del runtime
# #---------------------------------------------------------
# #
# # import metaffi
# # import metaffi.metaffi_runtime as mr
# # import metaffi.metaffi_types as mt
# #
# # rt = mr.MetaFFIRuntime('nodejs')
# # rt.load_runtime_plugin()
# #
# # mod_add = rt.load_module('add.js')
# #
# # # int32
# # params32 = [
# #     mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int32_type),
# #     mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int32_type),
# # ]
# # ret32 = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int32_type)]
# # add_ints = mod_add.load_entity('callable=add_ints', params32, ret32)
# # print("add_ints(7, 2) =", add_ints(7, 2))
# #
# # # int64
# # params64 = [
# #     mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int64_type),
# #     mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int64_type),
# # ]
# # ret64 = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int64_type)]
# # add_ints64 = mod_add.load_entity('callable=add_ints64', params64, ret64)
# # print("add_ints64(2**40, 2**40) =", add_ints64(2**40, 2**40))
# #
# # # cleanup
# # del add_ints, add_ints64
# # del mod_add
# # rt.release_runtime_plugin()
# # del rt
#----------------------------------------------------------------------------

import metaffi
import metaffi.metaffi_runtime as mr
import metaffi.metaffi_types as mt

rt = mr.MetaFFIRuntime('nodejs')
rt.load_runtime_plugin()

# -------------------------------------------------
# hello.js (no params, no return)
# -------------------------------------------------
mod_hello = rt.load_module('hello.js')
hello = mod_hello.load_entity('callable=helloMetaFFI', None, None)

print("=== hello.js ===")
hello()


# --- load add module ---
mod_add = rt.load_module('add.js')

# --- int32 ---
params_i32 = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int32_type),
              mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int32_type)]
ret_i32 = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int32_type)]
add_ints = mod_add.load_entity('callable=add_ints', params_i32, ret_i32)
print("add_ints(7, 2) =", add_ints(7, 2))

# --- int64 ---
params_i64 = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int64_type),
              mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int64_type)]
ret_i64 = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int64_type)]
add_ints64 = mod_add.load_entity('callable=add_ints64', params_i64, ret_i64)
print("add_ints64(2**40, 2**40) =", add_ints64(2**40, 2**40))

# --- float32 (add_f32 ב-add.js) ---
params_f32 = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_float32_type),
              mt.metaffi_type_info(mt.MetaFFITypes.metaffi_float32_type)]
ret_f32 = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_float32_type)]
add_f32 = mod_add.load_entity('callable=add_f32', params_f32, ret_f32)
print("add_f32(1.5, 2.25) =", add_f32(1.5, 2.25))

# --- float64 (add_f64 ב-add.js) ---
params_f64 = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_float64_type),
              mt.metaffi_type_info(mt.MetaFFITypes.metaffi_float64_type)]
ret_f64 = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_float64_type)]
add_f64 = mod_add.load_entity('callable=add_f64', params_f64, ret_f64)
print("add_f64(1.5, 2.25) =", add_f64(1.5, 2.25))

# --- מודול הבוליאני ---
mod_bool = rt.load_module('booleanTest.js')

# invert_bool: bool -> bool
params_b = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_bool_type)]
ret_b    = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_bool_type)]
invert_bool = mod_bool.load_entity('callable=invert_bool', params_b, ret_b)
print("invert_bool(True)  =", invert_bool(True))
print("invert_bool(False) =", invert_bool(False))

# and_bool: bool,bool -> bool (אם יש לך גם פונקציה כזו)
params_bb = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_bool_type),
             mt.metaffi_type_info(mt.MetaFFITypes.metaffi_bool_type)]
ret_bb    = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_bool_type)]
and_bool = mod_bool.load_entity('callable=and_bool', params_bb, ret_bb)
print("and_bool(True, False) =", and_bool(True, False))

# --- ניקוי ---
del add_ints, add_ints64, add_f32, add_f64, invert_bool, and_bool
del mod_add, mod_bool
rt.release_runtime_plugin()
del rt

#working
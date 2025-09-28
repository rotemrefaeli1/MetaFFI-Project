#---------------------------------------------------------------------------------------
#working:

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
#-------------------------------------------------------------------------------------------------------
import metaffi
import metaffi.metaffi_runtime as mr
import metaffi.metaffi_types as mt

rt = mr.MetaFFIRuntime('nodejs')
rt.load_runtime_plugin()

mod_add = rt.load_module('add.js')

params = [
    mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int32_type),
    mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int32_type),
]
ret = [mt.metaffi_type_info(mt.MetaFFITypes.metaffi_int32_type)]


add_ints = mod_add.load_entity('callable=add_ints', params, ret)

print("add_ints(7,2) =", add_ints(7, 2))

# cleanup
del add_ints
del mod_add
rt.release_runtime_plugin()
del rt



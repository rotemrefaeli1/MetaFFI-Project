import metaffi
import metaffi.metaffi_runtime
import metaffi.metaffi_module

runtime = metaffi.metaffi_runtime.MetaFFIRuntime('nodejs')
runtime.load_runtime_plugin()

mod = runtime.load_module('hello.js')
func = mod.load_entity('callable=helloMetaFFI', None, None)

try:
    func()  # הקריאה עצמה
finally:
    # שחרור מפורש של ה-entity לפני runtime
    del func
    del mod
    runtime.release_runtime_plugin()  # משחרר את ה-isolate בסוף
    del runtime

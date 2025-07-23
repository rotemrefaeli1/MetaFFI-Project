import metaffi
import metaffi.metaffi_runtime
import metaffi.metaffi_module

# טען את ה־runtime של Node.js
runtime = metaffi.metaffi_runtime.MetaFFIRuntime('nodejs')
runtime.load_runtime_plugin()

# טען את קובץ ה־JS
mod = runtime.load_module('hello.js')

# טען את הפונקציה
func = mod.load_entity('callable=helloMetaFFI', None, None)

# הרץ את הפונקציה
func()

import metaffi
import metaffi.metaffi_runtime
import metaffi.metaffi_module

# Load the Node.js runtime into MetaFFI
runtime = metaffi.metaffi_runtime.MetaFFIRuntime('nodejs')
runtime.load_runtime_plugin()

# Load the JavaScript module (hello.js)
mod = runtime.load_module('hello.js')

# Load the JavaScript function by specifying its callable name
func = mod.load_entity('callable=helloMetaFFI', None, None)

# Execute the loaded function
func()

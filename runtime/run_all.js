const ffi = require('ffi-napi');
const ref = require('ref-napi');
const path = require('path');

/*
 * char**
 */
const charPtrPtr = ref.refType(ref.types.CString);

/*
 * Load XLLR
 * IMPORTANT: use absolute path
 */
const xllrPath = "/usr/local/metaffi/xllr.so";

const xllr = ffi.Library(xllrPath, {
    load_runtime_plugin: ['void', ['string', charPtrPtr]]
});

/*
 * Helper to call XLLR and handle error
 */
function loadRuntime(runtimeName) {
    const errPtr = ref.alloc(charPtrPtr);

    xllr.load_runtime_plugin(runtimeName, errPtr);

    const errStrPtr = errPtr.deref();

    if (!errStrPtr.isNull()) {
        const msg = errStrPtr.readCString();
        throw new Error(`XLLR error loading ${runtimeName}: ${msg}`);
    }

    console.log(`✔ Runtime "${runtimeName}" loaded successfully`);
}

/*
 * === TEST ===
 * Load Python and Java runtimes
 */
try {
    loadRuntime("xllr.python311");
    loadRuntime("xllr.openjdk");

    console.log("All runtimes loaded successfully");
} catch (e) {
    console.error(e.message);
    process.exit(1);
}

// runtimes already loaded here

const py = require('./py_api.py');
const java = require('./JavaAPI.class');

console.log('Python add:', py.add(2, 3));
console.log('Java add:', java.add(4, 5));


const ffi = require('ffi-napi');

// load dynamic library
const lib = ffi.Library('./hello_world', {
    'hello_world': ['void', []]  // function signature
});

// function caller
lib.hello_world();

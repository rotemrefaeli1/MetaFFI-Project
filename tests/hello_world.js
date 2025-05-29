const ffi = require('ffi-napi');
const ref = require('ref-napi');

// Define a char array pointer type for output buffer
const charPtr = ref.refType('char');

const lib = ffi.Library('./hello_world', {
    'hello_world': ['void', []],
    'uppercase': ['void', ['string', 'char *']],
    'add': ['int', ['int', 'int', 'int']],
    'integer_div': ['int', ['int', 'int']]
});

// Call hello
lib.hello_world();

// Call uppercase
const inputStr = "hello";
const outputBuf = Buffer.alloc(100);  // allocate output buffer
lib.uppercase(inputStr, outputBuf);
console.log("Uppercase:", outputBuf.toString('utf8').split('\0')[0]);

// Call add
const resultAdd = lib.add(10, 20, 30);
console.log("Add result:", resultAdd);

// Call integer_div
const resultDiv = lib.integer_div(100, 5);
console.log("Integer division result:", resultDiv);

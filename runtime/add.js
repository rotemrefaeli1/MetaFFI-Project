// Force int32 on inputs and result
// add.js
function add_ints(a, b) {
    // Force Number so BigInt from V8 לא יפיל |0
    return (Number(a) | 0) + (Number(b) | 0) | 0;
}
globalThis.add_ints = add_ints;

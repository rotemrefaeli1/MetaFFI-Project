// Force int32 on inputs and result
// add.js
function add_ints(a, b) {
    // Force Number so BigInt from V8 לא יפיל |0
    return (Number(a) | 0) + (Number(b) | 0) | 0;
}
globalThis.add_ints = add_ints;


//******************************another type for checking

function add_ints64(a, b) {
    return BigInt(a) + BigInt(b);
}
globalThis.add_ints64 = add_ints64;


// --- FLOATS ---

// float32: נשתמש ב-Math.fround כדי לדמות דיוק יחיד (single precision)
function add_f32(a, b) {
    const x = Math.fround(Number(a));
    const y = Math.fround(Number(b));
    return Math.fround(x + y);
}
globalThis.add_f32 = add_f32;

// float64: Number רגיל (double precision)
function add_f64(a, b) {
    return Number(a) + Number(b);
}
globalThis.add_f64 = add_f64;


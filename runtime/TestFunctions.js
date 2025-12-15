function helloMetaFFI() {
    console.log("Hello from Node.js via MetaFFI!");
}

// Force int32 on inputs and result
// add.js
function add_ints(a, b) {
    // Force Number so BigInt from V8 will not break the |0 operation
    return (Number(a) | 0) + (Number(b) | 0) | 0;
}

// ****************************** another type for checking

function add_ints64(a, b) {
    return BigInt(a) + BigInt(b);
}

// --- FLOATS ---

// float32: use Math.fround to simulate single-precision behavior
function add_f32(a, b) {
    const x = Math.fround(Number(a));
    const y = Math.fround(Number(b));
    return Math.fround(x + y);
}

// float64: regular Number (double precision)
function add_f64(a, b) {
    return Number(a) + Number(b);
}

function div_i64(a, b) {
    console.log(typeof a, typeof b);
    return (a / b);
}

// Boolean test function: invert true/false
function invert_bool(b) {
    return !Boolean(b);
}

// Optional: logical AND
function and_bool(a, b) {
    return Boolean(a) && Boolean(b);
}

function echo(s) {
    return s;
}

function greet(name) {
    return `Hello, ${name}!`;
}

function to_upper_char(c) {
    // Ensure we are working with a string
    if (typeof c !== 'string') {
        c = String(c);
    }
    if (c.length === 0) {
        return "";
    }
    return c[0].toUpperCase();
}

// --------------------------- OBJECT / HANDLE TEST ---------------------------

// A simple object with internal state
class Counter {
    constructor(start) {
        this.value = start || 0;
    }

    inc(delta) {
        this.value += delta;
        return this.value;
    }

    get() {
        return this.value;
    }
}

// Create a new Counter object (MetaFFI will wrap this inside a handle)
function create_counter(start) {
    return new Counter(start);
}

// Methods that receive the object via a MetaFFI handle
function counter_get(self) {
    return self.get();
}

function counter_inc(self, delta) {
    return self.inc(delta);
}

// ===== Array<any> tests =====

// ===== string8[] =====
function make_string_array() {
    return ["hello", "from", "MetaFFI"];
}

function join_string_array(arr) {
    // Real operation: join + type check
    if (!Array.isArray(arr)) throw new Error("join_string_array: expected Array");
    return arr.map(x => String(x)).join("-");
}

// ===== int32[] =====
function make_int32_array() {
    return [1, 2, 3, -7, 42];
}

function sum_int32_array(arr) {
    // Real operation: sum (with int32 semantics)
    if (!Array.isArray(arr)) throw new Error("sum_int32_array: expected Array");
    let s = 0;
    for (const x of arr) {
        // Enforce int32 semantics
        s = (s + (Number(x) | 0)) | 0;
    }
    return s;
}

function make_object_array() {
    return [
        { id: 1, name: "Alice" },
        { id: 2, name: "Bob" },
        { id: 3, name: "Meta" }
    ];
}

function sum_ids(objs) {
    if (!Array.isArray(objs)) throw new Error("sum_ids: expected Array");
    let s = 0;
    for (const o of objs) {
        if (o == null || typeof o !== "object") {
            throw new Error("sum_ids: element is not an object");
        }
        s += Number(o.id) || 0;
    }
    return s; // Number -> usually float64 (or int32 if enforced)
}

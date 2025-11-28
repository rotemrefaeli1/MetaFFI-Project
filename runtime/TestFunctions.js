function helloMetaFFI() {
    console.log("Hello from Node.js via MetaFFI!");

}

// Force int32 on inputs and result
// add.js
function add_ints(a, b) {
    // Force Number so BigInt from V8 לא יפיל |0
    return (Number(a) | 0) + (Number(b) | 0) | 0;
}


//******************************another type for checking

function add_ints64(a, b) {
    return BigInt(a) + BigInt(b);
}


// --- FLOATS ---

// float32: נשתמש ב-Math.fround כדי לדמות דיוק יחיד (single precision)
function add_f32(a, b) {
    const x = Math.fround(Number(a));
    const y = Math.fround(Number(b));
    return Math.fround(x + y);
}

// float64: Number רגיל (double precision)
function add_f64(a, b) {
    return Number(a) + Number(b);
}
function div_i64(a, b){
    console.log(typeof a, typeof b);

    return (a / b) ;
}
// פונקציה לבדיקת bool: הופכת אמת/שקר
function invert_bool(b) {
    return !Boolean(b);
}

// אופציונלי: גם AND לוגי
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
    // נוודא שמקבלים string
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
// run_all.js (fixed)

// ---------- imports ----------
const ffi = require("ffi-napi");
const ref = require("ref-napi");
const Struct = require("ref-struct-di")(ref);
const ArrayType = require("ref-array-di")(ref);

// ---------- basic types ----------
const voidPtr = ref.refType(ref.types.void);
const charPtr = ref.refType(ref.types.char);
const charPtrPtr = ref.refType(charPtr);

const u8 = ref.types.uint8;
const i8 = ref.types.int8;
const u64 = ref.types.uint64;
const i64 = ref.types.int64;

// MetaFFI type constants
const metaffi_float64_type = 1n;
const metaffi_int32_type = 16n;
const metaffi_int64_type = 32n;
const metaffi_char8_type = 2048n;
const metaffi_string8_type = 4096n;

const MIXED_OR_UNKNOWN_DIMENSIONS = -1n;

function u64arg(x) {
    return typeof x === "bigint" ? x.toString() : String(x);
}
function i64arg(x) {
    return typeof x === "bigint" ? x.toString() : String(x);
}
function i8arg(x) {
    return x | 0;
}

const UChar7 = ArrayType(u8, 7);

// struct metaffi_type_info:
// u64 type, char* alias, u8 is_free_alias, pad[7], i64 fixed_dimensions
const metaffi_type_info = Struct({
    type: u64,
    alias: charPtr,
    is_free_alias: u8,
    _pad: UChar7,
    fixed_dimensions: i64,
});

// struct cdt:
// u64 type, union(8 bytes) modeled as u64, u8 free_required, pad[7]
const cdt = Struct({
    type: u64,
    cdt_val: u64,
    free_required: u8,
    _pad: UChar7,
});

// struct cdts:
// cdt* arr, u64 length, i64 fixed_dimensions, u8 allocated_on_cache, pad[7]
const cdtPtr = ref.refType(cdt);
const cdts = Struct({
    arr: cdtPtr,
    length: u64,
    fixed_dimensions: i64,
    allocated_on_cache: u8,
    _pad: UChar7,
});
const cdtsPtr = ref.refType(cdts);

const Cdts2 = ArrayType(cdts, 2);

// ---------- xllr ----------
const xllrPath = "/usr/local/metaffi/xllr.so";
const xllr = ffi.Library(xllrPath, {
    load_runtime_plugin: ["void", ["string", charPtrPtr]],
    free_runtime_plugin: ["void", ["string", charPtrPtr]],

    load_entity: [
        voidPtr,
        [
            "string",
            "string",
            "string",
            ref.refType(metaffi_type_info),
            i8,
            ref.refType(metaffi_type_info),
            i8,
            charPtrPtr,
        ],
    ],
    free_xcall: ["void", ["string", voidPtr, charPtrPtr]],

    // cdts params_ret[2] -> pass pointer to the first element (cdts*)
    xcall_params_ret: ["void", [voidPtr, cdtsPtr, charPtrPtr]],

    free_string: ["void", [charPtr]],
});

// ---------- error handling ----------
// IMPORTANT: openjdk wrapper allocates errors with malloc() (per your C++ code).
// If xllr.free_string is not guaranteed to match that allocator in your build,
// freeing openjdk error strings can corrupt the heap.
// We therefore skip freeing openjdk errors TEMPORARILY for stability.
function throwIfErr(errPtrPtr, where, runtimeName) {
    const p = errPtrPtr.deref();
    if (!p.isNull()) {
        const msg = ref.readCString(p, 0);

        throw new Error(`${where}: ${msg}`);
    }
}

function loadRuntime(name) {
    const err = ref.alloc(charPtrPtr);
    xllr.load_runtime_plugin(name, err);
    throwIfErr(err, `load_runtime_plugin(${name})`, name);
    console.log(`✔ Runtime "${name}" loaded successfully`);
}

function freeRuntime(name) {
    const err = ref.alloc(charPtrPtr);
    xllr.free_runtime_plugin(name, err);
    throwIfErr(err, `free_runtime_plugin(${name})`, name);
}

function makeTypeInfoBuffer(typesBigInt) {
    const items = typesBigInt.map(
        (t) =>
            new metaffi_type_info({
                type: u64arg(t),
                alias: ref.NULL,
                is_free_alias: 0,
                _pad: new UChar7(),
                fixed_dimensions: i64arg(MIXED_OR_UNKNOWN_DIMENSIONS),
            })
    );

    const Arr = ArrayType(metaffi_type_info, items.length);
    const arr = new Arr(items);
    return { buf: arr.buffer, len: items.length };
}

function loadEntity(runtime, modulePath, entityPath, paramTypes, retTypes) {
    const err = ref.alloc(charPtrPtr);

    const p = makeTypeInfoBuffer(paramTypes);
    const r = makeTypeInfoBuffer(retTypes);

    const xcallPtr = xllr.load_entity(
        runtime,
        modulePath,
        entityPath,
        p.buf,
        i8arg(p.len),
        r.buf,
        i8arg(r.len),
        err
    );

    throwIfErr(err, `load_entity(${runtime}, ${entityPath})`, runtime);
    return xcallPtr;
}

function freeXcall(runtime, xcallPtr, label) {
    const err = ref.alloc(charPtrPtr);
    xllr.free_xcall(runtime, xcallPtr, err);
    throwIfErr(err, `free_xcall(${label})`, runtime);
}

// ----- CDT writers -----

function writeCdtInt32(rawBuf, index, value) {
    const off = index * cdt.size;
    rawBuf.writeBigUInt64LE(metaffi_int32_type, off + 0);
    rawBuf.writeInt32LE(value | 0, off + 8);
    rawBuf.writeInt32LE(0, off + 12);
    rawBuf.writeUInt8(0, off + 16);
    rawBuf.fill(0, off + 17, off + 24);
}

function writeCdtFloat64(rawBuf, index, value) {
    const off = index * cdt.size;
    rawBuf.writeBigUInt64LE(metaffi_float64_type, off + 0);
    rawBuf.writeDoubleLE(Number(value), off + 8);
    rawBuf.writeUInt8(0, off + 16);
    rawBuf.fill(0, off + 17, off + 24);
}

function writeCdtString8(rawBuf, index, cstrBuf) {
    const off = index * cdt.size;
    rawBuf.writeBigUInt64LE(metaffi_string8_type, off + 0);
    rawBuf.writeBigUInt64LE(BigInt(ref.address(cstrBuf)), off + 8);
    rawBuf.writeUInt8(0, off + 16);
    rawBuf.fill(0, off + 17, off + 24);
}

// ----- CDT readers -----

function readCdtInt64(rawBuf, index) {
    const off = index * cdt.size;
    const t = rawBuf.readBigUInt64LE(off + 0);
    if (t !== metaffi_int64_type) {
        throw new Error(`Unexpected return type: ${t.toString()}`);
    }
    const v = rawBuf.readBigInt64LE(off + 8);

    const asNum = Number(v);
    if (BigInt(asNum) === v) return asNum;
    return v;
}

function readCdtFloat64(rawBuf, index) {
    const off = index * cdt.size;
    const t = rawBuf.readBigUInt64LE(off + 0);
    if (t !== metaffi_float64_type) {
        throw new Error(`Unexpected return type: ${t.toString()}`);
    }
    return rawBuf.readDoubleLE(off + 8);
}

function readCdtString8(rawBuf, index, runtimeName) {
    const off = index * cdt.size;

    const t = rawBuf.readBigUInt64LE(off + 0);
    if (t !== metaffi_string8_type) {
        throw new Error(`Unexpected return type: ${t.toString()}`);
    }

    const addr = rawBuf.readBigUInt64LE(off + 8);
    if (addr === 0n) return null;


    const freeReq = rawBuf.readUInt8(off + 16);

    const ptrBuf = Buffer.alloc(ref.sizeof.pointer);
    ptrBuf.writeBigUInt64LE(addr, 0);
    const cstrPtr = ref.readPointer(ptrBuf, 0);

    const s = ref.readCString(cstrPtr, 0);


    if (freeReq) {
        xllr.free_string(cstrPtr);
    }

    return s;
}


// ----- call -----

function callParamsRet(runtime, xcallPtr, paramsRaw, retsRaw) {
    const err = ref.alloc(charPtrPtr);

    const pr = new Cdts2([
        new cdts({
            arr: paramsRaw,
            length: u64arg(BigInt(paramsRaw.length / cdt.size)),
            fixed_dimensions: i64arg(1n),
            allocated_on_cache: 0,
            _pad: new UChar7(),
        }),
        new cdts({
            arr: retsRaw,
            length: u64arg(BigInt(retsRaw.length / cdt.size)),
            fixed_dimensions: i64arg(1n),
            allocated_on_cache: 0,
            _pad: new UChar7(),
        }),
    ]);

    xllr.xcall_params_ret(xcallPtr, pr.buffer, err);
    throwIfErr(err, `xcall_params_ret(${runtime})`, runtime);
}

// >>> ADDED FOR CDT TESTING (Array-as-single-argument helper) <<<
// For Python sum_int_array(arr): we must send ONE argument (the array),
// not N separate arguments. We do this by setting params.length=1 and
// params.fixed_dimensions=arrayLength, while still pointing arr to the
// contiguous CDT elements buffer.
function callParamsRetArray(runtime, xcallPtr, paramsRaw, arrayLength, retsRaw) {
    const err = ref.alloc(charPtrPtr);

    const pr = new Cdts2([
        new cdts({
            arr: paramsRaw,
            length: u64arg(1n), // ONE parameter: the array
            fixed_dimensions: i64arg(BigInt(arrayLength)), // array length
            allocated_on_cache: 0,
            _pad: new UChar7(),
        }),
        new cdts({
            arr: retsRaw,
            length: u64arg(BigInt(retsRaw.length / cdt.size)),
            fixed_dimensions: i64arg(1n),
            allocated_on_cache: 0,
            _pad: new UChar7(),
        }),
    ]);

    xllr.xcall_params_ret(xcallPtr, pr.buffer, err);
    throwIfErr(err, `xcall_params_ret(${runtime})`, runtime);
}



// ----- main -----

(function main() {
    const PY = "xllr.python311";
    const JAVA = "xllr.openjdk";

    loadRuntime(PY);
    //  loadRuntime(JAVA);

    // ---------------------------
    // Python
    // ---------------------------
    const pyModule = "/usr/local/metaffi/nodejs/py_api.py";

    const pyAdd = loadEntity(
        PY,
        pyModule,
        "callable=add",
        [metaffi_int32_type, metaffi_int32_type],
        [metaffi_int64_type]
    );

    {
        const paramsRaw = Buffer.alloc(cdt.size * 2);
        const retsRaw = Buffer.alloc(cdt.size * 1);

        writeCdtInt32(paramsRaw, 0, 2);
        writeCdtInt32(paramsRaw, 1, 3);

        callParamsRet(PY, pyAdd, paramsRaw, retsRaw);
        console.log("Python add(2,3) =", readCdtInt64(retsRaw, 0));
    }

    const pyGreet = loadEntity(
        PY,
        pyModule,
        "callable=greet",
        [metaffi_string8_type],
        [metaffi_string8_type]
    );

    {
        const paramsRaw = Buffer.alloc(cdt.size * 1);
        const retsRaw = Buffer.alloc(cdt.size * 1);

        const nameBuf = Buffer.from("Sagi\0", "utf8");
        writeCdtString8(paramsRaw, 0, nameBuf);

        callParamsRet(PY, pyGreet, paramsRaw, retsRaw);
        console.log("Python greet('Sagi') =", readCdtString8(retsRaw, 0, PY));
    }

    // >>> ADDED FOR CDT TESTING (Python float) <<<
    const pyMulFloat = loadEntity(
        PY,
        pyModule,
        "callable=mul_float",
        [metaffi_float64_type, metaffi_float64_type],
        [metaffi_float64_type]
    );

    // >>> ADDED FOR CDT TESTING (Python char) <<<
    const pyNextChar = loadEntity(
        PY,
        pyModule,
        "callable=next_char",
        [metaffi_string8_type],
        [metaffi_string8_type]
    );

    // >>> ADDED FOR CDT TESTING (Python string) <<<
    const pyRepeatString = loadEntity(
        PY,
        pyModule,
        "callable=repeat_string",
        [metaffi_string8_type, metaffi_int32_type],
        [metaffi_string8_type]
    );

    // >>> ADDED FOR CDT TESTING (Python array) <<<
    const pySumIntArray = loadEntity(
        PY,
        pyModule,
        "callable=sum_int_array",
        [metaffi_int32_type],   // element type
        [metaffi_int64_type]
    );
    {
        const paramsRaw = Buffer.alloc(cdt.size * 2);
        const retsRaw = Buffer.alloc(cdt.size * 1);

        writeCdtFloat64(paramsRaw, 0, 1.5);
        writeCdtFloat64(paramsRaw, 1, 2.25);

        callParamsRet(PY, pyMulFloat, paramsRaw, retsRaw);
        console.log("Python mul_float(1.5,2.25) =", readCdtFloat64(retsRaw, 0));

        const strBuf = Buffer.from("Hi\0", "utf8");

        writeCdtString8(paramsRaw, 0, strBuf);
        writeCdtInt32(paramsRaw, 1, 3);

        callParamsRet(PY, pyRepeatString, paramsRaw, retsRaw);

        console.log("Python repeat_string('Hi', 3) =", readCdtString8(retsRaw, 0, PY));

    }
    {
        const paramsRaw = Buffer.alloc(cdt.size * 1);
        const retsRaw = Buffer.alloc(cdt.size * 1);

        const chBuf = Buffer.from("A\0", "utf8");
        writeCdtString8(paramsRaw, 0, chBuf);

        callParamsRet(PY, pyNextChar, paramsRaw, retsRaw);
        console.log("Python next_char('A') =", readCdtString8(retsRaw, 0, PY));
    }
    {
        const values = [1, 2, 3, 4, 5];

        const paramsRaw = Buffer.alloc(cdt.size * values.length);
        const retsRaw = Buffer.alloc(cdt.size * 1);

        for (let i = 0; i < values.length; i++) {
            writeCdtInt32(paramsRaw, i, values[i]);
        }

        // Python array test: pass N int32 arguments (runtime does NOT wrap into a list)
        callParamsRet(PY, pySumIntArray, paramsRaw, retsRaw);

        console.log(
            "Python sum_int_array([1,2,3,4,5]) =",
            readCdtInt64(retsRaw, 0)
        );
    }

    freeXcall(PY, pyMulFloat, "python mul_float");
    freeXcall(PY, pyNextChar, "python next_char");
    freeXcall(PY, pyRepeatString, "python repeat_string");
    freeXcall(PY, pySumIntArray, "python sum_int_array");
    freeXcall(PY, pyAdd, "python add");
    freeXcall(PY, pyGreet, "python greet");

    // ---------------------------
// Go
// ---------------------------
    const GO_CANDIDATES = ["go_runtime", "xllr.go_runtime", "xllr.golang", "xllr.go"];
    let GO = null;

    for (const cand of GO_CANDIDATES) {
        try {
            loadRuntime(cand);
            GO = cand;
            break;
        } catch (e) {
            // ignore and try next
        }
    }
    if (!GO) throw new Error("Could not load any Go runtime plugin: " + GO_CANDIDATES.join(", "));

    const goModule = "/usr/local/metaffi/nodejs/libgoapi.so";

    const goAdd = loadEntity(
        GO,
        goModule,
        "callable=add",
        [metaffi_int32_type, metaffi_int32_type],
        [metaffi_int64_type]
    );

    {
        const paramsRaw = Buffer.alloc(cdt.size * 2);
        const retsRaw = Buffer.alloc(cdt.size * 1);

        writeCdtInt32(paramsRaw, 0, 7);
        writeCdtInt32(paramsRaw, 1, 5);

        callParamsRet(GO, goAdd, paramsRaw, retsRaw);
        console.log("Go add(7,5) =", readCdtInt64(retsRaw, 0));
    }

    freeXcall(GO, goAdd, "go add");


    const goGreet = loadEntity(
        GO,
        goModule,
        "callable=greet",
        [metaffi_string8_type],
        [metaffi_string8_type]
    );

    {
        const paramsRaw = Buffer.alloc(cdt.size * 1);
        const retsRaw = Buffer.alloc(cdt.size * 1);

        const nameBuf = Buffer.from("Rotem\0", "utf8");
        writeCdtString8(paramsRaw, 0, nameBuf);

        callParamsRet(GO, goGreet, paramsRaw, retsRaw);
        console.log("Go greet('Rotem') =", readCdtString8(retsRaw, 0, GO));
    }

    // >>> ADDED FOR CDT TESTING (Go float) <<<
    const goAddFloat = loadEntity(
        GO,
        goModule,
        "callable=add_float",
        [metaffi_float64_type, metaffi_float64_type],
        [metaffi_float64_type]
    );

    // >>> ADDED FOR CDT TESTING (Go char) <<<
    const goNextChar = loadEntity(
        GO,
        goModule,
        "callable=next_char",
        [metaffi_char8_type],
        [metaffi_char8_type]
    );

    // >>> ADDED FOR CDT TESTING (Go string) <<<
    const goEchoString = loadEntity(
        GO,
        goModule,
        "callable=echo_string",
        [metaffi_string8_type],
        [metaffi_string8_type]
    );
    // >>> ADDED FOR CDT TESTING (Go array) <<<
    const goSumIntArray = loadEntity(
        GO,
        goModule,
        "callable=sum_int_array",
        [metaffi_int32_type],
        [metaffi_int64_type]
    );


    {
        const paramsRaw = Buffer.alloc(cdt.size * 2);
        const retsRaw = Buffer.alloc(cdt.size * 1);

        writeCdtFloat64(paramsRaw, 0, 3.5);
        writeCdtFloat64(paramsRaw, 1, 4.25);


        callParamsRet(GO, goAddFloat, paramsRaw, retsRaw);
        console.log("Go add_float(3.5,4.25) =", readCdtFloat64(retsRaw, 0));

        // pass char as int8 (ASCII)
        paramsRaw.writeBigUInt64LE(metaffi_char8_type, 0);
        paramsRaw.writeUInt8("A".charCodeAt(0), 8);
        paramsRaw.writeUInt8(0, 16);

        callParamsRet(GO, goNextChar, paramsRaw, retsRaw);

        const nextCharCode = retsRaw.readUInt8(8);
        console.log("Go next_char('A') =", String.fromCharCode(nextCharCode));

        const msgBuf = Buffer.from("MetaFFI\0", "utf8");
        writeCdtString8(paramsRaw, 0, msgBuf);

        callParamsRet(GO, goEchoString, paramsRaw, retsRaw);

        console.log("Go echo_string('MetaFFI') =", readCdtString8(retsRaw, 0, GO));

    }
    {
        const values = [10, 20, 30];

        const paramsRaw = Buffer.alloc(cdt.size * values.length);
        const retsRaw = Buffer.alloc(cdt.size * 1);

        for (let i = 0; i < values.length; i++) {
            writeCdtInt32(paramsRaw, i, values[i]);
        }

        callParamsRet(GO, goSumIntArray, paramsRaw, retsRaw);

        console.log(
            "Go sum_int_array([10,20,30]) =",
            readCdtInt64(retsRaw, 0)
        );
    }

    freeXcall(GO, goAddFloat, "go add_float");
    freeXcall(GO, goGreet, "go greet");
    freeXcall(GO, goNextChar, "go next_char");
    freeXcall(GO, goEchoString, "go echo_string");
    freeXcall(GO, goSumIntArray, "go sum_int_array");


    // ---------------------------
    // Java-FIX
    // ---------------------------
    // Ensure JavaAPI.class exists in this folder:
    //   cd /usr/local/metaffi/nodejs && javac JavaAPI.java
    // const javaModule = "/usr/local/metaffi/nodejs";
    //
    // // Make '.' in JVM classpath point to the correct directory
    // try {
    //     process.chdir(javaModule);
    // } catch (e) {
    //     console.warn(`Warning: failed to chdir(${javaModule}): ${e.message}`);
    // }
    //
    // const jAdd = loadEntity(
    //     JAVA,
    //     javaModule,
    //     "class=JavaAPI,callable=add",
    //     [metaffi_float64_type, metaffi_float64_type],
    //     [metaffi_float64_type]
    // );
    //
    // {
    //     const paramsRaw = Buffer.alloc(cdt.size * 2);
    //     const retsRaw = Buffer.alloc(cdt.size * 1);
    //
    //     writeCdtFloat64(paramsRaw, 0, 4.0);
    //     writeCdtFloat64(paramsRaw, 1, 5.0);
    //
    //     callParamsRet(JAVA, jAdd, paramsRaw, retsRaw);
    //     console.log("Java add(4,5) =", readCdtFloat64(retsRaw, 0));
    // }
    //
    // const jGreet = loadEntity(
    //     JAVA,
    //     javaModule,
    //     "class=JavaAPI,callable=greet",
    //     [metaffi_string8_type],
    //     [metaffi_string8_type]
    // );
    //
    // {
    //     const paramsRaw = Buffer.alloc(cdt.size * 1);
    //     const retsRaw = Buffer.alloc(cdt.size * 1);
    //
    //     const nameBuf = Buffer.from("Sagi\0", "utf8");
    //     writeCdtString8(paramsRaw, 0, nameBuf);
    //
    //     callParamsRet(JAVA, jGreet, paramsRaw, retsRaw);
    //     console.log("Java greet('Sagi') =", readCdtString8(retsRaw, 0, JAVA));
    // }
    //
    // freeXcall(JAVA, jAdd, "java add");
    // freeXcall(JAVA, jGreet, "java greet");

    // freeRuntime(JAVA);
    //freeRuntime(PY);
})();

/**
 * run_all.js
 * ----------
 * MetaFFI "smoke test" runner for:
 *  - Python runtime (xllr.python311)
 *  - Go runtime (xllr.go / candidates)
 *
 * What this file does:
 *  - Loads a runtime plugin
 *  - Loads entities (functions) from modules
 *  - Builds CDT/CDTS buffers manually (ffi-napi + ref-napi)
 *  - Calls xcall_params_ret and prints returned values
 *  - Frees xcalls (and returned strings when flagged free_required)
 *
 * Notes:
 *  - We model MetaFFI CDT as:
 *      [ u64 type | 8-byte union payload | u8 free_required | 7 bytes padding ]
 *  - Strings are passed as pointers to null-terminated UTF-8 buffers.
 *  - Returned strings may require freeing via xllr.free_string depending on free_required flag.
 */

// ---------- imports ----------
const ffi = require("ffi-napi");
const ref = require("ref-napi");
const Struct = require("ref-struct-di")(ref);
const ArrayType = require("ref-array-di")(ref);

// ---------- basic FFI types ----------
const voidPtr = ref.refType(ref.types.void);
const charPtr = ref.refType(ref.types.char);
const charPtrPtr = ref.refType(charPtr);

const u8 = ref.types.uint8;
const i8 = ref.types.int8;
const u64 = ref.types.uint64;
const i64 = ref.types.int64;

// ---------- MetaFFI type constants ----------
const metaffi_float64_type = 1n;
const metaffi_int32_type = 16n;
const metaffi_int64_type = 32n;
const metaffi_char8_type = 2048n;
const metaffi_string8_type = 4096n;

// Used in metaffi_type_info.fixed_dimensions when dimensions are not fixed
const MIXED_OR_UNKNOWN_DIMENSIONS = -1n;

// Small helpers for ref-napi numeric conversions
function u64arg(x) {
    return typeof x === "bigint" ? x.toString() : String(x);
}
function i64arg(x) {
    return typeof x === "bigint" ? x.toString() : String(x);
}
function i8arg(x) {
    return x | 0;
}

// 7-byte padding field used in a few structs
const UChar7 = ArrayType(u8, 7);

/**
 * struct metaffi_type_info:
 *   u64 type
 *   char* alias
 *   u8 is_free_alias
 *   u8 pad[7]
 *   i64 fixed_dimensions
 */
const metaffi_type_info = Struct({
    type: u64,
    alias: charPtr,
    is_free_alias: u8,
    _pad: UChar7,
    fixed_dimensions: i64,
});

/**
 * struct cdt:
 *   u64 type
 *   union payload (8 bytes) -> modeled as u64
 *   u8 free_required
 *   u8 pad[7]
 */
const cdt = Struct({
    type: u64,
    cdt_val: u64,
    free_required: u8,
    _pad: UChar7,
});

/**
 * struct cdts:
 *   cdt* arr
 *   u64 length
 *   i64 fixed_dimensions
 *   u8 allocated_on_cache
 *   u8 pad[7]
 */
const cdtPtr = ref.refType(cdt);
const cdts = Struct({
    arr: cdtPtr,
    length: u64,
    fixed_dimensions: i64,
    allocated_on_cache: u8,
    _pad: UChar7,
});
const cdtsPtr = ref.refType(cdts);

// We pass params/ret as cdts[2] contiguous array
const Cdts2 = ArrayType(cdts, 2);

// ---------- xllr bindings ----------
const xllrPath = "/usr/local/metaffi/xllr.so";
const xllr = ffi.Library(xllrPath, {
    load_runtime_plugin: ["void", ["string", charPtrPtr]],
    free_runtime_plugin: ["void", ["string", charPtrPtr]],

    load_entity: [
        voidPtr,
        [
            "string", // runtime name
            "string", // module path
            "string", // entity path
            ref.refType(metaffi_type_info), // params type info buffer
            i8, // params count
            ref.refType(metaffi_type_info), // returns type info buffer
            i8, // returns count
            charPtrPtr, // out_err
        ],
    ],

    free_xcall: ["void", ["string", voidPtr, charPtrPtr]],

    // xcall_params_ret(xcall, cdts[2], out_err)
    xcall_params_ret: ["void", [voidPtr, cdtsPtr, charPtrPtr]],

    // used to free returned strings when CDT.free_required=1
    free_string: ["void", [charPtr]],
});

// ---------- generic error handling ----------
function throwIfErr(errPtrPtr, where, runtimeName) {
    const p = errPtrPtr.deref();
    if (!p.isNull()) {
        const msg = ref.readCString(p, 0);
        throw new Error(`${where}: ${msg}`);
    }
}

// ---------- runtime/plugin helpers ----------
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

// ---------- entity loading helpers ----------
function makeTypeInfoBuffer(typesBigInt) {
    const items = typesBigInt.map((t) => {
        return new metaffi_type_info({
            type: u64arg(t),
            alias: ref.NULL,
            is_free_alias: 0,
            _pad: new UChar7(),
            fixed_dimensions: i64arg(MIXED_OR_UNKNOWN_DIMENSIONS),
        });
    });

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

// ---------- CDT write helpers ----------
function writeCdtInt32(rawBuf, index, value) {
    const off = index * cdt.size;

    rawBuf.writeBigUInt64LE(metaffi_int32_type, off + 0);
    rawBuf.writeInt32LE(value | 0, off + 8);
    rawBuf.writeInt32LE(0, off + 12); // keep union 8 bytes clean
    rawBuf.writeUInt8(0, off + 16); // free_required
    rawBuf.fill(0, off + 17, off + 24); // pad
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

    // IMPORTANT: we pass pointer to an existing Node Buffer that contains a '\0'
    rawBuf.writeBigUInt64LE(metaffi_string8_type, off + 0);
    rawBuf.writeBigUInt64LE(BigInt(ref.address(cstrBuf)), off + 8);
    rawBuf.writeUInt8(0, off + 16);
    rawBuf.fill(0, off + 17, off + 24);
}

// ---------- CDT read helpers ----------
function readCdtInt64(rawBuf, index) {
    const off = index * cdt.size;
    const t = rawBuf.readBigUInt64LE(off + 0);

    if (t !== metaffi_int64_type) {
        throw new Error(`Unexpected return type: ${t.toString()}`);
    }

    const v = rawBuf.readBigInt64LE(off + 8);

    // Return Number when safe; otherwise return BigInt
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

function readCdtString8(rawBuf, index) {
    const off = index * cdt.size;
    const t = rawBuf.readBigUInt64LE(off + 0);

    if (t !== metaffi_string8_type) {
        throw new Error(`Unexpected return type: ${t.toString()}`);
    }

    const addr = rawBuf.readBigUInt64LE(off + 8);
    if (addr === 0n) return null;

    const freeReq = rawBuf.readUInt8(off + 16);

    // Convert u64 address to a pointer ref-napi understands
    const ptrBuf = Buffer.alloc(ref.sizeof.pointer);
    ptrBuf.writeBigUInt64LE(addr, 0);
    const cstrPtr = ref.readPointer(ptrBuf, 0);

    const s = ref.readCString(cstrPtr, 0);

    // If runtime says "free_required", release via xllr.free_string
    if (freeReq) {
        xllr.free_string(cstrPtr);
    }

    return s;
}

// ---------- xcall invocation helpers ----------
function callParamsRet(runtime, xcallPtr, paramsRaw, retsRaw) {
    const err = ref.alloc(charPtrPtr);

    // Build cdts[2] (params, rets)
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

/**
 * Array-as-single-argument call helper (kept for experiments).
 * In our current Python sum_int_array implementation (varargs),
 * we don't need this for arrays; but it’s useful when a function
 * strictly expects a single list/iterable argument.
 */
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

// ============================================================
// main
// ============================================================
(function main() {
    // ---------- runtime names ----------
    const PY = "xllr.python311";
    const JAVA = "xllr.openjdk"; // Java section kept intact at the end

    // ---------- load Python runtime ----------
    loadRuntime(PY);

    // ---------------------------
    // Python entities
    // ---------------------------
    const pyModule = "/usr/local/metaffi/nodejs/py_api.py";

    // Basic int
    const pyAdd = loadEntity(PY, pyModule, "callable=add", [
        metaffi_int32_type,
        metaffi_int32_type,
    ], [metaffi_int64_type]);

    // Basic string
    const pyGreet = loadEntity(PY, pyModule, "callable=greet", [
        metaffi_string8_type,
    ], [metaffi_string8_type]);

    // Float
    const pyMulFloat = loadEntity(PY, pyModule, "callable=mul_float", [
        metaffi_float64_type,
        metaffi_float64_type,
    ], [metaffi_float64_type]);

    // Char (implemented as string8 in python test)
    const pyNextChar = loadEntity(PY, pyModule, "callable=next_char", [
        metaffi_string8_type,
    ], [metaffi_string8_type]);

    // String repetition
    const pyRepeatString = loadEntity(PY, pyModule, "callable=repeat_string", [
        metaffi_string8_type,
        metaffi_int32_type,
    ], [metaffi_string8_type]);

    // Array sum (python uses *arr varargs)
    const pySumIntArray = loadEntity(PY, pyModule, "callable=sum_int_array", [
        metaffi_int32_type, // element type
    ], [metaffi_int64_type]);

    // "Complex object" (JSON string) tests
    const pyStudentCreate = loadEntity(PY, pyModule, "callable=student_create", [
        metaffi_string8_type, // name
        metaffi_int32_type,   // id
        metaffi_int32_type,   // age
        metaffi_float64_type, // gpa
    ], [metaffi_string8_type]); // json

    const pyStudentGetGpa = loadEntity(PY, pyModule, "callable=student_get_gpa", [
        metaffi_string8_type, // json
    ], [metaffi_float64_type]);

    const pyStudentSetGpa = loadEntity(PY, pyModule, "callable=student_set_gpa", [
        metaffi_string8_type, // json
        metaffi_float64_type, // new gpa
    ], [metaffi_string8_type]); // json

    // ---------------------------
    // Python calls
    // ---------------------------

    // Python add(2,3)
    {
        const paramsRaw = Buffer.alloc(cdt.size * 2);
        const retsRaw = Buffer.alloc(cdt.size * 1);

        writeCdtInt32(paramsRaw, 0, 2);
        writeCdtInt32(paramsRaw, 1, 3);

        callParamsRet(PY, pyAdd, paramsRaw, retsRaw);
        console.log("Python add(2,3) =", readCdtInt64(retsRaw, 0));
    }

    // Python greet('Sagi')
    {
        const paramsRaw = Buffer.alloc(cdt.size * 1);
        const retsRaw = Buffer.alloc(cdt.size * 1);

        const nameBuf = Buffer.from("Sagi\0", "utf8");
        writeCdtString8(paramsRaw, 0, nameBuf);

        callParamsRet(PY, pyGreet, paramsRaw, retsRaw);
        console.log("Python greet('Sagi') =", readCdtString8(retsRaw, 0));
    }

    // Python mul_float(1.5, 2.25) and repeat_string('Hi', 3)
    {
        const paramsRaw = Buffer.alloc(cdt.size * 2);
        const retsRaw = Buffer.alloc(cdt.size * 1);

        // mul_float
        writeCdtFloat64(paramsRaw, 0, 1.5);
        writeCdtFloat64(paramsRaw, 1, 2.25);

        callParamsRet(PY, pyMulFloat, paramsRaw, retsRaw);
        console.log("Python mul_float(1.5,2.25) =", readCdtFloat64(retsRaw, 0));

        // repeat_string
        const strBuf = Buffer.from("Hi\0", "utf8");
        writeCdtString8(paramsRaw, 0, strBuf);
        writeCdtInt32(paramsRaw, 1, 3);

        callParamsRet(PY, pyRepeatString, paramsRaw, retsRaw);
        console.log("Python repeat_string('Hi', 3) =", readCdtString8(retsRaw, 0));
    }

    // Python next_char('A')
    {
        const paramsRaw = Buffer.alloc(cdt.size * 1);
        const retsRaw = Buffer.alloc(cdt.size * 1);

        const chBuf = Buffer.from("A\0", "utf8");
        writeCdtString8(paramsRaw, 0, chBuf);

        callParamsRet(PY, pyNextChar, paramsRaw, retsRaw);
        console.log("Python next_char('A') =", readCdtString8(retsRaw, 0));
    }

    // Python sum_int_array([1,2,3,4,5])  (varargs style)
    {
        const values = [1, 2, 3, 4, 5];
        const paramsRaw = Buffer.alloc(cdt.size * values.length);
        const retsRaw = Buffer.alloc(cdt.size * 1);

        for (let i = 0; i < values.length; i++) {
            writeCdtInt32(paramsRaw, i, values[i]);
        }

        callParamsRet(PY, pySumIntArray, paramsRaw, retsRaw);
        console.log("Python sum_int_array([1,2,3,4,5]) =", readCdtInt64(retsRaw, 0));
    }

    // Python Student JSON flow
    {
        // student_create
        const paramsRaw = Buffer.alloc(cdt.size * 4);
        const retsRaw = Buffer.alloc(cdt.size * 1);

        const nameBuf = Buffer.from("Rotem\0", "utf8");
        writeCdtString8(paramsRaw, 0, nameBuf);
        writeCdtInt32(paramsRaw, 1, 12345);
        writeCdtInt32(paramsRaw, 2, 21);
        writeCdtFloat64(paramsRaw, 3, 3.7);

        callParamsRet(PY, pyStudentCreate, paramsRaw, retsRaw);
        const studentJson = readCdtString8(retsRaw, 0);
        console.log("Python student_create(...) =", studentJson);

        // student_get_gpa
        {
            const p = Buffer.alloc(cdt.size * 1);
            const r = Buffer.alloc(cdt.size * 1);

            const jsonBuf = Buffer.from(studentJson + "\0", "utf8");
            writeCdtString8(p, 0, jsonBuf);

            callParamsRet(PY, pyStudentGetGpa, p, r);
            console.log("Python student_get_gpa(student) =", readCdtFloat64(r, 0));
        }

        // student_set_gpa
        let updatedJson = null;
        {
            const p = Buffer.alloc(cdt.size * 2);
            const r = Buffer.alloc(cdt.size * 1);

            const jsonBuf = Buffer.from(studentJson + "\0", "utf8");
            writeCdtString8(p, 0, jsonBuf);
            writeCdtFloat64(p, 1, 3.95);

            callParamsRet(PY, pyStudentSetGpa, p, r);
            updatedJson = readCdtString8(r, 0);
            console.log("Python student_set_gpa(student, 3.95) =", updatedJson);
        }

        // student_get_gpa(updated)
        {
            const p = Buffer.alloc(cdt.size * 1);
            const r = Buffer.alloc(cdt.size * 1);

            const jsonBuf = Buffer.from(updatedJson + "\0", "utf8");
            writeCdtString8(p, 0, jsonBuf);

            callParamsRet(PY, pyStudentGetGpa, p, r);
            console.log("Python student_get_gpa(updated) =", readCdtFloat64(r, 0));
        }
    }

    // ---------- free Python xcalls ----------
    freeXcall(PY, pyStudentSetGpa, "python student_set_gpa");
    freeXcall(PY, pyStudentGetGpa, "python student_get_gpa");
    freeXcall(PY, pyStudentCreate, "python student_create");
    freeXcall(PY, pyMulFloat, "python mul_float");
    freeXcall(PY, pyNextChar, "python next_char");
    freeXcall(PY, pyRepeatString, "python repeat_string");
    freeXcall(PY, pySumIntArray, "python sum_int_array");
    freeXcall(PY, pyAdd, "python add");
    freeXcall(PY, pyGreet, "python greet");

    // ============================================================
    // Go
    // ============================================================

    // Try multiple possible plugin names; pick first that loads
    const GO_CANDIDATES = ["go_runtime", "xllr.go_runtime", "xllr.golang", "xllr.go"];
    let GO = null;

    for (const cand of GO_CANDIDATES) {
        try {
            loadRuntime(cand);
            GO = cand;
            break;
        } catch (_) {
            // ignore and try next
        }
    }

    if (!GO) {
        throw new Error("Could not load any Go runtime plugin: " + GO_CANDIDATES.join(", "));
    }

    const goModule = "/usr/local/metaffi/nodejs/libgoapi.so";

    // ---------------------------
    // Go entities
    // ---------------------------
    const goAdd = loadEntity(GO, goModule, "callable=add", [
        metaffi_int32_type,
        metaffi_int32_type,
    ], [metaffi_int64_type]);

    const goGreet = loadEntity(GO, goModule, "callable=greet", [
        metaffi_string8_type,
    ], [metaffi_string8_type]);

    const goAddFloat = loadEntity(GO, goModule, "callable=add_float", [
        metaffi_float64_type,
        metaffi_float64_type,
    ], [metaffi_float64_type]);

    const goNextChar = loadEntity(GO, goModule, "callable=next_char", [
        metaffi_char8_type,
    ], [metaffi_char8_type]);

    const goEchoString = loadEntity(GO, goModule, "callable=echo_string", [
        metaffi_string8_type,
    ], [metaffi_string8_type]);

    const goSumIntArray = loadEntity(GO, goModule, "callable=sum_int_array", [
        metaffi_int32_type,
    ], [metaffi_int64_type]);

    const goStudentCreate = loadEntity(GO, goModule, "callable=student_create", [
        metaffi_string8_type,
        metaffi_int32_type,
        metaffi_int32_type,
        metaffi_float64_type,
    ], [metaffi_string8_type]);

    const goStudentGetGpa = loadEntity(GO, goModule, "callable=student_get_gpa", [
        metaffi_string8_type,
    ], [metaffi_float64_type]);

    const goStudentSetGpa = loadEntity(GO, goModule, "callable=student_set_gpa", [
        metaffi_string8_type,
        metaffi_float64_type,
    ], [metaffi_string8_type]);

    // ---------------------------
    // Go calls
    // ---------------------------

    // Go add(7,5)
    {
        const paramsRaw = Buffer.alloc(cdt.size * 2);
        const retsRaw = Buffer.alloc(cdt.size * 1);

        writeCdtInt32(paramsRaw, 0, 7);
        writeCdtInt32(paramsRaw, 1, 5);

        callParamsRet(GO, goAdd, paramsRaw, retsRaw);
        console.log("Go add(7,5) =", readCdtInt64(retsRaw, 0));
    }

    // Go greet('Rotem')
    {
        const paramsRaw = Buffer.alloc(cdt.size * 1);
        const retsRaw = Buffer.alloc(cdt.size * 1);

        const nameBuf = Buffer.from("Rotem\0", "utf8");
        writeCdtString8(paramsRaw, 0, nameBuf);

        callParamsRet(GO, goGreet, paramsRaw, retsRaw);
        console.log("Go greet('Rotem') =", readCdtString8(retsRaw, 0));
    }

    // Go add_float, next_char, echo_string
    {
        const paramsRaw = Buffer.alloc(cdt.size * 2);
        const retsRaw = Buffer.alloc(cdt.size * 1);

        // add_float
        writeCdtFloat64(paramsRaw, 0, 3.5);
        writeCdtFloat64(paramsRaw, 1, 4.25);

        callParamsRet(GO, goAddFloat, paramsRaw, retsRaw);
        console.log("Go add_float(3.5,4.25) =", readCdtFloat64(retsRaw, 0));

        // next_char: pass char as int8 payload
        paramsRaw.writeBigUInt64LE(metaffi_char8_type, 0);
        paramsRaw.writeUInt8("A".charCodeAt(0), 8);
        paramsRaw.writeUInt8(0, 16);

        callParamsRet(GO, goNextChar, paramsRaw, retsRaw);

        const nextCharCode = retsRaw.readUInt8(8);
        console.log("Go next_char('A') =", String.fromCharCode(nextCharCode));

        // echo_string
        const msgBuf = Buffer.from("MetaFFI\0", "utf8");
        writeCdtString8(paramsRaw, 0, msgBuf);

        callParamsRet(GO, goEchoString, paramsRaw, retsRaw);
        console.log("Go echo_string('MetaFFI') =", readCdtString8(retsRaw, 0));
    }

    // Go sum_int_array([10,20,30])
    {
        const values = [10, 20, 30];
        const paramsRaw = Buffer.alloc(cdt.size * values.length);
        const retsRaw = Buffer.alloc(cdt.size * 1);

        for (let i = 0; i < values.length; i++) {
            writeCdtInt32(paramsRaw, i, values[i]);
        }

        callParamsRet(GO, goSumIntArray, paramsRaw, retsRaw);
        console.log("Go sum_int_array([10,20,30]) =", readCdtInt64(retsRaw, 0));
    }

    // Go Student JSON flow
    {
        // student_create
        const paramsRaw = Buffer.alloc(cdt.size * 4);
        const retsRaw = Buffer.alloc(cdt.size * 1);

        const nameBuf = Buffer.from("Sagi\0", "utf8");
        writeCdtString8(paramsRaw, 0, nameBuf);
        writeCdtInt32(paramsRaw, 1, 777);
        writeCdtInt32(paramsRaw, 2, 18);
        writeCdtFloat64(paramsRaw, 3, 3.2);

        callParamsRet(GO, goStudentCreate, paramsRaw, retsRaw);
        const studentJson = readCdtString8(retsRaw, 0);
        console.log("Go student_create(...) =", studentJson);

        // student_get_gpa
        {
            const p = Buffer.alloc(cdt.size * 1);
            const r = Buffer.alloc(cdt.size * 1);

            const jsonBuf = Buffer.from(studentJson + "\0", "utf8");
            writeCdtString8(p, 0, jsonBuf);

            callParamsRet(GO, goStudentGetGpa, p, r);
            console.log("Go student_get_gpa(student) =", readCdtFloat64(r, 0));
        }

        // student_set_gpa
        let updatedJson = null;
        {
            const p = Buffer.alloc(cdt.size * 2);
            const r = Buffer.alloc(cdt.size * 1);

            const jsonBuf = Buffer.from(studentJson + "\0", "utf8");
            writeCdtString8(p, 0, jsonBuf);
            writeCdtFloat64(p, 1, 3.85);

            callParamsRet(GO, goStudentSetGpa, p, r);
            updatedJson = readCdtString8(r, 0);
            console.log("Go student_set_gpa(student, 3.85) =", updatedJson);
        }

        // student_get_gpa(updated)
        {
            const p = Buffer.alloc(cdt.size * 1);
            const r = Buffer.alloc(cdt.size * 1);

            const jsonBuf = Buffer.from(updatedJson + "\0", "utf8");
            writeCdtString8(p, 0, jsonBuf);

            callParamsRet(GO, goStudentGetGpa, p, r);
            console.log("Go student_get_gpa(updated) =", readCdtFloat64(r, 0));
        }
    }

    // ---------- free Go xcalls ----------
    freeXcall(GO, goStudentSetGpa, "go student_set_gpa");
    freeXcall(GO, goStudentGetGpa, "go student_get_gpa");
    freeXcall(GO, goStudentCreate, "go student_create");
    freeXcall(GO, goAddFloat, "go add_float");
    freeXcall(GO, goGreet, "go greet");
    freeXcall(GO, goNextChar, "go next_char");
    freeXcall(GO, goEchoString, "go echo_string");
    freeXcall(GO, goSumIntArray, "go sum_int_array");
    freeXcall(GO, goAdd, "go add");

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

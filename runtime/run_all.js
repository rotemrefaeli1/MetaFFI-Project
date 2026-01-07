const ffi = require('ffi-napi');
const ref = require('ref-napi');
const Struct = require('ref-struct-di')(ref);
const ArrayType = require('ref-array-di')(ref);

const voidPtr = ref.refType(ref.types.void);
const charPtr = ref.refType(ref.types.char);
const charPtrPtr = ref.refType(charPtr);

const u8 = ref.types.uint8;
const i8 = ref.types.int8;
const u64 = ref.types.uint64;
const i64 = ref.types.int64;

// MetaFFI type constants
const metaffi_int32_type = 16n;
const metaffi_int64_type = 32n;

const MIXED_OR_UNKNOWN_DIMENSIONS = -1n;

function u64arg(x) { return (typeof x === 'bigint') ? x.toString() : String(x); }
function i64arg(x) { return (typeof x === 'bigint') ? x.toString() : String(x); }
function i8arg(x) { return (x | 0); }

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
// u64 type, union(8 bytes) -> model as u64, u8 free_required, pad[7]
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

const xllrPath = "/usr/local/metaffi/xllr.so";

const xllr = ffi.Library(xllrPath, {
    load_runtime_plugin: ['void', ['string', charPtrPtr]],
    free_runtime_plugin: ['void', ['string', charPtrPtr]],

    load_entity: [
        voidPtr,
        ['string', 'string', 'string', ref.refType(metaffi_type_info), i8, ref.refType(metaffi_type_info), i8, charPtrPtr]
    ],
    free_xcall: ['void', ['string', voidPtr, charPtrPtr]],

    // cdts params_ret[2] -> cdts*
    xcall_params_ret: ['void', [voidPtr, cdtsPtr, charPtrPtr]],

    free_string: ['void', [charPtr]],
});

function throwIfErr(errPtrPtr, where) {
    const p = errPtrPtr.deref();
    if (!p.isNull()) {
        const msg = ref.readCString(p, 0);
        xllr.free_string(p);
        throw new Error(`${where}: ${msg}`);
    }
}

function loadRuntime(name) {
    const err = ref.alloc(charPtrPtr);
    xllr.load_runtime_plugin(name, err);
    throwIfErr(err, `load_runtime_plugin(${name})`);
    console.log(`✔ Runtime "${name}" loaded successfully`);
}

function makeTypeInfoBuffer(typesBigInt) {
    const items = typesBigInt.map(t => new metaffi_type_info({
        type: u64arg(t),
        alias: ref.NULL,
        is_free_alias: 0,
        _pad: new UChar7(),
        fixed_dimensions: i64arg(MIXED_OR_UNKNOWN_DIMENSIONS),
    }));

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

    throwIfErr(err, `load_entity(${runtime}, ${entityPath})`);
    return xcallPtr;
}

// Write CDT int32 into raw Buffer
function writeCdtInt32(rawBuf, index, value) {
    const off = index * cdt.size;

    rawBuf.writeBigUInt64LE(metaffi_int32_type, off + 0);

    // union slot (8 bytes): int32 in low 4 bytes
    rawBuf.writeInt32LE((value | 0), off + 8);
    rawBuf.writeInt32LE(0, off + 12);

    rawBuf.writeUInt8(0, off + 16);
    rawBuf.fill(0, off + 17, off + 24);
}

// Read CDT int64 from raw Buffer
function readCdtInt64(rawBuf, index) {
    const off = index * cdt.size;
    const t = rawBuf.readBigUInt64LE(off + 0);

    if (t !== metaffi_int64_type) {
        throw new Error(`Unexpected return type: ${t.toString()}`);
    }

    const v = rawBuf.readBigInt64LE(off + 8);

    // If you prefer Number when safe:
    const asNum = Number(v);
    if (BigInt(asNum) === v) return asNum;

    // Otherwise return BigInt for very large values
    return v;
}

function callParamsRet_Int32_Int32_to_Int64(runtime, xcallPtr, a, b) {
    const err = ref.alloc(charPtrPtr);

    const paramsRaw = Buffer.alloc(cdt.size * 2);
    const retsRaw = Buffer.alloc(cdt.size * 1);

    writeCdtInt32(paramsRaw, 0, a);
    writeCdtInt32(paramsRaw, 1, b);

    const pr = new Cdts2([
        new cdts({
            arr: paramsRaw,
            length: u64arg(2n),
            fixed_dimensions: i64arg(1n),
            allocated_on_cache: 0,
            _pad: new UChar7(),
        }),
        new cdts({
            arr: retsRaw,
            length: u64arg(1n),
            fixed_dimensions: i64arg(1n),
            allocated_on_cache: 0,
            _pad: new UChar7(),
        }),
    ]);

    xllr.xcall_params_ret(xcallPtr, pr.buffer, err);
    throwIfErr(err, `xcall_params_ret(${runtime})`);

    return readCdtInt64(retsRaw, 0);
}

(function main() {
    const PY = "xllr.python311";
    loadRuntime(PY);

    const pyModule = "/usr/local/metaffi/nodejs/py_api.py";
    const addPath = "callable=add";

    // IMPORTANT: Python returns int as int64 in this runtime
    const add = loadEntity(
        PY,
        pyModule,
        addPath,
        [metaffi_int32_type, metaffi_int32_type],
        [metaffi_int64_type]
    );

    const res = callParamsRet_Int32_Int32_to_Int64(PY, add, 2, 3);
    console.log("Python add(2,3) =", res);

    const err = ref.alloc(charPtrPtr);
    xllr.free_xcall(PY, add, err);
    throwIfErr(err, "free_xcall(python add)");
})();

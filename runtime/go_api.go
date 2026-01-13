package main

/*
#include <stdint.h>
#include <stdlib.h>

typedef uint64_t metaffi_type;

struct cdt {
	metaffi_type t;
	union {
		uint64_t u64_val;
		int64_t  i64_val;
		uint64_t handle_val;
	} cdt_val;
	uint8_t free_required;
	uint8_t _pad[7];
};

struct cdts {
	struct cdt* arr;
	uint64_t length;
	int64_t  fixed_dimensions;
	uint8_t  allocated_on_cache;
	uint8_t  _pad[7];
};
*/
import "C"

import (
	"fmt"
	"unsafe"
)

const (
	metaffi_int32_type   C.metaffi_type = 16
	metaffi_int64_type   C.metaffi_type = 32
	metaffi_float64_type C.metaffi_type = 1
	metaffi_string8_type C.metaffi_type = 4096
	metaffi_char8_type   C.metaffi_type = 2048 // int8 / char
)

func cdtAt(base *C.struct_cdt, i int) *C.struct_cdt {
	return (*C.struct_cdt)(unsafe.Add(unsafe.Pointer(base), uintptr(i)*unsafe.Sizeof(*base)))
}

func unionAsU64(p *C.struct_cdt) uint64 {
	return *(*uint64)(unsafe.Pointer(&p.cdt_val[0]))
}

func unionSetI64(p *C.struct_cdt, v int64) {
	*(*int64)(unsafe.Pointer(&p.cdt_val[0])) = v
}

func unionSetU64(p *C.struct_cdt, v uint64) {
	*(*uint64)(unsafe.Pointer(&p.cdt_val[0])) = v
}

func unionAsPtr(p *C.struct_cdt) unsafe.Pointer {
	u := unionAsU64(p)
	return unsafe.Pointer(uintptr(u))
}

func unionSetPtr(p *C.struct_cdt, ptr unsafe.Pointer) {
	*(*uint64)(unsafe.Pointer(&p.cdt_val[0])) = uint64(uintptr(ptr))
}

//export EntryPoint_add
func EntryPoint_add(_ *C.void, xcall_params *C.struct_cdts, out_err **C.char) {
	params := (*C.struct_cdts)(unsafe.Add(unsafe.Pointer(xcall_params), 0*unsafe.Sizeof(*xcall_params)))
	rets := (*C.struct_cdts)(unsafe.Add(unsafe.Pointer(xcall_params), 1*unsafe.Sizeof(*xcall_params)))

	if params.length < 2 || rets.length < 1 {
		*out_err = C.CString("EntryPoint_add: invalid params/ret lengths")
		return
	}

	p0 := cdtAt(params.arr, 0)
	p1 := cdtAt(params.arr, 1)

	if p0.t != metaffi_int32_type || p1.t != metaffi_int32_type {
		*out_err = C.CString("EntryPoint_add: expected int32,int32")
		return
	}

	a := int32(uint32(unionAsU64(p0)))
	b := int32(uint32(unionAsU64(p1)))
	sum := int64(a) + int64(b)

	r0 := cdtAt(rets.arr, 0)
	*r0 = C.struct_cdt{}
	r0.t = metaffi_int64_type
	unionSetI64(r0, sum)
	r0.free_required = 0
}

//export EntryPoint_greet
func EntryPoint_greet(_ *C.void, xcall_params *C.struct_cdts, out_err **C.char) {
	params := (*C.struct_cdts)(unsafe.Add(unsafe.Pointer(xcall_params), 0*unsafe.Sizeof(*xcall_params)))
	rets := (*C.struct_cdts)(unsafe.Add(unsafe.Pointer(xcall_params), 1*unsafe.Sizeof(*xcall_params)))

	if params.length < 1 || rets.length < 1 {
		*out_err = C.CString("EntryPoint_greet: invalid params/ret lengths")
		return
	}

	p0 := cdtAt(params.arr, 0)
	if p0.t != metaffi_string8_type {
		*out_err = C.CString("EntryPoint_greet: expected string8")
		return
	}

	cstr := (*C.char)(unionAsPtr(p0))
	if cstr == nil {
		*out_err = C.CString("EntryPoint_greet: null string pointer")
		return
	}

	name := C.GoString(cstr)
	out := fmt.Sprintf("Hello from Go, %s", name)

	outC := C.CString(out)

	r0 := cdtAt(rets.arr, 0)
	*r0 = C.struct_cdt{}
	r0.t = metaffi_string8_type
	unionSetPtr(r0, unsafe.Pointer(outC))
	r0.free_required = 1
}

//export EntryPoint_add_float
func EntryPoint_add_float(_ *C.void, xcall_params *C.struct_cdts, out_err **C.char) {
	params := (*C.struct_cdts)(unsafe.Add(unsafe.Pointer(xcall_params), 0))
	rets := (*C.struct_cdts)(unsafe.Add(unsafe.Pointer(xcall_params), unsafe.Sizeof(*xcall_params)))

	if params.length < 2 || rets.length < 1 {
		*out_err = C.CString("EntryPoint_add_float: invalid params/ret lengths")
		return
	}

	p0 := cdtAt(params.arr, 0)
	p1 := cdtAt(params.arr, 1)

	if p0.t != metaffi_float64_type || p1.t != metaffi_float64_type {
		*out_err = C.CString("EntryPoint_add_float: expected float64,float64")
		return
	}

	a := *(*float64)(unsafe.Pointer(&p0.cdt_val))
	b := *(*float64)(unsafe.Pointer(&p1.cdt_val))

	sum := a + b

	r0 := cdtAt(rets.arr, 0)
	*r0 = C.struct_cdt{}
	r0.t = metaffi_float64_type
	*(*float64)(unsafe.Pointer(&r0.cdt_val)) = sum
	r0.free_required = 0
}

//export EntryPoint_next_char
func EntryPoint_next_char(_ *C.void, xcall_params *C.struct_cdts, out_err **C.char) {
	params := (*C.struct_cdts)(unsafe.Add(unsafe.Pointer(xcall_params), 0))
	rets := (*C.struct_cdts)(unsafe.Add(unsafe.Pointer(xcall_params), unsafe.Sizeof(*xcall_params)))

	p := cdtAt(params.arr, 0)
	if p.t != metaffi_char8_type {
		*out_err = C.CString("EntryPoint_next_char: expected char8")
		return
	}

	ch := byte(unionAsU64(p))
	next := ch + 1

	r0 := cdtAt(rets.arr, 0)
	*r0 = C.struct_cdt{}
	r0.t = metaffi_char8_type
	unionSetU64(r0, uint64(next))
	r0.free_required = 0
}

//export EntryPoint_sum_int_array
func EntryPoint_sum_int_array(_ *C.void, xcall_params *C.struct_cdts, out_err **C.char) {
	params := (*C.struct_cdts)(unsafe.Add(unsafe.Pointer(xcall_params), 0))
	rets := (*C.struct_cdts)(unsafe.Add(unsafe.Pointer(xcall_params), unsafe.Sizeof(*xcall_params)))

	var sum int64 = 0
	length := uint64(params.length)

	for i := uint64(0); i < length; i++ {
		p := cdtAt(params.arr, int(i))
		if p.t != metaffi_int32_type {
			*out_err = C.CString("EntryPoint_sum_int_array: expected int32 array")
			return
		}
		sum += int64(int32(unionAsU64(p)))
	}

	r0 := cdtAt(rets.arr, 0)
	*r0 = C.struct_cdt{}
	r0.t = metaffi_int64_type
	unionSetI64(r0, sum)
	r0.free_required = 0
}

//export EntryPoint_echo_char
func EntryPoint_echo_char(_ *C.void, xcall_params *C.struct_cdts, out_err **C.char) {
	params := (*C.struct_cdts)(unsafe.Add(unsafe.Pointer(xcall_params), 0))
	rets := (*C.struct_cdts)(unsafe.Add(unsafe.Pointer(xcall_params), unsafe.Sizeof(*xcall_params)))

	p := cdtAt(params.arr, 0)
	if p.t != metaffi_char8_type {
		*out_err = C.CString("EntryPoint_echo_char: expected char8")
		return
	}

	ch := unionAsU64(p)

	r0 := cdtAt(rets.arr, 0)
	*r0 = C.struct_cdt{}
	r0.t = metaffi_char8_type
	unionSetU64(r0, ch)
	r0.free_required = 0
}

//export EntryPoint_echo_string
func EntryPoint_echo_string(_ *C.void, xcall_params *C.struct_cdts, out_err **C.char) {
	params := (*C.struct_cdts)(unsafe.Add(unsafe.Pointer(xcall_params), 0))
	rets := (*C.struct_cdts)(unsafe.Add(unsafe.Pointer(xcall_params), unsafe.Sizeof(*xcall_params)))

	p := cdtAt(params.arr, 0)
	if p.t != metaffi_string8_type {
		*out_err = C.CString("EntryPoint_echo_string: expected string8")
		return
	}

	cstr := (*C.char)(unionAsPtr(p))
	if cstr == nil {
		*out_err = C.CString("EntryPoint_echo_string: null string")
		return
	}

	goStr := C.GoString(cstr)
	outC := C.CString(goStr)

	r0 := cdtAt(rets.arr, 0)
	*r0 = C.struct_cdt{}
	r0.t = metaffi_string8_type
	unionSetPtr(r0, unsafe.Pointer(outC))
	r0.free_required = 1
}

func main() {}

# ------------------------------
# Python API for MetaFFI tests
# ------------------------------

# ---- basic int test ----
def add(a, b):
    return a + b

# ---- basic string test ----
def greet(name):
    return f"Hello from Python, {name}"

# ---- global state test ----
counter = 100

# ---- float test ----
def mul_float(a, b):
    # expects float64, float64 -> float64
    return a * b

# ---- char test ----
def next_char(c: str):
    # MetaFFI char is passed as int8
    # return next ASCII character
    return chr(ord(c) + 1)

# ---- array test ----
def sum_int_array(*arr):
    # MetaFFI may pass arrays as:
    #   - multiple int arguments (arr = (1,2,3,...))
    #   - OR a single iterable argument (arr = ([1,2,3],))
    if len(arr) == 1 and not isinstance(arr[0], (int, float, str, bytes)):
        return sum(arr[0])
    return sum(arr)

# ---- mixed types ----
def repeat_string(s: str, n: int) -> str:
    # string + int -> string
    return s * n

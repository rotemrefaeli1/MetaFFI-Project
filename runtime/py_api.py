# ------------------------------
# Python API for MetaFFI tests
# ------------------------------

import json

# ---- basic int test ----
def add(a, b):
    return a + b

# ---- basic string test ----
def greet(name):
    return f"Hello from Python, {name}"

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

# -------------------------------------------------
# Complex object test: Student (JSON over string8)
# Fields: name (string), id (int), age (int), gpa (float)
# -------------------------------------------------

def student_create(name: str, student_id: int, age: int, gpa: float) -> str:
    # returns a JSON string representing a student object
    obj = {
        "name": name,
        "id": int(student_id),
        "age": int(age),
        "gpa": float(gpa),
    }
    return json.dumps(obj)

def student_get_name(student_json: str) -> str:
    obj = json.loads(student_json)
    return obj.get("name", "")

def student_get_id(student_json: str) -> int:
    obj = json.loads(student_json)
    return int(obj.get("id", 0))

def student_get_age(student_json: str) -> int:
    obj = json.loads(student_json)
    return int(obj.get("age", 0))

def student_get_gpa(student_json: str) -> float:
    obj = json.loads(student_json)
    return float(obj.get("gpa", 0.0))

def student_set_gpa(student_json: str, new_gpa: float) -> str:
    # returns updated student JSON (immutability-style)
    obj = json.loads(student_json)
    obj["gpa"] = float(new_gpa)
    return json.dumps(obj)

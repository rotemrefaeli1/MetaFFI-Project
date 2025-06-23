const ffi = require('ffi-napi');
const ref = require('ref-napi');
const Struct = require('ref-struct-napi');

// Define the Student struct on the Node.js side
const Student = Struct({
    name: 'string',
    GPA: 'float',
    id: 'string',
    age: 'int',
    num_courses: 'int'
});

const StudentPtr = ref.refType(Student);

// Load the shared library
const lib = ffi.Library('./student', {
    update_student_gpa: ['float', [StudentPtr, 'float']],
    print_student: ['void', [StudentPtr]]
});
// Create an instance of Student
const student = new Student({
    name: "Sagi",
    GPA: 85.0,
    id: "123456789",
    age: 24,
    num_courses: 10
});

console.log("📚 Previous GPA:",student.GPA);
// Call the function
const newGrade = 92.0;
const updatedGPA = lib.update_student_gpa(student.ref(), newGrade);

lib.print_student(student.ref());


// console.log("👤 Student name:", student.name);
// console.log("📚 Updated GPA:", updatedGPA.toFixed(2));
// console.log("🆔 ID:", student.id);
// console.log("📅 Age:", student.age);
// console.log("📚 Completed courses:", student.num_courses);

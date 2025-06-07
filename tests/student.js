const ffi = require('ffi-napi');
const ref = require('ref-napi');
const Struct = require('ref-struct-napi');

// הגדרת מבנה Student בצד Node.js
const Student = Struct({
    name: 'string',
    GPA: 'float',
    id: 'string',
    age: 'int',
    num_courses: 'int'
});

const StudentPtr = ref.refType(Student);

// טעינת הספרייה
const lib = ffi.Library('./student', {
    update_student_gpa: ['float', [StudentPtr, 'float']],
    print_student: ['void', [StudentPtr]]
});
// יצירת מופע של Student
const student = new Student({
    name: "Sagi",
    GPA: 85.0,
    id: "123456789",
    age: 24,
    num_courses: 10
});

console.log("📚 Previous GPA:",student.GPA);
// קריאה לפונקציה
const newGrade = 92.0;
const updatedGPA = lib.update_student_gpa(student.ref(), newGrade);

lib.print_student(student.ref());


// console.log("👤 Student name:", student.name);
// console.log("📚 Updated GPA:", updatedGPA.toFixed(2));
// console.log("🆔 ID:", student.id);
// console.log("📅 Age:", student.age);
// console.log("📚 Completed courses:", student.num_courses);

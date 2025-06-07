function updateStudentGPA(student, newGrade) {
    if (student.num_courses < 0) throw new Error("Invalid course count");

    const total = student.GPA * student.num_courses + newGrade;
    student.num_courses += 1;
    student.GPA = total / student.num_courses;

    return student.GPA;
}

globalThis.student = {
    name: "Sagi",
    GPA: 85.0,
    id: "123456789",
    age: 24,
    num_courses: 10
};

globalThis.updateStudentGPA = updateStudentGPA;

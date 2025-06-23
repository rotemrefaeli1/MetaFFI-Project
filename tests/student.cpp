// hello_world.cpp
#include <iostream>
#include <cstdio>

extern "C" {
    // Structure to represent a student with basic academic and personal information
    struct Student {
        const char* name;
        float GPA;
        const char* id;
        int age;
        int num_courses;
    };
    // Updates a student's GPA based on a new grade
    float update_student_gpa(Student* s, float new_grade) {
        if (!s || s->num_courses < 0) {
            std::cerr << "Invalid student data" << std::endl;
            return -1.0f;
        }

        float total = s->GPA * s->num_courses;
        total += new_grade;
        s->num_courses += 1;
        s->GPA = total / s->num_courses;


        return s->GPA;
    }

    // Prints student details to the standard output
    void print_student(const Student* s) {
        if (!s) {
            printf("Invalid student pointer\n");
            fflush(stdout);
            return;
        }

        printf("👤 Student info:\n");
        printf("  Name: %s\n", s->name);
        printf("  ID: %s\n", s->id);
        printf("  GPA: %.2f\n", s->GPA);
        printf("  Age: %d\n", s->age);
        printf("  Completed Courses: %d\n", s->num_courses);
        fflush(stdout); // חשוב כשמריצים דרך Node.js
    }

}

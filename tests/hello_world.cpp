// hello_world.cpp
#include <iostream>
#include <cctype>   // for toupper
#include <cstring>  // for strlen

extern "C" {

    // מדפיס הודעה מה-C++
    void hello_world() {
        std::cout << "Hello from C++!" << std::endl;
    }

    // פונקציה שמקבלת מחרוזת ומחזירה אותה באותיות גדולות
    void uppercase(const char* input, char* output) {
        int i = 0;
        while (input[i] != '\0') {
            output[i] = toupper(static_cast<unsigned char>(input[i]));
            i++;
        }
        output[i] = '\0';  // סיום המחרוזת
    }

    // פונקציית חיבור לשלושה מספרים שלמים
    int add(int a, int b, int c) {
        return a + b + c;
    }

    // פונקציה לחלוקה עם טיפול בחלוקה באפס
    int integer_div(int a, int b) {
        if (b == 0) {
            return 0;  // לא נזרוק שגיאה – פשוט נחזיר 0
        }
        return a / b;
    }

}
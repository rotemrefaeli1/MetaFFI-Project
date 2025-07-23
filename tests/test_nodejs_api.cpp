#include "../runtime/nodejs_api.cpp"
#include "../plugin-sdk-main/runtime/xcall.h"

// הקפד לכלול את כל ההגדרות שאתה משתמש בהן

int main()
{
    load_runtime();

    xcall* pxcall = load_entity("hello.js", "helloMetaFFI", nullptr, 0, nullptr, 0, nullptr);
    if(pxcall)
    {
        pxcall->operator()(); // שקול ל־ xcall[0](xcall[1]) אבל עם טיפול בשגיאות
    }

    free_runtime();
    return 0;
}

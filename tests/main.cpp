#include <iostream>
#include <v8.h>
#include <libplatform/libplatform.h>

using namespace v8;

int main(int argc, char* argv[]) {
    // אתחול הפלטפורמה של V8
    V8::InitializeICUDefaultLocation(argv[0]);
    V8::InitializeExternalStartupData(argv[0]);
    std::unique_ptr<Platform> platform = platform::NewDefaultPlatform();
    V8::InitializePlatform(platform.get());
    V8::Initialize();

    // הגדרת הקצאת זיכרון ל־ArrayBuffer
    Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();

    // יצירת isolate והרצת הסקריפט
    Isolate* isolate = Isolate::New(create_params);
    {
        Isolate::Scope isolate_scope(isolate);
        HandleScope handle_scope(isolate);

        // יצירת context עבור הקוד שלנו
        Local<Context> context = Context::New(isolate);
        Context::Scope context_scope(context);

        // קוד JavaScript שבו הפונקציה hello מוגדרת כ-global
        const char* js_code = R"(
            function hello() {
                console.log("Hello from V8!");
            }
            globalThis.hello = hello;
        )";

        // קומפילציה והרצה
        Local<String> source = String::NewFromUtf8(isolate, js_code).ToLocalChecked();
        Local<Script> script = Script::Compile(context, source).ToLocalChecked();
        script->Run(context).ToLocalChecked();

        // שליפת הפונקציה hello מ-globalThis
        Local<Value> val =
            context->Global()->Get(context, String::NewFromUtf8(isolate, "hello").ToLocalChecked())
            .ToLocalChecked();

        if (val->IsFunction()) {
            Local<Function> hello_func = Local<Function>::Cast(val);
            hello_func->Call(context, context->Global(), 0, nullptr).ToLocalChecked();
        } else {
            std::cerr << "Function 'hello' not found." << std::endl;
        }
    }

    // סיום עבודה
    isolate->Dispose();
    V8::Dispose();
    delete create_params.array_buffer_allocator;

    return 0;
}
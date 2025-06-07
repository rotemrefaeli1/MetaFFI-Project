#include <iostream>
#include <fstream>
#include <sstream>
#include <v8.h>
#include <libplatform/libplatform.h>

using namespace v8;

std::string ReadFile(const std::string& filename) {
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    V8::InitializeICUDefaultLocation(".");
    V8::InitializeExternalStartupData(".");
    std::unique_ptr<Platform> platform = platform::NewDefaultPlatform();
    V8::InitializePlatform(platform.get());
    V8::Initialize();

    Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = ArrayBuffer::Allocator::NewDefaultAllocator();
    Isolate* isolate = Isolate::New(create_params);

    {
        Isolate::Scope isolate_scope(isolate);
        HandleScope handle_scope(isolate);
        Local<Context> context = Context::New(isolate);
        Context::Scope context_scope(context);

        std::string code = ReadFile("my_student.js");
        Local<String> source = String::NewFromUtf8(isolate, code.c_str()).ToLocalChecked();
        Script::Compile(context, source).ToLocalChecked()->Run(context).ToLocalChecked();

        Local<Object> global = context->Global();
        Local<Object> student = global->Get(context, String::NewFromUtf8(isolate, "student").ToLocalChecked()).ToLocalChecked()->ToObject(context).ToLocalChecked();
        Local<Function> update_func = Local<Function>::Cast(
            global->Get(context, String::NewFromUtf8(isolate, "updateStudentGPA").ToLocalChecked()).ToLocalChecked()
        );

        Local<Value> args[2] = {
            student,
            Number::New(isolate, 92.0)
        };

        TryCatch try_catch(isolate);
        Local<Value> result;
        if (!update_func->Call(context, global, 2, args).ToLocal(&result)) {
            String::Utf8Value err(isolate, try_catch.Exception());
            std::cerr << "❌ JS Exception: " << *err << std::endl;
            return 1;
        }

        double new_gpa = result->NumberValue(context).ToChecked();
        std::cout << "📘 New GPA: " << new_gpa << std::endl;

        auto get_str = [&](const char* key) {
            Local<Value> val = student->Get(context, String::NewFromUtf8(isolate, key).ToLocalChecked()).ToLocalChecked();
            String::Utf8Value str(isolate, val);
            return std::string(*str);
        };

        auto get_num = [&](const char* key) {
            Local<Value> val = student->Get(context, String::NewFromUtf8(isolate, key).ToLocalChecked()).ToLocalChecked();
            return val->NumberValue(context).ToChecked();
        };

        std::cout << "👤 Student: " << get_str("name") << ", ID: " << get_str("id") << std::endl;
        std::cout << "🎓 GPA: " << get_num("GPA") << ", Age: " << get_num("age")
                  << ", Courses: " << get_num("num_courses") << std::endl;
    }

    isolate->Dispose();
    V8::Dispose();
    V8::ShutdownPlatform();
    delete create_params.array_buffer_allocator;
    return 0;
}

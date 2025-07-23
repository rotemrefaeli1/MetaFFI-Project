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
    std::cout << "✅ main() started!" << std::endl;

    // Initialize V8 with "." as the default path
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

        // Print logs from JavaScript to the C++ console
        Local<Object> global = context->Global();
        Local<Object> console = Object::New(isolate);
        console->Set(context,
            String::NewFromUtf8(isolate, "log").ToLocalChecked(),
            Function::New(context, [](const FunctionCallbackInfo<Value>& args) {
                if (args.Length() > 0) {
                    String::Utf8Value utf8(args.GetIsolate(), args[0]);
                    std::cout << "🟢 JS log: " << *utf8 << std::endl;
                }
            }).ToLocalChecked()
        ).Check();
        global->Set(context, String::NewFromUtf8(isolate, "console").ToLocalChecked(), console).Check();

        // Read the code from tests.js
        std::string js_code = ReadFile("tests.js");
        if (js_code.empty()) {
            std::cerr << "❌ tests.js is missing or empty" << std::endl;
            return 1;
        }


        // Compilation and execution
        Local<String> source = String::NewFromUtf8(isolate, js_code.c_str()).ToLocalChecked();
        Local<Script> script = Script::Compile(context, source).ToLocalChecked();
        script->Run(context).ToLocalChecked();

        // Call the hello() function from globalThis
        Local<Value> val = global->Get(context, String::NewFromUtf8(isolate, "hello").ToLocalChecked()).ToLocalChecked();
        if (val->IsFunction()) {
            Local<Function> hello_func = Local<Function>::Cast(val);
            hello_func->Call(context, global, 0, nullptr).ToLocalChecked();
        } else {
            std::cerr << "❌ hello() not found!" << std::endl;
        }

        // Call uppercaseme("hello metaffi")
        {
            Local<Value> val = global->Get(context, String::NewFromUtf8(isolate, "uppercaseme").ToLocalChecked()).ToLocalChecked();
            if (val->IsFunction()) {
                Local<Function> func = Local<Function>::Cast(val);
                Local<Value> args[1] = { String::NewFromUtf8(isolate, "hello metaffi").ToLocalChecked() };
                Local<Value> result = func->Call(context, global, 1, args).ToLocalChecked();
                String::Utf8Value utf8(isolate, result);
                std::cout << "🔤 uppercaseme: " << *utf8 << std::endl;
            }
        }

        // Call add(10, 20, 30)
        {
            Local<Value> val = global->Get(context, String::NewFromUtf8(isolate, "add").ToLocalChecked()).ToLocalChecked();
            if (val->IsFunction()) {
                Local<Function> func = Local<Function>::Cast(val);
                Local<Value> args[3] = {
                    Integer::New(isolate, 10),
                    Integer::New(isolate, 20),
                    Integer::New(isolate, 30)
                };
                Local<Value> result = func->Call(context, global, 3, args).ToLocalChecked();
                std::cout << "➕ add: " << result->IntegerValue(context).ToChecked() << std::endl;
            }
        }

        // Call integer_div(10, 3)
        {
            Local<Value> val = global->Get(context, String::NewFromUtf8(isolate, "integer_div").ToLocalChecked()).ToLocalChecked();
            if (val->IsFunction()) {
                Local<Function> func = Local<Function>::Cast(val);
                Local<Value> args[2] = {
                    Integer::New(isolate, 10),
                    Integer::New(isolate, 3)
                };
                Local<Value> result = func->Call(context, global, 2, args).ToLocalChecked();
                std::cout << "➗ integer_div: " << result->NumberValue(context).ToChecked() << std::endl;
            }
        }



    // 📚 Student GPA update
    {
    Local<Value> student_val = global->Get(context, String::NewFromUtf8(isolate, "student").ToLocalChecked()).ToLocalChecked();
    if (!student_val->IsObject()) {
        std::cerr << "❌ student object not found" << std::endl;
    } else {
        Local<Object> student = student_val->ToObject(context).ToLocalChecked();

        // Print previous GPA
        double previous_gpa = student->Get(context, String::NewFromUtf8(isolate, "GPA").ToLocalChecked())
                                .ToLocalChecked()->NumberValue(context).ToChecked();
        std::cout << "📚 Previous GPA: " << previous_gpa << std::endl;

        // Fetch updateStudentGPA function
        Local<Value> func_val = global->Get(context, String::NewFromUtf8(isolate, "updateStudentGPA").ToLocalChecked()).ToLocalChecked();
        if (func_val->IsFunction()) {
            Local<Function> update_func = Local<Function>::Cast(func_val);
            Local<Value> args[2] = {
                student,
                Number::New(isolate, 92.0)
            };

            TryCatch try_catch(isolate);
            Local<Value> result;
            if (!update_func->Call(context, global, 2, args).ToLocal(&result)) {
                String::Utf8Value err(isolate, try_catch.Exception());
                std::cerr << "❌ JS Exception: " << *err << std::endl;
            } else {
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
              }
            }
        }
    }


    // Free V8 resources and finalize the engine
    isolate->Dispose();
    V8::Dispose();
    delete create_params.array_buffer_allocator;

    std::cout << "✅ Program finished." << std::endl;
    return 0;
}

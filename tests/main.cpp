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

    // אתחול V8 עם "." כנתיב ברירת מחדל
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

        // הדפסת לוגים מ-JavaScript ל-console של C++
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

        // קריאת הקוד מ-hello.js
        std::string js_code = ReadFile("hello.js");
        if (js_code.empty()) {
            std::cerr << "❌ hello.js is missing or empty" << std::endl;
            return 1;
        }


        // קומפילציה והרצה
        Local<String> source = String::NewFromUtf8(isolate, js_code.c_str()).ToLocalChecked();
        Local<Script> script = Script::Compile(context, source).ToLocalChecked();
        script->Run(context).ToLocalChecked();

        // הפעלת הפונקציה hello() מ-globalThis
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
    }
    isolate->Dispose();
    V8::Dispose();
    delete create_params.array_buffer_allocator;

    std::cout << "✅ Program finished." << std::endl;
    return 0;
}

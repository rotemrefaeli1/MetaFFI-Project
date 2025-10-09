import api.MetaFFIRuntime;
import api.MetaFFIModule;
import metaffi.Caller;

public class test {
    public static void main(String[] args) {
        try {
            // Initialize the Node.js runtime
            MetaFFIRuntime runtime = new MetaFFIRuntime("nodejs");
            runtime.loadRuntimePlugin();

            // Load the JavaScript module
            MetaFFIModule module = runtime.loadModule("/workspace/runtime/hello.js");

            // Get the exported JS function
            Caller func = module.load("callable=helloMetaFFI", null, null);

            // Call the function and get the return value
            Object result = func.call();

            // Cleanup
            runtime.releaseRuntimePlugin();

        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
        }
    }
}

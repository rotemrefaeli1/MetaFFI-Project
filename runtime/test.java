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

            // Print the returned value
            //if (result != null)
              //  System.out.println("Node.js returned: " + result.toString());
            //else
            //    System.out.println("Node.js returned null.");

            // Attempt to show the JS console output file (optional)
            try {
                java.nio.file.Path path = java.nio.file.Paths.get("/workspace/runtime/hello_output.txt");
                if (java.nio.file.Files.exists(path)) {
                    String content = java.nio.file.Files.readString(path);
                    System.out.println("Console output:\n" + content);
                }
            } catch (Exception ignored) {
                // Ignore missing file silently for a clean presentation
            }

            // Cleanup
            runtime.releaseRuntimePlugin();

        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
        }
    }
}

import api.MetaFFIRuntime;
import api.MetaFFIModule;
import metaffi.Caller;
import metaffi.MetaFFITypeInfo;

public class test {
    // Helper to unwrap single-return Caller result (MetaFFI returns Object[] for multiple slots)
    static Object unwrapSingleReturn(Object callResult) {
        if (callResult == null) return null;
        if (callResult.getClass().isArray()) {
            Object[] arr = (Object[]) callResult;
            return arr.length > 0 ? arr[0] : null;
        }
        return callResult;
    }

    public static void main(String[] args) {
        System.out.println("Java: Starting MetaFFI test");
        MetaFFIRuntime runtime = null;

        try {
            // Initialize Node.js runtime plugin
            runtime = new MetaFFIRuntime("nodejs");
            runtime.loadRuntimePlugin();
            System.out.println("Java: runtime.loadRuntimePlugin() done");

            // Load modules
            MetaFFIModule helloModule = runtime.loadModule("/workspace/runtime/hello.js");
            MetaFFIModule addModule   = runtime.loadModule("/workspace/runtime/add.js");
            MetaFFIModule boolModule  = runtime.loadModule("/workspace/runtime/booleanTest.js");
            System.out.println("Java: modules loaded");

            // -------------------- helloMetaFFI (no params, no return) --------------------
            try {
                Caller hello = helloModule.load("callable=helloMetaFFI", null, null);
                System.out.println("\nCalling helloMetaFFI() ...");
                hello.call();
                System.out.println("helloMetaFFI() done");
            } catch (Exception e) {
                System.err.println("Error calling helloMetaFFI: " + e.getMessage());
                e.printStackTrace();
            }

            // -------------------- add_ints (int32,int32) -> int32 --------------------
            try {
                MetaFFITypeInfo int32 = new MetaFFITypeInfo(MetaFFITypeInfo.MetaFFITypes.MetaFFIInt32);
                MetaFFITypeInfo[] add_ints_params = new MetaFFITypeInfo[]{ int32, int32 };
                MetaFFITypeInfo[] add_ints_rets   = new MetaFFITypeInfo[]{ int32 };

                Caller addInts = addModule.load("callable=add_ints", add_ints_params, add_ints_rets);
                System.out.println("\nCalling add_ints(7, 5) ...");
                Object raw = addInts.call(7, 5);
                Object val = unwrapSingleReturn(raw);
                System.out.println("add_ints -> " + val);
            } catch (Exception e) {
                System.err.println("Error calling add_ints: " + e.getMessage());
                e.printStackTrace();
            }

            // -------------------- add_ints64 (int64,int64) -> int64 --------------------
            try {
                MetaFFITypeInfo int64 = new MetaFFITypeInfo(MetaFFITypeInfo.MetaFFITypes.MetaFFIInt64);
                MetaFFITypeInfo[] params64 = new MetaFFITypeInfo[]{ int64, int64 };
                MetaFFITypeInfo[] rets64   = new MetaFFITypeInfo[]{ int64 };

                Caller addInts64 = addModule.load("callable=add_ints64", params64, rets64);
                System.out.println("\nCalling add_ints64(10000000000, 25000000000) ...");
                Object raw64 = addInts64.call(10000000000L, 25000000000L);
                Object v64 = unwrapSingleReturn(raw64);
                System.out.println("add_ints64 -> " + v64);
            } catch (Exception e) {
                System.err.println("Error calling add_ints64: " + e.getMessage());
                e.printStackTrace();
            }

            // -------------------- add_f32 (float32,float32) -> float32 --------------------
            try {
                MetaFFITypeInfo f32 = new MetaFFITypeInfo(MetaFFITypeInfo.MetaFFITypes.MetaFFIFloat32);
                MetaFFITypeInfo[] params_f32 = new MetaFFITypeInfo[]{ f32, f32 };
                MetaFFITypeInfo[] rets_f32   = new MetaFFITypeInfo[]{ f32 };

                Caller addF32 = addModule.load("callable=add_f32", params_f32, rets_f32);
                System.out.println("\nCalling add_f32(1.5f, 2.25f) ...");
                Object rawf32 = addF32.call(1.5f, 2.25f);
                Object vf32 = unwrapSingleReturn(rawf32);
                System.out.println("add_f32 -> " + vf32);
            } catch (Exception e) {
                System.err.println("Error calling add_f32: " + e.getMessage());
                e.printStackTrace();
            }

            // -------------------- add_f64 (float64,float64) -> float64 --------------------
            try {
                MetaFFITypeInfo f64 = new MetaFFITypeInfo(MetaFFITypeInfo.MetaFFITypes.MetaFFIFloat64);
                MetaFFITypeInfo[] params_f64 = new MetaFFITypeInfo[]{ f64, f64 };
                MetaFFITypeInfo[] rets_f64   = new MetaFFITypeInfo[]{ f64 };

                Caller addF64 = addModule.load("callable=add_f64", params_f64, rets_f64);
                System.out.println("\nCalling add_f64(3.14, 2.71) ...");
                Object rawf64 = addF64.call(3.14, 2.71);
                Object vf64 = unwrapSingleReturn(rawf64);
                System.out.println("add_f64 -> " + vf64);
            } catch (Exception e) {
                System.err.println("Error calling add_f64: " + e.getMessage());
                e.printStackTrace();
            }

            // -------------------- invert_bool (bool) -> bool --------------------
            try {
                MetaFFITypeInfo boolt = new MetaFFITypeInfo(MetaFFITypeInfo.MetaFFITypes.MetaFFIBool);
                MetaFFITypeInfo[] params_bool = new MetaFFITypeInfo[]{ boolt };
                MetaFFITypeInfo[] rets_bool   = new MetaFFITypeInfo[]{ boolt };

                Caller inv = boolModule.load("callable=invert_bool", params_bool, rets_bool);
                System.out.println("\nCalling invert_bool(true) ...");
                Object rawInv = inv.call(true);
                Object vInv = unwrapSingleReturn(rawInv);
                System.out.println("invert_bool -> " + vInv);
            } catch (Exception e) {
                System.err.println("Error calling invert_bool: " + e.getMessage());
                e.printStackTrace();
            }

            // -------------------- and_bool (bool,bool) -> bool --------------------
            try {
                MetaFFITypeInfo boolt = new MetaFFITypeInfo(MetaFFITypeInfo.MetaFFITypes.MetaFFIBool);
                MetaFFITypeInfo[] params_and = new MetaFFITypeInfo[]{ boolt, boolt };
                MetaFFITypeInfo[] rets_and   = new MetaFFITypeInfo[]{ boolt };

                Caller andb = boolModule.load("callable=and_bool", params_and, rets_and);
                System.out.println("\nCalling and_bool(true, false) ...");
                Object rawAnd = andb.call(true, false);
                Object vAnd = unwrapSingleReturn(rawAnd);
                System.out.println("and_bool -> " + vAnd);
            } catch (Exception e) {
                System.err.println("Error calling and_bool: " + e.getMessage());
                e.printStackTrace();
            }

        } catch (Exception e) {
            System.err.println("Fatal error: " + e.getMessage());
            e.printStackTrace();
        } finally {
            if (runtime != null) {
                try {
                    runtime.releaseRuntimePlugin();
                    System.out.println("\n[nodejs] free_runtime");
                    System.out.println("Java: runtime released");
                } catch (Exception ex) {
                    System.err.println("Error releasing runtime: " + ex.getMessage());
                    ex.printStackTrace();
                }
            }
        }

        System.out.println("\nJava: Test complete");
    }
}

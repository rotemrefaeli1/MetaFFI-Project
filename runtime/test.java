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

            // Load the single unified module
            MetaFFIModule testModule = runtime.loadModule("/workspace/runtime/TestFunctions.js");
            System.out.println("Java: TestFunctions.js loaded");

            // -------------------- helloMetaFFI (no params, no return) --------------------
            try {
                Caller hello = testModule.load("callable=helloMetaFFI", null, null);
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
                MetaFFITypeInfo[] params = new MetaFFITypeInfo[]{ int32, int32 };
                MetaFFITypeInfo[] rets   = new MetaFFITypeInfo[]{ int32 };

                Caller addInts = testModule.load("callable=add_ints", params, rets);
                System.out.println("\nCalling add_ints(7, 5) ...");
                Object val = unwrapSingleReturn(addInts.call(7, 5));
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

                Caller addInts64 = testModule.load("callable=add_ints64", params64, rets64);
                System.out.println("\nCalling add_ints64(10000000000, 25000000000) ...");
                Object v64 = unwrapSingleReturn(addInts64.call(10000000000L, 25000000000L));
                System.out.println("add_ints64 -> " + v64);
            } catch (Exception e) {
                System.err.println("Error calling add_ints64: " + e.getMessage());
                e.printStackTrace();
            }

            // -------------------- add_f32 (float32,float32) -> float32 --------------------
            try {
                MetaFFITypeInfo f32 = new MetaFFITypeInfo(MetaFFITypeInfo.MetaFFITypes.MetaFFIFloat32);
                MetaFFITypeInfo[] params = new MetaFFITypeInfo[]{ f32, f32 };
                MetaFFITypeInfo[] rets   = new MetaFFITypeInfo[]{ f32 };

                Caller addF32 = testModule.load("callable=add_f32", params, rets);
                System.out.println("\nCalling add_f32(1.5f, 2.25f) ...");
                Object vf32 = unwrapSingleReturn(addF32.call(1.5f, 2.25f));
                System.out.println("add_f32 -> " + vf32);
            } catch (Exception e) {
                System.err.println("Error calling add_f32: " + e.getMessage());
                e.printStackTrace();
            }

            // -------------------- add_f64 (float64,float64) -> float64 --------------------
            try {
                MetaFFITypeInfo f64 = new MetaFFITypeInfo(MetaFFITypeInfo.MetaFFITypes.MetaFFIFloat64);
                MetaFFITypeInfo[] params = new MetaFFITypeInfo[]{ f64, f64 };
                MetaFFITypeInfo[] rets   = new MetaFFITypeInfo[]{ f64 };

                Caller addF64 = testModule.load("callable=add_f64", params, rets);
                System.out.println("\nCalling add_f64(3.14, 2.71) ...");
                Object vf64 = unwrapSingleReturn(addF64.call(3.14, 2.71));
                System.out.println("add_f64 -> " + vf64);
            } catch (Exception e) {
                System.err.println("Error calling add_f64: " + e.getMessage());
                e.printStackTrace();
            }

            // -------------------- div_i64 (int64,int64) -> int64 --------------------
            try {
                MetaFFITypeInfo int64 = new MetaFFITypeInfo(MetaFFITypeInfo.MetaFFITypes.MetaFFIInt64);
                MetaFFITypeInfo[] params64 = new MetaFFITypeInfo[]{ int64, int64 };
                MetaFFITypeInfo[] rets64   = new MetaFFITypeInfo[]{ int64 };

                Caller div_i64 = testModule.load("callable=div_i64", params64, rets64);

                System.out.println("====div_i64====");
                Object r1 = unwrapSingleReturn(div_i64.call(6L, 3L));
                System.out.println("div_i64(6,3) -> " + r1);

                // Expecting exception on division by zero (will propagate naturally)
                Object r2 = unwrapSingleReturn(div_i64.call(5L, 0L));
                System.out.println("div_i64(5,0) -> " + r2);

            } catch (Exception e) {
                System.err.println("Error calling div_i64: " + e.getMessage());
                e.printStackTrace();
            }


            // -------------------- invert_bool (bool) -> bool --------------------
            try {
                MetaFFITypeInfo boolt = new MetaFFITypeInfo(MetaFFITypeInfo.MetaFFITypes.MetaFFIBool);
                MetaFFITypeInfo[] params = new MetaFFITypeInfo[]{ boolt };
                MetaFFITypeInfo[] rets   = new MetaFFITypeInfo[]{ boolt };

                Caller inv = testModule.load("callable=invert_bool", params, rets);
                System.out.println("\nCalling invert_bool(true) ...");
                Object vInv = unwrapSingleReturn(inv.call(true));
                System.out.println("invert_bool -> " + vInv);
            } catch (Exception e) {
                System.err.println("Error calling invert_bool: " + e.getMessage());
                e.printStackTrace();
            }

            // -------------------- and_bool (bool,bool) -> bool --------------------
            try {
                MetaFFITypeInfo boolt = new MetaFFITypeInfo(MetaFFITypeInfo.MetaFFITypes.MetaFFIBool);
                MetaFFITypeInfo[] params = new MetaFFITypeInfo[]{ boolt, boolt };
                MetaFFITypeInfo[] rets   = new MetaFFITypeInfo[]{ boolt };

                Caller andb = testModule.load("callable=and_bool", params, rets);
                System.out.println("\nCalling and_bool(true, false) ...");
                Object vAnd = unwrapSingleReturn(andb.call(true, false));
                System.out.println("and_bool -> " + vAnd);
            } catch (Exception e) {
                System.err.println("Error calling and_bool: " + e.getMessage());
                e.printStackTrace();
            }

            // -------------------- echo (string8 -> string8) --------------------
            try {
                MetaFFITypeInfo str8 = new MetaFFITypeInfo(MetaFFITypeInfo.MetaFFITypes.MetaFFIString8);
                MetaFFITypeInfo[] params = new MetaFFITypeInfo[]{ str8 };
                MetaFFITypeInfo[] rets   = new MetaFFITypeInfo[]{ str8 };

                Caller echo = testModule.load("callable=echo", params, rets);
                System.out.println("\nCalling echo('hello metaffi') ...");
                Object vEcho1 = unwrapSingleReturn(echo.call("hello metaffi"));
                System.out.println("echo('hello metaffi') -> " + vEcho1);

                System.out.println("Calling echo('metaffi ') ...");
                Object vEcho2 = unwrapSingleReturn(echo.call("metaffi "));
                System.out.println("echo('metaffi ') -> " + vEcho2);
            } catch (Exception e) {
                System.err.println("Error calling echo: " + e.getMessage());
                e.printStackTrace();
            }

            // -------------------- greet (string8 -> string8) --------------------
            try {
                MetaFFITypeInfo str8 = new MetaFFITypeInfo(MetaFFITypeInfo.MetaFFITypes.MetaFFIString8);
                MetaFFITypeInfo[] params = new MetaFFITypeInfo[]{ str8 };
                MetaFFITypeInfo[] rets   = new MetaFFITypeInfo[]{ str8 };

                Caller greet = testModule.load("callable=greet", params, rets);
                System.out.println("\nCalling greet('Sagi') ...");
                Object vGreet = unwrapSingleReturn(greet.call("Sagi"));
                System.out.println("greet('Sagi') -> " + vGreet);
            } catch (Exception e) {
                System.err.println("Error calling greet: " + e.getMessage());
                e.printStackTrace();
            }
            // -------------------- OBJECT / HANDLE TEST --------------------
            try {
                System.out.println("\n==== OBJECT / HANDLE TEST (Java) ====");

                MetaFFITypeInfo int64 = new MetaFFITypeInfo(MetaFFITypeInfo.MetaFFITypes.MetaFFIInt64);
                MetaFFITypeInfo handleType = new MetaFFITypeInfo(MetaFFITypeInfo.MetaFFITypes.MetaFFIHandle);

                // Load entities
                Caller createCounter = testModule.load(
                        "callable=create_counter",
                        new MetaFFITypeInfo[]{ int64 },
                        new MetaFFITypeInfo[]{ handleType }
                );

                Caller counterGet = testModule.load(
                        "callable=counter_get",
                        new MetaFFITypeInfo[]{ handleType },
                        new MetaFFITypeInfo[]{ int64 }
                );

                Caller counterInc = testModule.load(
                        "callable=counter_inc",
                        new MetaFFITypeInfo[]{ handleType, int64 },
                        new MetaFFITypeInfo[]{ int64 }
                );

                // === Calls ===
                Object hObj = unwrapSingleReturn(createCounter.call(10L));
                System.out.println("Java handle from create_counter(10) = " + hObj +
                                   (hObj != null ? " (" + hObj.getClass().getName() + ")" : ""));

                // --- counter_get ---
                try {
                    Object v1 = unwrapSingleReturn(counterGet.call(hObj));
                    System.out.println("counter_get(c) -> " + v1);
                } catch (Exception e) {
                    System.out.println("counter_get(c) threw exception: " + e.getMessage());
                }

                // --- counter_inc ---
                try {
                    Object v2 = unwrapSingleReturn(counterInc.call(hObj, 5L));
                    System.out.println("counter_inc(c, 5) -> " + v2);
                } catch (Exception e) {
                    System.out.println("counter_inc(c, 5) threw exception: " + e.getMessage());
                }

                // --- counter_get again ---
                try {
                    Object v3 = unwrapSingleReturn(counterGet.call(hObj));
                    System.out.println("counter_get(c) again -> " + v3);
                } catch (Exception e) {
                    System.out.println("counter_get(c) again threw exception: " + e.getMessage());
                }

            } catch (Exception e) {
                System.err.println("Error in OBJECT / HANDLE TEST: " + e.getMessage());
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

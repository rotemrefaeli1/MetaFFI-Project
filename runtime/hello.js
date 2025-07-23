function helloMetaFFI() {
    console.log("👋 Hello from Node.js via MetaFFI!");
}

globalThis.helloMetaFFI = helloMetaFFI;

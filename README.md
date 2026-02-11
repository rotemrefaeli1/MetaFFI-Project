# MetaFFI Nodejs Runtime Plugin (xllr.nodejs)

## Overview
This project extends the **MetaFFI** framework with full **Nodejs** runtime support by implementing a dedicated runtime plugin (`xllr.nodejs`). The plugin enables **typed and bidirectional** cross-language integration between **Nodejs** and other MetaFFI-supported languages, including **Python, Java, and Go**.

The goal is to allow Nodejs functions and objects to participate naturally in multi-language systems through MetaFFI, without requiring manual per-language-pair bindings.

---

## Key Capabilities
- **Runtime lifecycle management**: initialize and free the Nodejs runtime through MetaFFI.
- **Entity loading**: load Nodejs functions/modules as MetaFFI entities.
- **Typed invocation**: call Nodejs entities with parameters and return values using MetaFFI type descriptors.
- **CDT/CDTS conversion layer**: consistent type translation across languages (primitives, objects/handles, arrays).
- **Exception propagation**: catching and returning Nodejs exceptions through the MetaFFI error interface.
- **Cross-runtime validation**: tested across Python, Java, and Go.

> Note: Some directories/files may vary depending on your local MetaFFI installation layout.

---

## Requirements

Primary validation environment: **Linux (Docker-based)**

### Dependencies (Linux)

- `nodejs`
- `npm`
- `libnode-dev`
- `g++` with C++20 support

Install:

```bash
apt update
apt install nodejs npm libnode-dev

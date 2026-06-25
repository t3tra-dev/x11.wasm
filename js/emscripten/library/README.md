# x11.wasm Emscripten js-library shards

These files are concatenated by `ninja/x11wasm.ninja` into the generated
`x11wasm_js_library` input consumed by Emscripten. Each shard calls
`mergeInto(LibraryManager.library, ...)` in its own scope so the source files can
be typechecked independently.

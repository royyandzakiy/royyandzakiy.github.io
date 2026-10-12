# C++23 in a Blog Post: a 198-Byte WebAssembly Q15 Converter

Type a number between -1 and 1. The conversion below runs C++23 compiled to WebAssembly, right in this page.

<div class="q15-demo">
  <label>Value: <input id="q15-x" type="number" step="0.0001" value="0.3"></label>
  <pre id="q15-out">loading…</pre>
</div>

<script type="module">
  const out = document.getElementById('q15-out');
  const input = document.getElementById('q15-x');
  const { instance: { exports: w } } =
    await WebAssembly.instantiateStreaming(fetch('/assets/demos/2026-10-cpp23-wasm-q15/q15.wasm'));
  const update = () => {
    const x = +input.value;
    const q = w.to_q15(x), back = w.from_q15(q);
    out.textContent =
      `Q15:   ${q}  (0x${w.q15_bits(q).toString(16).padStart(4, '0')})\n` +
      `back:  ${back}\n` +
      `error: ${(x - back).toExponential(3)}`;
  };
  input.oninput = update;
  update();
</script>

## The C++

TODO: walk through `_demos/2026-10-cpp23-wasm-q15/q15.cpp`: constexpr conversion, saturation, `static_assert` tests that run at compile time.

## The build

```sh
clang++ -std=c++23 --target=wasm32 -O2 -ffreestanding -nostdlib \
    -Wl,--no-entry -Wl,--strip-all -o q15.wasm q15.cpp
```

No Emscripten, no runtime, no imports: stock LLVM ships the wasm32 backend and `wasm-ld`.

## Loading it

TODO: `WebAssembly.instantiateStreaming` + calling exports, scalars only, no memory sharing yet.

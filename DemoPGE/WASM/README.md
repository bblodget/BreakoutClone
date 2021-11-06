To build WASM outputs in the parent directory run

```bash
em++ -std=c++17 -O2 -s ALLOW_MEMORY_GROWTH=1 -s MAX_WEBGL_VERSION=2 -s MIN_WEBGL_VERSION=2  -s USE_LIBPNG=1 main.cpp -o WASM/breakout.html --preload-file ./assets
```

See this video for more info about compiling PGE for WASM.

https://youtube.com/watch?v=Mrl5kkVY6zk



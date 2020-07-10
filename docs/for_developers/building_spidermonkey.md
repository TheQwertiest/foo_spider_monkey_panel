---
title: Building Mozilla SpiderMonkey
parent: Building component
grand_parent: For component developers
nav_order: 1
---

# Building Mozilla SpiderMonkey
{: .no_toc }

---

Full instructions are available here: https://developer.mozilla.org/en-US/docs/Mozilla/Projects/SpiderMonkey/Build_Documentation

Preparation steps:
 - Download Mozilla SpiderMonkey sources via [git](https://github.com/mozilla/gecko-dev/tree/esr60) or [mercurial](https://hg.mozilla.org/mozilla-central). You need `esr60` branch.
 - Download and install [MozillaBuild](https://developer.mozilla.org/en-US/docs/Mozilla/Developer_guide/Build_Instructions/Windows_Prerequisites#Required_tools).
 - Download and install [LLVM](https://releases.llvm.org/download.html).

Build steps:
1. Open command prompt.
1. Execute the following commands:

~~~bash
# Switch to your `MozillaBuild` installation folder.
cd ${YOUR_MOZILLA_BUILD_PATH}
./start-shell.bat

# Switch to your SpiderMonkey sources folder.
cd ${YOUR_SPIDERMONKEY_SRC_PATH}
cd js/src
mkdir _build
cd _build

# Adjust LLVM installation path if needed.
# Add `--enable-debug` if you want to make a binary for debug build of SMP.
PATH=$PATH:"/c/Program Files/LLVM/bin/" JS_STANDALONE=1 ../configure --enable-nspr-build --disable-jemalloc --disable-js-shell --disable-tests --target=i686-pc-mingw32 --host=i686-pc-mingw32 --with-libclang-path="C:/Program Files/LLVM/bin" --with-clang-path="C:/Program Files/LLVM/bin/clang.exe"
mozmake
~~~
Build artifacts will be located in the following paths:
  - `_build/dist`: `.dll` and headers.
  - `_build/js/src/build`: `mozjs-68.lib` and `mozjs-68.pdb`.
  - `_build/config/external/nspr/ds`: `plds4.pdb`.
  - `_build/config/external/nspr/libc`: `plc4.pdb`.
  - `_build/config/external/nspr/pr`: `nspr4.pdb`.
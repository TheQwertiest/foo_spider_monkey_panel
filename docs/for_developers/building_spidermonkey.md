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

Build steps:
1. Launch `x86 Native Tools Command Prompt for VS2017` (supplied with Visual Studio 2017 installation).
1. Execute the following commands:

~~~bash
# Switch to your `MozillaBuild` installation folder.
cd ${YOUR_MOZILLA_BUILD_PATH}
set MOZ_NO_RESET_PATH=1
./start-shell.bat

# Switch to your SpiderMonkey sources folder.
cd ${YOUR_SPIDERMONKEY_SRC_PATH}
cd js/src
autoconf-2.13
mkdir _build
cd _build

# Add `--enable-debug` if you want to make a binary for debug build of SMP
../configure --enable-nspr-build --disable-jemalloc --disable-js-shell --disable-tests
mozmake
~~~
Build artifacts will be located in the following paths:
  - `_build/dist`: `.dll` and headers.
  - `_build/js/src/build`: `mozjs-60.lib` and `mozjs-60.pdb`.
  - `_build/config/external/nspr/ds`: `plds4.pdb`.
  - `_build/config/external/nspr/libc`: `plc4.pdb`.
  - `_build/config/external/nspr/pr`: `nspr4.pdb`.
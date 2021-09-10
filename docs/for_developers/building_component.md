---
title: Building component
parent: For component developers
nav_order: 1
has_children: true
---

# Building component
{: .no_toc }

## Table of contents
{: .no_toc .text-delta }

* TOC
{:toc}

---

## Prerequisites

 - Windows 7 or later.
 - Visual Studio 2019.
 - Git client (e.g. <https://desktop.github.com>). Must be present in `%PATH%`.
 - Git LFS extension (<https://github.com/git-lfs/git-lfs/releases/latest>). Execute `git lfs install` after installation.
 - Python 3.+ (<https://www.python.org/downloads/windows>). Must be present in `%PATH%`.
 - 7-Zip (<https://www.7-zip.org/download.html>). Must be present in `%PATH%`.

## Building foo_spider_monkey_panel.dll

1. Download this repository.
1. Change current directory to `foo_spider_monkey_panel/scripts`.
1. Prepare submodules:
   - If you want to use pre-built SpiderMonkey, execute `py setup.py`: this script will download and initialize all submodules and will also apply compatibility patches.
   - Otherwise, you can build and use your own Mozilla SpiderMonkey engine binaries (ESR68):
     <details><summary markdown="span">Instructions</summary>

     1. [Build SpiderMonkey](building_spidermonkey.md).<br>
     2. Put SpiderMonkey engine binaries and headers in `foo_spider_monkey_panel/mozjs` folder using the following pattern:<br>
        <blockquote>Configuration = Release or Debug<br>
        mozjs / %Configuration% / bin / *.dll | *.pdb<br>
        mozjs / %Configuration% / lib / *.lib<br>
        mozjs / %Configuration% / include / *.h</blockquote>
     3. Execute `py setup.py --skip_mozjs`.
     </details>
1. Main solution can be found at `foo_spider_monkey_panel/workspaces/foo_spider_monkey_panel.sln`.
1. Output artifacts will be placed in `foo_spider_monkey_panel/_result/%Configuration%/bin/`.

## Creating .fb2k-component package

1. Change current directory to `foo_spider_monkey_panel/scripts`.
1. Execute `py pack_component.py`: this script will pack all the required files to `_result/%Configuration%/foo_spider_monkey_panel.fb2k-component`.
   - `py pack_component.py --debug` will create the package from debug binaries.
---
title: Tips for component developers
parent: For component developers
nav_order: 2
---

# Tips for component developers
{: .no_toc }

## Table of contents
{: .no_toc .text-delta }

* TOC
{:toc}

---

### Build in Debug mode
Mozilla developers are very generous on `assert`s, but these will only trigger on `Debug` configuration.  
There are a lot of them and they will pick up most of the errors in handling SpiderMonkey API.

Note: to use debug `foo_spider_monkey_panel.dll` you need corresponding SpiderMonkey debug binaries.

### Test with GCZeal
If you make some changes related to JS API, don't forget to test them with GCZeal.  

GCZeal option can be enabled in `Preferences > Advanced > Display > Spider Monkey Panel > GC > Zeal > Enable`.  
Note: this option is only available in `Debug` build.
  
This option makes GC very zealous - it will try to GC everything every few allocations (frequency is set in `Preferences > Advanced > Display > Spider Monkey Panel > GC > Zeal > Frequency`).  

This WILL make component much slower, but it will also allow you to test for and reproduce GC related crashes.
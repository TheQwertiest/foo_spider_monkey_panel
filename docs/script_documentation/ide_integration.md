---
title: IDE integration
parent: Script documentation
nav_order: 2
---

# IDE integration
{: .no_toc }

## Table of contents
{: .no_toc .text-delta }

* TOC
{:toc}

---

SMP JavaScript interface can be integrated in various IDE for early error detection, auto-completion and type-checking.  

### Instructions
- [Visual Studio Code](https://code.visualstudio.com/download): 
   - Either add a comment to the top of each file pointing to `foo_spider_monkey_panel.js` interface file:
     ```javascript
     /// <reference path="./../../user-components/foo_spider_monkey_panel/docs/js/foo_spider_monkey_panel.js">
     ```
   - Or add a `jsconfig.json` file in the root of your Visual Studio Code project with the path to `docs\js` directory.
     For example (assuming that project is in `foobar2000/themes/mytheme`):
     ```jsonc
     {
       "compilerOptions": {
         // this is needed to suppress built-in browser objects (`window`, various DOM objects and etc)
         "lib": ["es2019"]
       },
       "include": [
         // project files
         "**/*.js",
         // docs
         "../../user-components/foo_spider_monkey_panel/docs/js/*.js"
       ]
     }
     ```
     
     Note: `include` works only with relative paths.  
     Note 2: project files must be also added to `include` for the linting to work (`**/*.js` in the example above).
     
- IntelliJ [Idea Ultimate](https://www.jetbrains.com/idea/download/#section=windows)/[WebStorm](https://www.jetbrains.com/webstorm/download/#section=windows):  
   - `File`>`Settings`>`Languages`>`JavaScript`>`Libraries`>`Add...`>`+`>`Attach Directories...`> Choose `foo_spider_monkey_panel\docs\js` directory.  
  <details markdown="0">
  <summary>
  Screenshot
  </summary>

  {% assign img = "assets/img/misc/intellij_integration.png" | relative_url %}
  {% include functions/clickable_img.html
    img = img
    alt = "Properties of multiple items"
    title = "Properties of multiple items"
  %}
  </details>

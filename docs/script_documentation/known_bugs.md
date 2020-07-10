---
title: Known bugs
parent: Script documentation
nav_order: 4
---

# Known bugs
{: .no_toc }

---

- Gibberish text in error report if script filename contains UTF8 characters ([Issue #1](https://github.com/TheQwertiest/foo_spider_monkey_panel/issues/1)).
- Non-conforming handling of strings that contain NULL character ([Issue #89](https://github.com/TheQwertiest/foo_spider_monkey_panel/issues/89))
- **\[Not Fixable]** JavaScript errors are suppressed if thrown inside ActiveX object method, this includes syntax errors as well: 
```javascript
xhr = new ActiveXObject('Microsoft.XMLHTTP');
xhr.open(method, url, true);
xhr.onreadystatechange = function () {
    undefined_function.invoke_fail(); ///< JS error thrown
}; ///< No JS error detected
xhr.send();
```

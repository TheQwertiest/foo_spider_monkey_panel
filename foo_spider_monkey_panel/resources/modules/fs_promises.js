let m = _internalModule;

export function readdir(...args) { return m.readdir(...args); };
export function readFile(...args) { return m.readFile(...args); };
export function stat(...args) { return m.stat(...args); };
export function writeFile(...args) { return m.writeFile(...args); };

export default m;

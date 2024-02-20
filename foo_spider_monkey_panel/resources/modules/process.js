let m = _internalModule;

export function cpuUsage(...args) { return m.cpuUsage(...args); };
export function cwd(...args) { return m.cwd(...args); };
export function exit(...args) { return m.exit(...args); };
export function memoryUsage(...args) { return m.memoryUsage(...args); };

export default m;

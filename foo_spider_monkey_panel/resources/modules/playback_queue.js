let m = _internalModule;

export const PlaybackQueueEvent = m.PlaybackQueueEvent;

export function clear(...args) { return m.clear(...args); };
export function indexOf(...args) { return m.indexOf(...args); };
export function pullAt(...args) { return m.pullAt(...args); };
export function push(...args) { return m.push(...args); };
export function toArray(...args) { return m.toArray(...args); };

export default m;

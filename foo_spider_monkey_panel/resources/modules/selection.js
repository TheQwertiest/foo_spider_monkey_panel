let m = _internalModule;

export function getSelection(...args) { return m.getSelection(...args); };
export function getSelectionType(...args) { return m.getSelectionType(...args); };
export function setSelection(...args) { return m.setSelection(...args); };

export const { constants } = m;

export default m;

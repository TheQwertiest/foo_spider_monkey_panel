let m = _internalModule;

export const MouseEvent = m.MouseEvent;
export const WheelEvent = m.WheelEvent;
export const Image = m.Image;

export function alert(...args) { return m.alert(...args); };
export function getContext(...args) { return m.getContext(...args); };
export function loadImage(...args) { return m.loadImage(...args); };
export function prompt(...args) { return m.prompt(...args); };
export function redraw(...args) { return m.redraw(...args); };
export function redrawRect(...args) { return m.redrawRect(...args); };
export function showFb2kPopup(...args) { return m.showFb2kPopup(...args); };

export default m;

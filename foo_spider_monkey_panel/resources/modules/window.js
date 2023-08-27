let m = _internalModule;

export const MouseEvent = m.MouseEvent;
export const WheelEvent = m.WheelEvent;
export const Image = m.Image;

export function loadImage(...args) { return m.loadImage(...args); };
export function redraw(...args) { return m.redraw(...args); };
export function redrawRect(...args) { return m.redrawRect(...args); };

export default m;

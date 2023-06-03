let m = _internalModule;

export const MouseEvent = m.MouseEvent;
export const WheelEvent = m.WheelEvent;
export const Image = m.Image;

export function repaint(...args) { return m.repaint(...args); };
export function repaintRect(...args) { return m.repaintRect(...args); };

export default m;

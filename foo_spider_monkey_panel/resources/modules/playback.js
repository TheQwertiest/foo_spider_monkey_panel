
let m = _internalModule;

export const PlaybackStopEvent = m.PlaybackStopEvent;

export function getNowPlayingTrack(...args) { return m.getNowPlayingTrack(...args); };
export function next(...args) { return m.next(...args); };
export function pause(...args) { return m.pause(...args); };
export function play(...args) { return m.play(...args); };
export function prev(...args) { return m.prev(...args); };
export function random(...args) { return m.random(...args); };
export function stop(...args) { return m.stop(...args); };
export function volumeDown(...args) { return m.volumeDown(...args); };
export function volumeMute(...args) { return m.volumeMute(...args); };
export function volumeUp(...args) { return m.volumeUp(...args); };

export const { constants } = m;

export default m;

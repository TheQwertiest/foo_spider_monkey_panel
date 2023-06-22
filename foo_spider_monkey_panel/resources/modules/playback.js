let m = _internalModule;

export function getCurrentlyPlayingTrack(...args) { return m.getCurrentlyPlayingTrack(...args); };
export function next(...args) { return m.next(...args); };
export function pause(...args) { return m.pause(...args); };
export function play(...args) { return m.play(...args); };
export function playRandom(...args) { return m.playRandom(...args); };
export function previous(...args) { return m.previous(...args); };
export function stop(...args) { return m.stop(...args); };
export function volumeDown(...args) { return m.volumeDown(...args); };
export function volumeMute(...args) { return m.volumeMute(...args); };
export function volumeUp(...args) { return m.volumeUp(...args); };

export default m;

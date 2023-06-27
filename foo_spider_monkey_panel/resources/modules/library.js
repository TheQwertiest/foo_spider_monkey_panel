let m = _internalModule;

export const LibraryTrackEvent = m.LibraryTrackEvent;

export function contains(...args) { return m.contains(...args); };
export function filterTracks(...args) { return m.filterTracks(...args); };
export function getTracks(...args) { return m.getTracks(...args); };
export function getTracksRelativePath(...args) { return m.getTracksRelativePath(...args); };
export function orderTracksByRelativePath(...args) { return m.orderTracksByRelativePath(...args); };

export default m;

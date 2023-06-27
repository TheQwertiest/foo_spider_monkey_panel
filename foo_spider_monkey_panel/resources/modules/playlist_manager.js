let m = _internalModule;

export const MultiPlaylistEvent = m.MultiPlaylistEvent;
export const PlaylistEvent = m.PlaylistEvent;
export const PlaylistMultiTrackEvent = m.PlaylistMultiTrackEvent;
export const PlaylistTrackEvent = m.PlaylistTrackEvent;

export function createAutoPlaylist(...args) { return m.createAutoPlaylist(...args); };
export function createPlaylist(...args) { return m.createPlaylist(...args); };
export function deletePlaylist(...args) { return m.deletePlaylist(...args); };
export function duplicatePlaylist(...args) { return m.duplicatePlaylist(...args); };
export function findPlaylist(...args) { return m.findPlaylist(...args); };
export function getActivePlaylist(...args) { return m.getActivePlaylist(...args); };
export function getCurrentlyPlayingPlaylist(...args) { return m.getCurrentlyPlayingPlaylist(...args); };
export function getCurrentlyPlayingTrackLocation(...args) { return m.getCurrentlyPlayingTrackLocation(...args); };
export function getPlaylist(...args) { return m.getPlaylist(...args); };
export function getPlaylistCount(...args) { return m.getPlaylistCount(...args); };
export function movePlaylist(...args) { return m.movePlaylist(...args); };
export function orderPlaylistsByName(...args) { return m.orderPlaylistsByName(...args); };
export function setActivePlaylistAsUiEditContext(...args) { return m.setActivePlaylistAsUiEditContext(...args); };

export default m;

_.mixin({
	listenbrainz : function (x, y, size) {
		this.playback_new_track = function (metadb) {
			if (!metadb) {
				return;
			}
			this.metadb = metadb;
			this.time_elapsed = 0;
			this.timestamp = _.ts();
			this.target_time = this.properties.listenbrainz.enabled ? Math.min(Math.ceil(fb.PlaybackLength / 2), 240) : -1;
		}
		
		this.playback_time = function () {
			this.time_elapsed++;
			if (this.time_elapsed == 3) {
				this.listen(this.metadb, 'playing_now');
			} else if (this.time_elapsed == this.target_time) {
				this.listen(this.metadb, 'single');
			}
		}
		
		this.listen = function (metadb, listen_type) {
			if (!metadb || !this.properties.listenbrainz.enabled) {
				return;
			}
			
			var single = listen_type == 'single';
			
			if (!_.isUUID(this.token)) {
				return console.log(N, 'Token invalid/not set.');
			}
			
			if (this.properties.library.enabled && !fb.IsMetadbInMediaLibrary(metadb)) {
				if (single) {
					console.log(N, 'Skipping... Track not in Media Library.');
				}
				return;
			}
			
			var tags = this.get_tags(metadb);
			
			if (!tags.artist || !tags.title) {
				if (single) {
					console.log(N, 'Artist/title tag missing. Not submitting.');
				}
				return;
			}
			
			var payload = {
				track_metadata : {
					artist_name : _.first(tags.artist),
					release_name : _.first(tags.album),
					track_name : _.first(tags.title)
				}
			};
			
			if (single) {
				payload.listened_at = this.timestamp;
				
				payload.track_metadata.additional_info = {
					// must be arrays
					artist_mbids : tags.musicbrainz_artistid,
					work_mbids : tags.musicbrainz_workid,
					// must be strings
					albumartist : _.first(tags.albumartist),
					date : _.first(tags.date),
					discnumber : _.first(tags.discnumber),
					isrc : _.first(tags.isrc),
					recording_mbid : _.first(tags.musicbrainz_trackid),
					release_group_mbid : _.first(tags.musicbrainz_releasegroupid),
					release_mbid : _.first(tags.musicbrainz_albumid),
					totaldiscs : _.first(tags.totaldiscs),
					totaltracks: _.first(tags.totaltracks),
					track_mbid : _.first(tags.musicbrainz_releasetrackid),
					tracknumber : _.first(tags.tracknumber)
				};
				
				if (this.properties.genres.enabled && tags.genre) {
					payload.track_metadata.additional_info.tags = _(tags.genre)
						.take(50)
						.map(function (item) {
							return item.substring(0, 64);
						})
						.value();
				}
				
				console.log(N, 'Submitting', _.q(tags.title), 'by', _.q(tags.artist));
				
				if (this.properties.show.enabled) {
					console.log(JSON.stringify(payload, null, 4));
				}
			}
			
			this.post([payload], listen_type);
		}
		
		this.retry = function () {
			if (this.cache_is_bad) {
				return;
			}
			var payload = _.take(this.open_cache(), this.max_listens);
			this.post(payload, 'import');
		}
		
		this.post = function (payload, listen_type) {
			var data = {
				listen_type : listen_type,
				payload : payload
			}
			this.xmlhttp.open('POST', 'https://api.listenbrainz.org/1/submit-listens', true);
			this.xmlhttp.setRequestHeader('Authorization' , 'Token ' + this.token);
			this.xmlhttp.send(JSON.stringify(data));
			this.xmlhttp.onreadystatechange = _.bind(function () {
				if (this.xmlhttp.readyState == 4) {
					switch (listen_type) {
					case 'playing_now':
						if (this.xmlhttp.responseText) {
							var response = _.jsonParse(this.xmlhttp.responseText);
							if (response.status == 'ok') {
								console.log(N, 'Playing now notification updated OK!');
							}
						}
						break;
					case 'single':
						if (this.xmlhttp.responseText) {
							var response = _.jsonParse(this.xmlhttp.responseText);
							if (response.status == 'ok') {
								console.log(N, 'Listen submitted OK!');
								// now would be a good time to retry any listens in the cache
								if (this.open_cache().length) {
									this.retry();
								}
							} else if (response.code && response.error) {
								console.log(N, 'Error code:', response.code);
								console.log(N, 'Error message:', response.error);
								if (response.code == 400) {
									console.log(N, 'Not caching listen with a 400 response as it is malformed and will get rejected again. See note in main script about reporting errors.');
								} else {
									this.cache(data);
								}
							} else {
								console.log(N, this.xmlhttp.responseText);
								this.cache(data);
							}
						} else {
							console.log(N, 'The server response was empty, status code:', this.xmlhttp.status);
							if (this.xmlhttp.status == 0) {
								console.log(N, 'A possible cause of this may be an invalid authorization token.');
							}
							this.cache(data);
						}
						break;
					case 'import':
						if (this.xmlhttp.responseText) {
							var response = _.jsonParse(this.xmlhttp.responseText);
							if (response.status == 'ok') {
								console.log(N, data.payload.length, 'cached listen(s) submitted OK!');
								_.save(this.cache_file, JSON.stringify(_.drop(this.open_cache(), this.max_listens)));
								if (this.open_cache().length) {
									window.SetTimeout(_.bind(function () {
										this.retry();
									}, this), 1000);
								} else {
									console.log(N, 'Cache is now clear!');
								}
							} else if (response.code == 400) {
								if (response.error) {
									console.log(N, response.error);
								}
								console.log(N, 'Cannot retry submitting cache until bad entry is fixed/removed. See note in main script about reporting errors.');
								this.cache_is_bad = true;
							}
						}
						break;
					}
				}
			}, this);
		}
		
		this.cache = function (data) {
			var tmp = this.open_cache();
			tmp.push(data.payload[0]);
			console.log(N, 'Cache contains', tmp.length, 'listen(s).');
			_.save(this.cache_file, JSON.stringify(tmp));
		}
		
		this.open_cache = function () {
			return _.jsonParseFile(this.cache_file);
		}
		
		this.get_tags = function (metadb) {
			var tmp = {};
			var f = metadb.GetFileInfo();
			for (var i = 0; i < f.MetaCount; i++) {
				var name = f.MetaName(i).toLowerCase();
				var key = this.mapping[name] || name;
				tmp[key] = [];
				for (var j = 0; j < f.MetaValueCount(i); j++) {
					var value = f.MetaValue(i, j);
					if (key.indexOf('musicbrainz') == 0) {
						// if Picard has written multiple MBIDs as a string, use the first one
						value = value.substring(0, 36);
						if (_.isUUID(value)) {
							tmp[key].push(value);
						}
					} else {
						tmp[key].push(value);
					}
				}
			}
			_.dispose(f);
			return tmp;
		}
		
		this.options = function () {
			var flag = _.isUUID(this.token) && this.properties.listenbrainz.enabled ? MF_STRING : MF_GRAYED;
			var m = window.CreatePopupMenu();
			m.AppendMenuItem(MF_STRING, 1, 'Set token...');
			m.AppendMenuSeparator();
			m.AppendMenuItem(MF_STRING, 2, 'Set username...');
			m.AppendMenuItem(this.username.length ? MF_STRING : MF_GRAYED, 3, 'View profile');
			m.AppendMenuSeparator();
			m.AppendMenuItem(MF_STRING, 4, 'Enabled');
			m.CheckMenuItem(4, this.properties.listenbrainz.enabled);
			m.AppendMenuSeparator();
			m.AppendMenuItem(flag, 5, 'Show submission data in Console when sending');
			m.CheckMenuItem(5, this.properties.show.enabled);
			m.AppendMenuItem(flag, 6, 'Submit Media Library tracks only');
			m.CheckMenuItem(6, this.properties.library.enabled);
			m.AppendMenuItem(flag, 7, 'Submit genre tags');
			m.CheckMenuItem(7, this.properties.genres.enabled);
			m.AppendMenuSeparator();
			m.AppendMenuItem(MF_GRAYED, 8, 'Cache contains ' + this.open_cache().length + ' listen(s).');
			var idx = m.TrackPopupMenu(this.x, this.y + this.size);
			switch (idx) {
			case 1:
				var token = _.input('Enter your token\n\nhttps://listenbrainz.org/user/import', window.Name, this.token);
				if (token != this.token) {
					this.token = token;
					utils.WriteINI(this.ini_file, 'Listenbrainz', 'token', this.token);
					this.update_button();
				}
				break;
			case 2:
				var username = _.input('Enter your username.', window.Name, this.username);
				if (username != this.username) {
					this.username = username;
					utils.WriteINI(this.ini_file, 'Listenbrainz', 'username', this.username);
				}
				break;
			case 3:
				_.run('https://listenbrainz.org/user/' + this.username);
				break;
			case 4:
				this.properties.listenbrainz.toggle();
				this.update_button();
				break;
			case 5:
				this.properties.show.toggle();
				break;
			case 6:
				this.properties.library.toggle();
				break;
			case 7:
				this.properties.genres.toggle();
				break;
			}
			m.Dispose();
		}
		
		this.update_button = function () {
			buttons.buttons.listenbrainz = new _.button(this.x, this.y, this.size, this.size, {normal : this.properties.listenbrainz.enabled && _.isUUID(this.token) ? 'misc\\listenbrainz_active.png' : 'misc\\listenbrainz_inactive.png'}, _.bind(function () { this.options(); }, this), 'Listenbrainz Options');
			window.RepaintRect(this.x, this.y, this.size, this.size);
		}
		
		_.createFolder(folders.data);
		this.x = x;
		this.y = y;
		this.size = size;
		this.cache_is_bad = false;
		this.cache_file = folders.data + 'listenbrainz.json';
		this.ini_file = folders.data + 'listenbrainz.ini';
		this.token = utils.ReadINI(this.ini_file, 'Listenbrainz', 'token');
		this.username = utils.ReadINI(this.ini_file, 'Listenbrainz', 'username');
		this.xmlhttp = new ActiveXObject('Microsoft.XMLHTTP');
		this.time_elapsed = 0;
		this.target_time = 0;
		this.timestamp = 0;
		this.max_listens = 50;
		this.mapping = {
			'acoustid id' : 'acoustid_id',
			'acoustid fingerprint' : 'acoustid_fingerprint',
			'album artist' : 'albumartist',
			'albumartistsortorder' : 'albumartistsort',
			'albumsortorder' : 'albumsort',
			'artistsortorder' : 'artistsort',
			'artist webpage url' : 'website',
			'composersortorder' : 'composersort',
			'content group' : 'grouping',
			'copyright url' : 'license',
			'encoded by' : 'encodedby',
			'encoding settings' : 'encodersettings',
			'initial key' : 'initialkey',
			'itunescompilation' : 'compilation',
			'musicbrainz album artist id' : 'musicbrainz_albumartistid',
			'musicbrainz album id' : 'musicbrainz_albumid',
			'musicbrainz album release country' : 'releasecountry',
			'musicbrainz album status' : 'releasestatus',
			'musicbrainz album type' : 'releasetype',
			'musicbrainz artist id' : 'musicbrainz_artistid',
			'musicbrainz disc id' : 'musicbrainz_discid',
			'musicbrainz release group id' : 'musicbrainz_releasegroupid',
			'musicbrainz release track id' : 'musicbrainz_releasetrackid',
			'musicbrainz track id' : 'musicbrainz_trackid',
			'musicbrainz trm id' : 'musicbrainz_trmid',
			'musicbrainz work id' : 'musicbrainz_workid',
			'musicip puid' : 'musicip_puid',
			'titlesortorder' : 'titlesort'
		};
		this.properties = {
			listenbrainz : new _.p('2K3.LISTENBRAINZ.ENABLED', true),
			library : new _.p('2K3.LISTENBRAINZ.IN.LIBRARY', false),
			show : new _.p('2K3.LISTENBRAINZ.SHOW.DATA', false),
			genres : new _.p('2K3.LISTENBRAINZ.SUBMIT.GENRES', true)
		};
		this.update_button();
	}
});

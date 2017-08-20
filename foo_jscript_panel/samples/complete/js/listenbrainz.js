_.mixin({
	listenbrainz : function (x, y, size) {
		this.playback_new_track = function () {
			this.metadb = fb.GetNowPlaying();
			this.time_elapsed = 0;
			this.timestamp = Math.floor(_.now() / 1000);
			this.target_time = Math.min(Math.ceil(fb.PlaybackLength / 2), 240); //half the track length or 4 minutes, whichever is lower - same as Last.fm
		}
		
		this.playback_time = function () {
			this.time_elapsed++;
			if (this.time_elapsed == this.target_time)
				this.listen();
		}
		
		this.listen = function () {
			if (this.token.length != 36)
				return console.log('Token not set.');
			
			if (this.in_library && !fb.IsMetadbInMediaLibrary(this.metadb))
				return console.log('Skipping... Track not in Media Library.');
			
			var tags = this.get_tags(this.metadb);
			
			if (!tags.artist || !tags.title)
				return console.log('Artist/title tag missing. Not submitting.');
			
			var data = {
				listen_type : 'single',
				payload : [
					{
						listened_at : this.timestamp,
						track_metadata : {
							additional_info : {
								albumartist : tags.albumartist,
								artist_mbids : _.isString(tags.musicbrainz_artistid) ? [tags.musicbrainz_artistid] : tags.musicbrainz_artistid,
								date : tags.date,
								discnumber : tags.discnumber,
								recording_mbid : tags.musicbrainz_trackid,
								release_group_mbid : tags.musicbrainz_releasegroupid,
								release_mbid : tags.musicbrainz_albumid,
								tags : _.isString(tags.genre) ? [tags.genre] : tags.genre,
								totaldiscs : tags.totaldiscs,
								totaltracks: tags.totaltracks,
								track_mbid : tags.musicbrainz_releasetrackid,
								tracknumber : tags.tracknumber,
								work_mbids : _.isString(tags.musicbrainz_workid) ? [tags.musicbrainz_workid] : tags.musicbrainz_workid
							},
							artist_name : _.isString(tags.artist) ? tags.artist : tags.artist[0],
							release_name : tags.album,
							track_name : tags.title
						}
					}
				]
			};
			
			console.log('Submitting ' + _.q(tags.artist + ' - ' + tags.title));
			
			if (this.show_data)
				fb.Trace(JSON.stringify(data, null, '    '));
			
			this.post(data);
		}
		
		this.post = function (data) {
			this.xmlhttp.open('POST', 'https://api.listenbrainz.org/1/submit-listens', true);
			this.xmlhttp.setRequestHeader('Authorization' , 'Token ' + this.token);
			this.xmlhttp.send(JSON.stringify(data));
			this.xmlhttp.onreadystatechange = _.bind(function () {
				if (this.xmlhttp.readyState == 4) {
					if (this.xmlhttp.status == 0) {
						console.log('An unknown error occurred. A possible cause may be an invalid authorization token.');
					} else if (this.xmlhttp.responseText) {
						var data = _.jsonParse(this.xmlhttp.responseText);
						if (data.status == 'ok')
							console.log('Listen submitted OK!');
						else
							console.log(this.xmlhttp.responseText);
					} else {
						console.log('The server response was empty, status code: ' + this.xmlhttp.status);
					}
				}
			}, this);
		}
		
		this.get_tags = function (metadb) {
			var tmp = {};
			var f = metadb.GetFileInfo();
			for (var i = 0; i < f.MetaCount; i++) {
				var name = f.MetaName(i).toLowerCase();
				if (name == 'genre' && !this.submit_genres)
					continue;
				
				var key = this.mb_names[name] || name;
				
				var num = f.MetaValueCount(i);
				if (num == 1) {
					tmp[key] = f.MetaValue(i, 0);
				} else {
					tmp[key] = [];
					for (var j = 0; j < num; j++) {
						tmp[key].push(f.MetaValue(i, j));
					}
				}
			}
			_.dispose(f);
			return tmp;
		}
		
		this.options = function () {
			var flag = this.token.length == 36 ? MF_STRING : MF_GRAYED;
			var m = window.CreatePopupMenu();
			m.AppendMenuItem(MF_STRING, 1, 'Set token...');
			m.AppendMenuSeparator();
			m.AppendMenuItem(MF_STRING, 2, 'Set username...');
			m.AppendMenuItem(this.username.length ? MF_STRING : MF_GRAYED, 3, 'View profile');
			m.AppendMenuSeparator();
			m.AppendMenuItem(flag, 4, 'Show submission data in Console when sending');
			m.CheckMenuItem(4, this.show_data);
			m.AppendMenuItem(flag, 5, 'Submit Media Library tracks only');
			m.CheckMenuItem(5, this.in_library);
			m.AppendMenuItem(flag, 6, 'Submit genre tags');
			m.CheckMenuItem(6, this.submit_genres);
			var idx = m.TrackPopupMenu(this.x, this.y + this.size);
			switch (idx) {
			case 1:
				var token = _.input('Enter your token\n\nhttps://listenbrainz.org/user/import', panel.name, this.token);
				if (token != this.token) {
					this.token = token;
					utils.WriteINI(this.ini_file, 'Listenbrainz', 'token', this.token);
					this.update_button();
				}
				break;
			case 2:
				var username = _.input('Enter your username.', panel.name, this.username);
				if (username != this.username) {
					this.username = username;
					utils.WriteINI(this.ini_file, 'Listenbrainz', 'username', this.username);
				}
				break;
			case 3:
				_.run('https://listenbrainz.org/user/' + this.username);
				break;
			case 4:
				this.show_data = !this.show_data;
				window.SetProperty('2K3.LISTENBRAINZ.SHOW.DATA', this.show_data);
				break;
			case 5:
				this.in_library = !this.in_library;
				window.SetProperty('2K3.LISTENBRAINZ.IN.LIBRARY', this.in_library);
				break;
			case 6:
				this.submit_genres = !this.submit_genres;
				window.SetProperty('2K3.LISTENBRAINZ.SUBMIT.GENRES', this.submit_genres);
				break;
			}
			m.Dispose();
		}
		
		this.update_button = function () {
			buttons.buttons.listenbrainz = new _.button(this.x, this.y, this.size, this.size, {normal : this.token.length == 36 ? 'misc\\listenbrainz_active.png' : 'misc\\listenbrainz_inactive.png'}, _.bind(function () { this.options(); }, this), 'Listenbrainz Options');
			window.RepaintRect(this.x, this.y, this.size, this.size);
		}
		
		_.createFolder(folders.settings);
		this.x = x;
		this.y = y;
		this.size = size;
		this.ini_file = folders.settings + 'listenbrainz.ini';
		this.token = utils.ReadINI(this.ini_file, 'Listenbrainz', 'token');
		this.username = utils.ReadINI(this.ini_file, 'Listenbrainz', 'username');
		this.show_data = window.GetProperty('2K3.LISTENBRAINZ.SHOW.DATA', false);
		this.in_library = window.GetProperty('2K3.LISTENBRAINZ.IN.LIBRARY', false);
		this.submit_genres = window.GetProperty('2K3.LISTENBRAINZ.SUBMIT.GENRES', true);
		this.xmlhttp = new ActiveXObject('Microsoft.XMLHTTP');
		this.time_elapsed = 0;
		this.target_time = 0;
		this.timestamp = 0;
		this.mb_names = {
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
		this.update_button();
	}
});

_.mixin({
	list : function (mode, x, y, w, h) {
		this.size = function () {
			this.index = 0;
			this.offset = 0;
			this.rows = _.floor((this.h - 32) / panel.row_height);
			this.up_btn.x = this.x + _.round((this.w - 16) / 2);
			this.down_btn.x = this.up_btn.x;
			this.up_btn.y = this.y;
			this.down_btn.y = this.y + this.h - 16;
		}
		
		this.paint = function (gr) {
			if (this.items == 0)
				return;
			switch (this.mode) {
			case "autoplaylists":
				gr.SetTextRenderingHint(4);
				this.text_width = this.w - 30;
				for (var i = 0; i < Math.min(this.items, this.rows); i++) {
					gr.GdiDrawText(this.data[i + this.offset].name, panel.fonts.normal, panel.colours.text, this.x, this.y + 16 + (i * panel.row_height), this.text_width, panel.row_height, LEFT);
					if (!this.editing && this.hover && this.index == i + this.offset)
						gr.DrawString(guifx.drop, this.guifx_font, panel.colours.highlight, this.x + this.w - 20, this.y + 18 + (i * panel.row_height), panel.row_height, panel.row_height, SF_CENTRE);
				}
				break;
			case "lastfm_info":
				switch (this.lastfm_mode) {
				case 0:
					this.text_x = 0;
					this.text_width = this.w;
					for (var i = 0; i < Math.min(this.items, this.rows); i++) {
						gr.GdiDrawText(this.data[i + this.offset].name, panel.fonts.normal, panel.colours.text, this.x, this.y + 16 + (i * panel.row_height), this.text_width, panel.row_height, LEFT);
					}
					break;
				case 1:
					this.text_x = this.spacer_w + 5;
					this.text_width = _.round(this.w / 2) + 30;
					var lastfm_charts_bar_x = this.x + this.text_x + this.text_width + 10;
					var unit_width = (this.w - lastfm_charts_bar_x - 40) / this.data[0].playcount;
					var bar_colour = _.splitRGB(this.lastfm_charts_colour);
					for (var i = 0; i < Math.min(this.items, this.rows); i++) {
						var bar_width = _.ceil(unit_width * this.data[i + this.offset].playcount);
						gr.GdiDrawText(this.data[i + this.offset].rank + ".", panel.fonts.normal, panel.colours.highlight, this.x, this.y + 16 + (i * panel.row_height), this.text_x - 5, panel.row_height, RIGHT);
						gr.GdiDrawText(this.data[i + this.offset].name, panel.fonts.normal, panel.colours.text, this.x + this.text_x, this.y + 16 + (i * panel.row_height), this.text_width, panel.row_height, LEFT);
						gr.FillSolidRect(lastfm_charts_bar_x, this.y + 18 + (i * panel.row_height), bar_width, panel.row_height - 3, bar_colour);
						gr.GdiDrawText(_.formatNumber(this.data[i + this.offset].playcount, ","), panel.fonts.normal, panel.colours.text, lastfm_charts_bar_x + bar_width + 5, this.y + 16 + (i * panel.row_height), 60, panel.row_height, LEFT);
					}
					break;
				}
				break;
			case "musicbrainz":
				switch (this.mb_mode) {
				case 0:
					this.text_x = 0;
					this.text_width = this.w - this.spacer_w - 10;
					for (var i = 0; i < Math.min(this.items, this.rows); i++) {
						gr.GdiDrawText(this.data[i + this.offset].name, panel.fonts.normal, this.data[i + this.offset].width == 0 ? panel.colours.highlight : panel.colours.text, this.x + this.text_x, this.y + 16 + (i * panel.row_height), this.text_width, panel.row_height, LEFT);
						gr.GdiDrawText(this.data[i + this.offset].date, panel.fonts.normal, panel.colours.highlight, this.x, this.y + 16 + (i * panel.row_height), this.w, panel.row_height, RIGHT);
					}
					break;
				case 1:
					this.text_x = this.mb_icons ? 20 : 0;
					this.text_width = this.w - this.text_x;
					for (var i = 0; i < Math.min(this.items, this.rows); i++) {
						var y = this.y + 16 + (i * panel.row_height);
						if (this.mb_icons)
							_.drawImage(gr, this.mb_images[this.data[i + this.offset].image], this.x, y + _.round((panel.row_height - 16) / 2), 16, 16);
						gr.GdiDrawText(this.data[i + this.offset].name, panel.fonts.normal, panel.colours.text, this.x + this.text_x, y, this.text_width, panel.row_height, LEFT);
					}
					break;
				}
				break;
			case "properties":
				this.text_width = this.w - this.text_x;
				for (var i = 0; i < Math.min(this.items, this.rows); i++) {
					gr.GdiDrawText(this.data[i + this.offset].name, panel.fonts.normal, panel.colours.highlight, this.x, this.y + 16 + (i * panel.row_height), this.text_x - 10, panel.row_height, LEFT);
					gr.GdiDrawText(this.data[i + this.offset].value, panel.fonts.normal, panel.colours.text, this.x + this.text_x, this.y + 16 + (i * panel.row_height), this.text_width, panel.row_height, LEFT);
				}
				break;
			default:
				this.text_width = this.w;
				for (var i = 0; i < Math.min(this.items, this.rows); i++) {
					gr.GdiDrawText(this.data[i + this.offset].name, panel.fonts.normal, panel.colours.text, this.x, this.y + 16 + (i * panel.row_height), this.text_width, panel.row_height, LEFT);
				}
				break;
			}
			this.up_btn.paint(gr, panel.colours.text);
			this.down_btn.paint(gr, panel.colours.text);
		}
		
		this.metadb_changed = function () {
			switch (true) {
			case this.mode == "autoplaylists":
			case this.mode == "lastfm_info" && this.lastfm_mode == 1:
				break;
			case !panel.metadb:
				this.artist = "";
				this.data = [];
				this.items = 0;
				window.Repaint();
				break;
			case this.mode == "properties":
				this.update();
				break;
			case this.mode == "musicbrainz":
				var temp_artist = panel.tf(DEFAULT_ARTIST);
				var temp_id = panel.tf("$if3($meta(musicbrainz_artistid,0),$meta(musicbrainz artist id,0),)");
				if (this.artist == temp_artist && this.mb_id == temp_id)
					return;
				this.artist = temp_artist;
				this.mb_id = temp_id;
				this.update();
				break;
			default:
				var temp_artist = panel.tf(DEFAULT_ARTIST);
				if (this.artist == temp_artist)
					return;
				this.artist = temp_artist;
				this.update();
				break;
			}
		}
		
		this.trace = function (x, y) {
			return x > this.x && x < this.x + this.w && y > this.y && y < this.y + this.h;
		}
		
		this.wheel = function (s) {
			if (this.trace(this.mx, this.my)) {
				if (this.items > this.rows) {
					var offset = this.offset - (s * 3);
					if (offset < 0)
						offset = 0;
					if (offset + this.rows > this.items)
						offset = this.items - this.rows;
					if (this.offset != offset) {
						this.offset = offset;
						window.RepaintRect(this.x, this.y, this.w, this.h);
					}
				}
				return true;
			} else {
				return false;
			}
		}
		
		this.move = function (x, y) {
			this.mx = x;
			this.my = y;
			this.index = _.floor((y - this.y - 16) / panel.row_height) + this.offset;
			this.in_range = this.index >= this.offset && this.index < this.offset + Math.min(this.rows, this.items);
			switch (true) {
			case !this.trace(x, y):
			case this.mode == "autoplaylists" && this.editing:
				window.SetCursor(IDC_ARROW);
				this.leave();
				return false;
			case this.up_btn.move(x, y):
			case this.down_btn.move(x, y):
				break;
			case !this.in_range:
				window.SetCursor(IDC_ARROW);
				this.leave();
				break;
			case this.mode == "autoplaylists":
				switch (true) {
				case x > this.x && x < this.x + Math.min(this.data[this.index].width, this.text_width):
					window.SetCursor(IDC_HAND);
					_.tt("Autoplaylist: " + this.data[this.index].name);
					break;
				case x > this.x + this.w - 20 && x < this.x + this.w:
					window.SetCursor(IDC_HAND);
					_.tt("Edit " + _.q(this.data[this.index].name));
					break;
				default:
					window.SetCursor(IDC_ARROW);
					_.tt("");
					this.leave();
					break;
				}
				this.hover = true;
				window.RepaintRect(this.x + this.w - 20, this.y, 20, this.h);
				break;
			case x > this.x + this.text_x && x < this.x + this.text_x + Math.min(this.data[this.index].width, this.text_width):
				window.SetCursor(IDC_HAND);
				if (this.data[this.index].url.indexOf("http") == 0)
					_.tt(this.data[this.index].url);
				else
					_.tt("Autoplaylist: " + this.data[this.index].url);
				break;
			default:
				window.SetCursor(IDC_ARROW);
				_.tt("");
				break;
			}
			return true;
		}
		
		this.leave = function () {
			if (this.mode == "autoplaylists" && this.hover) {
				this.hover = false;
				window.RepaintRect(this.x + this.w - 20, this.y, 20, this.h);
			}
		}
		
		this.lbtn_up = function (x, y) {
			switch (true) {
			case !this.trace(x, y):
				return false;
			case this.mode == "autoplaylists" && this.editing:
			case this.up_btn.lbtn_up(x, y):
			case this.down_btn.lbtn_up(x, y):
			case !this.in_range:
				break;
			case this.mode == "autoplaylists":
				switch (true) {
				case x > this.x && x < this.x + Math.min(this.data[this.index].width, this.text_width):
					this.run_query(this.data[this.index].name, this.data[this.index].query, this.data[this.index].sort, this.data[this.index].forced);
					break;
				case x > this.x + this.w - 20 && x < this.x + this.w:
					this.edit(x, y);
					break;
				}
				break;
			case x > this.x + this.text_x && x < this.x + this.text_x + Math.min(this.data[this.index].width, this.text_width):
				if (this.data[this.index].url.indexOf("http") == 0) {
					_.browser(this.data[this.index].url);
				} else {
					plman.CreateAutoPlaylist(plman.PlaylistCount, this.data[this.index].name, this.data[this.index].url);
					plman.ActivePlaylist = plman.PlaylistCount - 1;
				}
				break;
			}
			return true;
		}
		
		this.rbtn_up = function (x, y) {
			switch (this.mode) {
			case "autoplaylists":
				panel.m.AppendMenuItem(this.editing ? MF_GRAYED: MF_STRING, 3000, "Add new autoplaylist...");
				panel.m.AppendMenuSeparator();
				if (this.deleted_items.length) {
					_(this.deleted_items)
						.take(8)
						.forEach(function (item, i) {
							panel.s10.AppendMenuItem(MF_STRING, i + 3010, item.name);
						})
						.value();
					panel.s10.AppendTo(panel.m, MF_STRING, "Restore");
					panel.m.AppendMenuSeparator();
				}
				break;
			case "lastfm_info":
				panel.m.AppendMenuItem(MF_STRING, 3100, "Similar artists");
				panel.m.AppendMenuItem(MF_STRING, 3101, "User Charts");
				panel.m.CheckMenuRadioItem(3100, 3101, this.lastfm_mode + 3100);
				panel.m.AppendMenuSeparator();
				if (this.lastfm_mode == 1) {
					_.forEach(this.lastfm_charts_methods, function (item, i) {
						panel.m.AppendMenuItem(MF_STRING, i + 3120, _.capitalize(item.display));
					});
					panel.m.CheckMenuRadioItem(3120, 3122, this.lastfm_charts_method + 3120);
					panel.m.AppendMenuSeparator();
					_.forEach(this.lastfm_charts_periods, function (item, i) {
						panel.m.AppendMenuItem(MF_STRING, i + 3130, _.capitalize(item.display));
					});
					panel.m.CheckMenuRadioItem(3130, 3135, this.lastfm_charts_period + 3130);
					panel.m.AppendMenuSeparator();
					panel.m.AppendMenuItem(MF_STRING, 3140, "Bar colour...");
					panel.m.AppendMenuSeparator();
				}
				panel.m.AppendMenuItem(lastfm.api_key.length == 32 ? MF_STRING : MF_GRAYED, 3150, "Last.fm username...");
				panel.m.AppendMenuSeparator();
				break;
			case "musicbrainz":
				panel.m.AppendMenuItem(MF_STRING, 3200, "Releases");
				panel.m.AppendMenuItem(MF_STRING, 3201, "Links");
				panel.m.CheckMenuRadioItem(3200, 3201, this.mb_mode + 3200);
				panel.m.AppendMenuSeparator();
				if (this.mb_id.length != 36) {
					panel.m.AppendMenuItem(MF_GRAYED, 3203, "Artist MBID missing. Use Musicbrainz Picard or foo_musicbrainz to tag your files.");
					panel.m.AppendMenuSeparator();
				}
				if (this.mb_mode == 1) {
					panel.m.AppendMenuItem(MF_STRING, 3210, "Show icons");
					panel.m.CheckMenuItem(3210, this.mb_icons);
					panel.m.AppendMenuSeparator();
				}
				break;
			case "properties":
				panel.m.AppendMenuItem(MF_STRING, 3300, "Metadata");
				panel.m.CheckMenuItem(3300, this.properties.meta);
				panel.m.AppendMenuItem(MF_STRING, 3301, "Location");
				panel.m.CheckMenuItem(3301, this.properties.location);
				panel.m.AppendMenuItem(MF_STRING, 3302, "Tech Info");
				panel.m.CheckMenuItem(3302, this.properties.tech);
				panel.m.AppendMenuItem(_.cc("foo_customdb") ? MF_STRING : MF_GRAYED, 3303, "Last.fm Playcount (foo_customdb)");
				panel.m.CheckMenuItem(3303, this.properties.customdb);
				panel.m.AppendMenuItem(_.cc("foo_playcount") ? MF_STRING : MF_GRAYED, 3304, "Playback Statistics (foo_playcount)");
				panel.m.CheckMenuItem(3304, this.properties.playcount);
				panel.m.AppendMenuItem(MF_STRING, 3305, "Replaygain");
				panel.m.CheckMenuItem(3305, this.properties.rg);
				panel.m.AppendMenuSeparator();
				break;
			}
			panel.m.AppendMenuItem(_.isFile(this.filename) ? MF_STRING : MF_GRAYED, 3999, "Open containing folder");
			panel.m.AppendMenuSeparator();
		}
		
		this.rbtn_up_done = function (idx) {
			switch (idx) {
			case 3000:
				this.add();
				break;
			case 3010:
			case 3011:
			case 3012:
			case 3013:
			case 3014:
			case 3015:
			case 3016:
			case 3017:
				this.data.push(this.deleted_items[idx - 3010]);
				this.deleted_items.splice(idx - 3010, 1);
				this.save();
				break;
			case 3100:
			case 3101:
				this.lastfm_mode = idx - 3100;
				window.SetProperty("2K3.LIST.LASTFM.MODE", this.lastfm_mode);
				if (this.lastfm_mode == 0)
					this.reset();
				else
					this.update();
				break;
			case 3120:
			case 3121:
			case 3122:
				this.lastfm_charts_method = idx - 3120;
				window.SetProperty("2K3.LIST.LASTFM.CHARTS.METHOD", this.lastfm_charts_method);
				this.update();
				break;
			case 3130:
			case 3131:
			case 3132:
			case 3133:
			case 3134:
			case 3135:
				this.lastfm_charts_period = idx - 3130;
				window.SetProperty("2K3.LIST.LASTFM.CHARTS.PERIOD", this.lastfm_charts_period);
				this.update();
				break;
			case 3140:
				this.lastfm_charts_colour = _.input("Enter a custom colour for the bars. Uses RGB. Example usage:\n\n72-127-221", panel.name, this.lastfm_charts_colour);
				window.SetProperty("2K3.LIST.LASTFM.CHARTS.COLOUR", this.lastfm_charts_colour);
				window.Repaint();
				break;
			case 3150:
				lastfm.update_username();
				break;
			case 3151:
				lastfm.update_password();
				break;
			case 3200:
			case 3201:
				this.mb_mode = idx - 3200;
				window.SetProperty("2K3.LIST.MUSICBRAINZ.MODE", this.mb_mode);
				this.reset();
				break;
			case 3210:
				this.mb_icons = !this.mb_icons;
				window.SetProperty("2K3.LIST.MUSICBRAINZ.SHOW.ICONS", this.mb_icons);
				window.RepaintRect(this.x, this.y, this.w, this.h);
				break;
			case 3300:
				this.properties.meta = !this.properties.meta;
				window.SetProperty("2K3.LIST.PROPERTIES.META", this.properties.meta);
				panel.item_focus_change();
				break;
			case 3301:
				this.properties.location = !this.properties.location;
				window.SetProperty("2K3.LIST.PROPERTIES.LOCATION", this.properties.location);
				panel.item_focus_change();
				break;
			case 3302:
				this.properties.tech = !this.properties.tech;
				window.SetProperty("2K3.LIST.PROPERTIES.TECH", this.properties.tech);
				panel.item_focus_change();
				break;
			case 3303:
				this.properties.customdb = !this.properties.customdb;
				window.SetProperty("2K3.LIST.PROPERTIES.CUSTOMDB", this.properties.customdb);
				panel.item_focus_change();
				break;
			case 3304:
				this.properties.playcount = !this.properties.playcount;
				window.SetProperty("2K3.LIST.PROPERTIES.PLAYCOUNT", this.properties.playcount);
				panel.item_focus_change();
				break;
			case 3305:
				this.properties.rg = !this.properties.rg;
				window.SetProperty("2K3.LIST.PROPERTIES.RG", this.properties.rg);
				panel.item_focus_change();
				break;
			case 3999:
				_.explorer(this.filename);
				break;
			}
		}
		
		this.key_down = function (k) {
			switch (k) {
			case VK_UP:
				this.wheel(1);
				return true;
			case VK_DOWN:
				this.wheel(-1);
				return true;
			default:
				return false;
			}
		}
		
		this.update = function () {
			this.data = [];
			this.spacer_w = _.textWidth("0000", panel.fonts.normal);
			switch (this.mode) {
			case "autoplaylists":
				this.data = _(_.jsonParse(_.open(this.filename)))
					.forEach(function (item) {
						item.width = _.textWidth(item.name, panel.fonts.normal);
					})
					.value();
				break;
			case "lastfm_info":
				this.filename = "";
				if (lastfm.api_key.length != 32) {
					console.log("Last.fm API KEY not set.");
					break;
				}
				if (this.lastfm_mode == 1 && !lastfm.username.length) {
					console.log("Last.fm Username not set.");
					break;
				}
				switch (this.lastfm_mode) {
				case 0:
					this.filename = _.artistFolder(this.artist) + "lastfm.artist.getSimilar.json";
					if (_.isFile(this.filename)) {
						this.data = _(_.get(_.jsonParse(_.open(this.filename)), "similarartists.artist", []))
							.map(function (item) {
								return {
									name : item.name,
									width : _.textWidth(item.name, panel.fonts.normal),
									url : item.url
								};
							})
							.value();
						if (_.fileExpired(this.filename, ONE_DAY))
							this.get();
					} else {
						this.get();
					}
					break;
				case 1:
					this.filename = folders.lastfm + lastfm.username + "." + this.lastfm_charts_methods[this.lastfm_charts_method].method + "." + this.lastfm_charts_periods[this.lastfm_charts_period].period + ".json";
					if (_.isFile(this.filename)) {
						var data = _.get(_.jsonParse(_.open(this.filename)), this.lastfm_charts_methods[this.lastfm_charts_method].json, []);
						for (var i = 0; i < data.length; i++) {
							var name = this.lastfm_charts_method == 0 ? data[i].name : data[i].artist.name + " - " + data[i].name;
							this.data[i] = {
								name : name,
								width : _.textWidth(name, panel.fonts.normal),
								url : data[i].url,
								playcount : data[i].playcount,
								rank : i > 0 && data[i].playcount == data[i - 1].playcount ? this.data[i - 1].rank : i + 1
							};
						}
						if (_.fileExpired(this.filename, ONE_DAY))
							this.get();
					} else {
						this.get();
					}
					break;
				}
				break;
			case "musicbrainz":
				if (this.mb_mode == 0) {
					this.mb_data = [];
					this.mb_offset = 0;
					this.attempt = 1;
					this.filename = _.artistFolder(this.artist) + "musicbrainz.releases." + this.mb_id + ".json";
					if (_.isFile(this.filename)) {
						var data = _(_.jsonParse(_.open(this.filename)))
							.sortByOrder(["first-release-date", "title"], ["desc", "asc"])
							.map(function (item) {
								return {
									name : item.title,
									width : _.textWidth(item.title, panel.fonts.normal),
									url : "https://musicbrainz.org/release-group/" + item.id,
									date : item["first-release-date"].substring(0, 4),
									primary : item["primary-type"],
									secondary : item["secondary-types"].sort()[0] || null
								};
							})
							.nest(["primary", "secondary"])
							.value();
						_.forEach(["Album", "Single", "EP", "Other", "Broadcast", "null"], function (primary) {
							_.forEach(["null", "Audiobook", "Compilation", "Demo", "DJ-mix", "Interview", "Live", "Mixtape/Street", "Remix", "Spokenword", "Soundtrack"], function (secondary) {
								var group = _.get(data, primary + "." + secondary);
								if (group) {
									var name = (primary + " + " + secondary).replace("null + null", "Unspecified type").replace("null + ", "").replace(" + null", "");
									this.data.push({name : name, width : 0, url : "", date : ""});
									this.data.push.apply(this.data, group);
									this.data.push({name : "", width : 0, url : "", date : ""});
								}
							}, this);
						}, this);
						this.data.pop();
						if (_.fileExpired(this.filename, ONE_DAY))
							this.get();
					} else {
						this.get();
					}
				} else {
					this.filename = _.artistFolder(this.artist) + "musicbrainz.links." + this.mb_id + ".json";
					if (_.isFile(this.filename)) {
						var url = "https://musicbrainz.org/artist/" + this.mb_id;
						this.data = _(_.get(_.jsonParse(_.open(this.filename)), "relations", []))
							.map(this.mb_parse_urls)
							.sortBy(function (item) {
								return item.name.split("//")[1].replace("www.", "");
							})
							.value();
						this.data.unshift({
							name : url,
							url : url,
							width : _.textWidth(url, panel.fonts.normal),
							image : "musicbrainz"
						});
						if (_.fileExpired(this.filename, ONE_DAY))
							this.get();
					} else {
						this.get();
					}
				}
				break;
			case "properties":
				this.text_x = 0;
				this.filename = panel.metadb.Path;
				var fileinfo = panel.metadb.GetFileInfo();
				if (this.properties.meta)
					this.add_meta(fileinfo);
				if (this.properties.location)
					this.add_location();
				if (this.properties.tech)
					this.add_tech(fileinfo);
				if (_.cc("foo_customdb") && this.properties.customdb)
					this.add_customdb();
				if (_.cc("foo_playcount") && this.properties.playcount)
					this.add_playcount();
				if (this.properties.rg)
					this.add_rg();
				this.data.pop();
				_.forEach(this.data, function (item) {
					item.width = _.textWidth(item.value, panel.fonts.normal);
					this.text_x = Math.max(this.text_x, _.textWidth(item.name, panel.fonts.normal) + 20);
				}, this);
				_.dispose(fileinfo);
				break;
			}
			this.items = this.data.length;
			this.offset = 0;
			this.index = 0;
			window.Repaint();
		}
		
		this.get = function () {
			var f = this.filename;
			switch (this.mode) {
			case "lastfm_info":
				switch (this.lastfm_mode) {
				case 0:
					if (!_.tagged(this.artist))
						return;
					var url = lastfm.get_base_url() + "&limit=100&method=artist.getSimilar&artist=" + encodeURIComponent(this.artist);
					break;
				case 1:
					var url = lastfm.get_base_url() + "&limit=100&method=" + this.lastfm_charts_methods[this.lastfm_charts_method].method + "&period=" + this.lastfm_charts_periods[this.lastfm_charts_period].period + "&user=" + lastfm.username;
					break;
				}
				break;
			case "musicbrainz":
				if (this.mb_id.length != 36)
					return console.log("Invalid/missing MBID");
				if (this.mb_mode == 0)
					var url = "https://beta.musicbrainz.org/ws/2/release-group?fmt=json&limit=100&offset=" + this.mb_offset + "&artist=" + this.mb_id;
				else
					var url = "https://beta.musicbrainz.org/ws/2/artist/" + this.mb_id + "?fmt=json&inc=url-rels";
				break;
			default:
				return;
			}
			this.xmlhttp.open("GET", url, true);
			this.xmlhttp.setRequestHeader("User-Agent", this.ua);
			this.xmlhttp.setRequestHeader("If-Modified-Since", "Thu, 01 Jan 1970 00:00:00 GMT");
			this.xmlhttp.send();
			this.xmlhttp.onreadystatechange = _.bind(function () {
				if (this.xmlhttp.readyState == 4) {
					switch (true) {
					case this.xmlhttp.status == 200:
						this.success(f);
						break;
					case this.xmlhttp.status == 404 && this.mode == "lastfm_info" && this.lastfm_mode == 1:
						console.log("Username not found.");
						break;
					case this.xmlhttp.status == 503 && this.mode == "musicbrainz" && this.attempt < 5:
						window.SetTimeout(this.mb_retry, 1500);
						break;
					default:
						console.log("HTTP error: " + this.xmlhttp.status);
						this.xmlhttp.responsetext && fb.trace(this.xmlhttp.responsetext);
						break;
					}
				}
			}, this);
		}
		
		this.success = function (f) {
			switch (true) {
			case this.mode == "musicbrainz" && this.mb_mode == 0: // releases
				var data = _.jsonParse(this.xmlhttp.responsetext);
				var max_offset = Math.min(500, data["release-group-count"] || 0) - 100;
				var rg = data["release-groups"] || [];
				if (rg.length)
					this.mb_data.push.apply(this.mb_data, rg);
				if (this.mb_offset < max_offset) {
					this.mb_offset += 100;
					this.get();
				} else {
					_.save(JSON.stringify(this.mb_data), f);
					this.reset();
				}
				break;
			case this.mode == "musicbrainz": // links
				_.save(this.xmlhttp.responsetext, f);
				this.reset();
				break;
			case this.mode == "lastfm_info":
				var data = _.jsonParse(this.xmlhttp.responsetext);
				if (data.error)
					return console.log(data.message);
				_.save(JSON.stringify(data), f);
				if (this.lastfm_mode == 0)
					this.reset();
				else
					this.update();
				break;
			}
		}
		
		this.header_text = function () {
			switch (this.mode) {
			case "autoplaylists":
				return "Autoplaylists";
			case "lastfm_info":
				switch (this.lastfm_mode) {
				case 0:
					return this.artist + ": similar artists";
				case 1:
					return lastfm.username + ": " + this.lastfm_charts_periods[this.lastfm_charts_period].display + " " + this.lastfm_charts_methods[this.lastfm_charts_method].display + " charts";
				}
			case "musicbrainz":
				return this.artist + ": " + (this.mb_mode == 0 ? "releases" : "links");
			case "properties":
				return panel.tf("%artist% - %title%");
			}
		}
		
		this.reset = function () {
			this.items = 0;
			this.data = [];
			this.artist = "";
			panel.item_focus_change();
		}
		
		this.init = function () {
			switch (this.mode) {
			case "autoplaylists":
				this.save = function () {
					_.save(JSON.stringify(this.data, this.replacer), this.filename);
					this.update();
				}
				
				this.replacer = function (key, value) {
					return key == "width" ? undefined : value;
				}
				
				this.add = function () {
					if (this.editing)
						return;
					this.editing = true;
					var new_name = _.input("Enter autoplaylist name", panel.name, "");
					if (new_name == "")
						return this.editing = false;
					var new_query = _.input("Enter autoplaylist query", panel.name, "");
					if (new_query == "")
						return this.editing = false;
					var new_sort = _.input("Enter sort pattern\n\n(optional)", panel.name, "");
					var new_forced = (new_sort.length ? WshShell.popup("Force sort?", 0, panel.name, popup.question + popup.yes_no) : popup.no) == popup.yes;
					this.data.push({
						name : new_name,
						query : new_query,
						sort : new_sort,
						forced : new_forced
					});
					this.edit_done(this.data.length - 1);
					this.editing = false;
				}
				
				this.edit = function (x, y) {
					var z = this.index;
					_.tt("");
					var m = window.CreatePopupMenu();
					m.AppendMenuItem(MF_STRING, 1, "Rename...");
					m.AppendMenuItem(MF_STRING, 2, "Edit query...");
					m.AppendMenuItem(MF_STRING, 3, "Edit sort pattern...");
					m.AppendMenuItem(MF_STRING, 4, "Force Sort");
					m.CheckMenuItem(4, this.data[z].forced);
					m.AppendMenuSeparator();
					m.AppendMenuItem(MF_STRING, 5, "Delete");
					this.editing = true;
					this.hover = false;
					window.RepaintRect(this.x + this.w - 20, this.y, 20, this.h);
					var idx = m.TrackPopupMenu(x, y);
					switch (idx) {
					case 1:
						var new_name = _.input("Rename autoplaylist", panel.name, this.data[z].name);
						if (new_name == "" || new_name == this.data[z].name)
							break;
						this.data[z].name = new_name;
						this.edit_done(z);
						break;
					case 2:
						var new_query = _.input("Enter autoplaylist query", panel.name, this.data[z].query);
						if (new_query == "" || new_query == this.data[z].query)
							break;
						this.data[z].query = new_query;
						this.edit_done(z);
						break;
					case 3:
						var new_sort = _.input("Enter sort pattern\n\n(optional)", panel.name, this.data[z].sort);
						if (new_sort == this.data[z].sort)
							break;
						this.data[z].sort = new_sort;
						if (new_sort.length)
							this.data[z].forced = WshShell.popup("Force sort?", 0, panel.name, popup.question + popup.yes_no) == popup.yes;
						this.edit_done(z);
						break;
					case 4:
						this.data[z].forced = !this.data[z].forced;
						this.edit_done(z);
						break;
					case 5:
						this.deleted_items.unshift(this.data[z]);
						this.data.splice(z, 1);
						this.save();
						break;
					}
					this.editing = false;
					_.dispose(m);
				}
				
				this.edit_done = function (z) {
					this.run_query(this.data[z].name, this.data[z].query, this.data[z].sort, this.data[z].forced);
					this.save();
				}
				
				this.run_query = function (n, q, s, f) {
					var i = 0;
					while (i < plman.PlaylistCount) {
						if (plman.GetPlaylistName(i) == n)
							plman.RemovePlaylist(i);
						else
							i++;
					}
					plman.CreateAutoPlaylist(plman.PlaylistCount, n, q, s, f);
					plman.ActivePlaylist = plman.PlaylistCount - 1;
				}
				
				_.createFolder(folders.settings);
				this.hover = false;
				this.editing = false;
				this.deleted_items = [];
				this.guifx_font = _.gdiFont(guifx.font, 12, 0);
				this.filename = folders.settings + "autoplaylists.json";
				this.update();
				break;
			case "lastfm_info":
				_.createFolder(folders.data);
				_.createFolder(folders.lastfm);
				_.createFolder(folders.artists);
				_.createFolder(folders.settings);
				this.ua = lastfm.ua;
				this.lastfm_mode = window.GetProperty("2K3.LIST.LASTFM.MODE", 0); // 0 similar artists 1 charts
				this.lastfm_charts_methods = [{
						method : "user.getTopArtists",
						json : "topartists.artist",
						display : "artist"
					}, {
						method : "user.getTopAlbums",
						json : "topalbums.album",
						display : "album"
					}, {
						method : "user.getTopTracks",
						json : "toptracks.track",
						display : "track"
					}
				];
				this.lastfm_charts_method = window.GetProperty("2K3.LIST.LASTFM.CHARTS.METHOD", 0);
				this.lastfm_charts_periods = [{
						period : "overall",
						display : "overall"
					}, {
						period : "7day",
						display : "last 7 days"
					}, {
						period : "1month",
						display : "1 month"
					}, {
						period : "3month",
						display : "3 month"
					}, {
						period : "6month",
						display : "6 month"
					}, {
						period : "12month",
						display : "12 month"
					}
				];
				this.lastfm_charts_period = window.GetProperty("2K3.LIST.LASTFM.CHARTS.PERIOD", 0);
				this.lastfm_charts_colour = window.GetProperty("2K3.LIST.LASTFM.CHARTS.COLOUR", "60-60-60");
				if (this.lastfm_mode == 1)
					this.update();
				break;
			case "musicbrainz":
				this.mb_retry = _.bind(function () {
					console.log("Retrying...");
					this.attempt++;
					this.get();
				}, this),
				
				this.mb_parse_urls = _.bind(function (item) {
					var url = decodeURIComponent(item.url.resource);
					var image = "external";
					if (item.type == "official homepage") {
						image = "home";
					} else {
						_.forEach(this.mb_images, function (item, i) {
							if (url.indexOf(i) > -1) {
								image = i;
								return false;
							}
						});
					}
					return {
						name : url,
						url : url,
						width : _.textWidth(url, panel.fonts.normal),
						image : image
					};
				}, this);
				
				_.createFolder(folders.data);
				_.createFolder(folders.artists);
				this.ua = "foo_jscript_panel_musicbrainz +https://github.com/19379";
				this.mb_mode = window.GetProperty("2K3.LIST.MUSICBRAINZ.MODE", 0); // 0 releases 1 links
				this.mb_icons = window.GetProperty("2K3.LIST.MUSICBRAINZ.SHOW.ICONS", true);
				this.mb_id = "";
				this.mb_images = {
					"wikipedia.org" : _.img("mb\\wikipedia.png"),
					"wikidata.org" : _.img("mb\\wikidata.png"),
					"youtube.com" : _.img("mb\\youtube.png"),
					"discogs.com" : _.img("mb\\discogs.png"),
					"last.fm" : _.img("mb\\lastfm.png"),
					"facebook.com" : _.img("mb\\facebook.png"),
					"viaf.org" : _.img("mb\\viaf.png"),
					"bbc.co.uk" : _.img("mb\\bbc.png"),
					"twitter.com" : _.img("mb\\twitter.png"),
					"allmusic.com" : _.img("mb\\allmusic.png"),
					"soundcloud.com" : _.img("mb\\soundcloud.png"),
					"myspace.com" : _.img("mb\\myspace.png"),
					"imdb.com" : _.img("mb\\imdb.png"),
					"plus.google.com" : _.img("mb\\google_plus.png"),
					"lyrics.wikia.com" : _.img("mb\\lyrics_wikia.png"),
					"flickr.com" : _.img("mb\\flickr.png"),
					"secondhandsongs.com" : _.img("mb\\secondhandsongs.png"),
					"vimeo.com" : _.img("mb\\vimeo.png"),
					"rateyourmusic.com" : _.img("mb\\rateyourmusic.png"),
					"instagram.com" : _.img("mb\\instagram.png"),
					"tumblr.com" : _.img("mb\\tumblr.png"),
					"decoda.com" : _.img("mb\\decoda.png"),
					"home" : _.img("mb\\home.png"),
					"external" : _.img("mb\\external.png"),
					"musicbrainz" : _.img("mb\\musicbrainz.png")
				};
				break;
			case "properties":
				this.add_meta = function (f) {
					for (var i = 0; i < f.MetaCount; i++) {
						var name = f.MetaName(i);
						var num = f.MetaValueCount(i);
						for (var j = 0; j < num; j++) {
							var value = f.MetaValue(i, j).replace(/\s{2,}/g, " ");
							switch (name.toUpperCase()) {
							case "MUSICBRAINZ_RELEASEGROUPID":
							case "MUSICBRAINZ RELEASE GROUP ID":
								var url = "https://musicbrainz.org/release-group/" + value;
								break;
							case "MUSICBRAINZ_ARTISTID":
							case "MUSICBRAINZ_ALBUMARTISTID":
							case "MUSICBRAINZ ARTIST ID":
							case "MUSICBRAINZ ALBUM ARTIST ID":
								var url = "https://musicbrainz.org/artist/" + value;
								break;
							case "MUSICBRAINZ_ALBUMID":
							case "MUSICBRAINZ ALBUM ID":
								var url = "https://musicbrainz.org/release/" + value;
								break;
							case "MUSICBRAINZ_TRACKID":
							case "MUSICBRAINZ TRACK ID":
								var url = "https://musicbrainz.org/recording/" + value;
								break;
							default:
								var url = name.toLowerCase() + (num == 1 ? " IS " : " HAS ") + value;
								break;
							}
							this.data.push({
								name : j == 0 ? name.toUpperCase() : "",
								value : value,
								url : url
							});
						}
					}
					if (this.data.length) // only add blank line if there is some metadata
						this.add();
				}
				
				this.add_location = function () {
					var names = ["FILE NAME", "FOLDER NAME", "FILE PATH", "SUBSONG INDEX", "FILE SIZE", "LAST MODIFIED"];
					var values = [panel.tf("%filename_ext%"), panel.tf("$directory_path(%path%)"), this.filename, panel.metadb.Subsong, panel.tf("[%filesize_natural%]"), panel.tf("[%last_modified%]")];
					var urls = ["%filename_ext% IS ", "\"$directory_path(%path%)\" IS ", "%path% IS ", "%subsong% IS ", "%filesize_natural% IS ", "%last_modified% IS "];
					for (var i = 0; i < 6; i++) {
						this.data.push({
							name : names[i],
							value : values[i],
							url : urls[i] + values[i]
						});
					}
					this.add();
				}
				
				this.add_tech = function (f) {
					var duration = utils.FormatDuration(Math.max(0, panel.metadb.Length));
					var samples = _.formatNumber(_.tf("%length_samples%", panel.metadb), " ");
					this.data.push({
						name : "DURATION",
						value : duration + " (" + samples + " samples)",
						url : "%length% IS " + duration
					});
					for (var i = 0; i < f.InfoCount; i++) {
						var name = f.InfoName(i);
						var value = f.InfoValue(i);
						this.data.push({
							name : name.toUpperCase(),
							value : value,
							url : "%__" + name.toLowerCase() + "% IS " + value
						});
					}
					this.add();
				}
				
				this.add_customdb = function () {
					this.add(["LASTFM_PLAYCOUNT_DB", "LASTFM_LOVED_DB"]);
					this.add();
				}
				
				this.add_playcount = function () {
					this.add(["PLAY_COUNT", "FIRST_PLAYED", "LAST_PLAYED", "ADDED", "RATING"]);
					this.add();
				}
				
				this.add_rg = function () {
					this.add(["REPLAYGAIN_ALBUM_GAIN", "REPLAYGAIN_ALBUM_PEAK", "REPLAYGAIN_TRACK_GAIN", "REPLAYGAIN_TRACK_PEAK"]);
					this.add();
				}
				
				this.add = function (names) {
					if (names) {
						this.data.push.apply(this.data, _.map(names, function (item) {
							return {
								name : item,
								value : panel.tf("[%" + item + "%]"),
								url : "%" + item + "% IS " + panel.tf("[%" + item + "%]")
							};
						}));
					} else {
						this.data.push({name : "", value : "", url : ""});
					}
				}
				
				this.properties = {
					meta : window.GetProperty("2K3.LIST.PROPERTIES.META", true),
					location : window.GetProperty("2K3.LIST.PROPERTIES.LOCATION", true),
					tech : window.GetProperty("2K3.LIST.PROPERTIES.TECH", true),
					customdb : window.GetProperty("2K3.LIST.PROPERTIES.CUSTOMDB", true),
					playcount : window.GetProperty("2K3.LIST.PROPERTIES.PLAYCOUNT", true),
					rg : window.GetProperty("2K3.LIST.PROPERTIES.RG", true)
				};
				break;
			}
		}
		
		panel.list_objects.push(this);
		this.mode = mode;
		this.x = x;
		this.y = y;
		this.w = w;
		this.h = h;
		this.mx = 0;
		this.my = 0;
		this.index = 0;
		this.offset = 0;
		this.items = 0;
		this.text_x = 0;
		this.spacer_w = 0;
		this.artist = "";
		this.filename = "";
		this.up_btn = new _.sb(guifx.up, this.x, this.y, 16, 16, _.bind(function () { return this.offset > 0; }, this), _.bind(function () { this.wheel(1); }, this));
		this.down_btn = new _.sb(guifx.down, this.x, this.y, 16, 16, _.bind(function () { return this.offset < this.items - this.rows; }, this), _.bind(function () { this.wheel(-1); }, this));
		this.xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
		this.init();
	}
});

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
			case "properties":
				this.text_width = this.w - this.text_x;
				for (var i = 0; i < Math.min(this.items, this.rows); i++) {
					gr.GdiDrawText(this.data[i + this.offset].name, panel.fonts.normal, panel.colours.highlight, this.x, this.y + 16 + (i * panel.row_height), this.text_x - 10, panel.row_height, LEFT);
					gr.GdiDrawText(this.data[i + this.offset].value, panel.fonts.normal, panel.colours.text, this.x + this.text_x, this.y + 16 + (i * panel.row_height), this.text_width, panel.row_height, LEFT);
				}
				break;
			}
			this.up_btn.paint(gr, panel.colours.text);
			this.down_btn.paint(gr, panel.colours.text);
		}
		
		this.metadb_changed = function () {
			switch (true) {
			case this.mode == "autoplaylists":
				break;
			case !panel.metadb:
				this.data = [];
				this.items = 0;
				window.Repaint();
				break;
			case this.mode == "properties":
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
				fileinfo.Dispose();
				break;
			}
			this.items = this.data.length;
			this.offset = 0;
			this.index = 0;
			window.Repaint();
		}
		
		this.header_text = function () {
			switch (this.mode) {
			case "autoplaylists":
				return "Autoplaylists";
			case "properties":
				return panel.tf("%artist% - %title%");
			}
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
					m.Dispose();
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
		this.filename = "";
		this.up_btn = new _.sb(guifx.up, this.x, this.y, 16, 16, _.bind(function () { return this.offset > 0; }, this), _.bind(function () { this.wheel(1); }, this));
		this.down_btn = new _.sb(guifx.down, this.x, this.y, 16, 16, _.bind(function () { return this.offset < this.items - this.rows; }, this), _.bind(function () { this.wheel(-1); }, this));
		this.init();
	}
});

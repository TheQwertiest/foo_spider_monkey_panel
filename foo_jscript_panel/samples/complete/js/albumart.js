_.mixin({
	albumart : function (x, y, w, h) {
		this.paint = function (gr) {
			if (this.properties.cd.enabled) {
				if (this.properties.shadow.enabled) {
					_.drawImage(gr, this.images.shadow, this.x, this.y, this.w, this.h);
				}
				_.drawImage(gr, this.images.case, this.x, this.y, this.w, this.h);
				if (this.img) {
					var ratio = Math.min(this.w / this.images.case.Width, this.h / this.images.case.Height);
					var nw = 488 * ratio;
					var nh = 476 * ratio;
					var nx = this.x + Math.floor((this.w - (452 * ratio)) / 2);
					var ny = this.y + Math.floor((this.h - nh) / 2);
					_.drawImage(gr, this.img, nx, ny, nw, nh, this.properties.aspect.value);
				}
				_.drawImage(gr, this.images.semi, this.x, this.y, this.w, this.h);
				if (this.properties.gloss.enabled) {
					_.drawImage(gr, this.images.gloss, this.x, this.y, this.w, this.h);
				}
			} else if (this.img) {
				_.drawImage(gr, this.img, this.x, this.y, this.w, this.h, this.properties.aspect.value);
			}
		}
		
		this.metadb_changed = function () {
			_.dispose(this.img);
			this.img = null;
			this.tooltip = this.path = '';
			if (panel.metadb) {
				this.img = utils.GetAlbumArtV2(panel.metadb, this.properties.id.value);
				if (this.img && panel.metadb.RawPath.indexOf('file') == 0) {
					utils.GetAlbumArtAsync(window.ID, panel.metadb, this.properties.id.value, true, false, true);
				}
			}
			window.Repaint();
		}
		
		this.get_album_art_done = function (p) {
			this.path = p;
			if (this.img) {
				this.tooltip = 'Original dimensions: ' + this.img.Width + 'x' + this.img.Height + 'px';
				if (_.isFile(this.path)) {
					this.tooltip += '\nPath: ' + this.path;
					if (panel.metadb.Path != this.path) {
						this.tooltip += '\nSize: ' + utils.FormatFileSize(fso.GetFile(this.path).Size);
					}
				}
			}
		}
		
		this.trace = function (x, y) {
			return x > this.x && x < this.x + this.w && y > this.y && y < this.y + this.h;
		}
		
		this.wheel = function (s) {
			if (this.trace(this.mx, this.my)) {
				var id = this.properties.id.value - s;
				if (id < 0) {
					id = 4;
				}
				if (id > 4) {
					id = 0;
				}
				this.properties.id.value = id;
				_.tt('');
				panel.item_focus_change();
				return true;
			} else {
				return false;
			}
		}
		
		this.move = function (x, y) {
			this.mx = x;
			this.my = y;
			if (this.trace(x, y)) {
				if (this.img) {
					_.tt(this.tooltip);
				}
				this.hover = true;
				return true;
			} else {
				if (this.hover) {
					_.tt('');
				}
				this.hover = false;
				return false;
			}
		}
		
		this.lbtn_dblclk = function (x, y) {
			if (this.trace(x, y)) {
				if (panel.metadb && panel.metadb.Path == this.path) {
					_.explorer(this.path);
				} else if (_.isFile(this.path)) {
					_.run(this.path);
				}
				return true;
			} else {
				return false;
			}
		}
		
		this.rbtn_up = function (x, y) {
			panel.m.AppendMenuItem(MF_STRING, 2000, 'Refresh');
			panel.m.AppendMenuSeparator();
			panel.m.AppendMenuItem(MF_STRING, 2001, 'CD Jewel Case');
			panel.m.CheckMenuItem(2001, this.properties.cd.enabled);
			panel.m.AppendMenuItem(this.properties.cd.enabled ? MF_STRING : MF_GRAYED, 2002, 'Gloss effect');
			panel.m.CheckMenuItem(2002, this.properties.gloss.enabled);
			panel.m.AppendMenuItem(this.properties.cd.enabled ? MF_STRING : MF_GRAYED, 2003, 'Shadow effect');
			panel.m.CheckMenuItem(2003, this.properties.shadow.enabled);
			panel.m.AppendMenuSeparator();
			_.forEach(this.ids, function (item, i) {
				panel.m.AppendMenuItem(MF_STRING, i + 2010, item);
			});
			panel.m.CheckMenuRadioItem(2010, 2014, this.properties.id.value + 2010);
			panel.m.AppendMenuSeparator();
			panel.m.AppendMenuItem(MF_STRING, 2020, 'Crop (focus on centre)');
			panel.m.AppendMenuItem(MF_STRING, 2021, 'Crop (focus on top)');
			panel.m.AppendMenuItem(MF_STRING, 2022, 'Stretch');
			panel.m.AppendMenuItem(MF_STRING, 2023, 'Centre');
			panel.m.CheckMenuRadioItem(2020, 2023, this.properties.aspect.value + 2020);
			panel.m.AppendMenuSeparator();
			panel.m.AppendMenuItem(_.isFile(this.path) ? MF_STRING : MF_GRAYED, 2030, 'Open containing folder');
			panel.m.AppendMenuSeparator();
			panel.m.AppendMenuItem(panel.metadb ? MF_STRING : MF_GRAYED, 2040, 'Google image search');
			panel.m.AppendMenuSeparator();
		}
		
		this.rbtn_up_done = function (idx) {
			switch (idx) {
			case 2000:
				panel.item_focus_change();
				break;
			case 2001:
				this.properties.cd.toggle();
				window.Repaint();
				break;
			case 2002:
				this.properties.gloss.toggle();
				window.RepaintRect(this.x, this.y, this.w, this.h);
				break;
			case 2003:
				this.properties.shadow.toggle();
				window.RepaintRect(this.x, this.y, this.w, this.h);
				break;
			case 2010:
			case 2011:
			case 2012:
			case 2013:
			case 2014:
				this.properties.id.value = idx - 2010;
				panel.item_focus_change();
				break;
			case 2020:
			case 2021:
			case 2022:
			case 2023:
				this.properties.aspect.value = idx - 2020;
				window.RepaintRect(this.x, this.y, this.w, this.h);
				break;
			case 2030:
				_.explorer(this.path);
				break;
			case 2040:
				_.run('https://www.google.com/search?tbm=isch&q=' + encodeURIComponent(panel.tf('%album artist%[ %album%]')));
				break;
			}
		}
		
		this.key_down = function (k) {
			switch (k) {
			case VK_LEFT:
			case VK_UP:
				this.wheel(1);
				return true;
			case VK_RIGHT:
			case VK_DOWN:
				this.wheel(-1);
				return true;
			default:
				return false;
			}
		}
		
		this.x = x;
		this.y = y;
		this.w = w;
		this.h = h;
		this.mx = 0;
		this.my = 0;
		this.tooltip = '';
		this.img = null;
		this.path = null;
		this.hover = false;
		this.ids = ['Front', 'Back', 'Disc', 'Icon', 'Artist'];
		this.images = {
			shadow : _.img('cd\\shadow.png'),
			case : _.img('cd\\case.png'),
			semi : _.img('cd\\semi.png'),
			gloss : _.img('cd\\gloss.png')
		};
		this.properties = {
			aspect : new _.p('2K3.ARTREADER.ASPECT', image.crop),
			gloss : new _.p('2K3.ARTREADER.GLOSS', false),
			cd : new _.p('2K3.ARTREADER.CD', false),
			id : new _.p('2K3.ARTREADER.ID', 0),
			shadow : new _.p('2K3.ARTREADER.SHADOW', false)
		};
	}
});

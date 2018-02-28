_.mixin({
	rating : function (x, y, h, colour) {
		this.paint = function (gr) {
			if (panel.metadb) {
				gr.SetTextRenderingHint(4);
				for (var i = 0; i < this.get_max(); i++) {
					gr.DrawString(i + 1 > (this.hover ? this.hrating : this.rating) ? chars.rating_off : chars.rating_on, this.font, this.colour, this.x + (i * this.h), this.y, this.h, this.h, SF_CENTRE);
				}
			}
		}
		
		this.metadb_changed = function () {
			if (panel.metadb) {
				this.hover = false;
				this.rating = this.get_rating();
				this.hrating = this.rating;
				this.tiptext = this.properties.mode.value == 0 ? 'Choose a mode first.' : panel.tf('Rate "%title%" by "%artist%".');
			}
			window.Repaint();
		}
		
		this.trace = function (x, y) {
			return x > this.x && x < this.x + this.w && y > this.y && y < this.y + this.h;
		}
		
		this.move = function (x, y) {
			if (this.trace(x, y)) {
				if (panel.metadb) {
					_.tt(this.tiptext);
					this.hover = true;
					this.hrating = Math.ceil((x - this.x) / this.h);
					window.RepaintRect(this.x, this.y, this.w, this.h);
				}
				return true;
			} else {
				this.leave();
				return false;
			}
		}
		
		this.leave = function () {
			if (this.hover) {
				_.tt('');
				this.hover = false;
				window.RepaintRect(this.x, this.y, this.w, this.h);
			}
		}
		
		this.lbtn_up = function (x, y) {
			if (this.trace(x, y)) {
				if (panel.metadb) {
					this.set_rating();
				}
				return true;
			} else {
				return false;
			}
		}
		
		this.rbtn_up = function (x, y) {
			_.forEach(this.modes, function (item, i) {
				panel.s10.AppendMenuItem(i == 1 && !this.foo_playcount ? MF_GRAYED : MF_STRING, i + 1000, item);
			}, this);
			panel.s10.CheckMenuRadioItem(1000, 1003, this.properties.mode.value + 1000);
			panel.s10.AppendTo(panel.m, MF_STRING, 'Mode');
			panel.m.AppendMenuItem(this.properties.mode.value == 2 ? MF_STRING : MF_GRAYED, 1004, 'Tag name');
			panel.m.AppendMenuItem(this.properties.mode.value > 1 ? MF_STRING : MF_GRAYED, 1005, 'Max value...');
			panel.m.AppendMenuSeparator();
		}
		
		this.rbtn_up_done = function (idx) {
			switch (true) {
			case idx <= 1003:
				this.properties.mode.set(idx - 1000);
				break;
			case idx == 1004:
				var tmp = _.input('Enter a custom tag name. Do not use %%. Defaults to "rating" if left blank.', window.Name, this.properties.tag.value);
				this.properties.tag.set(tmp || this.properties.tag.default_);
				break;
			case idx == 1005:
				var tmp = _.input('Enter a maximum value. Defaults to "5" if left blank.', window.Name, this.properties.max.value);
				this.properties.max.set(tmp || this.properties.max.default_);
				break;
			}
			this.w = this.h * this.get_max();
			panel.item_focus_change();
		}
		
		this.get_rating = function () {
			switch (this.properties.mode.value) {
			case 1: // foo_playcount
				return panel.tf('$if2(%rating%,0)');
			case 2: // file tag
				var f = panel.metadb.GetFileInfo();
				var idx = f.MetaFind(this.properties.tag.value);
				var ret = idx > -1 ? f.MetaValue(idx, 0) : 0;
				f.Dispose();
				return ret;
			case 3: // JScript Panel DB
				return panel.tf('$if2(%jsp_rating%,0)');
			default:
				return 0;
			}
		}
		
		this.set_rating = function () {
			switch (this.properties.mode.value) {
			case 1: // foo_playcount
				fb.RunContextCommandWithMetadb('Rating/' + (this.hrating == this.rating ? '<not set>' : this.hrating), panel.metadb, 8);
				break;
			case 2: // file tag
				var tmp = this.hrating == this.rating ? '' : this.hrating;
				var obj = {};
				obj[this.properties.tag.value] = tmp;
				var handles = fb.CreateHandleList();
				handles.Add(panel.metadb);
				handles.UpdateFileInfoFromJSON(JSON.stringify(obj));
				handles.Dispose();
				break;
			case 3: // JScript Panel db
				panel.metadb.SetRating(this.hrating == this.rating ? 0 : this.hrating);
				panel.metadb.RefreshStats();
				break;
			}
		}
		
		this.get_max = function () {
			return this.properties.mode.value < 2 ? 5 : this.properties.max.value;
		}
		
		this.properties = {
			mode : new _.p('2K3.RATING.MODE', 0), // 0 not set 1 foo_playcount 2 file tag 3 JScript Panel db
			max : new _.p('2K3.RATING.MAX', 5), // only use for file tag/JScript Panel db mode
			tag: new _.p('2K3.RATING.TAG', 'rating')
		};
		this.x = x;
		this.y = y;
		this.h = _.scale(h);
		this.w = this.h * this.get_max();
		this.colour = colour;
		this.hover = false;
		this.rating = 0;
		this.hrating = 0;
		this.font = gdi.Font('FontAwesome', this.h - 2);
		this.modes = ['Not Set', 'foo_playcount', 'File Tag', 'JScript Panel DB'];
		this.foo_playcount = _.cc('foo_playcount');
		window.SetTimeout(_.bind(function () {
			if (this.properties.mode.value == 1 && !this.foo_playcount) { // if mode is set to 1 (foo_playcount) but component is missing, reset to 0.
				this.properties.mode.set(0);
			}
			if (this.properties.mode.value == 0) {
				fb.ShowPopupMessage('This script has now been updated and supports 3 different modes.\n\nAs before, you can use foo_playcount which is limited to 5 stars.\n\nThe 2nd option is writing to your file tags. You can choose the tag name and a max value via the right click menu.\n\nLastly, a new "Playback Stats" database has been built into JScript Panel. It is bound to just "%artist% - %title%". This uses %jsp_rating% which can be accessed via title formatting in all other components/search dialogs. This also supports a custom max value.\n\nAll options are available on the right click menu. If you do not see the new options when right clicking, make sure you have the latest "rating.txt" imported from the "samples\\complete" folder.', window.Name);
			}
		}, this), 500);
	}
});

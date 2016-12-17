_.mixin({
	text : function (mode, x, y, w, h) {
		this.size = function () {
			this.rows = _.floor((this.h - 32) / panel.row_height);
			this.up_btn.x = this.x + _.round((this.w - 16) / 2);
			this.down_btn.x = this.up_btn.x;
			this.up_btn.y = this.y;
			this.down_btn.y = this.y + this.h - 16;
			this.update();
		}
		
		this.paint = function (gr) {
			for (var i = 0; i < Math.min(this.rows, this.lines.length); i++) {
				gr.GdiDrawText(this.lines[i + this.offset], this.fixed ? panel.fonts.fixed : panel.fonts.normal, panel.colours.text, this.x, (this.fixed ? _.floor(panel.row_height / 2) : 0) + 16 + this.y + (i * panel.row_height), this.w, panel.row_height, LEFT);
			}
			this.up_btn.paint(gr, panel.colours.text);
			this.down_btn.paint(gr, panel.colours.text);
		}
		
		this.metadb_changed = function () {
			if (panel.metadb) {
				var temp_filename = panel.tf(this.filename_tf);
				if (this.filename == temp_filename)
					return;
				this.filename = temp_filename;
				this.content = "";
				if (_.isFolder(this.filename)) { // yes really!
					var folder = this.filename + "\\";
					this.content = _.open(_.getFiles(folder, this.exts)[0]);
				} else if (_.isFile(this.filename)) {
					this.content = _.open(this.filename);
				}
				this.content = this.content.replace(/\t/g, "    ");
			} else {
				this.filename = "";
				this.content = "";
			}
			this.update();
			window.Repaint();
		}
		
		this.trace = function (x, y) {
			return x > this.x && x < this.x + this.w && y > this.y && y < this.y + this.h;
		}
		
		this.wheel = function (s) {
			if (this.trace(this.mx, this.my)) {
				if (this.lines.length > this.rows) {
					var offset = this.offset - (s * 3);
					if (offset < 0)
						offset = 0;
					if (offset + this.rows > this.lines.length)
						offset = this.lines.length - this.rows;
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
			switch (true) {
			case !this.trace(x, y):
				window.SetCursor(IDC_ARROW);
				return false;
			case this.up_btn.move(x, y):
			case this.down_btn.move(x, y):
				break;
			default:
				window.SetCursor(IDC_ARROW);
				break;
			}
			return true;
		}
		
		this.lbtn_up = function (x, y) {
			if (this.trace(x, y)) {
				this.up_btn.lbtn_up(x, y);
				this.down_btn.lbtn_up(x, y);
				return true;
			} else {
				return false;
			}
		}
		
		this.rbtn_up = function (x, y) {
			panel.m.AppendMenuItem(MF_STRING, 5200, "Refresh");
			panel.m.AppendMenuSeparator();
			panel.m.AppendMenuItem(MF_STRING, 5210, "Custom title...");
			panel.m.AppendMenuItem(MF_STRING, 5220, "Custom path...");
			panel.m.AppendMenuSeparator();
			panel.m.AppendMenuItem(MF_STRING, 5230, "Fixed width font");
			panel.m.CheckMenuItem(5230, this.fixed);
			panel.m.AppendMenuSeparator();
			panel.m.AppendMenuItem(_.isFile(this.filename) || _.isFolder(this.filename) ? MF_STRING : MF_GRAYED, 5999, "Open containing folder");
			panel.m.AppendMenuSeparator();
		}
		
		this.rbtn_up_done = function (idx) {
			switch (idx) {
			case 5200:
				this.filename = "";
				panel.item_focus_change();
				break;
			case 5210:
				this.title_tf = _.input("You can use full title formatting here.", panel.name, this.title_tf);
				window.SetProperty("2K3.TEXT.TITLE.TF", this.title_tf);
				window.Repaint();
				break;
			case 5220:
				this.filename_tf = _.input("Use title formatting to specify a path to a text file. eg: $directory_path(%path%)\\info.txt\n\nIf you prefer, you can specify just the path to a folder and the first txt or log file will be used.", panel.name, this.filename_tf);
				window.SetProperty("2K3.TEXT.FILENAME.TF", this.filename_tf);
				panel.item_focus_change();
				break;
			case 5230:
				this.fixed = !this.fixed;
				window.SetProperty("2K3.TEXT.FONTS.FIXED", this.fixed);
				this.update();
				window.RepaintRect(this.x, this.y, this.w, this.h);
				break;
			case 5999:
				if (_.isFile(this.filename))
					_.explorer(this.filename);
				else
					_.run(this.filename);
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
			this.offset = 0;
			switch (true) {
			case this.w < 100 || !this.content.length:
				this.lines = [];
				break;
			case this.fixed:
				this.lines = this.content.split("\n");
				break;
			default:
				this.lines = _.lineWrap(this.content, panel.fonts.normal, this.w);
				break;
			}
		}
		
		this.header_text = function () {
			return panel.tf(this.title_tf);
		}
		
		this.init = function () {
			this.title_tf = window.GetProperty("2K3.TEXT.TITLE.TF", "$directory_path(%path%)");
			this.filename_tf = window.GetProperty("2K3.TEXT.FILENAME.TF", "$directory_path(%path%)");
			this.fixed = window.GetProperty("2K3.TEXT.FONTS.FIXED", true);
			this.exts = "txt|log";
		}
		
		panel.text_objects.push(this);
		this.mode = mode;
		this.x = x;
		this.y = y;
		this.w = w;
		this.h = h;
		this.mx = 0;
		this.my = 0;
		this.offset = 0;
		this.fixed = false;
		this.content = "";
		this.filename = "";
		this.up_btn = new _.sb(guifx.up, this.x, this.y, 16, 16, _.bind(function () { return this.offset > 0; }, this), _.bind(function () { this.wheel(1); }, this));
		this.down_btn = new _.sb(guifx.down, this.x, this.y, 16, 16, _.bind(function () { return this.offset < this.lines.length - this.rows; }, this), _.bind(function () { this.wheel(-1); }, this));
		this.init();
	}
});

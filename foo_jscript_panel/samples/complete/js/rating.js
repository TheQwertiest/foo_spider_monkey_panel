_.mixin({
	rating : function (x, y, h, off, on) {
		this.paint = function (gr) {
			if (panel.metadb) {
				gr.SetTextRenderingHint(4);
				for (var i = 0; i < 5; i++) {
					gr.DrawString(guifx.star, this.guifx_font, i + 1 > (this.hover ? this.hrating : this.rating) ? this.off : this.on, this.x + (i * this.h), this.y, this.h, this.h, SF_CENTRE);
				}
			}
		}
		
		this.metadb_changed = function () {
			if (panel.metadb) {
				this.hover = false;
				this.rating = _.tf('$if2(%rating%,0)', panel.metadb);
				this.tiptext = _.tf(this.tiptext_tf, panel.metadb);
				this.hrating = this.rating;
			}
			window.RepaintRect(this.x, this.y, this.w, this.h);
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
					fb.RunContextCommandWithMetadb('Rating/' + (this.hrating == this.rating ? '<not set>' : this.hrating), panel.metadb, 8);
				}
				return true;
			} else {
				return false;
			}
		}
		
		this.x = x;
		this.y = y;
		this.h = _.scale(h);
		this.w = this.h * 5;
		this.on = on;
		this.off = off;
		this.hover = false;
		this.rating = 0;
		this.hrating = 0;
		this.guifx_font = gdi.Font(guifx.font, this.h, 0);
		this.tiptext_tf = 'Rate "%title%" by "%artist%".';
		if (!_.cc('foo_playcount')) {
			window.SetTimeout(function () {
				WshShell.popup('This script requires foo_playcount.', 0, window.Name, popup.stop);
			}, 500);
		}
	}
});

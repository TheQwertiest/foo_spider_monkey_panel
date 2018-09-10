_.mixin({
	volume : function (x, y, w, h) {
		this.volume_change = function () {
			window.RepaintRect(this.x, this.y, this.w, this.h);
		}
		
		this.trace = function (x, y) {
			var m = this.drag ? 200 : 0;
			return x > this.x - m && x < this.x + this.w + m && y > this.y - m && y < this.y + this.h + m;
		}
		
		this.wheel = function (s) {
			if (this.trace(this.mx, this.my)) {
				if (s == 1) {
					fb.VolumeUp();
				} else {
					fb.VolumeDown();
				}
				_.tt('');
				return true;
			} else {
				return false;
			}
		}
		
		this.move = function (x, y) {
			this.mx = x;
			this.my = y;
			if (this.trace(x, y)) {
				x -= this.x;
				var pos = x < 0 ? 0 : x > this.w ? 1 : x / this.w;
				this.drag_vol = 50 * Math.log(0.99 * pos + 0.01) / Math.LN10;
				_.tt(this.drag_vol.toFixed(2) + ' dB');
				if (this.drag) {
					fb.Volume = this.drag_vol;
				}
				this.hover = true;
				return true;
			} else {
				if (this.hover) {
					_.tt('');
				}
				this.hover = false;
				this.drag = false;
				return false;
			}
		}
		
		this.lbtn_down = function (x, y) {
			if (this.trace(x, y)) {
				this.drag = true;
				return true;
			} else {
				return false;
			}
		}
		
		this.lbtn_up = function (x, y) {
			if (this.trace(x, y)) {
				if (this.drag) {
					this.drag = false;
					fb.Volume = this.drag_vol;
				}
				return true;
			} else {
				return false;
			}
		}
		
		this.pos = function (type) {
			return Math.ceil((type == 'h' ? this.h : this.w) * (Math.pow(10, fb.Volume / 50) - 0.01) / 0.99);
		}
		
		this.x = x;
		this.y = y;
		this.w = w;
		this.h = h;
		this.mx = 0;
		this.my = 0;
		this.hover = false;
		this.drag = false;
		this.drag_vol = 0;
	}
});

_.mixin({
	seekbar : function (x, y, w, h) {
		this.playback_seek = function () {
			window.RepaintRect(this.x - _.scale(75), this.y - _.scale(10), this.w + _.scale(150), this.h + _.scale(20));
		}
		
		this.playback_stop = function () {
			this.playback_seek();
		}
		
		this.trace = function (x, y) {
			var m = this.drag ? 200 : 0;
			return x > this.x - m && x < this.x + this.w + m && y > this.y - m && y < this.y + this.h + m;
		}
		
		this.wheel = function (s) {
			if (this.trace(this.mx, this.my)) {
				switch (true) {
				case !fb.IsPlaying:
				case fb.PlaybackLength <= 0:
					break;
				case fb.PlaybackLength < 60:
					fb.PlaybackTime += s * 5;
					break;
				case fb.PlaybackLength < 600:
					fb.PlaybackTime += s * 10;
					break;
				default:
					fb.PlaybackTime += s * 60;
					break;
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
				if (fb.IsPlaying && fb.PlaybackLength > 0) {
					x -= this.x;
					this.drag_seek = x < 0 ? 0 : x > this.w ? 1 : x / this.w;
					_.tt(utils.FormatDuration(fb.PlaybackLength * this.drag_seek));
					if (this.drag) {
						this.playback_seek();
					}
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
				if (fb.IsPlaying && fb.PlaybackLength > 0) {
					this.drag = true;
				}
				return true;
			} else {
				return false;
			}
		}
		
		this.lbtn_up = function (x, y) {
			if (this.trace(x, y)) {
				if (this.drag) {
					this.drag = false;
					fb.PlaybackTime = fb.PlaybackLength * this.drag_seek;
				}
				return true;
			} else {
				return false;
			}
		}
		
		this.pos = function () {
			return Math.ceil(this.w * (this.drag ? this.drag_seek : fb.PlaybackTime / fb.PlaybackLength));
		}
		
		this.interval_func = _.bind(function () {
			if (fb.IsPlaying && !fb.IsPaused && fb.PlaybackLength > 0) {
				this.playback_seek();
			}
		}, this);
		
		this.x = x;
		this.y = y;
		this.w = w;
		this.h = h;
		this.mx = 0;
		this.my = 0;
		this.hover = false;
		this.drag = false;
		this.drag_seek = 0;
		window.SetInterval(this.interval_func, 150);
	}
});

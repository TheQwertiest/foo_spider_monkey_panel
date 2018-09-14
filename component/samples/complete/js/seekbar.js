_.mixin({
    /** @constructor */
    seekbar: function (x, y, w, h) {
        this.playback_seek = function () {
            this.repaint();
        };

        this.repaint = function () {
            var expXY = 2,
                expWH = expXY * 2;

            window.RepaintRect(this.x - expXY, this.y - expXY, this.w + expWH, this.h + expWH);
        };

        this.playback_stop = function () {
            this.playback_seek();
        };

        this.playback_start = function () {
            this.playback_seek();
        };

        this.playback_pause = function (isPlaying) {
            this.playback_seek();
        };

        this.trace = function (x, y) {
            var m = this.drag ? 200 : 0;
            return x > this.x - m && x < this.x + this.w + m && y > this.y - m && y < this.y + this.h + m;
        };

        this.move = function (x, y) {
            if (this.trace(x, y) || this.drag) {
                if (fb.IsPlaying && fb.PlaybackLength > 0) {
                    x -= this.x;
                    this.drag_seek = x < 0 ? 0 : x > this.w ? 1 : x / this.w;

                    if (this.drag) {
                        if (this.show_tt) {
                            this.tt.showImmediate(utils.FormatDuration(fb.PlaybackLength * this.drag_seek));
                        }
                        this.playback_seek();
                    }
                }
                if (!this.hover) {
                    this.hover = true;
                    if (this.show_tt) {
                        this.tt.showDelayed("Seek");
                    }
                    alpha_timer.start();
                }

                return true;
            }
            else {
                if (this.hover) {
                    this.tt.clear();

                    this.hover = false;
                    alpha_timer.start();
                }
                this.drag = false;

                return false;
            }
        };

        this.lbtn_down = function (x, y) {
            if (this.trace(x, y)) {
                if (fb.IsPlaying && fb.PlaybackLength > 0)
                    this.drag = true;
                return true;
            }
            else {
                return false;
            }
        };

        this.lbtn_up = function (x, y) {
            if (this.trace(x, y) || this.drag) {
                if (this.drag) {
                    this.drag = false;
                    fb.PlaybackTime = fb.PlaybackLength * this.drag_seek;
                }
                return true;
            }
            else {
                return false;
            }
        };

        this.leave = function () {
            if (this.drag) {
                return;
            }
            if (this.hover) {
                this.tt.clear();

                this.hover = false;
                alpha_timer.start();
            }
            this.drag = false;
        };

        this.pos = function () {
            return _.ceil(this.w * (this.drag ? this.drag_seek : fb.PlaybackTime / fb.PlaybackLength));
        };

        this.x = x;
        this.y = y;
        this.w = w;
        this.h = h;
        this.hover = false;
        this.hover_alpha = 0;
        this.drag = false;
        this.drag_seek = 0;
        this.show_tt = false;
        this.tt = new _.tt_handler;

        var that = this;

        var alpha_timer = new _.alpha_timer([that], function (item) {
            return item.hover;
        });
    }
});
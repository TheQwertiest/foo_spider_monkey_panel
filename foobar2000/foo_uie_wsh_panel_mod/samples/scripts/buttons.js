var tooltip = window.CreateTooltip();

function tt(value) {
	if (tooltip.Text != value) {
		tooltip.Text = value;
		tooltip.Activate();
	}
}

function button(x, y, w, h, img_src, fn, tiptext) {
	this.paint = function (gr) {
		this.img && gr.DrawImage(this.img, this.x, this.y, this.w, this.h, 0, 0, this.img.Width, this.img.Height);
	}
	
	this.trace = function (x, y) {
		return x > this.x && x < this.x + this.w && y > this.y && y < this.y + this.h;
	}
	
	this.lbtn_up = function (x, y) {
		this.fn && this.fn(x, y);
	}
	
	this.cs = function (s) {
		if (s == "hover") {
			this.img = this.img_hover;
			tt(this.tiptext);
		} else {
			this.img = this.img_normal;
		}
		window.RepaintRect(this.x, this.y, this.w, this.h);
	}
	
	this.x = x;
	this.y = y;
	this.w = w;
	this.h = h;
	this.fn = fn;
	this.tiptext = tiptext;
	this.img_normal = gdi.Image(img_src.normal);
	this.img_hover = img_src.hover ? gdi.Image(img_src.hover) : this.img_normal;
	this.img = this.img_normal;
}

function buttons() {
	this.paint = function (gr) {
		for (var i in this.buttons) {
			this.buttons[i].paint(gr);
		}
	}
	
	this.move = function (x, y) {
		var temp_btn = null;
		for (var i in this.buttons) {
			if (this.buttons[i].trace(x, y))
				temp_btn = i;
		}
		if (this.btn == temp_btn)
			return this.btn;
		if (this.btn)
			this.buttons[this.btn].cs("normal");
		if (temp_btn)
			this.buttons[temp_btn].cs("hover");
		else
			tt("");
		this.btn = temp_btn;
		return this.btn;
	}
	
	this.leave = function () {
		if (this.btn) {
			tt("");
			this.buttons[this.btn].cs("normal");
		}
		this.btn = null;
	}
	
	this.lbtn_up = function (x, y) {
		if (this.btn) {
			this.buttons[this.btn].lbtn_up(x, y);
			return true;
		} else {
			return false;
		}
	}
	
	this.buttons = {};
	this.btn = null;
}
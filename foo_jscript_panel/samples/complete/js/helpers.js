Array.prototype.srt=function(){for(var z=0,t;t=this[z];z++){this[z]=[];var x=0,y=-1,n=true,i,j;while(i=(j=t.charAt(x++)).charCodeAt(0)){var m=(i==46||(i>=48&&i<=57));if(m!==n){this[z][++y]='';n=m;}
this[z][y]+=j;}}
this.sort(function(a,b){for(var x=0,aa,bb;(aa=a[x])&&(bb=b[x]);x++){aa=aa.toLowerCase();bb=bb.toLowerCase();if(aa!==bb){var c=Number(aa),d=Number(bb);if(c==aa&&d==bb){return c-d;}else return(aa>bb)?1:-1;}}
return a.length-b.length;});for(var z=0;z<this.length;z++)
this[z]=this[z].join('');}

function on_script_unload() {
	_.tt('');
}

_.mixin({
	artistFolder : function (artist) {
		var a = _.fbSanitise(artist);
		var folder = folders.artists + a;
		if (_.isFolder(folder)) {
			return fso.GetFolder(folder) + '\\';
		} else {
			folder = folders.artists + _.trunc(a, 64);
			_.createFolder(folder);
			return fso.GetFolder(folder) + '\\';
		}
	},
	blendColours : function (c1, c2, f) {
		c1 = _.toRGB(c1);
		c2 = _.toRGB(c2);
		var r = Math.round(c1[0] + f * (c2[0] - c1[0]));
		var g = Math.round(c1[1] + f * (c2[1] - c1[1]));
		var b = Math.round(c1[2] + f * (c2[2] - c1[2]));
		return _.RGB(r, g, b);
	},
	button : function (x, y, w, h, img_src, fn, tiptext) {
		this.paint = function (gr) {
			if (this.img) {
				_.drawImage(gr, this.img, this.x, this.y, this.w, this.h);
			}
		}
		
		this.trace = function (x, y) {
			return x > this.x && x < this.x + this.w && y > this.y && y < this.y + this.h;
		}
		
		this.lbtn_up = function (x, y, mask) {
			if (this.fn) {
				this.fn(x, y, mask);
			}
		}
		
		this.cs = function (s) {
			if (s == 'hover') {
				this.img = this.img_hover;
				_.tt(this.tiptext);
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
		this.img_normal = typeof img_src.normal == 'string' ? _.img(img_src.normal) : img_src.normal;
		this.img_hover = img_src.hover ? (typeof img_src.hover == 'string' ? _.img(img_src.hover) : img_src.hover) : this.img_normal;
		this.img = this.img_normal;
	},
	buttons : function () {
		this.paint = function (gr) {
			_.invoke(this.buttons, 'paint', gr);
		}
		
		this.move = function (x, y) {
			var temp_btn = null;
			_.forEach(this.buttons, function (item, i) {
				if (item.trace(x, y)) {
					temp_btn = i;
				}
			});
			if (this.btn == temp_btn) {
				return this.btn;
			}
			if (this.btn) {
				this.buttons[this.btn].cs('normal');
			}
			if (temp_btn) {
				this.buttons[temp_btn].cs('hover');
			} else {
				_.tt('');
			}
			this.btn = temp_btn;
			return this.btn;
		}
		
		this.leave = function () {
			if (this.btn) {
				_.tt('');
				this.buttons[this.btn].cs('normal');
			}
			this.btn = null;
		}
		
		this.lbtn_up = function (x, y, mask) {
			if (this.btn) {
				this.buttons[this.btn].lbtn_up(x, y, mask);
				return true;
			} else {
				return false;
			}
		}
		
		this.buttons = {};
		this.btn = null;
	},
	cc : function (name) {
		return utils.CheckComponent(name, true);
	},
	chrToImg : function (chr, colour) {
		var size = 96;
		var temp_bmp = gdi.CreateImage(size, size);
		var temp_gr = temp_bmp.GetGraphics();
		temp_gr.SetTextRenderingHint(4);
		temp_gr.DrawString(chr, fontawesome, colour, 0, 0, size, size, SF_CENTRE);
		temp_bmp.ReleaseGraphics(temp_gr);
		temp_gr = null;
		return temp_bmp;
	},
	createFolder : function (folder) {
		if (!_.isFolder(folder)) {
			fso.CreateFolder(folder);
		}
	},
	deleteFile : function (file) {
		if (_.isFile(file)) {
			try {
				fso.DeleteFile(file);
			} catch (e) {
			}
		}
	},
	dispose : function () {
		_.forEach(arguments, function (item) {
			if (item) {
				item.Dispose();
			}
		});
	},
	drawImage : function (gr, img, src_x, src_y, src_w, src_h, aspect, border, alpha) {
		if (!img) {
			return [];
		}
		gr.SetInterpolationMode(7);
		switch (aspect) {
		case image.crop:
		case image.crop_top:
			if (img.Width / img.Height < src_w / src_h) {
				var dst_w = img.Width;
				var dst_h = Math.round(src_h * img.Width / src_w);
				var dst_x = 0;
				var dst_y = Math.round((img.Height - dst_h) / (aspect == image.crop_top ? 4 : 2));
			} else {
				var dst_w = Math.round(src_w * img.Height / src_h);
				var dst_h = img.Height;
				var dst_x = Math.round((img.Width - dst_w) / 2);
				var dst_y = 0;
			}
			gr.DrawImage(img, src_x, src_y, src_w, src_h, dst_x + 3, dst_y + 3, dst_w - 6, dst_h - 6, 0, alpha || 255);
			break;
		case image.stretch:
			gr.DrawImage(img, src_x, src_y, src_w, src_h, 0, 0, img.Width, img.Height, 0, alpha || 255);
			break;
		case image.centre:
		default:
			var s = Math.min(src_w / img.Width, src_h / img.Height);
			var w = Math.floor(img.Width * s);
			var h = Math.floor(img.Height * s);
			src_x += Math.round((src_w - w) / 2);
			src_y += Math.round((src_h - h) / 2);
			src_w = w;
			src_h = h;
			var dst_x = 0;
			var dst_y = 0;
			var dst_w = img.Width;
			var dst_h = img.Height;
			gr.DrawImage(img, src_x, src_y, src_w, src_h, dst_x, dst_y, dst_w, dst_h, 0, alpha || 255);
			break;
		}
		if (border) {
			gr.DrawRect(src_x, src_y, src_w - 1, src_h - 1, 1, border);
		}
		return [src_x, src_y, src_w, src_h];
	},
	drawOverlay : function (gr, x, y, w, h) {
		gr.FillGradRect(x, y, w, h, 90, _.RGBA(0, 0, 0, 230), _.RGBA(0, 0, 0, 200));
	},
	explorer : function (file) {
		if (_.isFile(file)) {
			WshShell.Run('explorer /select,' + _.q(file));
		}
	},
	fbDate : function (t) {
		var offset = new Date().getTimezoneOffset() * 60;
		if (typeof t == 'number') {
			t -= offset;
			var tmp = new Date(t * 1000).toISOString(); // ES5 only
			return tmp.substring(0, 10) + ' ' + tmp.substring(11, 19);
		} else {
			var tmp = new Date(t.substring(0, 10) + "T" + t.substring(11, 19) + "Z");
			return (Date.parse(tmp) / 1000) + offset;
		}
	},
	fbEscape : function (value) {
		return value.replace(/'/g, "''").replace(/[\(\)\[\],$]/g, "'$&'");
	},
	fbSanitise : function (value) {
		return value.replace(/[\/\\|:]/g, '-').replace(/\*/g, 'x').replace(/"/g, "''").replace(/[<>]/g, '_').replace(/\?/g, '').replace(/(?! )\s/g, '');
	},
	fileExpired : function (file, period) {
		return _.now() - _.lastModified(file) > period;
	},
	formatNumber : function (number, separator) {
		return number.replace(/\B(?=(\d{3})+(?!\d))/g, separator);
	},
	gdiFont : function (name, size, style) {
		return gdi.Font(name, _.scale(size), style);
	},
	getClipboardData : function () {
		return doc.parentWindow.clipboardData.getData('Text');
	},
	getElementsByTagName : function (value, tag) {
		doc.open();
		var div = doc.createElement('div');
		div.innerHTML = value;
		var data = div.getElementsByTagName(tag);
		doc.close();
		return data;
	},
	getFiles : function (folder, exts, newest_first) {
		var files = [];
		if (_.isFolder(folder)) {
			var e = new Enumerator(fso.GetFolder(folder).Files);
			for (; !e.atEnd(); e.moveNext()) {
				var path = e.item().Path;
				if (exts.toLowerCase().indexOf(path.split('.').pop().toLowerCase()) > -1) {
					files.push(path);
				}
			}
		}
		if (newest_first) {
			return _.sortByOrder(files, function (item) {
				return _.lastModified(item);
			}, 'desc');
		} else {
			files.srt();
			return files;
		}
	},
	hacks : function () {
		this.disable = function () {
			this.uih.MainMenuState = this.MainMenuState.Show;
			this.uih.FrameStyle = this.FrameStyle.Default;
			this.uih.StatusBarState = true;
		}
		
		this.enable = function () {
			this.uih.MainMenuState = this.MainMenuState.Hide;
			this.uih.FrameStyle = this.FrameStyle.NoBorder;
			this.uih.StatusBarState = false;
		}
		
		this.set_caption = function (x, y, w, h) {
			this.uih.SetPseudoCaption(x, y, w, h);
		}
		
		this.MainMenuState = { Show : 0, Hide : 1, Auto : 2 };
		this.FrameStyle = { Default : 0, SmallCaption : 1, NoCaption : 2, NoBorder : 3 };
		this.MoveStyle = { Default : 0, Middle : 1, Left : 2, Both : 3 };
		
		this.uih = new ActiveXObject('UIHacks');
		this.uih.MoveStyle = this.MoveStyle.Default;
		this.uih.DisableSizing = false;
		this.uih.BlockMaximize = false;
		this.uih.MinSize = false;
		this.uih.MaxSize = false;
	},
	help : function (x, y, flags) {
		var m = window.CreatePopupMenu();
		_.forEach(ha_links, function (item, i) {
			m.AppendMenuItem(MF_STRING, i + 100, item[0]);
			if (i == 1) {
				m.AppendMenuSeparator();
			}
		});
		m.AppendMenuSeparator();
		m.AppendMenuItem(MF_STRING, 1, 'Configure...');
		var idx = m.TrackPopupMenu(x, y, flags);
		switch (true) {
		case idx == 0:
			break;
		case idx == 1:
			window.ShowConfigure();
			break;
		default:
			_.run(ha_links[idx - 100][1]);
			break;
		}
		_.dispose(m);
	},
	img : function (value) {
		if (_.isFile(value)) {
			return gdi.Image(value);
		} else {
			return gdi.Image(folders.images + value);
		}
	},
	input : function (prompt, title, value) {
		var p = prompt.toString().replace(/"/g, _.q(' + Chr(34) + ')).replace(/\n/g, _.q(' + Chr(13) + '));
		var t = title.toString().replace(/"/g, _.q(' + Chr(34) + '));
		var v = value.toString().replace(/"/g, _.q(' + Chr(34) + '));
		var tmp = vb.eval('InputBox(' + _.q(p) + ', ' + _.q(t) + ', ' + _.q(v) + ')');
		return _.isString(tmp) ? _.trim(tmp) : value;
	},
	isFile : function (file) {
		return _.isString(file) ? fso.FileExists(file) : false;
	},
	isFolder : function (folder) {
		return _.isString(folder) ? fso.FolderExists(folder) : false;
	},
	isUUID : function (value) {
		var re = /^[0-9a-f]{8}-[0-9a-f]{4}-[345][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/
		return re.test(value);
	},
	jsonParse : function (value) {
		try {
			var data = JSON.parse(value);
			return data;
		} catch (e) {
			return [];
		}
	},
	jsonParseFile : function (file) {
		return _.jsonParse(_.open(file));
	},
	lastModified : function (file) {
		return Date.parse(fso.Getfile(file).DateLastModified);
	},
	lineWrap : function (value, font, width) {
		var temp_bmp = gdi.CreateImage(1, 1);
		var temp_gr = temp_bmp.GetGraphics();
		var result = [];
		_.forEach(value.split('\n'), function (paragraph) {
			var lines = _.filter(temp_gr.EstimateLineWrap(paragraph, font, width).toArray(), function (item, i) {
				return i % 2 == 0;
			});
			result.push.apply(result, _.map(lines, _.trim));
		});
		temp_bmp.ReleaseGraphics(temp_gr);
		_.dispose(temp_bmp);
		temp_gr = null;
		temp_bmp = null;
		return result;
	},
	lockSize : function (w, h) {
		window.MinWidth = window.MaxWidth = w;
		window.MinHeight = window.MaxHeight = h;
	},
	menu : function (x, y, flags) {
		var m1 = window.CreatePopupMenu();
		var s1 = window.CreatePopupMenu();
		var s2 = window.CreatePopupMenu();
		var s3 = window.CreatePopupMenu();
		var s4 = window.CreatePopupMenu();
		var s5 = window.CreatePopupMenu();
		var s6 = window.CreatePopupMenu();
		var mm1 = fb.CreateMainMenuManager();
		var mm2 = fb.CreateMainMenuManager();
		var mm3 = fb.CreateMainMenuManager();
		var mm4 = fb.CreateMainMenuManager();
		var mm5 = fb.CreateMainMenuManager();
		var mm6 = fb.CreateMainMenuManager();
		mm1.Init('File');
		mm2.Init('Edit');
		mm3.Init('View');
		mm4.Init('Playback');
		mm5.Init('Library');
		mm6.Init('Help');
		mm1.BuildMenu(s1, 1000, 999);
		mm2.BuildMenu(s2, 2000, 999);
		mm3.BuildMenu(s3, 3000, 999);
		mm4.BuildMenu(s4, 4000, 999);
		mm5.BuildMenu(s5, 5000, 999);
		mm6.BuildMenu(s6, 6000, 999);
		s1.AppendTo(m1, MF_STRING, 'File');
		s2.AppendTo(m1, MF_STRING, 'Edit');
		s3.AppendTo(m1, MF_STRING, 'View');
		s4.AppendTo(m1, MF_STRING, 'Playback');
		s5.AppendTo(m1, MF_STRING, 'Library');
		s6.AppendTo(m1, MF_STRING, 'Help');
		var idx = m1.TrackPopupMenu(x, y, flags);
		switch (true) {
		case idx == 0:
			break;
		case idx < 2000:
			mm1.ExecuteByID(idx - 1000);
			break;
		case idx < 3000:
			mm2.ExecuteByID(idx - 2000);
			break;
		case idx < 4000:
			mm3.ExecuteByID(idx - 3000);
			break;
		case idx < 5000:
			mm4.ExecuteByID(idx - 4000);
			break;
		case idx < 6000:
			mm5.ExecuteByID(idx - 5000);
			break;
		case idx < 7000:
			mm6.ExecuteByID(idx - 6000);
			break;
		}
		_.dispose(m1, s1 ,s2, s3, s4, s5, s6, mm1, mm2, mm3, mm4, mm5, mm6);
	},
	nest : function (collection, keys) {
		if (!keys.length) {
			return collection;
		} else {
			return _(collection)
				.groupBy(keys[0])
				.mapValues(function (values) {
					return _.nest(values, keys.slice(1));
				})
				.value();
		}
	},
	open : function (file) {
		return utils.ReadTextFile(file);
	},
	p : function (property, default_) {
		this.set = function (value) {
			this.value = value;
			window.SetProperty(this.property, this.value);
		}
		
		this.toggle = function () {
			this.enabled = !this.enabled;
			window.SetProperty(this.property, this.enabled);
		}
		
		this.property = property;
		this.default_ = default_;
		if (_.isBoolean(this.default_)) {
			this.enabled = window.GetProperty(this.property, this.default_);
		} else {
			this.value = window.GetProperty(this.property, this.default_);
		}
	},
	q : function (value) {
		return '"' + value + '"';
	},
	recycleFile : function (file) {
		if (_.isFile(file)) {
			app.Namespace(10).MoveHere(file);
		}
	},
	RGB : function (r, g, b) {
		return 0xFF000000 | r << 16 | g << 8 | b;
	},
	RGBA : function (r, g, b, a) {
		return a << 24 | r << 16 | g << 8 | b;
	},
	run : function () {
		try {
			WshShell.Run(_.map(arguments, _.q).join(' '));
			return true;
		} catch (e) {
			return false;
		}
	},
	runCmd : function (command, wait) {
		try {
			WshShell.Run(command, 0, wait);
		} catch (e) {
		}
	},
	save : function (file, value) {
		if (_.isFolder(utils.FileTest(file, 'split').toArray()[0]) && utils.WriteTextFile(file, value)) {
			return true;
		}
		console.log('Error saving to ' + file);
		return false;
	},
	sb : function (t, x, y, w, h, v, fn) {
		this.paint = function (gr, colour) {
			gr.SetTextRenderingHint(4);
			if (this.v()) {
				gr.DrawString(this.t, this.font, colour, this.x, this.y, this.w, this.h, SF_CENTRE);
			}
		}
		
		this.trace = function (x, y) {
			return x > this.x && x < this.x + this.w && y > this.y && y < this.y + this.h && this.v();
		}
		
		this.move = function (x, y) {
			if (this.trace(x, y)) {
				window.SetCursor(IDC_HAND);
				return true;
			} else {
				//window.SetCursor(IDC_ARROW);
				return false;
			}
		}
		
		this.lbtn_up = function (x, y) {
			if (this.trace(x, y)) {
				if (this.fn) {
					this.fn(x, y);
				}
				return true;
			} else {
				return false;
			}
		}
		
		this.t = t;
		this.x = x;
		this.y = y;
		this.w = w;
		this.h = h;
		this.v = v;
		this.fn = fn;
		this.font = gdi.Font('FontAwesome', this.h);
	},
	setClipboardData : function (value) {
		doc.parentWindow.clipboardData.setData('Text', value.toString());
	},
	scale : function (size) {
		return Math.round(size * DPI / 72);
	},
	shortPath : function (file) {
		return fso.GetFile(file).ShortPath;
	},
	splitRGB : function (c) {
		var tmp = c.split('-');
		if (tmp.length == 4) {
			return _.RGBA(tmp[0], tmp[1], tmp[2], tmp[3]);
		} else {
			return _.RGB(tmp[0], tmp[1], tmp[2]);
		}
	},
	stripTags : function (value) {
		doc.open();
		var div = doc.createElement('div');
		div.innerHTML = value.toString().replace(/<[Pp][^>]*>/g, '').replace(/<\/[Pp]>/g, '<br>').replace(/\n/g, '<br>');
		var tmp = _.trim(div.innerText);
		doc.close();
		return tmp;
	},
	tagged : function (value) {
		return value != '' && value != '?';
	},
	textWidth : function (value, font) {
		var temp_bmp = gdi.CreateImage(1, 1);
		var temp_gr = temp_bmp.GetGraphics();
		var width = temp_gr.CalcTextWidth(value, font);
		temp_bmp.ReleaseGraphics(temp_gr);
		_.dispose(temp_bmp);
		temp_gr = null;
		temp_bmp = null;
		return width;
	},
	tf : function (t, metadb) {
		if (!metadb) {
			return '';
		}
		var tfo = fb.TitleFormat(t);
		var str = tfo.EvalWithMetadb(metadb);
		_.dispose(tfo);
		return str;
	},
	tfe : function (t, force) {
		var tfo = fb.TitleFormat(t);
		var str = tfo.Eval(force);
		_.dispose(tfo);
		return str;
	},
	toRGB : function (a) {
		var b = a - 0xFF000000;
		return [b >> 16, b >> 8 & 0xFF, b & 0xFF];
	},
	ts : function () {
		return Math.floor(_.now() / 1000);
	},
	tt : function (value) {
		if (tooltip.Text != value) {
			tooltip.Text = value;
			tooltip.Activate();
		}
	}
});

var doc = new ActiveXObject('htmlfile');
var app = new ActiveXObject('Shell.Application');
var WshShell = new ActiveXObject('WScript.Shell');
var fso = new ActiveXObject('Scripting.FileSystemObject');
var vb = new ActiveXObject('ScriptControl');
vb.Language = 'VBScript';

var DT_LEFT = 0x00000000;
var DT_CENTER = 0x00000001;
var DT_RIGHT = 0x00000002;
var DT_VCENTER = 0x00000004;
var DT_WORDBREAK = 0x00000010;
var DT_CALCRECT = 0x00000400;
var DT_NOPREFIX = 0x00000800;
var DT_END_ELLIPSIS = 0x00008000;

var LEFT = DT_VCENTER | DT_END_ELLIPSIS | DT_CALCRECT | DT_NOPREFIX;
var RIGHT = DT_VCENTER | DT_RIGHT | DT_END_ELLIPSIS | DT_CALCRECT | DT_NOPREFIX;
var CENTRE = DT_VCENTER | DT_CENTER | DT_END_ELLIPSIS | DT_CALCRECT | DT_NOPREFIX;
var SF_CENTRE = 285212672;

var VK_ESCAPE = 0x1B;
var VK_SHIFT = 0x10;
var VK_LEFT = 0x25;
var VK_UP = 0x26;
var VK_RIGHT = 0x27;
var VK_DOWN = 0x28;

var MF_STRING = 0x00000000;
var MF_GRAYED = 0x00000001;

var IDC_ARROW = 32512;
var IDC_HAND = 32649;

var TPM_RIGHTALIGN = 0x0008;
var TPM_BOTTOMALIGN = 0x0020;

var DLGC_WANTALLKEYS = 0x0004;

var ONE_DAY = 86400000;
var ONE_WEEK = 604800000;

var DEFAULT_ARTIST = '$meta(artist,0)';
var N = window.Name + ':';

try {
	var DPI = WshShell.RegRead('HKCU\\Control Panel\\Desktop\\WindowMetrics\\AppliedDPI');
} catch (e) {
	var DPI = 96;
}

var LM = _.scale(5);
var TM = _.scale(20);

var tooltip = window.CreateTooltip('Segoe UI', _.scale(12));
tooltip.SetMaxWidth(1200);

var folders = {};
folders.home = fb.ComponentPath + 'samples\\complete\\';
folders.images = folders.home + 'images\\';
folders.data = fb.ProfilePath + 'js_data\\';
folders.artists = folders.data + 'artists\\';
folders.lastfm = folders.data + 'lastfm\\';

var fontawesome = gdi.Font('FontAwesome', 48);
var chars = {
	up : '\uF077',
	down : '\uF078',
	close : '\uF00D',
	rating_on : '\uF005',
	rating_off : '\uF006',
	heart_on : '\uF004',
	heart_off : '\uF08A',
	prev : '\uF049',
	next : '\uF050',
	play : '\uF04B',
	pause : '\uF04C',
	stop : '\uF04D',
	preferences : '\uF013',
	search : '\uF002',
	console : '\uF120',
	info : '\uF05A',
	audioscrobbler : '\uF202',
	minus : '\uF068',
	music : '\uF001',
	menu : '\uF0C9'
};

var popup = {
	ok : 0,
	yes_no : 4,
	yes : 6,
	no : 7,
	stop : 16,
	question : 32,
	info : 64
};

var image = {
	crop : 0,
	crop_top : 1,
	stretch : 2,
	centre : 3
};

var ha_links = [
	['Title Formatting Reference', 'http://wiki.hydrogenaud.io/index.php?title=Foobar2000:Title_Formatting_Reference'],
	['Query Syntax', 'http://wiki.hydrogenaud.io/index.php?title=Foobar2000:Query_syntax'],
	['Homepage', 'https://www.foobar2000.org/'],
	['Components', 'https://www.foobar2000.org/components'],
	['Wiki', 'http://wiki.hydrogenaud.io/index.php?title=Foobar2000:Foobar2000'],
	['Forums', 'https://hydrogenaud.io/index.php/board,28.0.html']
];

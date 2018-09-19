Array.prototype.srt=function(){for(var z=0,t;t=this[z];z++){this[z]=[];var x=0,y=-1,n=true,i,j;while(i=(j=t.charAt(x++)).charCodeAt(0)){var m=(i==46||(i>=48&&i<=57));if(m!==n){this[z][++y]='';n=m;}
this[z][y]+=j;}}
this.sort(function(a,b){for(var x=0,aa,bb;(aa=a[x])&&(bb=b[x]);x++){aa=aa.toLowerCase();bb=bb.toLowerCase();if(aa!==bb){var c=Number(aa),d=Number(bb);if(c==aa&&d==bb){return c-d;}else return(aa>bb)?1:-1;}}
return a.length-b.length;});for(var z=0;z<this.length;z++)
this[z]=this[z].join('');};

//<editor-fold desc="timeout and interval shims">

/**
 * @param {function()} func
 * @param {number} wait
 * @return {number}
 */
function setInterval(func, wait){
    return window.SetInterval(func, wait);
}
/**
 * @param {number} id
 */
function clearInterval(id) {
    window.ClearInterval(id);
}
/**
 * @param {function()} func
 * @param {number} wait
 * @return {number}
 */
function setTimeout(func, wait){
    return window.SetTimeout(func, wait);
}
/**
 * @param {number} id
 */
function clearTimeout(id) {
    window.ClearTimeout(id);
}

//</editor-fold>

g_callbacks = {
    /**
     * @param {string} event_name
     * @param {...*} var_args
     */
    invoke:                 function (event_name, var_args) {
        this.validate_event_name(event_name);

        var callbacks = this[event_name];
        if (!callbacks || !_.isArray(callbacks)) {
            return;
        }

        var args = _.drop([].slice.call(arguments));
        _.over(callbacks)(args);
    },
    register:               function (event_name, callback) {
        if (!_.isFunction(callback)) {
            throw Error('Type Error: callback is not a function');
        }

        this.validate_event_name(event_name);

        if (!this[event_name]) {
            this[event_name] = [];
        }
        this[event_name].push(callback);
    },
    validate_event_name: function (event_name) {
        if (!_.isString(event_name)) {
            throw Error('Type Error: event name is not a string');
        }

        if (event_name === 'invoke'
            || event_name === 'register'
            || event_name === 'unregister') {
            throw Error('Argument Error: event name is occupied "' + event_name + '"');
        }
    }
};

function on_script_unload() {
    g_callbacks.invoke('on_script_unload');
}

_.mixin({
    alpha_timer:          function (items_arg, hover_predicate_arg) {
        this.start = function () {
            var hover_in_step = 50;
            var hover_out_step = 15;

            if (!alpha_timer_internal) {
                alpha_timer_internal = window.SetInterval(_.bind(function () {
                    _.forEach(items, function (item) {
                        var saved_alpha = item.hover_alpha;
                        if (hover_predicate(item)) {
                            item.hover_alpha = Math.min(255, item.hover_alpha += hover_in_step);
                        }
                        else {
                            item.hover_alpha = Math.max(0, item.hover_alpha -= hover_out_step);
                        }

                        if (saved_alpha !== item.hover_alpha) {
                            item.repaint();
                        }
                    });

                    var alpha_in_progress = _.some(items, function (item) {
                        return item.hover_alpha > 0 && item.hover_alpha < 255;
                    });

                    if (!alpha_in_progress) {
                        this.stop();
                    }
                }, this), 25);
            }
        };

        this.stop = function () {
            if (alpha_timer_internal) {
                window.ClearInterval(alpha_timer_internal);
                alpha_timer_internal = null;
            }
        };

        var alpha_timer_internal = null;
        var items = items_arg;
        var hover_predicate = hover_predicate_arg;
    },
    artistFolder:         function (artist) {
        var a = _.fbSanitise(artist);
        var folder = folders.artists + a;
        if (_.isFolder(folder)) {
            return fso.GetFolder(folder) + '\\';
        }
        else {
            folder = folders.artists + _.truncate(a, {
                'length': 64
            });
            _.createFolder(folder);
            return fso.GetFolder(folder) + '\\';
        }
    },
    blendColours:         function (c1, c2, f) {
        c1 = _.toRGB(c1);
        c2 = _.toRGB(c2);
        var r = Math.round(c1[0] + f * (c2[0] - c1[0]));
        var g = Math.round(c1[1] + f * (c2[1] - c1[1]));
        var b = Math.round(c1[2] + f * (c2[2] - c1[2]));
        return _.RGB(r, g, b);
    },
    /** @constructor */
    button:               function (x, y, w, h, img_src, fn, tiptext) {
        this.paint = function (gr, alpha) {
            if (this.state !== "pressed") {
                var hoverAlpha = !_.isNil(alpha) ? Math.min(this.hover_alpha, alpha) : this.hover_alpha;
                if (this.img_normal) {
                    _.drawImage(gr, this.img_normal, this.x, this.y, this.w, this.h, undefined, undefined, alpha);
                }
                if (this.img_hover) {
                    _.drawImage(gr, this.img_hover, this.x, this.y, this.w, this.h, undefined, undefined, hoverAlpha);
                }
            }
            else {
                if (this.img_pressed) {
                    _.drawImage(gr, this.img_pressed, this.x, this.y, this.w, this.h, undefined, undefined, alpha);
                }
            }
        };

        this.repaint = function () {
            var expXY = 2,
                expWH = expXY * 2;

            window.RepaintRect(this.x - expXY, this.y - expXY, this.w + expWH, this.h + expWH);
        };

        this.trace = function (x, y) {
            return x > this.x && x < this.x + this.w && y > this.y && y < this.y + this.h;
        };

        this.lbtn_up = function (x, y) {
            if (this.fn) {
                this.fn(x, y, this.x, this.y, this.h, this.w);
            }
        };

        this.cs = function (s) {
            this.state = s;
            if (s === "pressed" || s === "normal") {
                this.tt.clear();
            }
            this.repaint();
        };

        this.set_image = function (img_src) {
            this.img_normal = _.isString(img_src.normal) ? _.img(img_src.normal) : img_src.normal;
            this.img_hover = img_src.hover ? (_.isString(img_src.hover) ? _.img(img_src.hover) : img_src.hover) : this.img_normal;
            this.img_pressed = img_src.pressed ? (_.isString(img_src.pressed) ? _.img(img_src.pressed) : img_src.pressed) : this.img_normal;
        };

        this.x = x;
        this.y = y;
        this.w = w;
        this.h = h;
        this.fn = fn;
        this.tt = new _.tt_handler;
        this.tiptext = tiptext;
        this.img_normal = undefined;
        this.img_hover = undefined;
        this.img_pressed = undefined;
        this.hover_alpha = 0;
        this.state = "normal";
        this.hide = false;

        this.set_image(img_src);
    },
    /** @constructor */
    buttons:              function () {
        this.reset = function () {
            alpha_timer.stop();
        };

        this.paint = function (gr, alpha) {
            _.forEach(this.buttons, function (item) {
                if (!item.hide) {
                    item.paint(gr, alpha);
                }
            });
        };

        this.move = function (x, y) {
            var hover_btn = _.find(this.buttons, function (item) {
                return item.trace(x, y);
            });

            if (hover_btn && hover_btn.hide) {// Button is hidden, ignore
                if (cur_btn) {
                    cur_btn.cs("normal");
                    alpha_timer.start();
                }
                cur_btn = null;
                return null;
            }

            if (cur_btn === hover_btn) {// Same button
                return cur_btn;
            }

            if (cur_btn) {// Return prev button to normal state
                cur_btn.cs("normal");
                alpha_timer.start();
            }

            if (hover_btn) {// Select current button
                hover_btn.cs("hover");
                if (this.show_tt) {
                    hover_btn.tt.showDelayed(hover_btn.tiptext);
                }
                alpha_timer.start();
            }

            cur_btn = hover_btn;
            return cur_btn;
        };

        this.leave = function () {
            if (cur_btn) {
                cur_btn.cs("normal");
                if (!cur_btn.hide) {
                    alpha_timer.start();
                }
            }
            cur_btn = null;
        };

        this.lbtn_down = function (x, y) {
            if (!cur_btn) {
                // Needed when pressing on button with context menu open
                this.move(x, y);
            }

            if (!cur_btn || cur_btn.hide) {
                return false;
            }

            cur_btn.cs("pressed");
            return true;
        };

        this.lbtn_up = function (x, y) {
            if (!cur_btn || cur_btn.hide || cur_btn.state !== "pressed") {
                return false;
            }

            if (cur_btn.trace(x, y)) {
                cur_btn.cs("hover");
            }
            cur_btn.lbtn_up(x, y);

            return true;
        };

        this.buttons = {};
        this.show_tt = false;

        var that = this;

        var cur_btn = null;
        var alpha_timer = new _.alpha_timer(that.buttons, function (item) {
            return item.state !== 'normal';
        });
    },
    cc:                   function (name) {
        return utils.CheckComponent(name, true);
    },
    chrToImg:             function (chr, colour) {
        var size = 96;
        var temp_bmp = gdi.CreateImage(size, size);
        var temp_gr = temp_bmp.GetGraphics();
        temp_gr.SetTextRenderingHint(4);
        temp_gr.DrawString(chr, fontawesome, colour, 0, 0, size, size, SF_CENTRE);
        temp_bmp.ReleaseGraphics(temp_gr);
        temp_gr = null;
        return temp_bmp;
    },
    count:                function (collection, predicate) {
        var count = 0;
        collection.forEach(function (item) {
            if (predicate(item)) {
                ++count;
            }
        });

        return count;
    },
    createFolder:         function (folder) {
        if (!_.isFolder(folder)) {
            fso.CreateFolder(folder);
        }
    },
    deleteFile:           function (file) {
        if (_.isFile(file)) {
            try {
                fso.DeleteFile(file);
            } catch (e) {
            }
        }
    },
    drawImage:            function (gr, img, src_x, src_y, src_w, src_h, aspect, border, alpha) {
        if (!img) {
            return [];
        }
        gr.SetInterpolationMode(InterpolationMode.HighQualityBicubic);

        var dst_w = 0;
        var dst_h = 0;
        var dst_x = 0;
        var dst_y = 0;
        switch (aspect) {
            case image.crop:
            case image.crop_top:
                if (img.Width / img.Height < src_w / src_h) {
                    dst_w = img.Width;
                    dst_h = Math.round(src_h * img.Width / src_w);
                    dst_x = 0;
                    dst_y = Math.round((img.Height - dst_h) / (aspect === image.crop_top ? 4 : 2));
                }
                else {
                    dst_w = Math.round(src_w * img.Height / src_h);
                    dst_h = img.Height;
                    dst_x = Math.round((img.Width - dst_w) / 2);
                    dst_y = 0;
                }
                break;
            case image.stretch:
                dst_x = 0;
                dst_y = 0;
                dst_w = img.Width;
                dst_h = img.Height;
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

                dst_x = 0;
                dst_y = 0;
                dst_w = img.Width;
                dst_h = img.Height;
                break;
        }
        if (_.isNil(aspect)) {
            gr.DrawImage(img, src_x, src_y, src_w, src_h, dst_x, dst_y, dst_w, dst_h, 0, _.isNil(alpha) ? 255 : alpha);
        }
        else {
            gr.DrawImage(img, src_x, src_y, src_w, src_h, dst_x + 5, dst_y + 5, dst_w - 10, dst_h - 10, 0, _.isNil(alpha) ? 255 : alpha);
        }
        if (border) {
            gr.DrawRect(src_x, src_y, src_w - 1, src_h - 1, 1, border);
        }
        return [src_x, src_y, src_w, src_h];
    },
    drawOverlay:          function (gr, x, y, w, h) {
        gr.FillGradRect(x, y, w, h, 90, _.RGBA(0, 0, 0, 230), _.RGBA(0, 0, 0, 200));
    },
    explorer:             function (file) {
        if (_.isFile(file)) {
            WshShell.Run('explorer /select,' + _.q(file));
        }
    },
    fbDate:               function (ts) {
        // ES5 only
        var tmp = new Date(ts * 1000).toISOString();
        return tmp.substring(0, 10) + ' ' + tmp.substring(11, 19);
    },
    fbEscape:             function (value) {
        return value.replace(/'/g, "''").replace(/[()\[\],$]/g, "'$&'");
    },
    fbSanitise:           function (value) {
        return value.replace(/[\/\\|:]/g, '-').replace(/\*/g, 'x').replace(/"/g, "''").replace(/[<>]/g, '_').replace(/\?/g, '').replace(/(?! )\s/g, '');
    },
    fileExpired:          function (file, period) {
        return _.now() - _.lastModified(file) > period;
    },
    formatNumber:         function (number, separator) {
        return number.replace(/\B(?=(\d{3})+(?!\d))/g, separator);
    },
    gdiFont:              function (name, size, style) {
        return gdi.Font(name, _.scale(size), style);
    },
    getClipboardData:     function () {
        return doc.parentWindow.clipboardData.getData("Text");
    },
    getElementsByTagName: function (value, tag) {
        doc.open();
        var div = doc.createElement('div');
        div.innerHTML = value;
        var data = div.getElementsByTagName(tag);
        doc.close();
        return data;
    },
    getFiles:             function (folder, exts, newest_first) {
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
            return _.orderBy(files, function (item) {
                return _.lastModified(item);
            }, 'desc');
        }
        else {
            files.srt();
            return files;
        }
    },
    hacks:                function () {
        this.disable = function () {
            this.uih.MainMenuState = this.MainMenuState.Show;
            this.uih.FrameStyle = this.FrameStyle.Default;
            this.uih.StatusBarState = true;
        };

        this.enable = function () {
            this.uih.MainMenuState = this.MainMenuState.Hide;
            this.uih.FrameStyle = this.FrameStyle.NoBorder;
            this.uih.StatusBarState = false;
        };

        this.set_caption = function (x, y, w, h) {
            this.uih.SetPseudoCaption(x, y, w, h);
        };

        this.MainMenuState = {
            Show: 0,
            Hide: 1,
            Auto: 2
        };
        this.FrameStyle = {
            Default:      0,
            SmallCaption: 1,
            NoCaption:    2,
            NoBorder:     3
        };
        this.MoveStyle = {
            Default: 0,
            Middle:  1,
            Left:    2,
            Both:    3
        };

        this.uih = new ActiveXObject('UIHacks');
        this.uih.MoveStyle = this.MoveStyle.Default;
        this.uih.DisableSizing = false;
        this.uih.BlockMaximize = false;
        this.uih.MinSize = false;
        this.uih.MaxSize = false;
    },
    help:                 function (x, y, flags) {
        var m = window.CreatePopupMenu();
        _.forEach(ha_links, function (item, i) {
            m.AppendMenuItem(MF_STRING, i + 100, item[0]);
            if (i === 1) {
                m.AppendMenuSeparator();
            }
        });
        m.AppendMenuSeparator();
        m.AppendMenuItem(MF_STRING, 1, 'Configure...');
        var idx = m.TrackPopupMenu(x, y, flags);
        switch (true) {
            case idx === 0:
                break;
            case idx === 1:
                window.ShowConfigure();
                break;
            default:
                _.run(ha_links[idx - 100][1]);
                break;
        }
    },
    img:                  function (value) {
        if (_.isFile(value)) {
            return gdi.Image(value);
        }
        else {
            return gdi.Image(folders.images + value);
        }
    },
    isFile:               function (file) {
        return _.isString(file) ? fso.FileExists(file) : false;
    },
    isFolder:             function (folder) {
        return _.isString(folder) ? fso.FolderExists(folder) : false;
    },
    /**
     * @param a
     * @param b
     * @return {boolean} a instanceof b
     */
    isInstanceOf:         function (a, b) {
        return (a instanceof b);
    },
    isUUID:               function (value) {
        var re = /^[0-9a-f]{8}-[0-9a-f]{4}-[345][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/;
        return re.test(value);
    },
    jsonParse:            function (value) {
        try {
            return JSON.parse(value);
        } catch (e) {
            return [];
        }
    },
    jsonParseFile:        function (file) {
        return _.jsonParse(_.open(file));
    },
    lastModified:         function (file) {
        return Date.parse(fso.GetFile(file).DateLastModified);
    },
    lineWrap:             function (value, font, width) {
        var temp_bmp = gdi.CreateImage(1, 1);
        var temp_gr = temp_bmp.GetGraphics();
        var result = [];
        _.forEach(value.split('\n'), function (paragraph) {
            var lines = _.filter(temp_gr.EstimateLineWrap(paragraph, font, width), function (item, i) {
                return i % 2 === 0;
            });
            result.push.apply(result, _.map(lines, _.trim));
        });
        temp_bmp.ReleaseGraphics(temp_gr);
        temp_gr = null;
        temp_bmp = null;
        return result;
    },
    lockSize:             function (w, h) {
        window.MinWidth = window.MaxWidth = w;
        window.MinHeight = window.MaxHeight = h;
    },
    menu:                 function (x, y, flags) {
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
            case idx === 0:
                break;
            case idx === 1:
                fb.RunMainMenuCommand('View/Switch to UI/' + (window.InstanceType ? 'Columns UI' : 'Default User Interface'));
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
    },
    menu_item:            function (x, y, name, flags) {
        var menuManager = fb.CreateMainMenuManager();

        var menu = window.CreatePopupMenu();
        if (name) {

            menuManager.Init(name);
            menuManager.BuildMenu(menu, 1, 128);

            var idx = menu.TrackPopupMenu(x, y, flags);

            if (idx > 0) {
                menuManager.ExecuteByID(idx - 1);
            }
        }

    },
    nest:                 function (collection, keys) {
        if (!keys.length) {
            return collection;
        }
        else {
            return _(collection)
                .groupBy(keys[0])
                .mapValues(function (values) {
                    return _.nest(values, keys.slice(1));
                })
                .value();
        }
    },
    open:                 function (file) {
        return utils.ReadTextFile(file);
    },
    p:                    function (property, default_) {
        this.set = function (value) {
            this.value = value;
            window.SetProperty(this.property, this.value);
        };

        this.toggle = function () {
            this.enabled = !this.enabled;
            window.SetProperty(this.property, this.enabled);
        };

        this.property = property;
        this.default_ = default_;
        if (_.isBoolean(this.default_)) {
            this.enabled = window.GetProperty(this.property, this.default_);
        }
        else {
            this.value = window.GetProperty(this.property, this.default_);
        }
    },
    q:                    function (value) {
        return '"' + value + '"';
    },
    recycleFile:          function (file) {
        if (_.isFile(file)) {
            app.Namespace(10).MoveHere(file);
        }
    },
    /**
     * @param {number} r
     * @param {number} g
     * @param {number} b
     * @return {number}
     */
    RGB:                  function (r, g, b) {
        return 0xFF000000 | r << 16 | g << 8 | b;
    },
    /**
     * @param {number} r
     * @param {number} g
     * @param {number} b
     * @param {number} a
     * @return {number}
     */
    RGBA:                 function (r, g, b, a) {
        return a << 24 | r << 16 | g << 8 | b;
    },
    /**
     * @param {number} rgb
     * @param {number} a
     * @return {number}
     */
    RGBtoRGBA:            function (rgb, a) {
        return a << 24 | (rgb & 0x00FFFFFF);
    },
    run:                  function () {
        try {
            WshShell.Run(_.map(arguments, _.q).join(" "));
            return true;
        } catch (e) {
            return false;
        }
    },
    runCmd:               function (command, wait, show) {
        try {
            WshShell.Run(command, show ? 1 : 0, !_.isNil(wait) ? wait : false);
            return true;
        } catch (e) {
            return false;
        }
    },
    save:                 function (file, value) {
        if (_.isFolder(utils.FileTest(file, 'split').toArray()[0]) && utils.WriteTextFile(file, value)) {
            return true;
        }
        console.log('Error saving to ' + file);
        return false;
    },
    sb:                   function (t, x, y, w, h, v, fn) {
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
            }
            else {
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
            }
            else {
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
    setClipboardData:     function (value) {
        doc.parentWindow.clipboardData.setData('Text', value.toString());
    },
    scale:                function (size) {
        return Math.round(size * DPI / 72);
    },
    shortPath:            function (file) {
        return fso.GetFile(file).ShortPath;
    },
    splitRGB:             function (c) {
        var tmp = c.split('-');
        if (tmp.length === 4) {
            return _.RGBA(tmp[0], tmp[1], tmp[2], tmp[3]);
        }
        else {
            return _.RGB(tmp[0], tmp[1], tmp[2]);
        }
    },
    stripTags:            function (value) {
        doc.open();
        var div = doc.createElement('div');
        div.innerHTML = value.toString().replace(/<[Pp][^>]*>/g, '').replace(/<\/[Pp]>/g, '<br>').replace(/\n/g, '<br>');
        var tmp = _.trim(div.innerText);
        doc.close();
        return tmp;
    },
    tagged:               function (value) {
        return value !== '' && value !== '?';
    },
    textWidth:            function (value, font) {
        var temp_bmp = gdi.CreateImage(1, 1);
        var temp_gr = temp_bmp.GetGraphics();
        var width = temp_gr.CalcTextWidth(value, font);
        temp_bmp.ReleaseGraphics(temp_gr);
        temp_gr = null;
        temp_bmp = null;
        return width;
    },
    tf:                   function (t, metadb) {
        if (!metadb) {
            return '';
        }
        var tfo = fb.TitleFormat(t);
        var str = tfo.EvalWithMetadb(metadb);
        return str;
    },
    tfe:                  function (t, force) {
        var tfo = fb.TitleFormat(t);
        var str = tfo.Eval(force);
        return str;
    },
    toDb:                 function (volume) {
        return 50 * Math.log(0.99 * volume + 0.01) / Math.LN10;
    },
    toRGB:                function (a) {
        var b = a - 0xFF000000;
        return [b >> 16, b >> 8 & 0xFF, b & 0xFF];
    },
    toVolume:             function (db) {
        if (db === -100) {
            return 0;
        }

        return Math.ceil(Math.pow(10.0, (100 + db) / 50));
    },
    /**
     * Note: Mutates array argument
     *
     * @param {Array} array
     * @param {Number} count
     * @param {boolean} fromStart
     */
    trimArray:            function (array, count, fromStart) {
        if (array.length === count) {
            array.length = 0;
            return;
        }

        /// Length deduction is much faster then _.drop or slice, since it does not create a new array
        if (fromStart) {
            array.reverse();
            array.length -= count;
            array.reverse();
        }
        else {
            array.length -= count;
        }
    },
    ts:                   function () {
        return Math.floor(_.now() / 1000);
    },
    tt:                   function (value, force) {
        if (tooltip.Text !== _.toString(value) || force) {
            tooltip.Text = value;
            tooltip.Activate();
        }
    },
    /** @constructor */
    tt_handler:           function () {
        this.showDelayed = function (text) {
            tt_timer.start(this.id, text);
        };
        this.showImmediate = function (text) {
            tt_timer.stop(this.id);
            _.tt(text);
        };
        this.clear = function () {
            tt_timer.stop(this.id);
        };
        this.stop = function () {
            tt_timer.force_stop();
        };
        this.id = Math.ceil(Math.random().toFixed(8) * 1000);

        var tt_timer = _.tt_handler.tt_timer;
    }
});

_.tt_handler.tt_timer = new function () {
    var tooltip_timer;
    var tt_caller = undefined;

    this.start = function (id, text) {
        var old_caller = tt_caller;
        tt_caller = id;

        if (!tooltip_timer && tooltip.Text) {
            _.tt(text, old_caller !== tt_caller );
        }
        else {
            this.force_stop(); /// < There can be only one tooltip present at all times, so we can kill the timer w/o any worries

            if (!tooltip_timer) {
                tooltip_timer = window.SetTimeout(_.bind(function () {
                    _.tt(text);
                    tooltip_timer = null;
                }, this), 500);
            }
        }
    };

    this.stop = function (id) {
        if (tt_caller === id) {// Do not stop other callers
            this.force_stop();
        }
    };

    this.force_stop = function () {
        _.tt("");
        if (tooltip_timer) {
            window.ClearTimeout(tooltip_timer);
            tooltip_timer = null;
        }
    };
};

g_callbacks.register('on_script_unload', function(){_.tt('');});

var doc = new ActiveXObject('htmlfile');
var app = new ActiveXObject('Shell.Application');
var WshShell = new ActiveXObject('WScript.Shell');
var fso = new ActiveXObject('Scripting.FileSystemObject');

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

// Mask for mouse callbacks
var MK_LBUTTON = 0x0001;
var MK_RBUTTON = 0x0002;
var MK_SHIFT = 0x0004; // The SHIFT key is down.
var MK_CONTROL = 0x0008; // The CTRL key is down.
var MK_MBUTTON = 0x0010;
var MK_XBUTTON1 = 0x0020;
var MK_XBUTTON2 = 0x0040;

//--->
var VK_BACKSPACE = 0x08;
var VK_SHIFT = 0x10;
var VK_CONTROL = 0x11;
var VK_MENU = 0x12; // ALT
var VK_PAUSE = 0x13;
var VK_ESCAPE = 0x1B;
var VK_SPACE = 0x20;
var VK_DELETE = 0x2E;
var VK_PRIOR = 0x21; // PAGE UP key
var VK_NEXT = 0x22; // PAGE DOWN key
var VK_END = 0x23;
var VK_HOME = 0x24;
var VK_LEFT = 0x25;
var VK_UP = 0x26;
var VK_RIGHT = 0x27;
var VK_DOWN = 0x28;
var VK_RETURN = 0x0D; // Enter
var VK_LSHIFT = 0xA0; // Left SHIFT key
var VK_RSHIFT = 0xA1; // Right SHIFT key
var VK_LCONTROL = 0xA2; // Left CONTROL key
var VK_RCONTROL = 0xA3; // Right CONTROL key
var VK_LMENU = 0xA4; // Left MENU key (Left Alt)
var VK_RMENU = 0xA5; // Right MENU key (Right Alt)

var VK_KEY_0 = 0x30; //	0
var VK_KEY_1 = 0x31; //	1
var VK_KEY_2 = 0x32; //	2
var VK_KEY_3 = 0x33; //	3
var VK_KEY_4 = 0x34; //	4
var VK_KEY_5 = 0x35; //	5
var VK_KEY_6 = 0x36; //	6
var VK_KEY_7 = 0x37; //	7
var VK_KEY_8 = 0x38; //	8
var VK_KEY_9 = 0x39; //	9
var VK_KEY_A = 0x41; //	A
var VK_KEY_B = 0x42; //	B
var VK_KEY_C = 0x43; //	C
var VK_KEY_D = 0x44; //	D
var VK_KEY_E = 0x45; //	E
var VK_KEY_F = 0x46; //	F
var VK_KEY_G = 0x47; //	G
var VK_KEY_H = 0x48; //	H
var VK_KEY_I = 0x49; //	I
var VK_KEY_J = 0x4A; //	J
var VK_KEY_K = 0x4B; //	K
var VK_KEY_L = 0x4C; //	L
var VK_KEY_M = 0x4D; //	M
var VK_KEY_N = 0x4E; //	N
var VK_KEY_O = 0x4F; //	O
var VK_KEY_P = 0x50; //	P
var VK_KEY_Q = 0x51; //	Q
var VK_KEY_R = 0x52; //	R
var VK_KEY_S = 0x53; //	S
var VK_KEY_T = 0x54; //	T
var VK_KEY_U = 0x55; //	U
var VK_KEY_V = 0x56; //	V
var VK_KEY_W = 0x57; //	W
var VK_KEY_X = 0x58; //	X
var VK_KEY_Y = 0x59; //	Y
var VK_KEY_Z = 0x5A; //	Z

var VK_F1 = 0x70; // F1
var VK_F10 = 0x79; // F10
var VK_F11 = 0x7A; // F11
var VK_F12 = 0x7B; // F12
var VK_F13 = 0x7C; // F13
var VK_F14 = 0x7D; // F14
var VK_F15 = 0x7E; // F15
var VK_F16 = 0x7F; // F16
var VK_F17 = 0x80; // F17
var VK_F18 = 0x81; // F18
var VK_F19 = 0x82; // F19
var VK_F2 = 0x71; // F2
var VK_F20 = 0x83; // F20
var VK_F21 = 0x84; // F21
var VK_F22 = 0x85; // F22
var VK_F23 = 0x86; // F23
var VK_F24 = 0x87; // F24
var VK_F3 = 0x72; // F3
var VK_F4 = 0x73; // F4
var VK_F5 = 0x74; // F5
var VK_F6 = 0x75; // F6
var VK_F7 = 0x76; // F7
var VK_F8 = 0x77; // F8
var VK_F9 = 0x78; // F9
//--->

var IDC_ARROW = 32512;
var IDC_IBEAM = 32513;
var IDC_WAIT = 32514;
var IDC_CROSS = 32515;
var IDC_UPARROW = 32516;
var IDC_SIZE = 32640;
var IDC_ICON = 32641;
var IDC_SIZENWSE = 32642;
var IDC_SIZENESW = 32643;
var IDC_SIZEWE = 32644;
var IDC_SIZENS = 32645;
var IDC_SIZEALL = 32646;
var IDC_NO = 32648;
var IDC_APPSTARTING = 32650;
var IDC_HAND = 32649;
var IDC_HELP = 32651;

//--->
// Used in SetTextRenderingHint()
// For more information, see: http://msdn.microsoft.com/en-us/library/ms534404(VS.85).aspx
var TextRenderingHint =
    {
        SystemDefault:            0,
        SingleBitPerPixelGridFit: 1,
        SingleBitPerPixel:        2,
        AntiAliasGridFit:         3,
        AntiAlias:                4,
        ClearTypeGridFit:         5
    };
//--->
// Used in SetSmoothingMode()
// For more information, see: http://msdn.microsoft.com/en-us/library/ms534173(VS.85).aspx
var SmoothingMode =
    {
        Invalid:     -1,
        Default:     0,
        HighSpeed:   1,
        HighQuality: 2,
        None:        3,
        AntiAlias:   4
    };
//--->
// Used in SetInterpolationMode()
// For more information, see: http://msdn.microsoft.com/en-us/library/ms534141(VS.85).aspx
var InterpolationMode =
    {
        Invalid:             -1,
        Default:             0,
        LowQuality:          1,
        HighQuality:         2,
        Bilinear:            3,
        Bicubic:             4,
        NearestNeighbor:     5,
        HighQualityBilinear: 6,
        HighQualityBicubic:  7
    };

var MF_STRING = 0x00000000;
var MF_GRAYED = 0x00000001;

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
var TM = _.scale(16);

var tooltip = window.CreateTooltip('Segoe UI', _.scale(9));
tooltip.SetMaxWidth(1200);

var folders = {};
folders.home = fb.FoobarPath + "themes\\CaTRoX\\Scripts\\js_marc2003\\";
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
    ok:       0,
    yes_no:   4,
    yes:      6,
    no:       7,
    stop:     16,
    question: 32,
    info:     64
};

var image = {
    crop:     0,
    crop_top: 1,
    stretch:  2,
    centre:   3
};

var ha_links = [
    ['Title Formatting Reference', 'http://wiki.hydrogenaud.io/index.php?title=Foobar2000:Title_Formatting_Reference'],
    ['Query Syntax', 'http://wiki.hydrogenaud.io/index.php?title=Foobar2000:Query_syntax'],
    ['Homepage', 'https://www.foobar2000.org/'],
    ['Components', 'https://www.foobar2000.org/components'],
    ['Wiki', 'http://wiki.hydrogenaud.io/index.php?title=Foobar2000:Foobar2000'],
    ['Forums', 'https://hydrogenaud.io/index.php/board,28.0.html']
];

// Use with GdiDrawText() 
// {{
var DT_TOP = 0x00000000;
var DT_CENTER = 0x00000001;
var DT_VCENTER = 0x00000004;
var DT_WORDBREAK = 0x00000010;
var DT_CALCRECT = 0x00000400;
var DT_NOPREFIX = 0x00000800;
// }}

// {{
// Used in window.GetColorCUI()
ColorTypeCUI = {
    text: 0,
    selection_text: 1,
    inactive_selection_text: 2,
    background: 3,
    selection_background: 4,
    inactive_selection_background: 5,
    active_item_frame: 6
};

// Used in window.GetFontCUI()
FontTypeCUI = {
    items: 0,
    labels: 1
};

// Used in window.GetColorDUI()
ColorTypeDUI = {
    text: 0,
    background: 1,
    highlight: 2,
    selection: 3
};

// Used in window.GetFontDUI()
FontTypeDUI = {
    defaults: 0,
    tabs: 1,
    lists: 2,
    playlists: 3,
    statusbar: 4,
    console: 5
};
//}}

//{{
// Used in window.SetCursor()
var IDC_HAND = 32649;
//}}

var g_instancetype = window.InstanceType;
var g_font = null;
var g_text = "Create your script\nClick here to open the editor.";
var ww = 0, wh = 0;
var g_textcolor = 0, g_textcolor_hl = 0;
var g_backcolor = 0;
var g_hot = false;

function get_font() {
    if (g_instancetype == 0) { // CUI
        g_font = window.GetFontCUI(FontTypeCUI.items);
    } else if (g_instancetype == 1) { // DUI
        g_font = window.GetFontDUI(FontTypeDUI.defaults);
    } else {
        // None
    }
}
get_font();

function get_colors() {
    if (g_instancetype == 0) { // CUI
        g_textcolor = window.GetColorCUI(ColorTypeCUI.text);
        g_textcolor_hl = window.GetColorCUI(ColorTypeCUI.text);
        g_backcolor = window.GetColorCUI(ColorTypeCUI.background);
    } else if (g_instancetype == 1) { // DUI
        g_textcolor = window.GetColorDUI(ColorTypeDUI.text);
        g_textcolor_hl = window.GetColorDUI(ColorTypeDUI.highlight);
        g_backcolor = window.GetColorDUI(ColorTypeDUI.background);
    } else {
        // None
    }
}
get_colors();

// START
function on_size() {
    ww = window.Width;
    wh = window.Height;
}

function on_paint(gr) {
    var text_color = g_hot ? g_textcolor_hl : g_textcolor;
    gr.FillSolidRect(0, 0, ww, wh, g_backcolor);
    gr.GdiDrawText(g_text, g_font, text_color, 0, 0, ww, wh, DT_VCENTER | DT_CENTER | DT_WORDBREAK | DT_CALCRECT | DT_NOPREFIX);
}

function on_mouse_lbtn_up(x, y) {
    window.ShowConfigure();
}

function on_mouse_move() {
    if (!g_hot) {
        window.SetCursor(IDC_HAND);
        g_hot = true;
        window.Repaint();
    }
}

function on_mouse_leave() {
    if (g_hot) {
        g_hot = false;
        window.Repaint();
    }
}

function on_font_changed() {
    get_font();
    window.Repaint();
}

function on_colors_changed() {
    get_colors();
    window.Repaint();
}

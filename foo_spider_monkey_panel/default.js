"use strict";

// Use with GdiDrawText()
const DT_CENTER = 0x00000001;
const DT_VCENTER = 0x00000004;
const DT_WORDBREAK = 0x00000010;
const DT_CALCRECT = 0x00000400;
const DT_NOPREFIX = 0x00000800;

// Used in window.GetColorCUI()
const ColourTypeCUI = {
	text: 0,
	selection_text: 1,
	inactive_selection_text: 2,
	background: 3,
	selection_background: 4,
	inactive_selection_background: 5,
	active_item_frame: 6
};

// Used in window.GetFontCUI()
const FontTypeCUI = {
	items: 0,
	labels: 1
};

// Used in window.GetColourDUI()
const ColourTypeDUI = {
	text: 0,
	background: 1,
	highlight: 2,
	selection: 3
};

// Used in window.GetFontDUI()
const FontTypeDUI = {
	defaults: 0,
	tabs: 1,
	lists: 2,
	playlists: 3,
	statusbar: 4,
	console: 5
};

// Used in window.SetCursor()
const IDC_HAND = 32649;

let g_is_default_ui = window.InstanceType;
let g_font = null;
let g_text = `Spider Monkey Panel v${utils.Version}\n\nClick here to open the editor`;
let g_textcolour = 0;
let g_textcolour_hl = 0;
let g_backcolour = 0;
let g_hot = false;

let ww = 0;
let wh = 0;

get_font();
get_colours();


function on_paint(gr) {
    gr.FillSolidRect(0, 0, ww, wh, g_backcolour);
    gr.GdiDrawText(g_text, g_font, g_hot ? g_textcolour_hl : g_textcolour, 0, 0, ww, wh, DT_VCENTER | DT_CENTER | DT_WORDBREAK | DT_CALCRECT | DT_NOPREFIX);
}

function on_size(width, height) {
    ww = width;
    wh = height;
}

function on_mouse_lbtn_up(x, y) {
	window.EditScript();
}

function on_mouse_move() {
	if (!g_hot) {
		g_hot = true;
		window.SetCursor(IDC_HAND);
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

function on_colours_changed() {
	get_colours();
	window.Repaint();
}

function get_font() {
    if (g_is_default_ui) { // DUI
        g_font = window.GetFontDUI(FontTypeDUI.defaults);
    } else { // CUI
        g_font = window.GetFontCUI(FontTypeCUI.items);
    }
}

function get_colours() {
    if (g_is_default_ui) { // DUI
        g_textcolour = window.GetColourDUI(ColourTypeDUI.text);
        g_textcolour_hl = window.GetColourDUI(ColourTypeDUI.highlight);
        g_backcolour = window.GetColourDUI(ColourTypeDUI.background);
    } else { // CUI
        g_textcolour = window.GetColourCUI(ColourTypeCUI.text);
        g_textcolour_hl = window.GetColourCUI(ColourTypeCUI.text);
        g_backcolour = window.GetColourCUI(ColourTypeCUI.background);
    }
}

"use strict";

// Use with GdiDrawText()
const DT_TOP = 0x00000000;
const DT_CENTER = 0x00000001;
const DT_VCENTER = 0x00000004;
const DT_WORDBREAK = 0x00000010;
const DT_CALCRECT = 0x00000400;
const DT_NOPREFIX = 0x00000800;
const DT_SINGLELINE = 0x00000020;

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

const open_dir = function (c) { try { var WshShell = new ActiveXObject("WScript.Shell"); WshShell.Run(c); return true; } catch (e) { return false; } };
const package_id = window.ScriptInfo.PackageId
const package_info = utils.GetPackageInfo(package_id);
const package_dir = package_info.Directories.Root
const backup_dir = `${fb.ProfilePath}/foo_spider_monkey_panel/tmp/package_backups/${package_id}`

let g_is_default_ui = window.InstanceType;
let g_font = null;
let g_font_under = null;
let g_textcolour = 0;
let g_textcolour_hl = 0;
let g_backcolour = 0;

let ww = 0;
let wh = 0;

const kTitle = `Spider Monkey Panel v${utils.Version}\n
RECOVERY PACKAGE`

const kInstructionText0_1 = `Your package has failed to update!`
const kInstructionText0_2 = `Follow the instructions below to restore your panel without losing any settings!`
const kInstructionText1 = `1. Backup your panel settings:`
const kInstructionText1_1 = `1.1. Click here to open panel properties.`
const kInstructionText1_2 = `1.2  Click "Export" button to save your settings to the file.`
const kInstructionText1_2_1 = `     IMPORTANT: This will be the only way to restore your settings if you make a mistake in later steps!`
const kInstructionText1_2_2 = `                Save the file properly!`
const kInstructionText2 = `2. Restore your old package:`
const kInstructionText2_1 = `2.1 Click here to open the backup folder: it contains your old package content.`
const kInstructionText2_2 = `2.2. Click here to open the current package folder: it contains the recovery package.`
const kInstructionText2_3 = `2.3. Replace restoration package with the contents of your old package.`
const kInstructionText3 = `3. Apply changes:`
const kInstructionText3_0_1 = `3.0. If there was a mistake in the step 2. you might see the following message`
const kInstructionText3_0_2 = `     after performing the next step:`
const kInstructionText3_0_3 = `     "Can't load panel settings. Your panel will be completely reset!"`
const kInstructionText3_0_4 = `     Don't fret! You still have your panel settings from step 1. and your old package from step 2.1.`
const kInstructionText3_0_5 = `     You can retry step 2.3 again and then reimport your settings via the Panel Properties panel`
const kInstructionText3_0_6 = `     by clicking on "Import" Button.`
const kInstructionText3_1 = `3.1. Click here to reload your panel and you'll see your old panel if there were no mistakes in previous steps.`

const kInstructions = [
    kInstructionText0_1,
    kInstructionText0_2,
    kInstructionText1,
    kInstructionText1_1,
    kInstructionText1_2,
    kInstructionText1_2_1,
    kInstructionText1_2_2,
    kInstructionText2,
    kInstructionText2_1,
    kInstructionText2_2,
    kInstructionText2_3,
    kInstructionText3,
    kInstructionText3_0_1,
    kInstructionText3_0_2,
    kInstructionText3_0_3,
    kInstructionText3_0_4,
    kInstructionText3_0_5,
    kInstructionText3_1
]
let g_over_cur_idx = null;
let g_idx_to_y = new Map();
let g_clickable_indises = [
    kInstructions.indexOf(kInstructionText1_1),
    kInstructions.indexOf(kInstructionText2_1),
    kInstructions.indexOf(kInstructionText2_2),
    kInstructions.indexOf(kInstructionText3_1)
];
let g_idx_to_call = new Map();
g_idx_to_call.set(kInstructions.indexOf(kInstructionText1_1), () => window.ShowProperties());
g_idx_to_call.set(kInstructions.indexOf(kInstructionText2_1), () => open_dir(backup_dir));
g_idx_to_call.set(kInstructions.indexOf(kInstructionText2_2), () => open_dir(package_dir));
g_idx_to_call.set(kInstructions.indexOf(kInstructionText3_1), () => window.Reload());

get_font();
get_colours();


function on_paint(gr) {
    gr.FillSolidRect(0, 0, ww, wh, g_backcolour);

    gr.GdiDrawText(kTitle, g_font, g_textcolour, 0, 0, ww, wh, DT_TOP | DT_CENTER | DT_CALCRECT | DT_NOPREFIX);

    const padding_y = 5;
    const padding_x = 15;
    let cur_y = 60;

    for (let [index, text] of kInstructions.entries()) {
        gr.GdiDrawText(text, g_over_cur_idx !== index ? g_font : g_font_under, g_over_cur_idx !== index ? g_textcolour_hl : g_textcolour, padding_x, cur_y, ww - padding_x, wh - cur_y, DT_WORDBREAK | DT_CALCRECT | DT_NOPREFIX);
        let height = gr.CalcTextHeight(text, g_font);
        g_idx_to_y.set(index, { y: cur_y, h: height });
        cur_y += height;
        cur_y += padding_y
    }
}

function on_size(width, height) {
    ww = width;
    wh = height;
}

function on_mouse_lbtn_up(x, y) {
    if (g_over_cur_idx !== null) {
        g_idx_to_call.get(g_over_cur_idx)();
    }
}

function on_mouse_move(x, y) {
    let was_over = g_over_cur_idx
    g_over_cur_idx = null

    for (let index of g_clickable_indises) {
        let data = g_idx_to_y.get(index)
        if (y >= data.y && y < data.y + data.h) {
            g_over_cur_idx = index;
            break;
        }
    }

    if (!!g_over_cur_idx != !!was_over) {
        window.Repaint();
    }
}

function on_mouse_leave() {
    if (g_over_cur_idx) {
        g_over_cur_idx = null;
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
    g_font_under = gdi.Font(g_font.Name, g_font.Size, 4)
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

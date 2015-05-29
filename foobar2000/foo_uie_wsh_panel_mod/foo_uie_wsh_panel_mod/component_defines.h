#pragma once

/* 
 * IMPORTANT NOTE:
 * For modders: You should change these Defines/Guids below, in order to 
 *   co-exist with the original release of WSH Panel Mod.
 */

#define WSPM_NAME "WSH Panel Mod"
#define WSPM_WINDOW_CLASS_NAME "uie_wsh_panel_mod_class"
#define WSPM_DLL_NAME "foo_uie_wsh_panel_mod.dll"

// {1624E0E0-049E-4927-B4DD-2DAF7FC2415F}
extern const GUID g_ui_pref_window_guid;
FOOGUIDDECL const GUID g_ui_pref_window_guid = 
{ 0x1624e0e0, 0x49e, 0x4927, { 0xb4, 0xdd, 0x2d, 0xaf, 0x7f, 0xc2, 0x41, 0x5f } };

// {75A7B642-786C-4f24-9B52-17D737DEA09A}
extern const GUID g_wsh_panel_window_extension_guid;
FOOGUIDDECL const GUID g_wsh_panel_window_extension_guid =
{ 0x75a7b642, 0x786c, 0x4f24, { 0x9b, 0x52, 0x17, 0xd7, 0x37, 0xde, 0xa0, 0x9a } };

// {A290D430-E431-45c5-BF76-EF1130EF1CF5}
extern const GUID g_wsh_panel_window_dui_guid;
FOOGUIDDECL const GUID g_wsh_panel_window_dui_guid = 
{ 0xa290d430, 0xe431, 0x45c5, { 0xbf, 0x76, 0xef, 0x11, 0x30, 0xef, 0x1c, 0xf5 } };

// {3E56779F-2884-4f98-9A61-992F664999D0}
extern const GUID g_guid_prop_sets;
FOOGUIDDECL const GUID g_guid_prop_sets = 
{ 0x3e56779f, 0x2884, 0x4f98, { 0x9a, 0x61, 0x99, 0x2f, 0x66, 0x49, 0x99, 0xd0 } };

// {8826D886-6E34-4796-9B61-1FEA996730F0}
extern const GUID g_guid_cfg_safe_mode;
FOOGUIDDECL const GUID g_guid_cfg_safe_mode = 
{ 0x8826d886, 0x6e34, 0x4796, { 0x9b, 0x61, 0x1f, 0xea, 0x99, 0x67, 0x30, 0xf0 } };

// {E0521E81-C2A4-4a3e-A5FC-A1E62B187053}
extern const GUID g_guid_cfg_cui_warning_reported;
FOOGUIDDECL const GUID g_guid_cfg_cui_warning_reported = 
{ 0xe0521e81, 0xc2a4, 0x4a3e, { 0xa5, 0xfc, 0xa1, 0xe6, 0x2b, 0x18, 0x70, 0x53 } };

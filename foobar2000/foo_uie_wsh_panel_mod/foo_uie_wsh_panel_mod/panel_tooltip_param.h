#pragma once

#include <memory>

struct panel_tooltip_param
{
	HWND tooltip_hwnd;
	SIZE tooltip_size;

	panel_tooltip_param() : tooltip_hwnd(0) {}
};

typedef std::shared_ptr<panel_tooltip_param> panel_tooltip_param_ptr;

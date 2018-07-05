#include "stdafx.h"
#include "ui_property.h"

LRESULT CDialogProperty::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
	DlgResize_Init();

	// Subclassing
	m_properties.SubclassWindow(GetDlgItem(IDC_LIST_PROPERTIES));
	m_properties.ModifyStyle(0, LBS_SORT | LBS_HASSTRINGS);
	m_properties.SetExtendedListStyle(PLS_EX_SORTED | PLS_EX_XPLOOK);

	LoadProperties();

	return TRUE; // set focus to default control
}

LRESULT CDialogProperty::OnCloseCmd(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	switch (wID)
	{
	case IDOK:
		Apply();
		break;

	case IDAPPLY:
		Apply();
		return 0;
	}

	EndDialog(wID);
	return 0;
}

LRESULT CDialogProperty::OnPinItemChanged(LPNMHDR pnmh)
{
	LPNMPROPERTYITEM pnpi = (LPNMPROPERTYITEM)pnmh;

	pfc::stringcvt::string_utf8_from_os uname = pnpi->prop->GetName();

	if (m_dup_prop_map.count(uname.toString()))
	{
		auto& val = *(m_dup_prop_map[uname.toString()].get());
		_variant_t var;

		if (pnpi->prop->GetValue(&var))
		{
            switch ( var.vt )
            {
                case VT_I1:
                {
                    memset( &val, 0, sizeof( val ) );
                    val.type = mozjs::JsValueType::pt_int32;
                    val.intVal = var.cVal;
                    break;
                }
                case VT_I2:
                {
                    memset( &val, 0, sizeof( val ) );
                    val.type = mozjs::JsValueType::pt_int32;
                    val.intVal = var.iVal;
                    break;
                }
                case VT_INT:
                case VT_I4: 
                {
                    memset( &val, 0, sizeof( val ) );
                    val.type = mozjs::JsValueType::pt_int32;
                    val.intVal = var.lVal;
                    break;
                }
                case VT_R4:
                {
                    memset( &val, 0, sizeof( val ) );
                    val.type = mozjs::JsValueType::pt_double;
                    val.doubleVal = var.fltVal;
                    break;
                }
                case VT_R8:
                {
                    memset( &val, 0, sizeof( val ) );
                    val.type = mozjs::JsValueType::pt_double;
                    val.doubleVal = var.dblVal;
                    break;
                }
                case VT_BOOL:
                {
                    memset( &val, 0, sizeof( val ) );
                    val.type = mozjs::JsValueType::pt_boolean;
                    val.boolVal = var.boolVal;
                    break;
                }
                case VT_UI1:
                {
                    memset( &val, 0, sizeof( val ) );
                    val.type = mozjs::JsValueType::pt_double;
                    val.doubleVal = var.bVal;
                    break;
                }
                case VT_UI2:
                {
                    memset( &val, 0, sizeof( val ) );
                    val.type = mozjs::JsValueType::pt_double;
                    val.doubleVal = var.uiVal;
                    break;
                }
                case VT_UINT:
                case VT_UI4:
                {
                    memset( &val, 0, sizeof( val ) );
                    val.type = mozjs::JsValueType::pt_double;
                    val.doubleVal = var.ulVal;
                    break;
                }
                default:
                {
                    break;
                }
            }
		}
	}

	return 0;
}

LRESULT CDialogProperty::OnClearallBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	m_dup_prop_map.clear();
	m_properties.ResetContent();

	return 0;
}

void CDialogProperty::Apply()
{
	// Copy back
	m_parent->get_config_prop().get_val() = m_dup_prop_map;
	m_parent->update_script();
	LoadProperties();
}

void CDialogProperty::LoadProperties(bool reload)
{
	m_properties.ResetContent();

	if (reload)
	{
		m_dup_prop_map = m_parent->get_config_prop().get_val();
	}

    for (auto& elem : m_dup_prop_map)
    {
        pfc::stringcvt::string_wide_from_utf8_fast wname( elem.first.c_str(), elem.first.length() );
        HPROPERTY hProp = nullptr;        
        _variant_t var;
        VariantInit( &var );

        auto& serializedValue = *(elem.second);

        switch ( elem.second->type )
        {
        case mozjs::JsValueType::pt_boolean:
        {
            hProp = PropCreateSimple( wname, serializedValue.boolVal );
            break;
        }
        case mozjs::JsValueType::pt_int32:
        {
            var.vt = VT_I4;
            var.lVal = serializedValue.intVal;
            hProp = PropCreateSimple( wname, var.lVal );
            break;
        }
        case mozjs::JsValueType::pt_double:
        {
            std::string strNumber = std::to_string( serializedValue.doubleVal );
            pfc::stringcvt::string_wide_from_utf8_fast wStrVal( strNumber.c_str(), strNumber.length() );
            var.vt = VT_BSTR;
            var.bstrVal = SysAllocString( wStrVal );
            hProp = PropCreateSimple( wname, var.bstrVal );
            break;
        }
        case mozjs::JsValueType::pt_string:
        {
            pfc::stringcvt::string_wide_from_utf8_fast wStrVal( serializedValue.strVal.c_str(), serializedValue.strVal.length() );
            var.vt = VT_BSTR;
            var.bstrVal = SysAllocString( wStrVal );
            hProp = PropCreateSimple( wname, var.bstrVal );
            break;
        }
        default:
            assert( 0 );
            break;
        }

        m_properties.AddItem( hProp );
    }
}

LRESULT CDialogProperty::OnDelBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	int idx = m_properties.GetCurSel();

	if (idx >= 0)
	{
		HPROPERTY hproperty = m_properties.GetProperty(idx);
		pfc::stringcvt::string_utf8_from_os uname = hproperty->GetName();

		m_properties.DeleteItem(hproperty);
		m_dup_prop_map.erase(uname.toString());
	}

	return 0;
}

LRESULT CDialogProperty::OnImportBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	pfc::string8 filename;

	if (uGetOpenFileName(m_hWnd, "Property files|*.wsp|All files|*.*", 0, "wsp", "Import from", nullptr, filename, FALSE))
	{
		file_ptr io;
		abort_callback_dummy abort;

		try
		{
			filesystem::g_open_read(io, filename, abort);
			prop_kv_config::g_load(m_dup_prop_map, io.get_ptr(), abort);
			LoadProperties(false);
		}
		catch (...)
		{
		}
	}
	return 0;
}

LRESULT CDialogProperty::OnExportBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	pfc::string8 path;

	if (uGetOpenFileName(m_hWnd, "Property files|*.wsp", 0, "wsp", "Save as", nullptr, path, TRUE))
	{
		file_ptr io;
		abort_callback_dummy abort;

		try
		{
			filesystem::g_open_write_new(io, path, abort);
			prop_kv_config::g_save(m_dup_prop_map, io.get_ptr(), abort);
		}
		catch (...)
		{
		}
	}
	return 0;
}

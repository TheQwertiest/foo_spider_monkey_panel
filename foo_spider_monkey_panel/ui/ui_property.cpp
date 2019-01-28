#include <stdafx.h>
#include "ui_property.h"

#include <utils/file_helpers.h>
#include <js_panel_window.h>
#include <abort_callback.h>

// stringstream
#include <sstream>
// precision
#include <iomanip>
#include <map>

LRESULT CDialogProperty::OnInitDialog( HWND hwndFocus, LPARAM lParam )
{
    DlgResize_Init();

    // Subclassing
    m_properties.SubclassWindow( GetDlgItem( IDC_LIST_PROPERTIES ) );
    m_properties.ModifyStyle( 0, LBS_SORT | LBS_HASSTRINGS );
    m_properties.SetExtendedListStyle( PLS_EX_SORTED | PLS_EX_XPLOOK );

    LoadProperties();

    return TRUE; // set focus to default control
}

LRESULT CDialogProperty::OnCloseCmd( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    switch ( wID )
    {
    case IDOK:
        Apply();
        break;

    case IDAPPLY:
        Apply();
        return 0;
    }

    EndDialog( wID );
    return 0;
}

LRESULT CDialogProperty::OnPinItemChanged( LPNMHDR pnmh )
{
    LPNMPROPERTYITEM pnpi = (LPNMPROPERTYITEM)pnmh;

    std::wstring name = pnpi->prop->GetName();

    if ( m_dup_prop_map.count( name ) )
    {
        auto& val = *( m_dup_prop_map[name].get() );
        _variant_t var;

        if ( pnpi->prop->GetValue( &var ) )
        {
            switch ( val.type )
            {
            case mozjs::JsValueType::pt_boolean:
            {
                var.ChangeType( VT_BOOL );
                val.boolVal = var.boolVal;
                break;
            }
            case mozjs::JsValueType::pt_int32:
            {
                var.ChangeType( VT_I4 );
                val.intVal = var.lVal;
                break;
            }
            case mozjs::JsValueType::pt_double:
            {
                if ( VT_BSTR == var.vt )
                {
                    val.doubleVal = std::stod( var.bstrVal );
                }
                else
                {
                    var.ChangeType( VT_R8 );
                    val.doubleVal = var.dblVal;
                }

                break;
            }
            case mozjs::JsValueType::pt_string:
            {
                var.ChangeType( VT_BSTR );
                val.strVal = pfc::stringcvt::string_utf8_from_wide( var.bstrVal );
                break;
            }
            default:
            {
                assert( 0 );
                break;
            }
            }
        }
    }

    return 0;
}

LRESULT CDialogProperty::OnClearallBnClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl )
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

void CDialogProperty::LoadProperties( bool reload )
{
    m_properties.ResetContent();

    if ( reload )
    {
        m_dup_prop_map = m_parent->get_config_prop().get_val();
    }

    auto doubleToString = []( double dVal ) {
        std::wostringstream out;
        out << std::setprecision( 16 ) << dVal;
        return out.str();
    };

    struct LowerLexCmp
    { // lexicographical comparison but with lower cased chars
        bool operator()( const std::wstring& a, const std::wstring& b ) const
        {
            return ranges::lexicographical_compare( a, b, []( wchar_t ca, wchar_t cb ) {
                return static_cast<wchar_t>( ::towlower( ca ) ) < static_cast<wchar_t>( ::towlower( cb ) );
            } );
        }
    };
    std::map<std::wstring, HPROPERTY, LowerLexCmp> propMap;
    for ( const auto& [name, pSerializedValue] : m_dup_prop_map )
    {
        HPROPERTY hProp = nullptr;
        _variant_t var;
        VariantInit( &var );

        auto& serializedValue = *pSerializedValue;

        switch ( serializedValue.type )
        {
        case mozjs::JsValueType::pt_boolean:
        {
            hProp = PropCreateSimple( name.c_str(), serializedValue.boolVal );
            break;
        }
        case mozjs::JsValueType::pt_int32:
        {
            var.vt = VT_I4;
            var.lVal = serializedValue.intVal;
            hProp = PropCreateSimple( name.c_str(), var.lVal );
            break;
        }
        case mozjs::JsValueType::pt_double:
        {
            const std::wstring strNumber = [dVal = serializedValue.doubleVal, &doubleToString] {
                if ( std::trunc( dVal ) == dVal )
                { // Most likely uint64_t
                    return std::to_wstring( static_cast<uint64_t>( dVal ) );
                }

                // std::to_string(double) has precision of float
                return doubleToString( dVal );
            }();

            var.vt = VT_BSTR;
            var.bstrVal = SysAllocString( strNumber.c_str() );
            hProp = PropCreateSimple( name.c_str(), var.bstrVal );
            break;
        }
        case mozjs::JsValueType::pt_string:
        {
            pfc::stringcvt::string_wide_from_utf8_fast wStrVal( serializedValue.strVal.c_str(), serializedValue.strVal.length() );
            var.vt = VT_BSTR;
            var.bstrVal = SysAllocString( wStrVal );
            hProp = PropCreateSimple( name.c_str(), var.bstrVal );
            break;
        }
        default:
        {
            assert( 0 );
            continue;
        }
        }

        propMap.emplace( name, hProp );
    }

    for ( auto& [name, hProp] : propMap )
    {
        m_properties.AddItem( hProp );
    }
}

LRESULT CDialogProperty::OnDelBnClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    int idx = m_properties.GetCurSel();

    if ( idx >= 0 )
    {
        HPROPERTY hproperty = m_properties.GetProperty( idx );
        std::wstring name = hproperty->GetName();

        m_properties.DeleteItem( hproperty );
        m_dup_prop_map.erase( name );
    }

    return 0;
}

LRESULT CDialogProperty::OnImportBnClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    constexpr COMDLG_FILTERSPEC k_DialogImportExtFilter[2] =
        {
            { L"Property files", L"*.json;*.smp;*.wsp" },
            { L"All files", L"*.*" },
        };

    const pfc::string8_fast filename = 
         pfc::stringcvt::string_utf8_from_os( smp::file::FileDialog( L"Import from", false, k_DialogImportExtFilter, L"json", L"props" ).c_str() ).get_ptr();
    if ( filename.is_empty() )
    {
        return 0;
    }

    file_ptr io;
    auto& abort = smp::GlobalAbortCallback::GetInstance();

    try
    {
        filesystem::g_open_read( io, filename, abort );

        if ( filename.has_suffix( ".json" ) )
        {
            smp::config::PanelProperties::g_load_json( m_dup_prop_map, *io, abort, true );
        }
        else if( filename.has_suffix( ".smp" ) )
        {
            smp::config::PanelProperties::g_load( m_dup_prop_map, io.get_ptr(), abort );
        }
        else if ( filename.has_suffix( ".wsp" ) )
        {
            smp::config::PanelProperties::g_load_legacy( m_dup_prop_map, io.get_ptr(), abort );
        }
        else
        {
            if ( !smp::config::PanelProperties::g_load_json( m_dup_prop_map, *io, abort, true )
                 && !smp::config::PanelProperties::g_load( m_dup_prop_map, io.get_ptr(), abort ) )
            {
                smp::config::PanelProperties::g_load_legacy( m_dup_prop_map, io.get_ptr(), abort );
            }
        }

        LoadProperties( false );
    }
    catch ( const pfc::exception& )
    {
    }

    return 0;
}

LRESULT CDialogProperty::OnExportBnClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl )
{
    constexpr COMDLG_FILTERSPEC k_DialogExportExtFilter[2] =
        {
            { L"Property files", L"*.json" },
            { L"All files", L"*.*" },
        };

    const pfc::stringcvt::string_utf8_from_os filename( smp::file::FileDialog( L"Save as", true, k_DialogExportExtFilter, L"json", L"props" ).c_str() );
    if ( filename.is_empty() )
    {
        return 0;
    }

    file_ptr io;
    auto& abort = smp::GlobalAbortCallback::GetInstance();

    try
    {
        filesystem::g_open_write_new( io, filename, abort );
        smp::config::PanelProperties::g_save_json( m_dup_prop_map, *io, abort, true );
    }
    catch ( const pfc::exception& )
    {
    }
    
    return 0;
}

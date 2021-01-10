#include <stdafx.h>

#include "ui_property.h"

#include <panel/js_panel_window.h>
#include <utils/array_x.h>

#include <qwr/abort_callback.h>
#include <qwr/error_popup.h>
#include <qwr/file_helpers.h>
#include <qwr/type_traits.h>

// precision
#include <filesystem>
#include <iomanip>
#include <map>

namespace fs = std::filesystem;

namespace smp::ui
{

CDialogProperty::CDialogProperty( smp::panel::js_panel_window* p_parent )
    : parentPanel_( p_parent )
{
}

LRESULT CDialogProperty::OnInitDialog( HWND, LPARAM )
{
    DlgResize_Init();

    // Subclassing
    propertyListCtrl_.SubclassWindow( GetDlgItem( IDC_LIST_PROPERTIES ) );
    propertyListCtrl_.ModifyStyle( 0, LBS_SORT | LBS_HASSTRINGS );
    propertyListCtrl_.SetExtendedListStyle( PLS_EX_SORTED | PLS_EX_XPLOOK );

    LoadProperties();

    return TRUE; // set focus to default control
}

LRESULT CDialogProperty::OnCloseCmd( WORD, WORD wID, HWND )
{
    switch ( wID )
    {
    case IDAPPLY:
        Apply();
        return 0;
    case IDOK:
        Apply();
        EndDialog( wID );
        return 0;
    default:
        EndDialog( wID );
        return 0;
    }
}

LRESULT CDialogProperty::OnPinItemChanged( LPNMHDR pnmh )
{
    auto pnpi = reinterpret_cast<LPNMPROPERTYITEM>( pnmh );

    auto& localPropertyValues = localProperties_.values;
    if ( auto it = localPropertyValues.find( pnpi->prop->GetName() );
         it != localPropertyValues.end() )
    {
        auto& val = *( it->second );
        _variant_t var;

        if ( pnpi->prop->GetValue( &var ) )
        {
            std::visit( [&var]( auto& arg ) {
                using T = std::decay_t<decltype( arg )>;
                if constexpr ( std::is_same_v<T, bool> )
                {
                    var.ChangeType( VT_BOOL );
                    arg = static_cast<bool>( var.boolVal );
                }
                else if constexpr ( std::is_same_v<T, int32_t> )
                {
                    var.ChangeType( VT_I4 );
                    arg = static_cast<int32_t>( var.lVal );
                }
                else if constexpr ( std::is_same_v<T, double> )
                {
                    if ( VT_BSTR == var.vt )
                    {
                        arg = std::stod( var.bstrVal );
                    }
                    else
                    {
                        var.ChangeType( VT_R8 );
                        arg = var.dblVal;
                    }
                }
                else if constexpr ( std::is_same_v<T, std::u8string> )
                {
                    var.ChangeType( VT_BSTR );
                    arg = qwr::unicode::ToU8( std::wstring_view{ var.bstrVal ? var.bstrVal : L"" } );
                }
                else
                {
                    static_assert( qwr::always_false_v<T>, "non-exhaustive visitor!" );
                }
            },
                        val );
        }
    }

    return 0;
}

LRESULT CDialogProperty::OnClearallBnClicked( WORD, WORD, HWND )
{
    localProperties_.values.clear();
    propertyListCtrl_.ResetContent();

    return 0;
}

void CDialogProperty::Apply()
{
    // Copy back
    parentPanel_->GetPanelProperties() = localProperties_;
    parentPanel_->ReloadScript();
    LoadProperties();
}

void CDialogProperty::LoadProperties( bool reload )
{
    propertyListCtrl_.ResetContent();

    if ( reload )
    {
        localProperties_ = parentPanel_->GetPanelProperties();
    }

    struct LowerLexCmp
    { // lexicographical comparison but with lower cased chars
        bool operator()( const std::wstring& a, const std::wstring& b ) const
        {
            return ( _wcsicmp( a.c_str(), b.c_str() ) < 0 );
        }
    };
    std::map<std::wstring, HPROPERTY, LowerLexCmp> propMap;
    for ( const auto& [name, pSerializedValue]: localProperties_.values )
    {
        HPROPERTY hProp = std::visit( [&name = name]( auto&& arg ) {
            using T = std::decay_t<decltype( arg )>;
            if constexpr ( std::is_same_v<T, bool> || std::is_same_v<T, int32_t> )
            {
                return PropCreateSimple( name.c_str(), arg );
            }
            else if constexpr ( std::is_same_v<T, double> )
            {
                const std::wstring strNumber = [arg] {
                    if ( std::trunc( arg ) == arg )
                    { // Most likely uint64_t
                        return std::to_wstring( static_cast<uint64_t>( arg ) );
                    }

                    // std::to_string(double) has precision of float
                    return fmt::format( L"{:.16g}", arg );
                }();
                return PropCreateSimple( name.c_str(), strNumber.c_str() );
            }
            else if constexpr ( std::is_same_v<T, std::u8string> )
            {
                return PropCreateSimple( name.c_str(), qwr::unicode::ToWide( arg ).c_str() );
            }
            else
            {
                static_assert( qwr::always_false_v<T>, "non-exhaustive visitor!" );
            }
        },
                                      *pSerializedValue );

        propMap.emplace( name, hProp );
    }

    for ( auto& [name, hProp]: propMap )
    {
        propertyListCtrl_.AddItem( hProp );
    }
}

LRESULT CDialogProperty::OnDelBnClicked( WORD, WORD, HWND )
{
    if ( int idx = propertyListCtrl_.GetCurSel();
         idx )
    {
        HPROPERTY hproperty = propertyListCtrl_.GetProperty( idx );
        std::wstring name = hproperty->GetName();

        propertyListCtrl_.DeleteItem( hproperty );
        localProperties_.values.erase( name );
    }

    return 0;
}

LRESULT CDialogProperty::OnImportBnClicked( WORD, WORD, HWND )
{
    constexpr auto k_DialogImportExtFilter = smp::to_array<COMDLG_FILTERSPEC>( {
        { L"Property files", L"*.json;*.smp;*.wsp" },
        { L"All files", L"*.*" },
    } );

    const auto path_opt = qwr::file::FileDialog( L"Import from", false, guid::dialog_path, k_DialogImportExtFilter, L"json", L"props" );
    if ( !path_opt || path_opt->empty() )
    {
        return 0;
    }
    const auto path = path_opt->lexically_normal();

    auto& abort = qwr::GlobalAbortCallback::GetInstance();

    try
    {
        file_ptr io;
        filesystem::g_open_read( io, path.u8string().c_str(), abort );

        const auto extension = path.extension();
        if ( extension == ".json" )
        {
            localProperties_ = config::PanelProperties::FromJson( qwr::pfc_x::ReadRawString( *io, abort ) );
        }
        else if ( extension == ".smp" )
        {
            localProperties_ = config::PanelProperties::Load( *io, abort, config::SerializationFormat::Binary );
        }
        else if ( extension == ".wsp" )
        {
            localProperties_ = config::PanelProperties::Load( *io, abort, config::SerializationFormat::Com );
        }
        else
        { // let's brute-force it!
            const auto tryParse = [&io, &abort]( smp::config::SerializationFormat format ) -> std::optional<config::PanelProperties> {
                try
                {
                    return config::PanelProperties::Load( *io, abort, format );
                }
                catch ( const qwr::QwrException& )
                {
                    return std::nullopt;
                }
            };

            bool success = false;
            for ( const auto format: { config::SerializationFormat::Json, config::SerializationFormat::Binary, config::SerializationFormat::Com } )
            {
                auto propOpt = tryParse( format );
                if ( propOpt )
                {
                    localProperties_ = *propOpt;
                    success = true;
                    break;
                }
            }
            if ( !success )
            {
                throw qwr::QwrException( "Failed to parse panel properties: unknown format" );
            }
        }

        LoadProperties( false );
    }
    catch ( const qwr::QwrException& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
    }
    catch ( const pfc::exception& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
    }

    return 0;
}

LRESULT CDialogProperty::OnExportBnClicked( WORD, WORD, HWND )
{
    constexpr auto k_DialogExportExtFilter = smp::to_array<COMDLG_FILTERSPEC>(
        {
            { L"Property files", L"*.json" },
            { L"All files", L"*.*" },
        } );

    const auto path_opt = qwr::file::FileDialog( L"Save as", true, guid::dialog_path, k_DialogExportExtFilter, L"json", L"props" );
    if ( !path_opt || path_opt->empty() )
    {
        return 0;
    }
    const auto path = path_opt->lexically_normal();

    try
    {
        auto& abort = qwr::GlobalAbortCallback::GetInstance();
        file_ptr io;
        filesystem::g_open_write_new( io, path.u8string().c_str(), abort );

        qwr::pfc_x::WriteStringRaw( *io, localProperties_.ToJson(), abort );
    }
    catch ( const pfc::exception& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
    }

    return 0;
}

} // namespace smp::ui

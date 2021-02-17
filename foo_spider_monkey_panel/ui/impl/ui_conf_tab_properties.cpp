#include <stdafx.h>

#include "ui_conf_tab_properties.h"

#include <panel/js_panel_window.h>
#include <ui/ui_conf.h>

#include <qwr/abort_callback.h>
#include <qwr/error_popup.h>
#include <qwr/file_helpers.h>
#include <qwr/type_traits.h>

#include <filesystem>
#include <iomanip>
#include <map>

namespace fs = std::filesystem;

namespace smp::ui
{

CConfigTabProperties::CConfigTabProperties( CDialogConf& parent, config::PanelProperties& properties )
    : parent_( parent )
    , properties_( properties )
{
}

HWND CConfigTabProperties::CreateTab( HWND hParent )
{
    return Create( hParent );
}

ATL::CDialogImplBase& CConfigTabProperties::Dialog()
{
    return *this;
}

const wchar_t* CConfigTabProperties::Name() const
{
    return L"Properties";
}

bool CConfigTabProperties::HasChanged()
{
    return false;
}

void CConfigTabProperties::Apply()
{
}

void CConfigTabProperties::Revert()
{
}

void CConfigTabProperties::Refresh()
{
    if ( m_hWnd )
    { // might be called while tab is inactive
        UpdateUiFromData();
    }
}

LRESULT CConfigTabProperties::OnInitDialog( HWND, LPARAM )
{
    DlgResize_Init( false, false, WS_CHILD );

    // Subclassing
    propertyListCtrl_.SubclassWindow( GetDlgItem( IDC_LIST_PROPERTIES ) );
    propertyListCtrl_.ModifyStyle( 0, LBS_SORT | LBS_HASSTRINGS );
    propertyListCtrl_.SetExtendedListStyle( PLS_EX_SORTED | PLS_EX_XPLOOK );

    CWindow{ GetDlgItem( IDC_DEL ) }.EnableWindow( propertyListCtrl_.GetCurSel() != -1 );

    UpdateUiFromData();

    return TRUE; // set focus to default control
}

LRESULT CConfigTabProperties::OnPinItemChanged( LPNMHDR pnmh )
{
    auto pnpi = (LPNMPROPERTYITEM)pnmh;

    const auto hasChanged = [pnpi, &properties = properties_]() {
        auto& propValues = properties.values;

        if ( !propValues.contains( pnpi->prop->GetName() ) )
        {
            return false;
        }

        auto& val = *propValues.at( pnpi->prop->GetName() );
        _variant_t var;

        if ( !pnpi->prop->GetValue( &var ) )
        {
            return false;
        }

        return std::visit( [&var]( auto& arg ) {
            using T = std::decay_t<decltype( arg )>;
            const auto prevArgValue = arg;
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
            else if constexpr ( std::is_same_v<T, qwr::u8string> )
            {
                var.ChangeType( VT_BSTR );
                arg = qwr::unicode::ToU8( std::wstring_view{ var.bstrVal ? var.bstrVal : L"" } );
            }
            else
            {
                static_assert( qwr::always_false_v<T>, "non-exhaustive visitor!" );
            }

            return ( prevArgValue != arg );
        },
                           val );
    }();

    if ( hasChanged )
    {
        parent_.OnDataChanged();
    }

    return 0;
}

LRESULT CConfigTabProperties::OnSelChanged( LPNMHDR )
{
    UpdateUiDelButton();
    return 0;
}

LRESULT CConfigTabProperties::OnClearAllBnClicked( WORD, WORD, HWND )
{
    properties_.values.clear();

    propertyListCtrl_.ResetContent();

    parent_.OnDataChanged();

    return 0;
}

LRESULT CConfigTabProperties::OnDelBnClicked( WORD, WORD, HWND )
{
    if ( int idx = propertyListCtrl_.GetCurSel();
         idx >= 0 )
    {
        HPROPERTY hproperty = propertyListCtrl_.GetProperty( idx );
        std::wstring name = hproperty->GetName();

        properties_.values.erase( name );

        propertyListCtrl_.DeleteItem( hproperty );
    }

    UpdateUiDelButton();
    parent_.OnDataChanged();

    return 0;
}

LRESULT CConfigTabProperties::OnImportBnClicked( WORD, WORD, HWND )
{
    using namespace smp::config;

    qwr::file::FileDialogOptions fdOpts{};
    fdOpts.savePathGuid = guid::dialog_path;
    fdOpts.filterSpec.assign( {
        { L"Property files", L"*.json;*.smp;*.wsp" },
        { L"All files", L"*.*" },
    } );
    fdOpts.defaultFilename = L"props";
    fdOpts.defaultExtension = L"json";

    auto path = fs::path( qwr::file::FileDialog( L"Import from", false, fdOpts ).value_or( std::wstring{} ) );
    if ( path.empty() )
    {
        return 0;
    }
    path = path.lexically_normal();

    try
    {
        auto& abort = qwr::GlobalAbortCallback::GetInstance();
        file_ptr io;
        filesystem::g_open_read( io, path.u8string().c_str(), abort );

        const auto extension = path.extension();
        if ( extension == ".json" )
        {
            properties_ = PanelProperties::FromJson( qwr::pfc_x::ReadRawString( *io, abort ) );
        }
        else if ( extension == ".smp" )
        {
            properties_ = PanelProperties::Load( *io, abort, smp::config::SerializationFormat::Binary );
        }
        else if ( extension == ".wsp" )
        {
            properties_ = PanelProperties::Load( *io, abort, smp::config::SerializationFormat::Com );
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
                    properties_ = *propOpt;
                    success = true;
                    break;
                }
            }
            if ( !success )
            {
                throw qwr::QwrException( "Failed to parse panel properties: unknown format" );
            }
        }

        UpdateUiFromData();
    }
    catch ( const qwr::QwrException& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
    }
    catch ( const pfc::exception& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
    }

    parent_.OnDataChanged();

    return 0;
}

LRESULT CConfigTabProperties::OnExportBnClicked( WORD, WORD, HWND )
{
    qwr::file::FileDialogOptions fdOpts{};
    fdOpts.savePathGuid = guid::dialog_path;
    fdOpts.filterSpec.assign( {
        { L"Property files", L"*.json" },
        { L"All files", L"*.*" },
    } );
    fdOpts.defaultFilename = L"props";
    fdOpts.defaultExtension = L"json";

    auto path = fs::path( qwr::file::FileDialog( L"Save as", true, fdOpts ).value_or( std::wstring{} ) );
    if ( path.empty() )
    {
        return 0;
    }
    path = path.lexically_normal();

    try
    {
        auto& abort = qwr::GlobalAbortCallback::GetInstance();
        file_ptr io;
        filesystem::g_open_write_new( io, path.u8string().c_str(), abort );

        qwr::pfc_x::WriteStringRaw( *io, properties_.ToJson(), abort );
    }
    catch ( const pfc::exception& e )
    {
        qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, e.what() );
    }

    return 0;
}

void CConfigTabProperties::UpdateUiFromData()
{
    propertyListCtrl_.ResetContent();

    struct LowerLexCmp
    { // lexicographical comparison but with lower cased chars
        bool operator()( const std::wstring& a, const std::wstring& b ) const
        {
            return ( _wcsicmp( a.c_str(), b.c_str() ) < 0 );
        }
    };
    std::map<std::wstring, HPROPERTY, LowerLexCmp> propMap;
    for ( const auto& [name, pSerializedValue]: properties_.values )
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
            else if constexpr ( std::is_same_v<T, qwr::u8string> )
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

void CConfigTabProperties::UpdateUiDelButton()
{
    CWindow{ GetDlgItem( IDC_DEL ) }.EnableWindow( propertyListCtrl_.GetCurSel() != -1 );
}

} // namespace smp::ui

#include <stdafx.h>

#include "edit_script.h"

#include <config/parsed_panel_config.h>
#include <utils/edit_text.h>

#include <qwr/error_popup.h>
#include <qwr/winapi_error_helpers.h>

namespace smp::panel
{

bool EditScript( HWND hParent, config::ParsedPanelSettings& settings )
{
    namespace fs = std::filesystem;

    try
    {
        switch ( settings.GetSourceType() )
        {
        case config::ScriptSourceType::Sample:
        {
            const int iRet = MessageBox(
                hParent,
                L"Are you sure?\n\n"
                L"You are trying to edit a sample script.\n"
                L"Any changes performed to the script will be applied to every panel that are using this sample.\n"
                L"These changes will also be lost when updating the component.",
                L"Editing script",
                MB_YESNO | MB_ICONWARNING );
            if ( iRet != IDYES )
            {
                break;
            }

            assert( settings.scriptPath );
            const auto filePath = *settings.scriptPath;
            if ( !fs::exists( filePath ) )
            {
                qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, fmt::format( "Sample script is missing: {}", filePath.u8string() ) );
                break;
            }

            smp::EditTextFile( hParent, filePath );
            break;
        }
        case config::ScriptSourceType::File:
        {
            assert( settings.scriptPath );
            const auto filePath = *settings.scriptPath;
            if ( !fs::exists( filePath ) )
            {
                qwr::ReportErrorWithPopup( SMP_UNDERSCORE_NAME, fmt::format( "Script is missing: {}", filePath.u8string() ) );
                break;
            }

            smp::EditTextFile( hParent, filePath );
            break;
        }
        case config::ScriptSourceType::InMemory:
        {
            assert( settings.script );
            auto script = *settings.script;
            smp::EditText( hParent, script );
            settings.script = script;

            return true;
        }
        case config::ScriptSourceType::Package:
        default:
        {
            assert( false );
            break;
        }
        }

        return false;
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e.what() );
    }
}

} // namespace smp::panel

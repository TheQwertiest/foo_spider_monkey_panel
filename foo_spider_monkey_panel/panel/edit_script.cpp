#include <stdafx.h>

#include "edit_script.h"

#include <config/resolved_panel_script_settings.h>
#include <utils/edit_text.h>

#include <qwr/error_popup.h>
#include <qwr/ui_centered_message_box.h>
#include <qwr/winapi_error_helpers.h>

namespace fs = std::filesystem;

namespace smp::panel
{

void EditScript( HWND hParent, qwr::u8string& script )
{
    smp::EditText( hParent, script, true );
}

void EditScriptFile( HWND hParent, const config::ResolvedPanelScriptSettings& settings )
{
    try
    {
        switch ( settings.GetSourceType() )
        {
        case config::ScriptSourceType::Sample:
        {
            const int iRet = qwr::ui::MessageBoxCentered(
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

            const auto filePath = settings.GetScriptPath();
            qwr::QwrException::ExpectTrue( fs::exists( filePath ), "Sample script is missing: {}", filePath.u8string() );

            smp::EditTextFile( hParent, filePath, true, true );
            break;
        }
        case config::ScriptSourceType::File:
        {
            const auto filePath = settings.GetScriptPath();
            qwr::QwrException::ExpectTrue( fs::exists( filePath ), "Script is missing: {}", filePath.u8string() );

            smp::EditTextFile( hParent, filePath, true, true );
            break;
        }
        default:
        {
            assert( false );
            break;
        }
        }
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

void EditPackageScript( HWND hParent, const std::filesystem::path& script )
{
    try
    {
        qwr::QwrException::ExpectTrue( fs::exists( script ), "Script is missing: {}", script.u8string() );

        smp::EditTextFile( hParent, script, true, true );
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

} // namespace smp::panel

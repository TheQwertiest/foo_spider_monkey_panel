#include <stdafx.h>

#include "edit_text.h"

#include <fb2k/config.h>
#include <ui/ui_edit_in_progress.h>
#include <ui/ui_editor.h>

#include <qwr/file_helpers.h>
#include <qwr/final_action.h>
#include <qwr/winapi_error_helpers.h>

#include <filesystem>

namespace
{

using namespace smp;

/// @remark For cases when modal might be called from another modal
class ConditionalModalScope
{
public:
    ConditionalModalScope( HWND hParent )
        : needsModalScope_( modal_dialog_scope::can_create() )
    {
        if ( needsModalScope_ )
        {
            scope_.initialize( hParent );
        }
    }

    ~ConditionalModalScope()
    {
        scope_.deinitialize();
    }

private:
    modal_dialog_scope scope_;
    bool needsModalScope_;
};

/// @throw qwr::QwrException
std::filesystem::path GetFixedEditorPath()
{
    namespace fs = std::filesystem;

    try
    {
        const auto editorPath = fs::u8path( static_cast<const std::u8string&>( smp::config::default_editor ) );
        if ( editorPath.empty() || !fs::exists( editorPath ) )
        {
            smp::config::default_editor = "";
            return fs::path{};
        }
        else
        {
            return editorPath;
        }
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

void NotifyParentPanel( HWND hParent )
{
    SendMessage( hParent, static_cast<INT>( InternalSyncMessage::ui_script_editor_saved ), 0, 0 );
}

void EditTextFileInternal( HWND hParent, const std::filesystem::path& file, bool isPanelScript )
{
    // TODO: handle BOM
    auto text = qwr::file::ReadFile( file, CP_ACP, true );
    {
        ConditionalModalScope scope( hParent );
        smp::ui::CEditor dlg( file.filename().u8string(), text, [&] {
            qwr::file::WriteFile( file, text );
            if ( isPanelScript )
            {
                NotifyParentPanel( hParent );
            }
        } );
        dlg.DoModal( hParent );
    }
}

bool EditTextFileExternal( HWND hParent, const std::filesystem::path& file, const std::filesystem::path& pathToEditor, bool isModal )
{
    if ( isModal )
    {
        ConditionalModalScope scope( hParent );
        ui::CEditInProgress dlg{ pathToEditor, file };
        return ( dlg.DoModal( hParent ) == IDOK );
    }
    else
    {
        const auto qPath = L"\"" + file.wstring() + L"\"";
        const auto hInstance = ShellExecute( nullptr,
                                             L"open",
                                             pathToEditor.c_str(),
                                             qPath.c_str(),
                                             nullptr,
                                             SW_SHOW );
        if ( (int)hInstance < 32 )
        { // As per WinAPI
            qwr::error::CheckWin32( (int)hInstance, "ShellExecute" );
        }

        return true;
    }
}

void EditTextInternal( HWND hParent, std::u8string& text, bool isPanelScript )
{
    ConditionalModalScope scope( hParent );
    smp::ui::CEditor dlg( "Temporary file", text, [&] {  
        if (isPanelScript)
            {
                NotifyParentPanel( hParent );
            } } );
    dlg.DoModal( hParent );
}

void EditTextExternal( HWND hParent, std::u8string& text, const std::filesystem::path& pathToEditor )
{
    namespace fs = std::filesystem;

    // keep .tmp for the uniqueness
    const auto fsTmpFilePath = [] {
        std::wstring tmpFilePath;
        tmpFilePath.resize( MAX_PATH - 14 ); // max allowed size of path in GetTempFileName

        DWORD dwRet = GetTempPath( tmpFilePath.size(), tmpFilePath.data() );
        qwr::error::CheckWinApi( dwRet && dwRet <= tmpFilePath.size(), "GetTempPath" );

        std::wstring filename;
        filename.resize( MAX_PATH );
        UINT uRet = GetTempFileName( tmpFilePath.c_str(),
                                     L"smp",
                                     0,
                                     filename.data() ); // buffer for name
        qwr::error::CheckWinApi( uRet, "GetTempFileName" );

        filename.resize( wcslen( filename.c_str() ) );

        return fs::path( tmpFilePath ) / filename;
    }();
    const qwr::final_action autoRemove( [&fsTmpFilePath] {
        try
            {
            fs::remove( fsTmpFilePath );
            }
            catch ( const fs::filesystem_error& )
            {
            } } );

    // use .tmp.js for proper file association
    const auto fsJsTmpFilePath = fs::path( fsTmpFilePath ).concat( L".js" );

    qwr::file::WriteFile( fsJsTmpFilePath, text );
    const qwr::final_action autoRemove2( [&fsJsTmpFilePath] { 
        try
            {
            fs::remove( fsJsTmpFilePath );
            }
            catch ( const fs::filesystem_error& )
            {
            } } );

    if ( !EditTextFileExternal( hParent, fsJsTmpFilePath, pathToEditor, true ) )
    {
        return;
    }

    text = qwr::file::ReadFile( fsJsTmpFilePath, CP_UTF8 );
}

} // namespace

namespace smp
{

void EditTextFile( HWND hParent, const std::filesystem::path& file, bool isPanelScript, bool isModal )
{
    const auto editorPath = GetFixedEditorPath();
    if ( editorPath.empty() )
    {
        EditTextFileInternal( hParent, file, isPanelScript );
    }
    else
    {
        EditTextFileExternal( hParent, file, editorPath, isModal );
        if ( isPanelScript )
        {
            NotifyParentPanel( hParent );
        }
    }
}

void EditText( HWND hParent, std::u8string& text, bool isPanelScript )
{
    const auto editorPath = GetFixedEditorPath();
    if ( editorPath.empty() )
    {
        EditTextInternal( hParent, text, isPanelScript );
    }
    else
    {
        EditTextExternal( hParent, text, editorPath );
        if ( isPanelScript )
        {
            NotifyParentPanel( hParent );
        }
    }
}

} // namespace smp

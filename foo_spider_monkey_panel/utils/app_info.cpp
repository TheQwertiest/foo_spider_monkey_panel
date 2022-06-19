#include <stdafx.h>

#include "app_info.h"

#include <commoncontrols.h>
#include <component_paths.h>
#include <shellapi.h>

#include <qwr/file_helpers.h>
#include <qwr/final_action.h>
#include <qwr/winapi_error_helpers.h>

_COM_SMARTPTR_TYPEDEF( IEnumAssocHandlers, IID_IEnumAssocHandlers );
_COM_SMARTPTR_TYPEDEF( IAssocHandler, IID_IAssocHandler );

namespace smp
{

std::vector<AppInfo> GetAppsAssociatedWithExtension( const std::wstring& ext )
{
    IEnumAssocHandlersPtr enumAssocHandler;
    HRESULT hr = SHAssocEnumHandlers( ext.c_str(), ASSOC_FILTER_RECOMMENDED, &enumAssocHandler );
    qwr::error::CheckHR( hr, "SHAssocEnumHandlers" );

    std::vector<smp::AppInfo> result;

    IAssocHandlerPtr assocHandler;
    ULONG celtFetched = 0;
    while ( SUCCEEDED( enumAssocHandler->Next( 1, &assocHandler, &celtFetched ) ) )
    {
        if ( !assocHandler )
        {
            break;
        }

        wchar_t* pFilePath = nullptr;
        wchar_t* pFileName = nullptr;
        if ( !SUCCEEDED( assocHandler->GetName( &pFilePath ) )
             || !SUCCEEDED( assocHandler->GetUIName( &pFileName ) ) )
        {
            continue;
        }

        smp::AppInfo editor;
        editor.appName = pFileName;
        editor.appPath = pFilePath;

        result.emplace_back( editor );
    }

    return result;
}

CIcon GetAppIcon( const std::filesystem::path& filePath )
{
    CImageList imageList;
    HRESULT hr = SHGetImageList( SHIL_SYSSMALL, IID_IImageList, (void**)&imageList.m_hImageList );
    if ( !SUCCEEDED( hr ) )
    {
        return nullptr;
    }

    SHFILEINFO sfi{};
    SHGetFileInfo( filePath.c_str(), 0, &sfi, sizeof( sfi ), SHGFI_SYSICONINDEX | SHGFI_SMALLICON );

    return imageList.GetIcon( sfi.iIcon, ILD_TRANSPARENT );
}

} // namespace smp

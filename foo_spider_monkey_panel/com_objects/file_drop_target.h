#pragma once

#include <com_objects/com_tools.h>
#include <com_objects/drop_target_impl.h>

#include <qwr/final_action.h>

namespace smp::com
{

class FileDropTarget
    : public smp::com::IDropTargetImpl
{
public:
    FileDropTarget( HWND hDropWnd, HWND hNotifyWnd );

    static UINT GetOnDropMsg();

    template <typename T>
    static LRESULT ProcessMessage( HWND hDropWnd, WPARAM wParam, LPARAM lParam, T processor )
    {
        assert( hDropWnd == reinterpret_cast<HWND>( wParam ) );

        auto pDataObj = reinterpret_cast<IDataObject*>( lParam );
        const auto autoDrop = qwr::final_action( [pDataObj] {
            pDataObj->Release();
        } );

        FORMATETC fmte = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM stgm;
        if ( !SUCCEEDED( pDataObj->GetData( &fmte, &stgm ) ) )
        {
            return 0;
        }
        const auto autoStgm = qwr::final_action( [&stgm] {
            ReleaseStgMedium( &stgm );
        } );

        const auto hDrop = reinterpret_cast<HDROP>( stgm.hGlobal );

        const auto fileCount = DragQueryFile( hDrop, 0xFFFFFFFF, nullptr, 0 );
        if ( !fileCount )
        {
            return 0;
        }

        for ( const auto i: ranges::views::indices( static_cast<int>( fileCount ) ) )
        {
            const auto pathLength = DragQueryFile( hDrop, i, nullptr, 0 );
            std::wstring path;
            path.resize( pathLength + 1 );

            DragQueryFile( hDrop, i, path.data(), path.size() );
            path.resize( path.size() - 1 );

            processor( path );
        }

        return 0;
    }

protected:
    void FinalRelease();

private:
    BEGIN_COM_QI_IMPL()
        COM_QI_ENTRY_MULTI( IUnknown, IDropTarget )
        COM_QI_ENTRY( IDropTarget )
    END_COM_QI_IMPL()

    // com::IDropTargetImpl
    DWORD OnDragEnter( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD dwEffect ) override;
    DWORD OnDragOver( DWORD grfKeyState, POINTL pt, DWORD dwEffect ) override;
    DWORD OnDrop( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD dwEffect ) override;
    void OnDragLeave() override;

    [[nodiscard]] DWORD GetEffect() const;

    [[nodiscard]] static bool IsFile( IDataObject* pDataObj );

private:
    HWND hDropWnd_ = nullptr;
    HWND hNotifyWnd_ = nullptr;
    bool isFile_ = false;
};

} // namespace smp::com

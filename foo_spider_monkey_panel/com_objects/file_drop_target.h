#pragma once

#include <com_objects/com_tools.h>
#include <com_objects/drop_target_impl.h>

namespace smp::com
{

class FileDropTarget
    : public smp::com::IDropTargetImpl
{
public:
    FileDropTarget( HWND hDropWnd, HWND hNotifyWnd );

    static UINT GetOnDropMsg();

protected:
    void FinalRelease();

private:
    BEGIN_COM_QI_IMPL()
        COM_QI_ENTRY_MULTI( IUnknown, IDropTarget )
        COM_QI_ENTRY( IDropTarget )
    END_COM_QI_IMPL()

    // com::IDropTargetImpl
    HRESULT OnDragEnter( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect ) override;
    HRESULT OnDragOver( DWORD grfKeyState, POINTL pt, DWORD* pdwEffect ) override;
    HRESULT OnDrop( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect ) override;
    HRESULT OnDragLeave() override;

    DWORD GetEffect() const;

    static bool IsFile( IDataObject* pDataObj );

private:
    HWND hDropWnd_ = nullptr;
    HWND hNotifyWnd_ = nullptr;
    bool isFile_ = false;
};

} // namespace smp::com

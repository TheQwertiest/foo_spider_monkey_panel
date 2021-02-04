#pragma once

namespace smp::ui
{

template <class T = CWindow>
class CFileDropListBox : public CWindowImpl<CFileDropListBox<CWindow>, CListBox>
{
public:
    CFileDropListBox( HWND hWnd = nullptr )
    {
    }

    CFileDropListBox& operator=( HWND hWnd )
    {
        m_hWnd = hWnd;
        return *this;
    }

    void Initialize()
    {
        this->DragAcceptFiles();
    }

    static UINT GetOnDropMsg()
    {
        static const UINT msgId = ::RegisterWindowMessage( L"smp_ui_file_drop" );
        return msgId;
    }

private:
    BEGIN_MSG_MAP( CFileDropListBox<T> )
        MSG_WM_DROPFILES( OnDropFiles )
    END_MSG_MAP()

    void OnDropFiles( HDROP hDropInfo )
    {
        this->GetParent().SendMessage( GetOnDropMsg(), (WPARAM)hDropInfo );
    }
};

} // namespace smp::ui

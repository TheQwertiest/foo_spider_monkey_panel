#pragma once

#include <qwr/type_traits.h>

#include <memory>

namespace smp
{

namespace impl
{

template <class T>
concept SupportedGdiType = qwr::is_any_same_v<T, HDC, HPEN, HBRUSH, HRGN, HPALETTE, HFONT, HBITMAP>;

}

template <impl::SupportedGdiType T>
class GdiObjectSelector
{
public:
    [[nodiscard]] GdiObjectSelector( HDC hDc, T hNewObject )
        : hDc_( hDc )
        , hOldObject_( ::SelectObject( hDc, hNewObject ) )
    {
    }

    ~GdiObjectSelector()
    {
        (void)::SelectObject( hDc_, hOldObject_ );
    }

private:
    HDC hDc_ = nullptr;
    HGDIOBJ hOldObject_ = nullptr;
};

} // namespace smp

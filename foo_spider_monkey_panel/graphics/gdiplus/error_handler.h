#pragma once

#include <optional>

namespace smp
{

namespace internal
{

// TODO: move to a better place

template <typename T>
concept SmartPtr = requires( T p ) {
    *p;
    p.operator->();
};

} // namespace internal

[[nodiscard]] const char* GdiPlusStatusToText( Gdiplus::Status errorCode );

/// @throw qwr::QwrException
void CheckGdiPlusStatus( Gdiplus::Status gdiStatus, std::string_view functionName );

/// @throw qwr::QwrException
template <internal::SmartPtr T, typename T_Parent = T::element_type>
void CheckGdiPlusObject( const T& pObj, const T_Parent* pParentObj = nullptr )
{
    // GetLastStatus() resets status, so it needs to be saved here
    const auto statusOpt = [&]() -> std::optional<Gdiplus::Status> {
        if ( pObj )
        {
            return pObj->GetLastStatus();
        }

        if ( pParentObj )
        {
            return pParentObj->GetLastStatus();
        }

        return std::nullopt;
    }();

    if ( pObj && Gdiplus::Status::Ok == statusOpt )
    {
        return;
    }

    if ( statusOpt )
    {
        throw qwr::QwrException( "Failed to create GdiPlus object ({:#x}): {}",
                                 static_cast<int>( *statusOpt ),
                                 GdiPlusStatusToText( *statusOpt ) );
    }
    else
    {
        throw qwr::QwrException( "Failed to create GdiPlus object" );
    }
}

} // namespace smp

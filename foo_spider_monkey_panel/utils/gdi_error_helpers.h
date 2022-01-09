#pragma once

#include <utils/gdi_helpers.h>

#include <optional>

namespace qwr::error
{

[[nodiscard]] const char* GdiErrorCodeToText( Gdiplus::Status errorCode );

/// @throw qwr::QwrException
void CheckGdi( Gdiplus::Status gdiStatus, std::string_view functionName );

/// @throw qwr::QwrException
template <typename T, typename T_Parent = T>
void CheckGdiPlusObject( const std::unique_ptr<T>& obj, const T_Parent* pParentObj = nullptr )
{
    // GetLastStatus() resets status, so it needs to be saved here
    const auto status = [&obj, pParentObj]() -> std::optional<Gdiplus::Status> {
        if ( obj )
        {
            return obj->GetLastStatus();
        }

        if ( pParentObj )
        {
            return pParentObj->GetLastStatus();
        }

        return std::nullopt;
    }();

    if ( obj && Gdiplus::Status::Ok == status )
    {
        return;
    }

    if ( status )
    {
        throw qwr::QwrException( "Failed to create GdiPlus object ({:#x}): {}", *status, GdiErrorCodeToText( *status ) );
    }
    else
    {
        throw qwr::QwrException( "Failed to create GdiPlus object" );
    }
}

} // namespace qwr::error

#pragma once

#include <string>

namespace smp::art
{

struct LoadingOptions
{
    /// @throw qwr::QwrException
    LoadingOptions( uint32_t artId, bool fallbackToStubImage, bool loadOnlyEmbedded, bool onlyGetPath );
    LoadingOptions( const LoadingOptions& other );

    const uint32_t artId;
    const bool fallbackToStubImage;
    const bool loadOnlyEmbedded;
    const bool onlyGetPath;
};

} // namespace smp::art

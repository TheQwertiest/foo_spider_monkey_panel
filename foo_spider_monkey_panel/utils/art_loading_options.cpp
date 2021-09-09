#include <stdafx.h>

#include "art_loading_options.h"

#include <utils/art_helpers.h>

namespace smp::art
{

LoadingOptions::LoadingOptions( uint32_t artId, bool fallbackToStubImage, bool loadOnlyEmbedded, bool onlyGetPath )
    : artId( artId )
    , fallbackToStubImage( fallbackToStubImage )
    , loadOnlyEmbedded( loadOnlyEmbedded )
    , onlyGetPath( onlyGetPath )
{
    (void)smp::art::GetGuidForArtId( artId );
}

LoadingOptions::LoadingOptions( const LoadingOptions& other )
    : artId( other.artId )
    , fallbackToStubImage( other.fallbackToStubImage )
    , loadOnlyEmbedded( other.loadOnlyEmbedded )
    , onlyGetPath( other.onlyGetPath )
{
}

} // namespace smp::art

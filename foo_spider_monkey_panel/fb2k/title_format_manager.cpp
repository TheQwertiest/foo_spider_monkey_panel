#include <stdafx.h>

#include "title_format_manager.h"

#include <qwr/algorithm.h>
#include <qwr/utility.h>

namespace smp
{

TitleFormatManager::TitleFormatManager()
{
}

TitleFormatManager& TitleFormatManager::Get()
{
    assert( core_api::is_main_thread() );
    static TitleFormatManager cache;
    return cache;
}

titleformat_object::ptr TitleFormatManager::Load( const qwr::u8string& spec, const qwr::u8string& fallback ) const
{
    const auto specWithFallback = spec + '\0' + fallback;
    if ( auto pTitleFormat = qwr::FindAsPointer( specToTitleFormat_, specWithFallback ) )
    {
        return *pTitleFormat;
    }

    titleformat_object::ptr titleFormat;
    titleformat_compiler::get()->compile_safe_ex( titleFormat, spec.c_str(), fallback.c_str() );
    const auto [it, isEmplaced] = specToTitleFormat_.try_emplace( specWithFallback, titleFormat );
    return it->second;
}

void TitleFormatManager::ClearCache()
{
    specToTitleFormat_.clear();
}

} // namespace smp

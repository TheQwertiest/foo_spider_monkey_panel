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

titleformat_object::ptr TitleFormatManager::Load( const qwr::u8string& query, const qwr::u8string& fallback ) const
{
    const auto queryWithFallback = query + '\0' + fallback;
    if ( auto pTitleFormat = qwr::FindAsPointer( queryToTitleFormat_, queryWithFallback ) )
    {
        return *pTitleFormat;
    }

    titleformat_object::ptr titleFormat;
    titleformat_compiler::get()->compile_safe_ex( titleFormat, query.c_str(), fallback.c_str() );
    const auto [it, isEmplaced] = queryToTitleFormat_.try_emplace( queryWithFallback, titleFormat );
    return it->second;
}

void TitleFormatManager::ClearCache()
{
    queryToTitleFormat_.clear();
}

} // namespace smp

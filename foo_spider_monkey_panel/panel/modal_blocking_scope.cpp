#include <stdafx.h>

#include "modal_blocking_scope.h"

#include <js_engine/js_engine.h>

namespace
{

std::atomic<int32_t> g_whitelistedModalCounter = 0;
std::atomic<int32_t> g_modalBlockingCounter = 0;

} // namespace

namespace smp::modal
{

ConditionalModalScope::ConditionalModalScope( HWND hParent, bool isWhitelistedModal )
    : needsModalScope_( modal_dialog_scope::can_create() )
    , isWhitelistedModal_( isWhitelistedModal )
{
    if ( needsModalScope_ )
    {
        scope_.initialize( hParent );
    }

    ++g_modalBlockingCounter;
    g_whitelistedModalCounter += ( isWhitelistedModal_ ? 1 : 0 );
}

ConditionalModalScope::~ConditionalModalScope()
{
    --g_modalBlockingCounter;
    g_whitelistedModalCounter -= ( isWhitelistedModal_ ? 1 : 0 );
}

MessageBlockingScope::MessageBlockingScope()
{
    ++g_modalBlockingCounter;
}

MessageBlockingScope::~MessageBlockingScope()
{
    --g_modalBlockingCounter;
}

ModalBlockingScope::ModalBlockingScope( HWND hParent, bool isWhitelistedModal )
    : isWhitelistedModal_( isWhitelistedModal )
{
    scope_.initialize( hParent );
    ++g_modalBlockingCounter;
    g_whitelistedModalCounter += ( isWhitelistedModal_ ? 1 : 0 );
}

ModalBlockingScope::~ModalBlockingScope()
{
    --g_modalBlockingCounter;
    g_whitelistedModalCounter -= ( isWhitelistedModal_ ? 1 : 0 );
}

bool IsModalBlocked()
{
    return ( ( core_api::is_main_thread() && !modal_dialog_scope::can_create() ) || g_modalBlockingCounter );
}

bool IsInWhitelistedModal()
{
    return g_whitelistedModalCounter;
}

WhitelistedScope::WhitelistedScope()
{
    ++g_whitelistedModalCounter;
}

WhitelistedScope::~WhitelistedScope()
{
    --g_whitelistedModalCounter;
}

} // namespace smp::modal

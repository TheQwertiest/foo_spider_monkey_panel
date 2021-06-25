#include <stdafx.h>

#include "modal_blocking_scope.h"

#include <js_engine/js_engine.h>

namespace
{

std::atomic<int32_t> whitelistedModalCounter = 0;
std::atomic<int32_t> modalBlockingCounter = 0;

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

    ++modalBlockingCounter;
    whitelistedModalCounter += ( isWhitelistedModal_ ? 1 : 0 );
}

ConditionalModalScope::~ConditionalModalScope()
{
    --modalBlockingCounter;
    whitelistedModalCounter -= ( isWhitelistedModal_ ? 1 : 0 );
}

MessageBlockingScope::MessageBlockingScope()
{
}

MessageBlockingScope::~MessageBlockingScope()
{
}

ModalBlockingScope::ModalBlockingScope( HWND hParent, bool isWhitelistedModal )
    : isWhitelistedModal_( isWhitelistedModal )
{
    scope_.initialize( hParent );
    ++modalBlockingCounter;
    whitelistedModalCounter += ( isWhitelistedModal_ ? 1 : 0 );
}

ModalBlockingScope::~ModalBlockingScope()
{
    --modalBlockingCounter;
    whitelistedModalCounter -= ( isWhitelistedModal_ ? 1 : 0 );
}

bool IsModalBlocked()
{
    return ( !modal_dialog_scope::can_create() || modalBlockingCounter );
}

bool IsInWhitelistedModal()
{
    return whitelistedModalCounter;
}

} // namespace smp::modal

#include <stdafx.h>

#include "engine.h"

#include <com/utils/com_destruction_handler.h>
#include <js_backend/engine/context.h>
#include <timeout/timer_manager_custom.h>
#include <timeout/timer_manager_native.h>

#include <js/Initialization.h>

using namespace smp;

namespace mozjs
{

JsEngine::JsEngine()
    : pContext_( std::unique_ptr<ContextInner>( new ContextInner() ) )
{
    JS_Init();
}

JsEngine::~JsEngine()
{ // Can't clean up here, since mozjs.dll might be already unloaded
    assert( !pContext_ );
}

JsEngine& JsEngine::Get()
{
    static JsEngine je;
    return je;
}

ContextInner& JsEngine::GetContext()
{
    assert( Get().pContext_ );
    return *Get().pContext_;
}

void JsEngine::PrepareForExit()
{
    shouldShutdown_ = true;
    if ( pContext_ )
    {
        pContext_->PrepareForExit();
    }
}

void JsEngine::Finalize()
{
    if ( shouldShutdown_ )
    {
        TimerManager_Custom::Get().Finalize();
        TimerManager_Native::Get().Finalize();
        pContext_.reset();
        JS_ShutDown();
        smp::com::DeleteAllStoredObject();
    }
}

} // namespace mozjs

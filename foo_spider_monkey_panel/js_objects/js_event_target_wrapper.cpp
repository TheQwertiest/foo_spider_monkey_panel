#include <stdafx.h>

#include "js_event_target_wrapper.h"

#include <js_objects/js_event_target.h>

using namespace smp;

namespace mozjs
{

JsEventTargetWrapper::JsEventTargetWrapper( JSContext* cx )
    : pEventTarget_( JsEventTarget::CreateNative( cx ) )
{
}

JsEventTargetWrapper::~JsEventTargetWrapper()
{
}

void JsEventTargetWrapper::AddEventListener( const qwr::u8string& type, JS::HandleValue listener )
{
    pEventTarget_->AddEventListener( type, listener );
}

void JsEventTargetWrapper::RemoveEventListener( const qwr::u8string& type, JS::HandleValue listener )
{
    pEventTarget_->RemoveEventListener( type, listener );
}

void JsEventTargetWrapper::DispatchEvent( JS::HandleValue event )
{
    pEventTarget_->DispatchEvent( event );
}

} // namespace mozjs

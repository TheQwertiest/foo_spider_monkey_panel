#pragma once

#include <js_objects/object_base.h>
#include <js_utils/js_fwd.h>

namespace mozjs
{

class JsEventTarget;

class JsEventTargetWrapper
{
public:
    JsEventTargetWrapper( JSContext* cx );
    virtual ~JsEventTargetWrapper();

public:
    void AddEventListener( const qwr::u8string& type, JS::HandleValue listener );
    void RemoveEventListener( const qwr::u8string& type, JS::HandleValue listener );
    void DispatchEvent( JS::HandleValue event );

private:
    std::unique_ptr<JsEventTarget> pEventTarget_;
};

} // namespace mozjs

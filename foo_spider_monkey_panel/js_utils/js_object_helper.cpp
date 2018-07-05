#include <stdafx.h>
#include "js_object_helper.h"


namespace mozjs
{

bool DummyGetter( JSContext* cx, unsigned argc, JS::Value* vp )
{
    vp->setUndefined();
    return true;
}

JSObject* GetJsObjectFromValue( JSContext* cx, JS::HandleValue jsValue )
{
    if ( !jsValue.isObject() )
    {        
        return nullptr;
    }

    return jsValue.toObjectOrNull();
}

}

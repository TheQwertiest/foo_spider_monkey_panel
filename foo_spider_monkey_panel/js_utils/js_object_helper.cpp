#include <stdafx.h>
#include "js_object_helper.h"


namespace mozjs
{

JSObject* GetJsObjectFromValue( JSContext* cx, JS::HandleValue jsValue )
{
    if ( !jsValue.isObject() )
    {        
        return nullptr;
    }

    return jsValue.toObjectOrNull();
}

}

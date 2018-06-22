#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  


namespace mozjs
{

template<typename MozjsObjectType>
class JsObjectWrapper final
{
public:
    ~JsObjectWrapper()
    {
        jsObject_.reset();
    }

    static JsObjectWrapper* Create( JSContext* cx, JS::HandleObject global )
    {
        JSAutoRequest ar( cx );
        JSAutoCompartment ac( cx, global );

        JS::RootedObject pJsObject( cx, MozjsObjectType::Create( cx ) );
        if ( !pJsObject )
        {
            return nullptr;
        }
        
        return new JsObjectWrapper( cx, pJsObject );
    }

// TODO: add enable_if<MozjsObjectType::HasPrivate> somehow    
    MozjsObjectType* GetWrappedObject()
    {
        return static_cast<MozjsObjectType *>(JS_GetPrivate( jsObject_ ));
    }
    
    JS::HandleObject GetJsObject()
    {
        return jsObject_;
    }

private: 
    JsObjectWrapper( JSContext* cx, JS::HandleObject jsObject )
    {
        jsObject_.init( cx, jsObject );
    }
    JsObjectWrapper( const JsObjectWrapper & ) = delete;

private:
    JS::PersistentRootedObject jsObject_;
};

}

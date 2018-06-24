#pragma once

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <js_objects/global_object.h>

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

    template<
        typename LocalMozjType = MozjsObjectType, 
        typename = typename std::enable_if<std::is_same<LocalMozjType, JsGlobalObject>::value >::type, 
        typename ... ArgTypes
    >
    static 
        JsObjectWrapper* Create( JSContext* cx, ArgTypes&&... args )
    {
        JSAutoRequest ar( cx );

        JS::RootedObject pJsObject( cx, MozjsObjectType::Create( cx, std::forward<ArgTypes>( args )... ) );
        if ( !pJsObject )
        {
            return nullptr;
        }

        return CreateInternal( cx, pJsObject );
    }

    template<
        typename LocalMozjType = MozjsObjectType,
        typename = typename std::enable_if<!std::is_same<LocalMozjType, JsGlobalObject>::value >::type,
        typename ... ArgTypes
    >
    static JsObjectWrapper* Create( JSContext* cx, JS::HandleObject global, ArgTypes&&... args )
    {
        JSAutoRequest ar( cx );
        JSAutoCompartment ac( cx, global );

        JS::RootedObject pJsObject( cx, MozjsObjectType::Create( cx, std::forward<ArgTypes>( args )... ) );
        if ( !pJsObject )
        {
            return nullptr;
        }

        return CreateInternal( cx, pJsObject );
    }

    template<
        typename LocalMozjType = MozjsObjectType,
        typename = typename std::enable_if<!std::is_same<LocalMozjType, JsGlobalObject>::value >::type        
    >
    static JsObjectWrapper* Create( JSContext* cx, JS::HandleObject global )
    {
        JSAutoRequest ar( cx );
        JSAutoCompartment ac( cx, global );

        JS::RootedObject pJsObject( cx, MozjsObjectType::Create( cx ) );
        if ( !pJsObject )
        {
            return nullptr;
        }

        return CreateInternal( cx, pJsObject );
    }

    explicit operator JS::HandleObject()
    {
        return GetJsObject();
    }
   
    JS::HandleObject GetJsObject()
    {
        return jsObject_;
    }

    MozjsObjectType * GetNativeObject()
    {
        assert( pNativeObject_ );
        return pNativeObject_;
    }

private: 
    JsObjectWrapper( JSContext* cx, JS::HandleObject jsObject, MozjsObjectType * pNativeObject )
    {
        jsObject_.init( cx, jsObject );
        pNativeObject_ = pNativeObject;
    }
    JsObjectWrapper( const JsObjectWrapper & ) = delete;

    static JsObjectWrapper* CreateInternal( JSContext* cx, JS::HandleObject jsObject )
    {
        MozjsObjectType * pNativeObject = NULL;
        const JSClass * jsClass = JS_GetClass( jsObject );
        if ( jsClass
             && (jsClass->flags & JSCLASS_HAS_PRIVATE) )
        {
            pNativeObject = static_cast<MozjsObjectType *>(JS_GetPrivate( jsObject ));
        }

        return new JsObjectWrapper( cx, jsObject, pNativeObject );
    }


private:
    JS::PersistentRootedObject jsObject_;
    MozjsObjectType * pNativeObject_;
};


class JsUnknownObjectWrapper final
{
public:
    JsUnknownObjectWrapper( const JsUnknownObjectWrapper & rhv )
        : pJsCtx_( rhv.pJsCtx_ )
        , jsObject_( rhv.pJsCtx_, rhv.jsObject_ )
    {
    }

    JsUnknownObjectWrapper( const JsUnknownObjectWrapper && rhv )
        : pJsCtx_( rhv.pJsCtx_ )
        , jsObject_( rhv.pJsCtx_, rhv.jsObject_ )
    {
    }

    ~JsUnknownObjectWrapper()
    {
        jsObject_.reset();
    }

    static JsUnknownObjectWrapper* Create( JSContext* cx, JS::HandleObject jsObject )
    {
        return new JsUnknownObjectWrapper( cx, jsObject );
    }

    static JsUnknownObjectWrapper* Create( JSContext* cx, JSObject* pJsObject )
    {
        JS::RootedObject jsObject( cx, pJsObject );
        return new JsUnknownObjectWrapper( cx, jsObject );
    }

    explicit operator JS::HandleObject()
    {
        return GetJsObject();
    }

    JS::HandleObject GetJsObject()
    {
        return jsObject_;
    }

private:
    JsUnknownObjectWrapper( JSContext* cx, JS::HandleObject jsObject )
        : pJsCtx_( cx )
        , jsObject_( cx, jsObject )
    {
    }

private:
    JS::PersistentRootedObject jsObject_;
    JSContext* pJsCtx_;
};

}

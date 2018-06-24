/// Code extracted from wrap_com.cpp from JSDB by Shanti Rao (see http://jsdb.org)

#include <stdafx.h>
#include "active_x.h"

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <oleauto.h>

#include <vector>

#define GETPRIVATE(type) (((JSPointer<type>*)JS_GetPrivate(cx,obj))->P)

#define GETPOINTER (((JSPointerBase*)JS_GetPrivate(cx,obj)))

#define GETOBJ(type,name) type * name = NULL;\
 JSPointer<type> *ptr_ ## name = (JSPointer<type>*)JS_GetPrivate(cx,obj); \
 if (ptr_ ## name) name = *ptr_ ## name;\
 if (!name) return JS_FALSE\

#define GETENV JSDBEnvironment* Env = \
 (JSDBEnvironment*)JS_GetContextPrivate(cx)

#define SETPRIVATE(obj,x,t,ad,parent) \
 JS_SetPrivate(cx,obj,new JSPointer<x>(parent,t,ad))

#define DELPRIVATE(x) \
JSPointer<x> * t = \
   (JSPointer<x>*)JS_GetPrivate(cx,obj);\
 if (t) delete t; JS_SetPrivate(cx,obj,NULL)

#define CLOSEPRIVATE(x) \
JSPointer<x> * t = \
   (JSPointer<x>*)JS_GetPrivate(cx,obj);\
 if (t) t->Close()


#define INITCLASS(name) \
 Env->name = JS_InitClass(cx, obj, NULL, name ## _Class(),\
 name ## _ ## name, 0,\
 name ## _properties, name ## _functions,NULL,name ## _fnstatic);

#define MAKENEW(name) \
obj = JS_NewObject(cx, JS_GetClass(Env->name),Env->name, NULL);\
JS_DefineFunctions(cx,obj,name ## _functions);\
JS_DefineProperties(cx,obj,name ## _properties)

#define RETINT(x) {*rval = INT_TO_JSVAL(x); return JS_TRUE;}


//#define DEBUG
//#define TRACE
//dword-word-word-byte[8]
//{85BBD920-42A0-1069-A2E4-08002B30309D}
//HRESULT CLSIDFromString(LPOLESTR lpsz,LPCLSID pclsid);
//S_OK == StringFromCLSID(REFCLSID rclsid,LPOLESTR * ppsz);

#ifndef DISPID_PROPERTYPUT
#   define DISPID_PROPERTYPUT (-3)
#endif


namespace
{

void ActiveX_JSFinalize(JSFreeOp* fop, JSObject *obj )
{//( JSContext *cx, JSObject *obj )
    DELPRIVATE( ActiveX );
}


//using namespace mozjs;

JSClassOps jsOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    ActiveX_JSFinalize,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "ActiveX",
    JSCLASS_HAS_PRIVATE,
    &jsOps
};

//shadow property 0 to 255
bool
ActiveX_JSGet( JSContext* cx, unsigned argc, JS::Value* vp )
{// JSContext *cx, JSObject *obj, jsid id, jsval *rval 
    if ( !JSVAL_IS_INT( id ) ) return false;
    int x = JSVAL_TO_INT( id );

    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );
    /*
    GETENV;
    GETOBJ2( ActiveX, ActiveX, t );*/
    ActiveX* t = static_cast<ActiveX*>(JS_GetPrivate( args.thisv().toObjectOrNull() ));
    if ( !t )
    {
        //JS_ReportErrorASCII( cx, "Internal error: JS_GetPrivate failed" );
        return false;
    }

    switch ( x )
    {
    case 255: 
    {
        RETSTRW( L"ActiveX" );
        return true;
    }
    }

    ActiveX::PropInfo * p = t->Properties[x].get();
    DISPID dispid = 0;
    if ( p && t->Id( p->name, dispid ) )
    {
        return t->Get( dispid, cx, 0, vp );
    }

    return false;
}

bool
ActiveX_JSSet( JSContext *cx, unsigned argc, JS::Value* vp )
{
    if ( !JSVAL_IS_INT( id ) ) return false;
    int x = JSVAL_TO_INT( id );

    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );
    /*
    GETENV;
    GETOBJ2( ActiveX, ActiveX, t );
    */

    ActiveX* t = static_cast<ActiveX*>(JS_GetPrivate( args.thisv().toObjectOrNull() ));
    if ( !t )
    {
        //JS_ReportErrorASCII( cx, "Internal error: JS_GetPrivate failed" );
        return false;
    }

    // DISPID dispid=0;
    // if (t->Id(id,dispid))

    ActiveX::PropInfo * p = t->Properties[x].get();
    DISPID dispid = 0;
    if ( p && t->Id( p->name, dispid ) )
    {
        return t->Set( dispid, cx, 0, vp, p->PutRef );
    }

    return false;
}
/*

static JSPropertySpec ActiveX_properties[] = {
{ "className",255, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT,ActiveX_JSGet },
{ 0 }
};

*/

const JSPropertySpec jsProperties[] = {
    JS_PSG( "className", ActiveX_JSGet, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT ),
    JS_PS_END
};

const JSFunctionSpec jsFunctions[] = {
    JS_FN( "get", ActiveX_Get, 1, 0 ),
    JS_FN( "set", ActiveX_Set, 2, 0 ),
    JS_FN( "exec", ActiveX_Exec, 2, 0 ),
    JS_FN( "at", ActiveX_Exec, 2, 0 ),
    JS_FN( "as", ActiveX_as, 2, 0 ),
    JS_FN( "close", ActiveX_Close, 0, 0 ),
    JS_FN( "toString", ActiveX_ToString, 0, 0 ),
    JS_FS_END
};

/*
static JSFunctionSpec ActiveX_functions[] = {
{ "get",     ActiveX_Get,      1 },
{ "set",    ActiveX_Set, 2 },
{ "exec",    ActiveX_Exec, 2 },
{ "at",    ActiveX_Exec, 2 },
{ "as",    ActiveX_as, 2 },
{ "close",ActiveX_Close,0 },
{ "toString",ActiveX_ToString,0 },
{ 0 }
};
*/
/*
WRAP_HELP( ActiveX,
           "name(index)\nextract(index)\nextract(index,string)\nsize(index)\n"
           "close()\n" );
*/
bool ActiveX_Run( JSContext* cx, unsigned argc, JS::Value* vp )
{
    /*
    GETENV;
    GETARGS;
    GETOBJ( ActiveX, ActiveX, t );
    */

    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    ActiveX* t = static_cast<ActiveX*>(JS_GetPrivate( args.thisv().toObjectOrNull() ));
    if ( !t )
    {
        //JS_ReportErrorASCII( cx, "Internal error: JS_GetPrivate failed" );
        return false;
    }

    JSString* s = JS_GetFunctionId( JS_ValueToFunction( cx, argv[-2] ) );
    if ( s )
    {
        const wchar_t* name = (wchar_t*)JS_GetStringChars( s );
        if ( !name )
        {
            ERR_MSG( ActiveX, Exec, "No function name" );
        }

        DISPID dispid;
        if ( !t->Id( (wchar_t*)name, dispid ) )
        {
            ERR_MSG( ActiveX, "This object does not have that function", TStr( name ) );
        }
        if ( !t->Invoke( dispid, cx, argc, vp ) )
        {
            return false;
            //      RETOBJ(0);
                  //JavaScript handles the exception with SetPendingException
            ERR_MSG( ActiveX, "IDispatch->Invoke failed", TStr( (wchar_t*)name ) );
        }
    }
    return true;
}

bool ActiveX_Exec( JSContext* cx, unsigned argc, JS::Value* vp )
{
    /*
    GETENV;
    GETOBJ( ActiveX, ActiveX, t );
    */

    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    ActiveX* t = static_cast<ActiveX*>(JS_GetPrivate( args.thisv().toObjectOrNull() ));
    if ( !t )
    {
        //JS_ReportErrorASCII( cx, "Internal error: JS_GetPrivate failed" );
        return false;
    }

    if ( argc < 1 )
    {
        ERR_COUNT( ActiveX, Exec );
    }
    if ( !ISSTR( 0 ) )
    {
        ERR_TYPE( ActiveX, Exec, 1, string );
    }
    JSString* s = JSVAL_TO_STRING( ARGV( 0 ) );
    if ( s )
    {
        jschar* name = JS_GetStringChars( s );
        if ( !name )
        {
            ERR_MSG( ActiveX, Exec, "No function name" );
        }
        DISPID dispid;
        if ( !t->Id( name, dispid ) )
        {
            ERR_MSG( ActiveX, "This object does not have that function", TStr( name ) );
        }
        if ( !t->Invoke( dispid, cx, argc - 1, args + 1, rval ) )
        {
            return false;
            //      ERR_MSG(ActiveX,"IDispatch->Invoke failed",TStr(name));
        }
    }
    return true;
}

///Get("property","index","index")
bool ActiveX_ToString( JSContext* cx, unsigned argc, JS::Value* vp )
{
    /*
    GETARGS;
    GETENV;
    GETOBJ( ActiveX, ActiveX, t );
    */

    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    ActiveX* t = static_cast<ActiveX*>(JS_GetPrivate( args.thisv().toObjectOrNull() ));
    if ( !t )
    {
        //JS_ReportErrorASCII( cx, "Internal error: JS_GetPrivate failed" );
        return false;
    }

    if ( t->variant.vt )
    {
        RETSTRW( L"variant" );
        return true;
    }

    DISPID dispid = 0;
    if ( t->Id( (wchar_t*)L"toString", dispid ) )
    {
        t->Invoke( dispid, cx, argc, vp );
        return true;
    }

    //if (!t->Get(dispid, cx, 0,vp, false))
    RETSTRW( L"" );
    return true;
}

bool ActiveX_Get( JSContext* cx, unsigned argc, JS::Value* vp )
{
    if ( argc == 0 ) ERR_COUNT( ActiveX, Get );
    if ( !ISSTR( 0 ) && !ISINT( 0 ) ) ERR_TYPE( ActiveX, Get, 1, String );

    /*GETENV;
    GETOBJ( ActiveX, ActiveX, t );*/

    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    ActiveX* t = static_cast<ActiveX*>(JS_GetPrivate( args.thisv().toObjectOrNull() ));
    if ( !t )
    {
        //JS_ReportErrorASCII( cx, "Internal error: JS_GetPrivate failed" );
        return false;
    }


    DISPID dispid = 0;
    if ( ISSTR( 0 ) )
    {
        JSString* s = JSVAL_TO_STRING( args[0] );

        if ( s )
        {
            jschar* name = JS_GetStringChars( s );
            if ( !name )
            {
                ERR_MSG( ActiveX, Exec, "No property name" );
            }
            if ( !t->Id( name, dispid ) )
            {
                ERR_MSG( ActiveX, "This object does not have that property", TStr( name ) );
            }
        }
    }

    if ( !t->Get( dispid, cx, argc - 1, argv + 1, rval ) )
    {
        //    ERR_MSG(ActiveX,"IDispatch->Invoke failed","");
        return false;
    }
    return true;
}

///Set("property","index","index","value")
bool ActiveX_Set( JSContext* cx, unsigned argc, JS::Value* vp )
{
    if ( argc < 2 ) ERR_COUNT( ActiveX, Set );
    if ( !ISSTR( 0 ) ) ERR_TYPE( ActiveX, Set, 1, String );

    /*GETENV;
    GETOBJ2( ActiveX, ActiveX, t );*/

    JS::CallArgs args = JS::CallArgsFromVp( argc, vp );

    ActiveX* t = static_cast<ActiveX*>(JS_GetPrivate( args.thisv().toObjectOrNull() ));
    if ( !t )
    {
        //JS_ReportErrorASCII( cx, "Internal error: JS_GetPrivate failed" );
        return false;
    }

    JSString* s = JSVAL_TO_STRING( args[0] );
    if ( s )
    {
        jschar* name = JS_GetStringChars( s );
        if ( !name )
        {
            ERR_MSG( ActiveX, Exec, "No property name" );
        }

        DISPID dispid;
        if ( !t->Id( name, dispid ) )
        {
            ERR_MSG( ActiveX, "This object does not have that property", TStr( name ) );
        }

        ActiveX::PropInfo *p = t->Find( name );
        RETBOOL( t->Set( dispid, cx, argc - 2, argv + 1, argv + argc - 1, p ? p->PutRef : false ) );
    }
    RETOBJ( 0 );
}

bool ActiveX_as( JSContext* cx, unsigned argc, JS::Value* vp )
{
    if ( argc < 1 ) ERR_COUNT( ActiveX, as );
    if ( !ISSTR( 0 ) ) ERR_TYPE( ActiveX, as, 1, String );
    GETENV;
    GETOBJ( ActiveX, ActiveX, t );

    HRESULT hresult;
    void* specific = nullptr;
    CLSID clsid;
    jschar * type = JS_GetStringChars( JSVAL_TO_STRING( argv[0] ) );

    if ( type[0] == L'{' )
        hresult = CLSIDFromString( (WCHAR*)type, &clsid );
    else
        hresult = CLSIDFromProgID( (WCHAR*)type, &clsid );

    if ( SUCCEEDED( hresult ) )
    {
        IUnknown * unk;
        hresult = t->unknown->QueryInterface( clsid, (void * *)&unk );
        if ( SUCCEEDED( hresult ) ) RETOBJ( ActiveX_Object( cx, new ActiveX( unk ), true, nullptr ) );

    }
    RETOBJ( nullptr );
}

bool ActiveX_Close( JSContext* cx, unsigned argc, JS::Value* vp )
{
    CLOSEPRIVATE( ActiveX, ActiveX );
    RETBOOL( true );
}

static JSFunctionSpec ActiveX_fnstatic[] = {
    { "help",  ActiveX_HELP,    0 },
#ifdef EXPERIMENT_COM
{ "typelib",  ActiveX_typelib,    1 },
#endif
{ 0 }
};

}

ActiveX::ActiveX( VARIANTARG& var )
{
    unknown = nullptr;
    typeinfo = nullptr;
    dispatch = nullptr;
    VariantInit( &variant );
    VariantCopyInd( &variant, &var );
}

ActiveX::ActiveX()
{
    unknown = nullptr;
    typeinfo = nullptr;
    dispatch = nullptr;
    memset( &variant, 0, sizeof( variant ) );
}

ActiveX::ActiveX( IDispatch *obj, bool addref )
{
    unknown = nullptr;
    typeinfo = nullptr;
    memset( &variant, 0, sizeof( variant ) );
    dispatch = obj;

    if ( !dispatch )
    {
        return;
    }
    if ( addref )
    {
        dispatch->AddRef();
    }
}

ActiveX::ActiveX( IUnknown* obj, bool addref )
{
    dispatch = nullptr;
    typeinfo = nullptr;
    memset( &variant, 0, sizeof( variant ) );

    unknown = obj;
    if ( !unknown )
    {
        return;
    }

    if ( addref )
    {
        unknown->AddRef();
    }

    HRESULT hresult;

    hresult = unknown->QueryInterface( IID_IDispatch, (void * *)&dispatch );

    if ( !SUCCEEDED( hresult ) )
    {
        dispatch = 0;
    }

    // else  QueryInterface calls AddRef() for you
      //  dispatch->AddRef();
}

ActiveX::ActiveX( CLSID& clsid )
{
    HRESULT hresult;
    unknown = nullptr;
    dispatch = nullptr;
    typeinfo = nullptr;
    memset( &variant, 0, sizeof( variant ) );

    hresult = CoCreateInstance( clsid, nullptr, CLSCTX_SERVER | CLSCTX_INPROC_HANDLER,
                                IID_IUnknown, (void **)&unknown );

    if ( !SUCCEEDED( hresult ) ) 
    { 
        sunknown = 0; return;
    } //throw xdb("CoCreateInstance Failure");

    hresult = unknown->QueryInterface( IID_IDispatch, (void * *)&dispatch );


    //maybe I don't know what to do with it, but it might get passed to
    //another COM function
    if ( !SUCCEEDED( hresult ) )
    {
        dispatch = nullptr;
        //unknown->Release();
        //unknown=nullptr;
        //throw xdb("IDispatch interface not found");
    }
}

ActiveX::~ActiveX()
{
    if ( dispatch )
    {
        dispatch->Release();
    }
    if ( unknown )
    {
        unknown->Release();
    }
    if ( typeinfo )
    {
        typeinfo->Release();
    }
    if ( variant.vt )
    {
        VariantClear( &variant );
    }
    CoFreeUnusedLibraries();
}

bool ActiveX::Id( std::wstring_view name, DISPID& dispid )
{
    if ( !dispatch )
    {
        return false;
    }

    dispid = 0;
    HRESULT hresult;

    if ( name.empty() || name[0] == L'0' ) 
    { 
        dispid = 0; return true; 
    }

    wchar_t* cname = const_cast<wchar_t*>(name.data());
    hresult = dispatch->GetIDsOfNames( IID_NULL, &cname, 1, LOCALE_USER_DEFAULT, &dispid );

    if ( !SUCCEEDED( hresult ) )
    {
        hresult = dispatch->GetIDsOfNames( IID_NULL, &cname, 1, LOCALE_SYSTEM_DEFAULT, &dispid );

    }
    return SUCCEEDED( hresult );
}

bool ActiveX::Invoke( DISPID dispid, JSContext* cx, unsigned argc, JS::Value* vp )
{
    if ( !dispatch )
    {
        return false;
    }

    JS::CallArgs callArgs = JS::CallArgsFromVp( argc, vp );

    VARIANT VarResult;
    VARIANTARG * args;
    DISPPARAMS dispparams = { nullptr,nullptr,0,0 };
    HRESULT hresult;
    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    if ( argc )
    {
        args = new VARIANTARG[argc];
        dispparams.rgvarg = args;
        dispparams.cArgs = argc;
        for ( size_t i = 0; i < argc; i++ )
        {
            if ( !SetupValue( args[argc - i - 1], cx, callArgs[i] ) )
            {
                args[argc - i - 1].vt = VT_ERROR;
                args[argc - i - 1].scode = 0;
            }
        }
    }

    VariantInit( &VarResult );

    // don't use DispInvoke, because we don't know the TypeInfo
    hresult = dispatch->Invoke(
        dispid,
        IID_NULL,
        LOCALE_USER_DEFAULT,
        DISPATCH_METHOD,
        &dispparams, &VarResult, &exception, &argerr );

    for ( size_t i = 0; i < argc; i++ )
    {
        CheckReturn( cx, callArgs[i] ); //in case any empty ActiveX objects were filled in by Invoke()
        VariantClear( &args[i] ); //decrement AddRefs() done in SetupValue
    }
    if ( argc ) delete[] args;

    if ( !SUCCEEDED( hresult ) )
    {
        VariantClear( &VarResult );
        callArgs.rval().setNull();
        ActiveXError( hresult, exception, argerr, cx );
        return false;
    }

    if ( !RetrieveValue( VarResult, cx, callArgs.rval() ) )
    {
        callArgs.rval().setNull();
    }

    VariantClear( &VarResult );
    return true;
}

bool ActiveX::Get( DISPID dispid, JSContext* cx, unsigned argc, JS::Value* vp, bool exceptions )
{
    if ( !dispatch )
    {
        return false;
    }

    JS::CallArgs callArgs = JS::CallArgsFromVp( argc, vp );

    VARIANT VarResult;
    VARIANTARG * args;
    DISPPARAMS dispparams = { nullptr,nullptr,0,0 };
    HRESULT hresult;
    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    if ( argc )
    {
        args = new VARIANTARG[argc];
        dispparams.rgvarg = args;
        dispparams.cArgs = argc;
        for ( size_t i = 0; i < argc; i++ )
        {
            if ( !SetupValue( args[argc - i - 1], cx, callArgs[i] ) )
            {
                args[argc - i - 1].vt = VT_ERROR;
                args[argc - i - 1].scode = 0;
            }
        }
    }

    VariantInit( &VarResult );

    hresult = dispatch->Invoke(
        dispid,
        IID_NULL,
        LOCALE_USER_DEFAULT,
        DISPATCH_PROPERTYGET,
        &dispparams, &VarResult, &exception, &argerr );

    for ( size_t i = 0; i < argc; i++ )
    {
        CheckReturn( cx, callArgs[i] );
        VariantClear( &args[i] );
    }

    if ( argc ) delete[] args;

    if ( !SUCCEEDED( hresult ) )
    {
        callArgs.rval().setNull();
        if ( exceptions )
        {
            ActiveXError( hresult, exception, argerr, cx );
        }
        return false;
    }
    else if ( !RetrieveValue( VarResult, cx, callArgs.rval() ) )
    {
        callArgs.rval().setNull();
    }
    VariantClear( &VarResult );

    return true;
}

bool ActiveX::Set( DISPID dispid, JSContext* cx, unsigned argc, JS::Value* vp, bool byref )
{
    if ( !dispatch )
    {
        return false;
    }

    JS::CallArgs callArgs = JS::CallArgsFromVp( argc, vp );

    VARIANTARG * args = new VARIANTARG[argc + 1];
    DISPID dispput = DISPID_PROPERTYPUT;
    DISPPARAMS dispparams = { args,&dispput,argc + 1,1 };
    HRESULT hresult;
    EXCEPINFO exception = { 0 };
    UINT argerr = 0;

    //the set value
    if ( !SetupValue( args[0], cx, callArgs.rval ) )
    {
        return false;
    }

    //the index values, in reverse order
    if ( argc )
    {
        //   dispparams.rgvarg = args; //initialized in the declaration
        //   dispparams.cArgs = argc+1;
        for ( size_t i = 0; i < argc; i++ )
        {
            if ( !SetupValue( args[argc - i], cx, callArgs[i] ) )
            {
                args[argc - i].vt = VT_ERROR;
                args[argc - i].scode = 0;
            }
        }
    }

    DWORD flag = DISPATCH_PROPERTYPUT;
    if ( byref && (args[0].vt & VT_DISPATCH || args[0].vt & VT_UNKNOWN) )
    {//must be passed by name
        flag = DISPATCH_PROPERTYPUTREF;
    }

    hresult = dispatch->Invoke(
        dispid,
        IID_NULL,
        LOCALE_USER_DEFAULT,
        flag,
        &dispparams, nullptr, &exception, &argerr ); //no result

    for ( size_t i = 0; i < argc; i++ )
    {
        CheckReturn( cx, callArgs[i] );
        VariantClear( &args[i] );
    }

    if ( argc ) delete[] args;

    if ( !SUCCEEDED( hresult ) )
    {
        ActiveXError( hresult, exception, argerr, cx );
        return false;
    }

    return true;
}

ActiveX::PropInfo* ActiveX::Find( const wchar_t* n )
{
    FOREACH( PropInfo*p, Properties )
        if ( !ucscmp( p->name, n ) ) return p.get();
    DONEFOREACH
        return nullptr;
}

bool ActiveX::SetupMembers( JSContext* cx, JSObject* obj )
{
    JS::RootedObject robj( obj );

    HRESULT hresult;
    if ( unknown && !dispatch )
    {
        hresult = unknown->QueryInterface( IID_IDispatch, (void * *)&dispatch );
    }
    if ( !dispatch )
    {
        return false;
    }
    ENTERNATIVE( cx );

    JS::RootedObject doc(cx, JS_NewObject( cx, nullptr, nullptr, obj ) );
    JS_DefineProperty( cx, robj, "members", doc, JSPROP_ENUMERATE );

    if ( !typeinfo )
    {
        unsigned ctinfo;
        hresult = dispatch->GetTypeInfoCount( &ctinfo );

        if ( SUCCEEDED( hresult ) )
            if ( ctinfo )
                dispatch->GetTypeInfo( 0, 0, &typeinfo );
    }

    if ( !typeinfo ) return false;

    size_t i;
    VARDESC * vardesc;
    for ( i = 0; typeinfo->GetVarDesc( i, &vardesc ) == S_OK && i < 255; i++ )
    {
        BSTR name = nullptr;
        BSTR desc = nullptr;
        if ( typeinfo->GetDocumentation( vardesc->memid, &name, &desc, nullptr, nullptr ) == S_OK )
        {
            PropInfo * p = Find( (wchar_t*)name );
            if ( !p ) p = new ActiveX::PropInfo( (wchar_t*)name );
            p->Get = p->Put = true;
            unsigned prop = Properties.Add( p );

            //JS_DefineUCPropertyWithTinyId
            JS_DefineUCProperty( cx, obj, 
                                 name, SysStringLen( name ), 
                                 (int8)(prop), OBJECT_TO_JSVAL( nullptr ),
                                           ActiveX_JSGet, ActiveX_JSSet, JSPROP_ENUMERATE );
            if ( doc )
            {
                jsval d = desc 
                    ? STRING_TO_JSVAL( ROOT( JS_NewUCStringCopyN( cx, (jschar*)desc, SysStringLen( desc ) ) ) ) 
                    : OBJECT_TO_JSVAL( nullptr );
                JS_DefineUCProperty( cx, doc, name, SysStringLen( name ), d, nullptr, nullptr, JSPROP_ENUMERATE );
            }
            SysFreeString( name );
            SysFreeString( desc );
        }
        typeinfo->ReleaseVarDesc( vardesc );
    }

    FUNCDESC * funcdesc;
    for ( i = 0; typeinfo->GetFuncDesc( i, &funcdesc ) == S_OK; i++ )
    {
        BSTR name = nullptr;
        BSTR desc = nullptr;

        if ( typeinfo->GetDocumentation( funcdesc->memid, &name, &desc, nullptr, nullptr ) == S_OK )
        {
            //    char* fname = DeflateString((jschar*)name,SysStringLen(name));

            if ( funcdesc->invkind == INVOKE_FUNC )
            {
                //       JS_DefineFunction(cx,obj,fname,*ActiveX_Run,funcdesc->cParams,0);
                JS_DefineUCFunction( cx, obj, (jschar*)name, SysStringLen( name ),
                                     *ActiveX_Run, funcdesc->cParams, 0 );
            }
            else
            {
                PropInfo * p = Find( (wchar_t*)name );

                if ( !p )
                {
                    p = new PropInfo( (wchar_t*)name );
                    unsigned prop = Properties.Add( p );
                    JS_DefineUCPropertyWithTinyId( cx, obj, (jschar *)name,
                                                   SysStringLen( name ), (int8)(prop), OBJECT_TO_JSVAL( nullptr ),
                                                   ActiveX_JSGet, ActiveX_JSSet, JSPROP_ENUMERATE );
                }

                if ( funcdesc->invkind & INVOKE_PROPERTYGET )
                    p->Get = true;
                if ( funcdesc->invkind & INVOKE_PROPERTYPUT )
                    p->Put = true;
                if ( funcdesc->invkind & INVOKE_PROPERTYPUTREF )
                    p->PutRef = true;
            }
            //    if (fname) free(fname);

            if ( doc )
            {
                jsval d = desc && *desc ? STRING_TO_JSVAL( ROOT( JS_NewUCStringCopyN( cx, (jschar*)desc, SysStringLen( desc ) ) ) ) : JSVAL_NULL;
                JS_DefineUCProperty( cx, doc, (jschar*)name, SysStringLen( name ),
                                     d, nullptr, nullptr, JSPROP_ENUMERATE );
            }

            SysFreeString( name );
            SysFreeString( desc );
        }
        typeinfo->ReleaseFuncDesc( funcdesc );
    }

    return true;
}

bool SetupValue( VARIANTARG& arg, JSContext* cx, JS::MutableHandleValue rval );

void CheckReturn( JSContext* cx, JS::MutableHandleValue rval );

bool RetrieveValue( VARIANTARG& arg, JSContext* cx, JS::MutableHandleValue rval );

JSClass* ActiveX_Class();

JSObject* ActiveX_Object( JSContext *cx, ActiveX* t, bool autodelete, JSPointerBase* Parent );

extern "C" JSObject *
js_NewDateObject( JSContext* cx, int year, int mon, int mday,
                  int hour, int min, int sec );

#define FETCH(x) (ref? * (var.p ## x) : var.x)

///RetrieveValue assumes that the caller will call VariantClear, so call AddRef on new objects
bool RetrieveValue( VARIANTARG& var, JSContext* cx, JS::MutableHandleValue rval )
{
    ENTERNATIVE( cx );
    bool ref = false;
    int type = var.vt;
    if ( type & VT_BYREF ) 
    { 
        ref = true; 
        type &= ~VT_BYREF; 
    }
    ActiveX* x;
    IDispatch * dispatch;
    IUnknown * unknown;

    try
    {
        switch ( type )
        {
        case VT_ERROR: rval.setUndefined(); break;
        case VT_NULL:
        case VT_EMPTY: rval.setNull(); break;
        case VT_I1: rval.setInt32( static_cast<int32_t>(FETCH( cVal )) ); break;
        case VT_I2: rval.setInt32( static_cast<int32_t>(FETCH( iVal )) ); break;
        case VT_INT:
        case VT_I4: rval.setInt32( FETCH( lVal ) ); break;
        case VT_R4: rval.setNumber( FETCH( fltVal ) ); break;
        case VT_R8: rval.setNumber( FETCH( dblVal ) ); break;

        case VT_BOOL: rval.setBoolean( FETCH( boolVal ) ? true : false ); break;

        case VT_UI1: rval.setNumber( static_cast<uint32_t>(FETCH( bVal )) ); break;
        case VT_UI2: rval.setNumber( static_cast<uint32_t>(FETCH( uiVal )) ); break;
        case VT_UINT:
        case VT_UI4: rval.setNumber( static_cast<uint32_t>(FETCH( ulVal )) ) ); break;

        case VT_BSTR: rval.setString( ROOT( JS_NewUCStringCopyN( cx, FETCH( bstrVal ), SysStringLen( FETCH( bstrVal ) ) ) ) );
            //              SysFreeString(FETCH(bstrVal));
            //              var.vt = VT_EMPTY;
            break;

        case VT_DATE:
        {
            DATE d = FETCH( date );
            SYSTEMTIME time;
            VariantTimeToSystemTime( d, &time );
            rval.setObjectOrNull( ROOT( JS_NewDateObject( cx, time.wYear, time.wMonth - 1, time.wDay,
                                                             time.wHour, time.wMinute, time.wSecond ) ) );

            break;
        }

        case VT_UNKNOWN:

            if ( !FETCH( punkVal ) ) { rval.setNull(); break; }
            x = new ActiveX( FETCH( punkVal ), true );
            if ( !x->unknown && !x->dispatch ) { delete x; return false; }
            rval.setObjectOrNull( ActiveX_Object( cx, x, true, nullptr ) );
            break;

        case VT_DISPATCH:

            if ( !FETCH( pdispVal ) ) { rval.setNull(); break; }
            x = new ActiveX( FETCH( pdispVal ), true );
            if ( !x->unknown && !x->dispatch ) { delete x; return false; }
            rval.setObjectOrNull( ActiveX_Object( cx, x, true, nullptr ) );
            break;

        case VT_VARIANT: //traverse the indirection list?
            if ( ref )
            {
                VARIANTARG* v = var.pvarVal;
                if ( v )
                    return RetrieveValue( *v, cx, rval );
            }
            break;

        default:
            if ( type <= VT_CLSID )
            {
                x = new ActiveX( var );
                rval.setObjectOrNull( ActiveX_Object( cx, x, true, nullptr ) );

                return true;
            }

            return false;
            //default: return false;
        }
    }
    catch ( ... )
    {
        return false;
    }
    return true;
}


void CheckReturn( JSContext* cx, JS::MutableHandleValue rval )
{
    if ( rval.isObject() )
    {
        HRESULT hresult;
        JS::RootedObject j0 (rval.toObjectOrNull());
        if ( j0 && JS_InstanceOf( cx, j0, ActiveX_Class(), 0 ) )
        {
            ActiveX* x = GETPRIVATE( ActiveX, j0 );
            if ( x->unknown && !x->dispatch )
            {
                hresult = x->unknown->QueryInterface( IID_IDispatch, (void **)&x->dispatch );
                if ( SUCCEEDED( hresult ) )
                {
                    x->SetupMembers( cx, j0 );
                }
                else
                {
                    x->dispatch = 0;
                }
            }
        }
    }
}

bool SetupValue( VARIANTARG& arg, JSContext* cx, JS::MutableHandleValue rval )
{
    VariantInit( &arg );
    // arg.vt = VT_EMPTY;

    if ( rval.isObject() )
    {
        JS::RootedObject j0( rval.toObjectOrNull() );
        if ( j0 && JS_InstanceOf( cx, j0, ActiveX_Class(), 0 ) )
        {
            ActiveX* x = static_cast<ActiveX*>(JS_GetPrivate( j0 ));
            if ( !x )
            {
                //JS_ReportErrorASCII( cx, "Internal error: JS_GetPrivate failed" );
                return false;
            }
            if ( x->variant.vt != VT_EMPTY )
            {
                //1.7.2.3
                VariantCopyInd( &arg, &x->variant );
                //VariantCopy(&arg,&x->variant);
                //1.7.2.2 could address invalid memory if x is freed before arg
                // arg.vt = VT_VARIANT | VT_BYREF;
                // arg.pvarVal = &x->variant;
                return true;
            }
            if ( x->dispatch )
            {
                arg.vt = VT_DISPATCH;
                arg.pdispVal = x->dispatch;
                x->dispatch->AddRef();
                return true;
            }
            else if ( x->unknown )
            {
                arg.vt = VT_UNKNOWN;
                arg.punkVal = x->unknown;
                x->unknown->AddRef();
                return true;
            }
            else
            {
                arg.vt = VT_BYREF | VT_UNKNOWN;
                arg.ppunkVal = &x->unknown;
                return true;
            }
        }
    }

    if ( rval.isBoolean() )
    {
        arg.vt = VT_BOOL;
        arg.boolVal = rval.toBoolean() ? -1 : 0;
        return true;
    }

    if ( rval.isInt32() )
    {
        arg.vt = VT_I4;
        arg.lVal = rval.toInt32();
        return true;
    }

    if ( rval.isDouble() )
    {
        arg.vt = VT_R8;
        arg.dblVal = rval.toDouble();

        return true;
    }

    if ( rval.isNull() )
    {
        arg.vt = VT_EMPTY;
        arg.scode = 0;
        return true;
    }

    if ( rval.isString() )
    {
        arg.vt = VT_BSTR;
        arg.bstrVal = SysAllocString( (WCHAR*)JS_GetStringChars( rval.toString() ) );
        return true;
    }

    return false;
}

#ifdef EXPERIMENT_COM
IDispatch* Recast( IUnknown* unk, ITypeLib* typelib, WCHAR* type )
{
    IUnknown* result = nullptr;
    IDispatch* dispatch = nullptr;
    ITypeInfo* typeinfo = nullptr;
    void* specific = nullptr;
    HRESULT hresult;
    unsigned short found = 1;
    MEMBERID memb = 0;
    CLSID clsid;

    if ( type[0] == L'{' )
        hresult = CLSIDFromString( (WCHAR*)type, &clsid );
    else
        hresult = CLSIDFromProgID( (WCHAR*)type, &clsid );
    if ( !SUCCEEDED( hresult ) )
        return nullptr;

    hresult = typelib->GetTypeInfoOfGuid( clsid, &typeinfo );
    if ( !SUCCEEDED( hresult ) || !found )
        return nullptr;

    hresult = unk->QueryInterface( clsid, &specific );
    if ( !SUCCEEDED( hresult ) || !specific )
        return nullptr;

    hresult = CreateStdDispatch( unk, specific, typeinfo, &result );

    if ( !SUCCEEDED( hresult ) || !result )
        return nullptr;

    hresult = result->QueryInterface( IID_IDispatch, (void * *)&dispatch );

    if ( !SUCCEEDED( hresult ) )
        dispatch = 0;

    return dispatch;
}

WRAP( ActiveX, typelib )
{
    if ( argc < 1 ) ERR_COUNT( ActiveX, typelib );
    const jschar * library = WSTR( 0 );
    ITypeLib * typelib = nullptr;

    HRESULT hresult = LoadTypeLib( (WCHAR*)library, &typelib );
    if ( SUCCEEDED( hresult ) )
    {
        //     MemoryStream s;
        WStr s;

        unsigned count = typelib->GetTypeInfoCount();
        for ( unsigned i = 0; i < count; i++ )
        {
            BSTR name, docstring, helpfile;
            unsigned long helpcontext;

            typelib->GetDocumentation( i, &name, &docstring, &helpcontext, &helpfile );
            WStr w1( (wchar_t*)name, SysStringLen( name ) );
            WStr w2( (wchar_t*)docstring, SysStringLen( docstring ) );

            s << (w1) << (wchar_t*)L": " << (w2) << (wchar_t*)L"\n";

            SysFreeString( name );
            SysFreeString( docstring );
            SysFreeString( helpfile );
        }

        RETSTRW( s );
    }
    RETSTRW( L"" );
}

WRAP( ActiveX, as )
{
    if ( argc < 2 ) ERR_COUNT( ActiveX, as );
    if ( !ISSTR( 0 ) || !ISSTR( 1 ) ) ERR_TYPE( ActiveX, as, 1, String );
    GETENV;
    GETOBJ( ActiveX, ActiveX, t );

    jschar * library = JS_GetStringChars( JSVAL_TO_STRING( argv[0] ) );
    jschar * name = JS_GetStringChars( JSVAL_TO_STRING( argv[1] ) );
    ITypeLib * typelib = nullptr;

    HRESULT hresult = LoadTypeLib( (WCHAR*)library, &typelib );
    if ( SUCCEEDED( hresult ) )
    {
        IDispatch* d = Recast( t->unknown, typelib, (WCHAR*)name );
        if ( d )
        {
            if ( t->dispatch ) t->dispatch->Release();
            t->dispatch = d;
            t->SetupMembers( cx, obj );
        }

        typelib->Release();

        if ( d )  RETOBJ( obj );
    }
    RETOBJ( nullptr );
}
#endif

/*
char * DeflateString(const jschar*chars, size_t length)
{
    size_t i, size;
    char *bytes;

    size = (length + 1) * sizeof(char);
    bytes = (char *) malloc(size);
    if (!bytes) return nullptr;
    for (i = 0; i < length; i++)
        bytes[i] = (char) chars[i];
    bytes[i] = 0;
    return bytes;
}
*/
void ActiveXError( HRESULT hresult, EXCEPINFO& exception, UINT& argerr, JSContext* cx )
{
    char errmsg[1024];
    //#define ReportError1(msg,arg)
    // sprintf(errmsg,"ActiveX:  msg,arg);
    // JS_ReportError(cx,STRING_TO_JSVAL(JS_NewStringCopyZ(cx,errmsg )));

#define ReportError1(msg,arg) \
 {std::string errMsg = "ActiveX:"; \
  errMsg += msg; \
  JS_ReportErrorASCII(cx,errMsg.c_str(), arg);}

#define ReportError(msg) \
 {std::string errMsg = "ActiveX:"; \
  errMsg += msg; \
  JS_ReportErrorASCII(cx,errMsg.c_str());}

//  #define ReportError1(msg,arg) JS_ReportError(cx,"ActiveX: " msg,arg);
//#define ReportError(msg) JS_ReportError(cx,"ActiveX: " msg);
// JS_SetPendingException(cx,STRING_TO_JSVAL(JS_NewStringCopyZ(cx,errmsg )));

    switch ( hresult )
    {
    case DISP_E_BADPARAMCOUNT: ReportError( "Wrong number of parameters" ); break;
    case DISP_E_BADVARTYPE: ReportError1( "Bad variable type %d", argerr ); break;
    case DISP_E_EXCEPTION:
    {
        if ( exception.bstrDescription )
        {
            std::wstring w1( (wchar_t*)exception.bstrDescription, SysStringLen( exception.bstrDescription ) );
            //                          TStr d(w1);

            std::wstring w2( (wchar_t*)exception.bstrSource, SysStringLen( exception.bstrSource ) );
            //                          TStr s(w2);
            std::wstring w;
            w.resize( w1.length() + w2.length() + 20 );
            swprintf( w.data(), L"ActiveX: (%s) %s", w2.c_str(), w1.c_str() );
            //                          TStr err(w);
            //                          printf("%s\n",(char*)err);

            JS_ReportErrorASCII( cx, STRING_TO_JSVAL( JS_NewUCStringCopyN( cx, w, w.length() ) ) );
            //                          char* err = DeflateString((jschar*)exception.bstrDescription,SysStringLen(exception.bstrDescription));
            //                          ReportError(err);
            //                          ReportError(cx,"%s",TStr("Activex: ",err));
            //                          JS_ReportError(cx,"%s",(char*)TStr(err));
            //                          free(err);
        }
        else
        {
            ReportError1( "Error code %d", exception.scode );
        }
        SysFreeString( exception.bstrSource );
        SysFreeString( exception.bstrDescription );
        SysFreeString( exception.bstrHelpFile );
        break;
    }
    case DISP_E_MEMBERNOTFOUND: ReportError( "Function not found" ); break;
    case DISP_E_OVERFLOW: ReportError1( "Can not convert variable %d", argerr ); break;
    case DISP_E_PARAMNOTFOUND: ReportError1( "Parameter %d not found", argerr ); break;
    case DISP_E_TYPEMISMATCH: ReportError1( "Parameter %d type mismatch", argerr ); break;
    case DISP_E_UNKNOWNINTERFACE: ReportError( "Unknown interface" ); break;
    case DISP_E_UNKNOWNLCID: ReportError( "Unknown LCID" ); break;
    case DISP_E_PARAMNOTOPTIONAL: ReportError1( "Parameter %d is required", argerr );
    }
}

JSObject*
ActiveX_Object( JSContext *cx, ActiveX* t, bool autodelete, JSPointerBase* Parent )
{
    GETENV;
    
    ENTERNATIVE( cx );
    JS::RootedObject obj( JS_NewObject( cx, JS_GetClass( Env->name ), Env->name, NULL )); 
    JS_DefineFunctions( cx, obj, jsFunctions ); 
    JS_DefineProperties( cx, obj, jsProperties );
    /*obj = JS_NewObject(cx, ActiveX_Class(),Env->ActiveX, nullptr);
    JS_DefineFunctions(cx,obj,ActiveX_functions);
    JS_DefineProperties(cx,obj,ActiveX_properties);   */
    if ( t )
    {        
        JS_SetPrivate( obj, new JSPointer<ActiveX>( Parent, t, autodelete ) );
        t->SetupMembers( cx, obj );
    }
    return obj;
}

NATIVE( ActiveX_ActiveX )
{
    GETENV;
    if ( Env->SafeMode ) ERR_MSG( ActiveX, ActiveX, "blocked by security settings" );

    if ( argc )
        if ( !ISSTR( 0 ) ) ERR_TYPE( ActiveX, ActiveX, 1, String );
    //ENTERNATIVE(cx);

    //argc > 0 if clsid is valid
    CLSID clsid;
    HRESULT hresult;
    jschar * name;

    if ( argc )
    {
        name = JS_GetStringChars( JSVAL_TO_STRING( ARGV( 0 ) ) );
        if ( name[0] == L'{' )
            hresult = CLSIDFromString( (WCHAR*)name, &clsid );
        else
            hresult = CLSIDFromProgID( (WCHAR*)name, &clsid );

        if ( !SUCCEEDED( hresult ) )
        {
            ERR_MSG( ActiveX, ActiveX, "invalid CLSID" );
        }
    }
    ActiveX* t = nullptr;

    if ( argc == 0 )
    {
        t = new ActiveX();
    }
    else
    {
        IUnknown* unk = nullptr;
        if ( argc == 1 )
        {
            hresult = GetActiveObject( clsid, nullptr, &unk );
        }

        if ( SUCCEEDED( hresult ) && unk )
        {
            t = new ActiveX( unk );
            if ( !t->unknown )
            {
                delete t;
                t = 0;
                ERR_MSG( ActiveX, "Can't create ActiveX object", TStr( name ) );
            }
        }
    }

    if ( !t )
    {
        t = new ActiveX( clsid );
        if ( !t->unknown )
        {
            delete t;
            ERR_MSG( ActiveX, "Can't create ActiveX object", TStr( name ) );
        }
    }
    if ( t )
    {
        CONSTRUCTOR( ActiveX, t, true, nullptr );
    }
    return true;
}

JSClass* ActiveX_Class() { return &jsClass; }

void ActiveX_InitClass( JSContext *cx, JSObject *obj )
{
    GETENV;
    INITCLASS( ActiveX );
}

/*
CoInitialize(nullptr);
ActiveX_InitClass(cx,obj);
CoFreeUnusedLibraries();
CoUninitialize();
*/


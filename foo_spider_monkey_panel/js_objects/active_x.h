/// Code extracted from wrap_com.cpp from JSDB by Shanti Rao (see http://jsdb.org)

#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <oleauto.h>

#include <map>

//#define DEBUG
//#define TRACE
//dword-word-word-byte[8]
//{85BBD920-42A0-1069-A2E4-08002B30309D}
//HRESULT CLSIDFromString(LPOLESTR lpsz,LPCLSID pclsid);
//S_OK == StringFromCLSID(REFCLSID rclsid,LPOLESTR * ppsz);

#ifndef DISPID_PROPERTYPUT
#   define DISPID_PROPERTYPUT (-3)
#endif

class ActiveX //takes ownership, calls Release() at the end
{
public:
    IDispatch * dispatch;
    IUnknown * unknown;
    ITypeInfo * typeinfo;
    VARIANT variant;

    struct PropInfo
    {
        std::wstring name;
        bool Get;
        bool Put;
        bool PutRef;
        PropInfo( const wchar_t* n ) : name( n ) { Get = Put = PutRef = false; }
    };

    std::map<std::wstring, std::shared_ptr<PropInfo>> Properties;

    ActiveX();
    ActiveX( CLSID& clsid );
    ActiveX( IUnknown *obj, bool addref = false );
    ActiveX( IDispatch *obj, bool addref = false );
    ActiveX( VARIANTARG& var );
    //TIntList properties;

    ~ActiveX();

    PropInfo* Find( std::wstring_view name );

    // bool Id(size_t x,DISPID &dispid);

    bool Id( std::wstring_view name, DISPID &dispid );
    bool Set( DISPID dispid, JSContext* cx, unsigned argc, JS::Value* vp, bool ref );
    bool Get( DISPID dispid, JSContext* cx, unsigned argc, JS::Value* vp, bool exceptions = true );

    //throws an xdb exception on error
    bool Invoke( DISPID dispid, JSContext* cx, unsigned argc, JS::Value* vp );
    bool SetupMembers( JSContext* cx, JS::HandleObject obj );
};

JSObject* CreateActiveXProto( JSContext *cx, JS::HandleObject obj );

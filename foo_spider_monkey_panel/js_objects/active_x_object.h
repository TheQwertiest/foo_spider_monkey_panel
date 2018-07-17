#pragma warning( push )  
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#include <jsapi.h>
#pragma warning( pop )  

#include <oleauto.h>

#include <map>

namespace mozjs
{

class ActiveXObject //takes ownership, calls Release() at the end
{
public:
    static const JSClass JsClass;

public:
    ActiveXObject( JSContext* cx, CLSID& clsid );
    ActiveXObject( JSContext* cx, IUnknown *obj, bool addref = false );
    ActiveXObject( JSContext* cx, IDispatch *obj, bool addref = false );
    ActiveXObject( JSContext* cx, VARIANTARG& var );
    ~ActiveXObject();

    ActiveXObject( const ActiveXObject& ) = delete;
    ActiveXObject& operator=( const ActiveXObject& ) = delete;

    static JSObject* InitPrototype( JSContext *cx, JS::HandleObject parentObject );
    static JSObject* Create( JSContext* cx, const std::wstring& name );
    static JSObject* Create( JSContext* cx, ActiveXObject* pPremadeNative );

public:
    IDispatch * pDispatch_ = nullptr;
    IUnknown * pUnknown_ = nullptr;
    ITypeInfo * pTypeInfo_ = nullptr;
    VARIANT variant_;

    bool GetDispId( std::wstring_view name, DISPID &dispid );

    bool Get( JS::HandleValue idxArg, JS::MutableHandleValue vp, DISPID dispid );
    bool Set( JS::HandleValue v, DISPID dispid, bool ref );
    bool Invoke(const JS::CallArgs& args, DISPID dispid );

private:
    bool GetAllPutProperties();

private:
    JSContext * pJsCtx_ = nullptr;

    struct PropInfo
    {
        PropInfo( std::wstring_view nameArg )
            : name( nameArg )
        {
        }
        std::wstring name;
        bool isPutRef = false;
        bool hasDispId = false;
        DISPID dispId;
    };
    std::map<std::wstring, std::unique_ptr<PropInfo>> properties_;
};

}

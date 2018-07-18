#include <js_objects/object_base.h>

#pragma warning( push )  
#pragma warning( disable : 4100 ) // unused variable
#pragma warning( disable : 4251 ) // dll interface warning
#pragma warning( disable : 4324 ) // structure was padded due to alignment specifier
#pragma warning( disable : 4996 ) // C++17 deprecation warning
#   include <js/Proxy.h>
#pragma warning( pop ) 

#include <oleauto.h>

#include <optional>
#include <map>

namespace mozjs
{

/// @details Takes ownership, calls Release() at the end
class ActiveXObject 
    : public JsObjectBase<ActiveXObject>
{
public:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;
    static constexpr bool HasProxy = true;
    static constexpr bool HasPostCreate = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const js::BaseProxyHandler& JsProxy;

public:
    ActiveXObject( JSContext* cx, CLSID& clsid );
    ActiveXObject( JSContext* cx, IUnknown *obj, bool addref = false );
    ActiveXObject( JSContext* cx, IDispatch *obj, bool addref = false );
    ActiveXObject( JSContext* cx, VARIANTARG& var );
    ~ActiveXObject();

    ActiveXObject( const ActiveXObject& ) = delete;
    ActiveXObject& operator=( const ActiveXObject& ) = delete;

    static JSObject* InitPrototype( JSContext *cx, JS::HandleObject parentObject );
    static std::unique_ptr<ActiveXObject> CreateNative( JSContext* cx, const std::wstring& name );
    static size_t GetInternalSize( const std::wstring& name );
    static bool PostCreate( JS::HandleObject self );

public:
    IDispatch * pDispatch_ = nullptr;
    IUnknown * pUnknown_ = nullptr;
    ITypeInfo * pTypeInfo_ = nullptr;
    VARIANT variant_;

    bool IsGet( const std::wstring& name );
    bool IsSet( const std::wstring& name );
    bool IsInvoke( const std::wstring& name );

    bool Get( const std::wstring& propName, JS::MutableHandleValue vp );
    bool Set( const std::wstring& propName, JS::HandleValue v );
    bool Invoke( const std::wstring& funcName, const JS::CallArgs& args );

private:
    struct MemberInfo
    {
        bool isGet = false;
        bool isPut = false;
        bool isPutRef = false;
        bool isInvoke = false;
        bool hasDispId = false;
        DISPID dispId;
    };

    using MemberMap = std::unordered_map<std::wstring, std::unique_ptr<MemberInfo>>;

private:
    std::optional<DISPID> GetDispId( const std::wstring& name );

    bool SetupMembers( JS::HandleObject jsObject );
    static bool ParseTypeInfoRecursive( JSContext * cx, ITypeInfo * pTypeInfo, MemberMap& members );
    static void ParseTypeInfo( ITypeInfo * pTypeInfo, MemberMap& members );
    bool SetupMembers_Impl( JS::HandleObject jsObject );

private:
    JSContext * pJsCtx_ = nullptr;
    bool areMembersSetup_ = false;

    MemberMap members_;
};

}

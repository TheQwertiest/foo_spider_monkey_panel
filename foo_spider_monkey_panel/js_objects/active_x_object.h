#include <js_objects/object_base.h>

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Proxy.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

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
    static const JSNative JsConstructor;
    static const js::BaseProxyHandler& JsProxy;

public:
    ActiveXObject( JSContext* cx, CLSID& clsid );
    ActiveXObject( JSContext* cx, IUnknown *obj, bool addref = false );
    ActiveXObject( JSContext* cx, IDispatch *obj, bool addref = false );
    ActiveXObject( JSContext* cx, VARIANTARG& var );
    ~ActiveXObject() override;

    ActiveXObject( const ActiveXObject& ) = delete;
    ActiveXObject& operator=( const ActiveXObject& ) = delete;
    
    static std::unique_ptr<ActiveXObject> CreateNative( JSContext* cx, const std::wstring& name );
    static size_t GetInternalSize( const std::wstring& name );
    static void PostCreate( JSContext* cx, JS::HandleObject self );

public: // ctor
    static JSObject* Constructor( JSContext* cx, const std::wstring& name );

    bool Has( const std::wstring& name );
    bool IsGet( const std::wstring& name );
    bool IsSet( const std::wstring& name );
    bool IsInvoke( const std::wstring& name );
    std::vector<std::wstring> GetAllMembers();

    std::wstring ToString();

    void Get( const std::wstring& propName, JS::MutableHandleValue vp );
    void Get( JS::CallArgs& args );
    void Set( const std::wstring& propName, JS::HandleValue v );
    void Set( const JS::CallArgs& args );
    void Invoke( const std::wstring& funcName, const JS::CallArgs& args );

private:
    struct MemberInfo
    {
        bool isGet = false;
        bool isPut = false;
        bool isPutRef = false;
        bool isInvoke = false;
        bool hasDispId = false;
        DISPID dispId{};
    };

    using MemberMap = std::unordered_map<std::wstring, std::unique_ptr<MemberInfo>>;

private:
    std::optional<DISPID> GetDispId( const std::wstring& name, bool reportError = true );

    void SetupMembers( JS::HandleObject jsObject );
    static void ParseTypeInfoRecursive( JSContext * cx, ITypeInfo * pTypeInfo, MemberMap& members );
    static void ParseTypeInfo( ITypeInfo * pTypeInfo, MemberMap& members );
    void SetupMembers_Impl( JS::HandleObject jsObject );

private:
    JSContext * pJsCtx_ = nullptr;
    bool areMembersSetup_ = false;

    MemberMap members_;

public:
    IDispatch* pDispatch_ = nullptr;
    IUnknown* pUnknown_ = nullptr;
    ITypeInfo* pTypeInfo_ = nullptr;
    _variant_t variant_;
    bool hasVariant_ = false;
};

}

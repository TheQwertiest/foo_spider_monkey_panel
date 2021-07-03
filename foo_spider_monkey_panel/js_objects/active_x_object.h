#include <js_objects/object_base.h>

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Proxy.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

#include <com_utils/com_destruction_handler.h>

#include <oleauto.h>

#include <map>
#include <optional>
#include <span>

namespace mozjs
{

/// @details Takes ownership, calls Release() at the end
class JsActiveXObject
    : public JsObjectBase<JsActiveXObject>
{
public:
    static constexpr bool HasProto = true;
    static constexpr bool HasGlobalProto = true;
    static constexpr bool HasStaticFunctions = true;
    static constexpr bool HasProxy = true;
    static constexpr bool HasPostCreate = true;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSFunctionSpec* JsStaticFunctions;
    static const JSPropertySpec* JsProperties;
    static const JsPrototypeId PrototypeId;
    static const JSNative JsConstructor;
    static const js::BaseProxyHandler& JsProxy;

public:
    JsActiveXObject( JSContext* cx, CLSID& clsid );
    JsActiveXObject( JSContext* cx, IUnknown* pUnknown, bool addref = false );
    JsActiveXObject( JSContext* cx, IDispatch* pDispatch, bool addref = false );
    JsActiveXObject( JSContext* cx, VARIANTARG& var );
    ~JsActiveXObject() override;

    JsActiveXObject( const JsActiveXObject& ) = delete;
    JsActiveXObject& operator=( const JsActiveXObject& ) = delete;

    static std::unique_ptr<JsActiveXObject> CreateNative( JSContext* cx, const std::wstring& name );
    static size_t GetInternalSize( const std::wstring& name );
    static void PostCreate( JSContext* cx, JS::HandleObject self );

public:
    static JSObject* Constructor( JSContext* cx, const std::wstring& name );
    static JSObject* CreateFromArray( JSContext* cx, JS::HandleValue arr, uint32_t elementVariantType );
    std::wstring ToString();

public:
    JSObject* CreateNewIterator();
    bool HasIterator() const;

    bool Has( const std::wstring& name );
    bool IsGet( const std::wstring& name );
    bool IsSet( const std::wstring& name );
    bool IsInvoke( const std::wstring& name );
    std::vector<std::wstring> GetAllMembers();

    void GetItem( int32_t index, JS::MutableHandleValue vp );
    bool TryGetProperty( const std::wstring& propName, JS::MutableHandleValue vp );
    void GetProperty( const std::wstring& propName, JS::MutableHandleValue vp );
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

    void GetImpl( int dispId, std::span<_variant_t> args, JS::MutableHandleValue vp, std::optional<std::function<void()>> refreshFn = {} );

    void SetupMembers( JS::HandleObject jsObject );
    static void ParseTypeInfoRecursive( JSContext* cx, ITypeInfo* pTypeInfo, MemberMap& members );
    static void ParseTypeInfo( ITypeInfo* pTypeInfo, MemberMap& members );
    void SetupMembers_Impl( JS::HandleObject jsObject );

private:
    JSContext* pJsCtx_ = nullptr;
    bool areMembersSetup_ = false;

    MemberMap members_;

public:
    smp::com::StorageObject* pStorage_ = smp::com::GetNewStoredObject();
    bool hasVariant_ = false;
};

} // namespace mozjs

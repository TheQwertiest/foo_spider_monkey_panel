#pragma once

#include <js_backend/objects/core/object_base.h>

#include <js/TypeDecls.h>

namespace mozjs
{

class ModulePath;

template <>
struct JsObjectTraits<ModulePath>
{
    static constexpr bool HasProto = false;

    static const JSClass JsClass;
    static const JSFunctionSpec* JsFunctions;
    static const JSPropertySpec* JsProperties;
};

class ModulePath
    : public JsObjectBase<ModulePath>
{
    MOZJS_ENABLE_OBJECT_BASE_ACCESS( ModulePath );

public:
    ~ModulePath() override;

    [[nodiscard]] static std::unique_ptr<ModulePath> CreateNative( JSContext* cx );
    [[nodiscard]] size_t GetInternalSize() const;

public:
    std::wstring Basename( const std::wstring& path, const std::wstring& suffix = L"" ) const;
    std::wstring BasenameWithOpt( size_t optArgCount, const std::wstring& path, const std::wstring& suffix ) const;
    std::wstring Dirname( const std::wstring& path ) const;
    std::wstring Extname( const std::wstring& path ) const;
    bool IsAbsolute( const std::wstring& path ) const;
    std::wstring Join( JS::HandleValueArray paths ) const;
    std::wstring Normalize( const std::wstring& path ) const;
    JSObject* Parse( const std::wstring& path ) const;
    std::wstring Relative( const std::wstring& from, const std::wstring& to ) const;

public:
    std::wstring get_Delimiter() const;
    std::wstring get_Sep() const;

private:
    [[nodiscard]] ModulePath( JSContext* cx );

private:
    JSContext* pJsCtx_ = nullptr;
};

} // namespace mozjs

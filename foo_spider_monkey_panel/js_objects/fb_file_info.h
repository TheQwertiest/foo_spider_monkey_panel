#pragma once

#include <optional>

class JSObject;
struct JSContext;
struct JSClass;


namespace mozjs
{

class JsFbFileInfo
{
public:
    ~JsFbFileInfo();
    
    static JSObject* Create( JSContext* cx, file_info_impl* pFileInfo );

    static const JSClass& GetClass();

public:
    std::optional<int32_t>InfoFind( const pfc::string8_fast& name );
    std::optional<pfc::string8_fast>InfoName( uint32_t idx );
    std::optional<pfc::string8_fast>InfoValue( uint32_t idx );
    std::optional<int32_t>MetaFind( const pfc::string8_fast& name );
    std::optional<pfc::string8_fast>MetaName( uint32_t idx );
    std::optional<pfc::string8_fast>MetaValue( uint32_t idx, uint32_t vidx );
    std::optional<uint32_t>MetaValueCount( uint32_t idx );

public:
    std::optional<uint32_t>get_InfoCount();
    std::optional<uint32_t>get_MetaCount();

private:
    JsFbFileInfo( JSContext* cx, file_info_impl* pFileInfo );
    JsFbFileInfo( const JsFbFileInfo& ) = delete;
    JsFbFileInfo& operator=( const JsFbFileInfo& ) = delete;

private:
    JSContext * pJsCtx_ = nullptr;
    std::unique_ptr<file_info_impl> fileInfo_;
};

}

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
    std::optional<int32_t>InfoFind( const std::string& name );
    std::optional<std::string>InfoName( uint32_t idx );
    std::optional<std::string>InfoValue( uint32_t idx );
    std::optional<int32_t>MetaFind( const std::string& name );
    std::optional<std::string>MetaName( uint32_t idx );
    std::optional<std::string>MetaValue( uint32_t idx, uint32_t vidx );
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

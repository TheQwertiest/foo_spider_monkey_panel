#include <stdafx.h>

#include "fs_stats.h"

#include <js_backend/engine/js_to_native_invoker.h>

using namespace smp;
namespace fs = std::filesystem;

namespace
{

using namespace mozjs;

JSClassOps jsOps = {
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    FsStats::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FsStats",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE( isDirectory, FsStats::IsDirectory );
MJS_DEFINE_JS_FN_FROM_NATIVE( isFile, FsStats::IsFile );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "isDirectory", isDirectory, 0, kDefaultPropsFlags ),
        JS_FN( "isFile", isFile, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_size, FsStats::get_Size );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "size", get_size, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::FsStats );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<FsStats>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<FsStats>::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsObjectTraits<FsStats>::JsProperties = jsProperties.data();

FsStats::FsStats( JSContext* cx, const std::filesystem::path& path )
    : pJsCtx_( cx )
    , path_( path )
{
}

FsStats::~FsStats()
{
}

std::unique_ptr<FsStats>
FsStats::CreateNative( JSContext* cx, const std::filesystem::path& path )
{
    return std::unique_ptr<FsStats>( new FsStats( cx, path ) );
}

size_t FsStats::GetInternalSize() const
{
    return 0;
}

bool FsStats::IsDirectory() const
{
    try
    {
        return fs::is_directory( path_ );
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

bool FsStats::IsFile() const
{
    try
    {
        return fs::is_regular_file( path_ );
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

JS::Value FsStats::get_Size() const
{
    try
    {
        auto size = static_cast<uint32_t>( fs::file_size( path_ ) );

        JS::RootedValue jsValue( pJsCtx_ );
        convert::to_js::ToValue( pJsCtx_, size, &jsValue );
        return jsValue;
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

} // namespace mozjs

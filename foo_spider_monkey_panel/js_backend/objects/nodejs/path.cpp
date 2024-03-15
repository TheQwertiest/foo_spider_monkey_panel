#include <stdafx.h>

#include "path.h"

#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/utils/js_property_helper.h>

using namespace smp;
namespace fs = std::filesystem;

namespace
{

std::wstring RemoveTrailingSeparators( const std::wstring& path )
{
    std::wstring trimmedPath{ path };
    while ( !trimmedPath.empty() && ( trimmedPath.ends_with( L"/" ) || trimmedPath.ends_with( L"\\" ) ) )
    {
        trimmedPath.resize( trimmedPath.size() - 1 );
    }
    return path;
}

} // namespace

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
    ModulePath::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "Path",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( basename, ModulePath::Basename, ModulePath::BasenameWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( dirname, ModulePath::Dirname );
MJS_DEFINE_JS_FN_FROM_NATIVE( extname, ModulePath::Extname );
MJS_DEFINE_JS_FN_FROM_NATIVE( isAbsolute, ModulePath::IsAbsolute );
MJS_DEFINE_JS_FN_FROM_NATIVE( join, ModulePath::Join );
MJS_DEFINE_JS_FN_FROM_NATIVE( normalize, ModulePath::Normalize );
MJS_DEFINE_JS_FN_FROM_NATIVE( parse, ModulePath::Parse );
MJS_DEFINE_JS_FN_FROM_NATIVE( relative, ModulePath::Relative );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "basename", basename, 1, kDefaultPropsFlags ),
        JS_FN( "dirname", dirname, 1, kDefaultPropsFlags ),
        JS_FN( "extname", extname, 1, kDefaultPropsFlags ),
        JS_FN( "isAbsolute", isAbsolute, 1, kDefaultPropsFlags ),
        JS_FN( "join", join, 0, kDefaultPropsFlags ),
        JS_FN( "normalize", normalize, 1, kDefaultPropsFlags ),
        JS_FN( "parse", parse, 1, kDefaultPropsFlags ),
        JS_FN( "relative", relative, 2, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_delimiter, ModulePath::get_Delimiter );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_sep, ModulePath::get_Sep );

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "delimiter", get_delimiter, kDefaultPropsFlags ),
        JS_PSG( "sep", get_sep, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::ModulePath );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<ModulePath>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<ModulePath>::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsObjectTraits<ModulePath>::JsProperties = jsProperties.data();

ModulePath::ModulePath( JSContext* cx )
    : pJsCtx_( cx )
{
}

ModulePath::~ModulePath()
{
}

std::unique_ptr<ModulePath>
ModulePath::CreateNative( JSContext* cx )
{
    return std::unique_ptr<ModulePath>( new ModulePath( cx ) );
}

size_t ModulePath::GetInternalSize() const
{
    return 0;
}

std::wstring ModulePath::Basename( const std::wstring& path, const std::wstring& suffix ) const
{
    try
    {
        auto result = fs::path{ RemoveTrailingSeparators( path ) }.filename().native();
        if ( !suffix.empty() && result.ends_with( suffix ) )
        {
            result.resize( result.size() - suffix.size() );
        }
        return result;
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

std::wstring ModulePath::BasenameWithOpt( size_t optArgCount, const std::wstring& path, const std::wstring& suffix ) const
{
    switch ( optArgCount )
    {
    case 0:
        return Basename( path, suffix );
    case 1:
        return Basename( path );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

std::wstring ModulePath::Dirname( const std::wstring& path ) const
{
    try
    {
        return fs::path{ RemoveTrailingSeparators( path ) }.parent_path().native();
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

std::wstring ModulePath::Extname( const std::wstring& path ) const
{
    try
    {
        return fs::path{ path }.extension();
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

bool ModulePath::IsAbsolute( const std::wstring& path ) const
{
    try
    {
        return fs::path{ path }.is_absolute();
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

std::wstring ModulePath::Join( JS::HandleValueArray paths ) const
{
    try
    {
        fs::path result;
        for ( size_t i = 0; i < paths.length(); ++i )
        {
            const auto path = convert::to_native::ToValue<std::wstring>( pJsCtx_, paths[i] );
            if ( result.empty() )
            {
                result = path;
                continue;
            }

            result /= path;
        }

        if ( result.empty() )
        {
            return L".";
        }
        else
        {
            return result.native();
        }
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

std::wstring ModulePath::Normalize( const std::wstring& path ) const
{
    try
    {
        return fs::path( path ).lexically_normal().native();
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

JSObject* ModulePath::Parse( const std::wstring& path ) const
{
    JS::RootedObject jsObject( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );
    smp::JsException::ExpectTrue( jsObject );

    try
    {
        fs::path fsPath{ RemoveTrailingSeparators( path ) };
        mozjs::utils::AddProperty( pJsCtx_, jsObject, "root", fsPath.is_absolute() ? fsPath.root_path().native() : L"" );
        mozjs::utils::AddProperty( pJsCtx_, jsObject, "dir", fsPath.parent_path().native() );
        mozjs::utils::AddProperty( pJsCtx_, jsObject, "base", fsPath.filename().native() );
        mozjs::utils::AddProperty( pJsCtx_, jsObject, "ext", fsPath.extension().native() );
        mozjs::utils::AddProperty( pJsCtx_, jsObject, "name", fsPath.stem().native() );

        return jsObject;
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

std::wstring ModulePath::Relative( const std::wstring& from, const std::wstring& to ) const
{
    try
    {
        fs::path basePath{ from };
        if ( basePath.empty() )
        {
            basePath = fs::current_path();
        }

        fs::path path{ to };
        if ( path.empty() )
        {
            path = fs::current_path();
        }

        if ( basePath.lexically_normal() == path.lexically_normal() )
        {
            return L"";
        }
        if ( basePath.is_absolute() )
        {
            if ( !path.is_absolute() )
            {
                path = fs::current_path() / path;
            }
            if ( basePath.root_name() != path.root_name() )
            {
                return path;
            }
        }

        return fs::relative( path, basePath ).native();
    }
    catch ( const fs::filesystem_error& e )
    {
        throw qwr::QwrException( e );
    }
}

std::wstring ModulePath::get_Delimiter() const
{
    return L";";
}

std::wstring ModulePath::get_Sep() const
{
    return { fs::path::preferred_separator };
}

} // namespace mozjs

#include <stdafx.h>

#include "fs_promises.h"

#include <convert/js_to_native.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/nodejs/fs_stats.h>
#include <js_backend/utils/js_property_helper.h>
#include <tasks/dispatcher/event_dispatcher.h>
#include <tasks/events/js_promise_event.h>

#include <js/experimental/TypedData.h>
#include <qwr/file_helpers.h>
#include <qwr/final_action.h>
#include <qwr/winapi_error_helpers.h>

SMP_MJS_SUPPRESS_WARNINGS_PUSH
#include <js/Promise.h>
SMP_MJS_SUPPRESS_WARNINGS_POP

#include <fcntl.h>

#include <qwr/text_helpers.h>

using namespace smp;
namespace fs = std::filesystem;

namespace
{

class FsThreadTask
{
public:
    [[nodiscard]] FsThreadTask( std::function<std::function<JS::Value()>()> fn,
                                JSContext* cx,
                                JS::HandleObject jsTarget,
                                HWND hPanelWnd );

    ~FsThreadTask() = default;

    FsThreadTask( const FsThreadTask& ) = delete;
    FsThreadTask& operator=( const FsThreadTask& ) = delete;

    void Run();

private:
    JSContext* pJsCtx_ = nullptr;
    mozjs::HeapDataHolder_Object heapHolder_;

    HWND hPanelWnd_ = nullptr;

    std::function<std::function<JS::Value()>()> fn_;
};

struct CreateFileParams
{
    uint32_t access = 0;
    uint32_t disposition = 0;
    uint32_t attributes = 0;
};

} // namespace

namespace
{

FsThreadTask::FsThreadTask( std::function<std::function<JS::Value()>()> fn,
                            JSContext* cx,
                            JS::HandleObject jsTarget,
                            HWND hPanelWnd )
    : pJsCtx_( cx )
    , heapHolder_( cx, jsTarget )
    , hPanelWnd_( hPanelWnd )
    , fn_( ( std::move( fn ) ) )
{
}

void FsThreadTask::Run()
{
    try
    {
        const auto promiseResolver = [&] {
            try
            {
                return fn_();
            }
            catch ( const fs::filesystem_error& e )
            {
                throw qwr::QwrException( e );
            }
        }();
        smp::EventDispatcher::Get().PutEvent( hPanelWnd_,
                                              std::make_unique<smp::JsPromiseEvent>(
                                                  pJsCtx_,
                                                  std::move( heapHolder_ ),
                                                  promiseResolver ) );
    }
    catch ( const qwr::QwrException& /*e*/ )
    {
        smp::EventDispatcher::Get().PutEvent( hPanelWnd_,
                                              std::make_unique<smp::JsPromiseEvent>(
                                                  pJsCtx_,
                                                  std::move( heapHolder_ ),
                                                  std::current_exception() ) );
    }
}

/// @throw qwr::QwrException
uint32_t ParseFlags( qwr::u8string_view flags )
{
    if ( flags == "r" )
    {
        return O_RDONLY;
    }
    if ( flags == "r+" )
    {
        return O_RDWR;
    }
    if ( flags == "w" )
    {
        return O_TRUNC | O_CREAT | O_WRONLY;
    }
    if ( flags == "wx" || flags == "xw" )
    {
        return O_TRUNC | O_CREAT | O_WRONLY | O_EXCL;
    }
    if ( flags == "w+" )
    {
        return O_TRUNC | O_CREAT | O_RDWR;
    }
    if ( flags == "wx+" || flags == "xw+" )
    {
        return O_TRUNC | O_CREAT | O_RDWR | O_EXCL;
    }
    if ( flags == "a" )
    {
        return O_APPEND | O_CREAT | O_WRONLY;
    }
    if ( flags == "ax" || flags == "xa" )
    {
        return O_APPEND | O_CREAT | O_WRONLY | O_EXCL;
    }
    if ( flags == "a+" )
    {
        return O_APPEND | O_CREAT | O_RDWR;
    }
    if ( flags == "ax+" || flags == "xa+" )
    {
        return O_APPEND | O_CREAT | O_RDWR | O_EXCL;
    }

    throw qwr::QwrException( "Unsupported flags value" );
}

// TODO: replace with smart pointer
CreateFileParams GenerateCreateFileParams( uint32_t flags )
{
    uint32_t access = 0;
    switch ( flags & ( O_RDONLY | O_WRONLY | O_RDWR ) )
    {
    case O_RDONLY:
        access = FILE_GENERIC_READ;
        break;
    case O_WRONLY:
        access = FILE_GENERIC_WRITE;
        break;
    case O_RDWR:
        access = FILE_GENERIC_READ | FILE_GENERIC_WRITE;
        break;
    default:
        assert( false );
    }
    if ( flags & O_APPEND )
    {
        access &= ~FILE_WRITE_DATA;
        access |= FILE_APPEND_DATA;
    }

    uint32_t disposition = 0;
    switch ( flags & ( O_CREAT | O_EXCL | O_TRUNC ) )
    {
    case 0:
    case O_EXCL:
        disposition = OPEN_EXISTING;
        break;
    case O_CREAT:
        disposition = OPEN_ALWAYS;
        break;
    case O_CREAT | O_EXCL:
    case O_CREAT | O_TRUNC | O_EXCL:
        disposition = CREATE_NEW;
        break;
    case O_TRUNC:
    case O_TRUNC | O_EXCL:
        disposition = TRUNCATE_EXISTING;
        break;
    case O_CREAT | O_TRUNC:
        disposition = CREATE_ALWAYS;
        break;
    default:
        assert( false );
    }

    uint32_t attributes = FILE_ATTRIBUTE_NORMAL;

    return { .access = access,
             .disposition = disposition,
             .attributes = attributes };
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
    FsPromises::FinalizeJsObject,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "FsPromises",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( readdir, FsPromises::ReadDir, FsPromises::ReadDirWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( readFile, FsPromises::ReadFile, FsPromises::ReadFileWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( stat, FsPromises::Stat, FsPromises::StatWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( writeFile, FsPromises::WriteFile, FsPromises::WriteFileWithOpt, 1 );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "readdir", readdir, 1, kDefaultPropsFlags ),
        JS_FN( "readFile", readFile, 1, kDefaultPropsFlags ),
        JS_FN( "stat", stat, 1, kDefaultPropsFlags ),
        JS_FN( "writeFile", writeFile, 1, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::FsPromises );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<FsPromises>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<FsPromises>::JsFunctions = jsFunctions.data();

FsPromises::FsPromises( JSContext* cx )
    : pJsCtx_( cx )
{
}

FsPromises::~FsPromises()
{
}

std::unique_ptr<FsPromises>
FsPromises::CreateNative( JSContext* cx )
{
    return std::unique_ptr<FsPromises>( new FsPromises( cx ) );
}

size_t FsPromises::GetInternalSize() const
{
    return 0;
}

// TODO: use nodejs like errors, codes and messages
// TODO: cleanup this crap

JSObject* FsPromises::ReadDir( JS::HandleValue path, JS::HandleValue options ) const
{
    qwr::QwrException::ExpectTrue( path.isString(), "path argument is not of supported type" );
    fs::path pathFs{ convert::to_native::ToValue<std::wstring>( pJsCtx_, path ) };

    bool isRecursive = false;
    std::optional<qwr::u8string> encodingOpt = "utf8";
    if ( options.isObject() )
    {
        JS::RootedObject jsOptions( pJsCtx_, &options.toObject() );
        if ( auto valueOpt = utils::GetOptionalProperty<bool>( pJsCtx_, jsOptions, "recursive" );
             valueOpt )
        {
            isRecursive = *valueOpt;
        }

        if ( auto valueOpt = utils::GetOptionalProperty<qwr::u8string>( pJsCtx_, jsOptions, "encoding" );
             valueOpt )
        {
            encodingOpt = *valueOpt;
        }
    }
    else if ( options.isString() )
    {
        encodingOpt = convert::to_native::ToValue<qwr::u8string>( pJsCtx_, options );
    }
    else
    {
        qwr::QwrException::ExpectTrue( options.isUndefined(), "Invalid options type" );
    }
    qwr::QwrException::ExpectTrue( encodingOpt == "utf8", "Unsupported encoding type" );

    auto taskFn = [pathFs, isRecursive, cx = pJsCtx_] {
        std::vector<std::wstring> files;
        if ( isRecursive )
        {
            // TODO: add nodejs-like sorting
            for ( const fs::directory_entry& e: fs::recursive_directory_iterator( pathFs ) )
            {
                files.emplace_back( fs::relative( e.path(), pathFs ) );
            }
        }
        else
        {
            for ( const fs::directory_entry& e: fs::directory_iterator( pathFs ) )
            {
                files.emplace_back( fs::relative( e.path(), pathFs ) );
            }
        }

        return [files = std::move( files ), cx]() -> JS::Value {
            JS::RootedValue jsValue( cx );
            convert::to_js::ToValue( cx, files, &jsValue );
            return jsValue;
        };
    };

    JS::RootedObject jsPromise( pJsCtx_, JS::NewPromiseObject( pJsCtx_, nullptr ) );
    JsException::ExpectTrue( jsPromise );

    EnqueueFsTask( std::move( taskFn ), jsPromise );

    return jsPromise;
}

JSObject* FsPromises::ReadDirWithOpt( size_t optArgCount, JS::HandleValue path, JS::HandleValue options ) const
{
    switch ( optArgCount )
    {
    case 0:
        return ReadDir( path, options );
    case 1:
        return ReadDir( path );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JSObject* FsPromises::ReadFile( JS::HandleValue path, JS::HandleValue options ) const
{
    qwr::QwrException::ExpectTrue( path.isString(), "path argument is not of supported type" );
    fs::path pathFs{ convert::to_native::ToValue<std::wstring>( pJsCtx_, path ) };

    qwr::u8string flagStr = "r";
    std::optional<qwr::u8string> encodingOpt;
    if ( options.isObject() )
    {
        JS::RootedObject jsOptions( pJsCtx_, &options.toObject() );
        if ( auto valueOpt = utils::GetOptionalProperty<qwr::u8string>( pJsCtx_, jsOptions, "flag" );
             valueOpt )
        {
            flagStr = *valueOpt;
        }

        encodingOpt = utils::GetOptionalProperty<qwr::u8string>( pJsCtx_, jsOptions, "encoding" );
    }
    else if ( options.isString() )
    {
        encodingOpt = convert::to_native::ToValue<qwr::u8string>( pJsCtx_, options );
    }
    else
    {
        qwr::QwrException::ExpectTrue( options.isUndefined(), "Invalid options type" );
    }
    qwr::QwrException::ExpectTrue( !encodingOpt || encodingOpt == "utf8", "Unsupported encoding type" );

    const auto flags = ParseFlags( flagStr );
    qwr::QwrException::ExpectTrue( flags != O_WRONLY, "Can't read file in write-only mode" );

    auto taskFn = [pathFs, flags, encodingOpt, cx = pJsCtx_] {
        std::vector<char> data;
        {
            const auto createFileParams = GenerateCreateFileParams( flags );

            auto hFile = CreateFile( pathFs.wstring().c_str(), createFileParams.access, FILE_SHARE_READ, nullptr, createFileParams.disposition, createFileParams.attributes, nullptr );
            qwr::error::CheckWinApi( ( INVALID_HANDLE_VALUE != hFile ), "CreateFile" );
            qwr::final_action autoFile( [hFile] {
                CloseHandle( hFile );
            } );

            // TODO: move chunk size to constant
            std::array<char, 512 * 1024> buffer{};
            DWORD bytesRead = 0;
            do
            {
                auto bRet = ::ReadFile( hFile, buffer.data(), buffer.size(), &bytesRead, nullptr );
                qwr::error::CheckWinApi( bRet, "ReadFile" );

                data.insert( data.end(), buffer.data(), buffer.data() + bytesRead );
            } while ( bytesRead == buffer.size() );
        }

        /*
        qwr::u8string_view dataView( data.data(), data.size() );
        const auto detectedCharset = qwr::DetectCharSet( dataView ).value_or( CP_ACP );
        // TODO: handle encoding
        auto convertedContent = [&] {
            if ( detectedCharset == CP_UTF8 )
            {
                return qwr::unicode::ToWide( dataView );
            }
            // TODO: test this encoding
            else if (detectedCharset == 1200) // utf16
            {
                std::wstring tmp;
                tmp.resize( dataView.size() >> 1 );
                // Can't use wstring.assign(), because of potential aliasing issues
                memcpy( tmp.data(), dataView.data(), dataView.size() );
                return tmp;
            }
            else
            {
                std::wstring tmpString;
                size_t outputSize = pfc::stringcvt::estimate_codepage_to_wide( detectedCharset, dataView.data(), dataView.size() );
                tmpString.resize( outputSize );

                outputSize = pfc::stringcvt::convert_codepage_to_wide( detectedCharset, tmpString.data(), outputSize, dataView.data(), dataView.size() );
                tmpString.resize( outputSize );

                return tmpString;
            }
        }();
        */

        return [data = std::move( data ), encodingOpt, cx]() -> JS::Value {
            JS::RootedValue jsValue( cx );
            if ( !encodingOpt )
            {
                qwr::u8string_view dataView( data.data(), data.size() );
                convert::to_js::ToValue( cx, dataView, &jsValue );

                JS::RootedObject jsArray( cx, JS_NewUint8ClampedArray( cx, data.size() ) );
                smp::JsException::ExpectTrue( jsArray );

                size_t arraySize = 0;
                bool isShared = false;
                uint8_t* jsArrayData = nullptr;
                js::GetUint8ClampedArrayLengthAndData( jsArray, &arraySize, &isShared, &jsArrayData );

                memcpy( jsArrayData, data.data(), data.size() );

                jsValue.setObjectOrNull( jsArray );
            }
            else
            {
                qwr::u8string_view dataView( data.data(), data.size() );
                convert::to_js::ToValue( cx, dataView, &jsValue );
            }
            return jsValue;
        };
    };

    JS::RootedObject jsPromise( pJsCtx_, JS::NewPromiseObject( pJsCtx_, nullptr ) );
    JsException::ExpectTrue( jsPromise );

    EnqueueFsTask( std::move( taskFn ), jsPromise );

    return jsPromise;
}

JSObject* FsPromises::ReadFileWithOpt( size_t optArgCount, JS::HandleValue path, JS::HandleValue options ) const
{
    switch ( optArgCount )
    {
    case 0:
        return ReadFile( path, options );
    case 1:
        return ReadFile( path );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JSObject* FsPromises::Stat( JS::HandleValue path, JS::HandleValue options ) const
{
    qwr::QwrException::ExpectTrue( path.isString(), "path argument is not of supported type" );
    fs::path pathFs{ convert::to_native::ToValue<std::wstring>( pJsCtx_, path ) };

    qwr::QwrException::ExpectTrue( fs::exists( pathFs ), "File does not exist" );

    auto taskFn = [pathFs, cx = pJsCtx_] {
        return [pathFs, cx]() -> JS::Value {
            JS::RootedValue jsValue( cx );
            jsValue.setObjectOrNull( FsStats::CreateJs( cx, pathFs ) );
            return jsValue;
        };
    };

    JS::RootedObject jsPromise( pJsCtx_, JS::NewPromiseObject( pJsCtx_, nullptr ) );
    JsException::ExpectTrue( jsPromise );

    EnqueueFsTask( std::move( taskFn ), jsPromise );

    return jsPromise;
}

JSObject* FsPromises::StatWithOpt( size_t optArgCount, JS::HandleValue path, JS::HandleValue options ) const
{
    switch ( optArgCount )
    {
    case 0:
        return Stat( path, options );
    case 1:
        return Stat( path );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

JSObject* FsPromises::WriteFile( JS::HandleValue path, JS::HandleValue data, JS::HandleValue options ) const
{
    qwr::QwrException::ExpectTrue( path.isString(), "path argument is not of supported type" );
    fs::path pathFs{ convert::to_native::ToValue<std::wstring>( pJsCtx_, path ) };

    qwr::QwrException::ExpectTrue( data.isString(), "data argument is not of supported type" );
    const auto content = convert::to_native::ToValue<qwr::u8string>( pJsCtx_, data );

    qwr::u8string flagStr = "w";
    std::optional<qwr::u8string> encodingOpt = "utf8";
    if ( options.isObject() )
    {
        JS::RootedObject jsOptions( pJsCtx_, &options.toObject() );
        if ( auto valueOpt = utils::GetOptionalProperty<qwr::u8string>( pJsCtx_, jsOptions, "flag" );
             valueOpt )
        {
            flagStr = *valueOpt;
        }

        if ( auto valueOpt = utils::GetOptionalProperty<qwr::u8string>( pJsCtx_, jsOptions, "encoding" );
             valueOpt )
        {
            encodingOpt = *valueOpt;
        }
    }
    else if ( options.isString() )
    {
        encodingOpt = convert::to_native::ToValue<qwr::u8string>( pJsCtx_, options );
    }
    else
    {
        qwr::QwrException::ExpectTrue( options.isUndefined(), "Invalid options type" );
    }
    qwr::QwrException::ExpectTrue( !encodingOpt || encodingOpt == "utf8", "Unsupported encoding type" );

    const auto flags = ParseFlags( flagStr );
    qwr::QwrException::ExpectTrue( flags != O_RDONLY, "Can't write file in read-only mode" );

    auto taskFn = [pathFs, flags, content] {
        const auto createFileParams = GenerateCreateFileParams( flags );

        HANDLE hFile = CreateFile( pathFs.wstring().c_str(), createFileParams.access, 0, nullptr, createFileParams.disposition, createFileParams.attributes, nullptr );
        qwr::error::CheckWinApi( hFile != INVALID_HANDLE_VALUE, "CreateFile" );
        qwr::final_action autoFile( [hFile] {
            CloseHandle( hFile );
        } );

        qwr::u8string_view dataView( content );
        // TODO: move chunk size to constant
        const size_t chunkSize = 512 * 1024;
        DWORD bytesWritten = 0;
        while ( !dataView.empty() )
        {
            auto bRet = ::WriteFile( hFile, dataView.data(), std::min( chunkSize, dataView.size() ), &bytesWritten, nullptr );
            qwr::error::CheckWinApi( bRet, "WriteFile" );

            dataView.remove_prefix( bytesWritten );
        }

        return []() -> JS::Value {
            return JS::UndefinedValue();
        };
    };

    JS::RootedObject jsPromise( pJsCtx_, JS::NewPromiseObject( pJsCtx_, nullptr ) );
    JsException::ExpectTrue( jsPromise );

    EnqueueFsTask( std::move( taskFn ), jsPromise );

    return jsPromise;
}

JSObject* FsPromises::WriteFileWithOpt( size_t optArgCount, JS::HandleValue path, JS::HandleValue data, JS::HandleValue options ) const
{
    switch ( optArgCount )
    {
    case 0:
        return WriteFile( path, data, options );
    case 1:
        return WriteFile( path, data );
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

void FsPromises::EnqueueFsTask( FsTaskFn taskFn, JS::HandleObject jsTarget ) const
{
    const auto pTask = std::make_shared<FsThreadTask>( std::move( taskFn ),
                                                       pJsCtx_,
                                                       jsTarget,
                                                       GetPanelHwndForCurrentGlobal( pJsCtx_ ) );
    fb2k::inCpuWorkerThread( [pTask] { pTask->Run(); } );
}

} // namespace mozjs

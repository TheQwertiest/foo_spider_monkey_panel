#include <stdafx.h>

#include "process.h"

#include <js_backend/engine/context.h>
#include <js_backend/engine/js_to_native_invoker.h>
#include <js_backend/objects/dom/event.h>
#include <js_backend/utils/js_property_helper.h>

#include <js/BigInt.h>
#include <qwr/fb2k_paths.h>
#include <qwr/winapi_error_helpers.h>

using namespace smp;

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
    JsObjectBase<Process>::FinalizeJsObject,
    nullptr,
    nullptr,
    Process::Trace
};

JSClass jsClass = {
    "Process",
    kDefaultClassFlags,
    &jsOps
};

MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_OPT( cpuUsage, Process::CpuUsage, Process::CpuUsageWithOpt, 1 );
MJS_DEFINE_JS_FN_FROM_NATIVE( cwd, Process::Cwd );
MJS_DEFINE_JS_FN_FROM_NATIVE_WITH_LOG( exitImpl, "exit", Process::Exit );
MJS_DEFINE_JS_FN_FROM_NATIVE( memoryUsage, Process::MemoryUsage );

constexpr auto jsFunctions = std::to_array<JSFunctionSpec>(
    {
        JS_FN( "cpuUsage", cpuUsage, 0, kDefaultPropsFlags ),
        JS_FN( "cwd", cwd, 0, kDefaultPropsFlags ),
        JS_FN( "exit", exitImpl, 0, kDefaultPropsFlags ),
        JS_FN( "memoryUsage", memoryUsage, 0, kDefaultPropsFlags ),
        JS_FS_END,
    } );

MJS_DEFINE_JS_FN_FROM_NATIVE( get_execPath, Process::get_ExecPath );
MJS_DEFINE_JS_FN_FROM_NATIVE( get_hrtime, Process::get_HrTime )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_launchOptions, Process::get_LaunchOptions )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_profilePath, Process::get_ProfilePath )
MJS_DEFINE_JS_FN_FROM_NATIVE( get_version, Process::get_Version )

constexpr auto jsProperties = std::to_array<JSPropertySpec>(
    {
        JS_PSG( "execPath", get_execPath, kDefaultPropsFlags ),
        JS_PSG( "hrtime", get_hrtime, kDefaultPropsFlags ),
        JS_PSG( "launchOptions", get_launchOptions, kDefaultPropsFlags ),
        JS_PSG( "profilePath", get_profilePath, kDefaultPropsFlags ),
        JS_PSG( "version", get_version, kDefaultPropsFlags ),
        JS_PS_END,
    } );

MJS_VERIFY_OBJECT( mozjs::Process );

} // namespace

namespace mozjs
{

const JSClass JsObjectTraits<Process>::JsClass = jsClass;
const JSFunctionSpec* JsObjectTraits<Process>::JsFunctions = jsFunctions.data();
const JSPropertySpec* JsObjectTraits<Process>::JsProperties = jsProperties.data();

const std::unordered_set<smp::EventId> Process::kHandledEvents{
    EventId::kNew_FbExit,
};

Process::Process( JSContext* cx )
    : JsEventTarget( cx )
    , pJsCtx_( cx )
{
}

Process::~Process()
{
}

std::unique_ptr<Process>
Process::CreateNative( JSContext* cx )
{
    return std::unique_ptr<Process>( new Process( cx ) );
}

size_t Process::GetInternalSize() const
{
    return 0;
}

void Process::Trace( JSTracer* trc, JSObject* obj )
{
    JsEventTarget::Trace( trc, obj );

    auto pNative = JsObjectBase<Process>::ExtractNativeUnchecked( obj );
    if ( !pNative )
    {
        return;
    }

    JS::TraceEdge( trc, &pNative->jsLaunchOptions_, "Heap: Process: launch options" );
}

const std::string& Process::EventIdToType( smp::EventId eventId )
{
    static const std::unordered_map<EventId, std::string> idToType{
        { EventId::kNew_FbExit, "exit" },
    };

    assert( idToType.size() == kHandledEvents.size() );
    assert( idToType.contains( eventId ) );
    return idToType.at( eventId );
}

mozjs::EventStatus Process::HandleEvent( JS::HandleObject self, const smp::EventBase& event )
{
    EventStatus status;

    const auto& eventType = EventIdToType( event.GetId() );
    if ( !HasEventListener( eventType ) )
    {
        return status;
    }
    status.isHandled = true;

    JS::RootedValue jsEvent( pJsCtx_ );
    jsEvent.setObjectOrNull( mozjs::JsEvent::CreateJs( pJsCtx_, eventType, JsEvent::EventProperties{ .cancelable = false } ) );
    DispatchEvent( self, jsEvent );

    const auto pNativeEvent = convert::to_native::ToValue<JsEvent*>( pJsCtx_, jsEvent );
    status.isDefaultSuppressed = pNativeEvent->get_DefaultPrevented();
    return status;
}

JSObject* Process::CpuUsage( JS::HandleValue previousValue ) const
{
    qwr::QwrException::ExpectTrue( previousValue.isUndefined() || previousValue.isObject(), "Invalid previousValue type" );

    double prevUserTimeNs = 0;
    double prevKernelTimeNs = 0;
    if ( previousValue.isObject() )
    {
        JS::RootedObject jsPrevObject( pJsCtx_, previousValue.toObjectOrNull() );
        if ( !utils::OptionalPropertyTo( pJsCtx_, jsPrevObject, "user", prevUserTimeNs ) )
        {
            throw qwr::QwrException( "Malformed previousValue: missing `user` field" );
        }
        if ( !utils::OptionalPropertyTo( pJsCtx_, jsPrevObject, "system", prevKernelTimeNs ) )
        {
            throw qwr::QwrException( "Malformed previousValue: missing `system` field" );
        }
    }

    FILETIME createTime{};
    FILETIME exitTime{};
    FILETIME kernelTime{};
    FILETIME userTime{};
    SYSTEMTIME kernelSystemTime{};
    SYSTEMTIME userSystemTime{};
    auto bRet = GetProcessTimes( GetCurrentProcess(), &createTime, &exitTime, &kernelTime, &userTime );
    qwr::error::CheckWinApi( bRet, "GetProcessTimes" );

    bRet = FileTimeToSystemTime( &kernelTime, &kernelSystemTime );
    qwr::error::CheckWinApi( bRet, "GetProcessTimes" );

    bRet = FileTimeToSystemTime( &userTime, &userSystemTime );
    qwr::error::CheckWinApi( bRet, "GetProcessTimes" );

    const double userTimeNs = ( userSystemTime.wHour * 3600 + userSystemTime.wMinute * 60 + userSystemTime.wSecond ) * 1'000'000 + userSystemTime.wMilliseconds * 1000;
    const double kernelTimeNs = ( kernelSystemTime.wHour * 3600 + kernelSystemTime.wMinute * 60 + kernelSystemTime.wSecond ) * 1'000'000 + kernelSystemTime.wMilliseconds * 1000;

    JS::RootedObject jsObject( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );
    smp::JsException::ExpectTrue( jsObject );

    utils::SetProperty( pJsCtx_, jsObject, "user", userTimeNs - prevUserTimeNs );
    utils::SetProperty( pJsCtx_, jsObject, "system", kernelTimeNs - prevKernelTimeNs );

    return jsObject;
}

JSObject* Process::CpuUsageWithOpt( size_t optArgCount, JS::HandleValue previousValue ) const
{
    switch ( optArgCount )
    {
    case 0:
        return CpuUsage( previousValue );
    case 1:
        return CpuUsage();
    default:
        throw qwr::QwrException( "Internal error: invalid number of optional arguments specified: {}", optArgCount );
    }
}

std::wstring Process::Cwd() const
{
    std::array<wchar_t, MAX_PATH + 1> buffer;
    auto bRet = _wgetcwd( buffer.data(), buffer.size() );
    qwr::QwrException::ExpectTrue( bRet, "Failed to get CWD" );

    return buffer.data();
}

void Process::Exit() const
{
    standard_commands::main_exit();
}

JSObject* Process::MemoryUsage() const
{
    JS::RootedObject jsObject( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );
    smp::JsException::ExpectTrue( jsObject );

    utils::AddProperty( pJsCtx_, jsObject, "heap", static_cast<double>( ContextInner::Get().GetGcEngine().GetGlobalHeapUsage() ) );
    utils::AddProperty( pJsCtx_, jsObject, "external", static_cast<double>( ContextInner::Get().GetGcEngine().GetExternalHeapUsage() ) );
    utils::AddProperty( pJsCtx_, jsObject, "limit", static_cast<double>( JsGc::GetMaxHeap() ) );

    return jsObject;
}

std::wstring Process::get_ExecPath() const
{
    std::array<wchar_t, MAX_PATH + 1> buffer;
    auto hr = GetModuleFileName( nullptr, buffer.data(), buffer.size() );
    qwr::error::CheckWinApi( hr, "GetModuleFileName" );

    return buffer.data();
}

JS::BigInt* Process::get_HrTime() const
{
    LARGE_INTEGER ticks;

    auto bRet = QueryPerformanceCounter( &ticks );
    qwr::error::CheckWinApi( bRet, "QueryPerformanceCounter" );

    JS::RootedBigInt jsInt( pJsCtx_, JS::NumberToBigInt( pJsCtx_, static_cast<uint64_t>( ticks.QuadPart ) ) );
    smp::JsException::ExpectTrue( jsInt );

    return jsInt;
}

JSObject* Process::get_LaunchOptions() const
{
    if ( !jsLaunchOptions_ )
    {
        JS::RootedObject jsObject( pJsCtx_, JS_NewPlainObject( pJsCtx_ ) );
        smp::JsException::ExpectTrue( jsObject );

        utils::AddProperty( pJsCtx_, jsObject, "isPortableModeEnabled", core_api::is_portable_mode_enabled() );

        jsLaunchOptions_ = jsObject;
    }

    return jsLaunchOptions_;
}

std::wstring Process::get_ProfilePath() const
{
    return qwr::path::Profile().wstring();
}

qwr::u8string Process::get_Version() const
{
    return core_version_info_v2::get()->get_version_as_text();
}

} // namespace mozjs

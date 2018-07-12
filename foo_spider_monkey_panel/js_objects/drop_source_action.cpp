#include <stdafx.h>

#include "drop_source_action.h"

#include <js_engine/js_to_native_invoker.h>
#include <js_utils/js_error_helper.h>
#include <js_utils/js_object_helper.h>


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
    JsFinalizeOp<JsDropSourceAction>,
    nullptr,
    nullptr,
    nullptr,
    nullptr
};

JSClass jsClass = {
    "DropSourceAction",
    DefaultClassFlags(),
    &jsOps
};

MJS_DEFINE_JS_TO_NATIVE_FN( JsDropSourceAction, get_Effect )
MJS_DEFINE_JS_TO_NATIVE_FN( JsDropSourceAction, put_Base )
MJS_DEFINE_JS_TO_NATIVE_FN( JsDropSourceAction, put_Effect )
MJS_DEFINE_JS_TO_NATIVE_FN( JsDropSourceAction, put_Playlist )
MJS_DEFINE_JS_TO_NATIVE_FN( JsDropSourceAction, put_ToSelect )

const JSPropertySpec jsProperties[] = {
    JS_PSGS( "Base", DummyGetter, put_Base, DefaultPropsFlags() ),
    JS_PSGS( "Effect", get_Effect, put_Effect, DefaultPropsFlags() ),
    JS_PSGS( "Playlist", DummyGetter, put_Playlist, DefaultPropsFlags() ),
    JS_PSGS( "ToSelect", DummyGetter, put_ToSelect, DefaultPropsFlags() ),
    JS_PS_END
};


const JSFunctionSpec jsFunctions[] = {    
    JS_FS_END
};

}

namespace mozjs
{

const JSClass JsDropSourceAction::JsClass = jsClass;
const JSFunctionSpec* JsDropSourceAction::JsFunctions = jsFunctions;
const JSPropertySpec* JsDropSourceAction::JsProperties = jsProperties;
const JsPrototypeId JsDropSourceAction::PrototypeId = JsPrototypeId::DropSourceAction;

JsDropSourceAction::JsDropSourceAction( JSContext* cx )
    : pJsCtx_( cx )
{
    Reset();
}
JsDropSourceAction::~JsDropSourceAction()
{
}

std::unique_ptr<mozjs::JsDropSourceAction> 
JsDropSourceAction::CreateNative( JSContext* cx )
{
    return std::unique_ptr<JsDropSourceAction>( new JsDropSourceAction( cx ) );
}

void JsDropSourceAction::Reset()
{
    playlistIdx_ = -1;
    base_ = 0;
    toSelect_ = true;
    effect_ = DROPEFFECT_NONE;
}

uint32_t & JsDropSourceAction::Base()
{
    return base_;
}

int32_t& JsDropSourceAction::Playlist()
{
    return playlistIdx_;
}

bool& JsDropSourceAction::ToSelect()
{
    return toSelect_;
}

uint32_t& JsDropSourceAction::Effect()
{
    return effect_;
}

std::optional<uint32_t> 
JsDropSourceAction::get_Effect()
{
    return effect_;
}

std::optional<std::nullptr_t> 
JsDropSourceAction::put_Base( uint32_t base )
{
    base_ = base;
    return nullptr;
}

std::optional<std::nullptr_t> 
JsDropSourceAction::put_Effect( uint32_t effect )
{
    effect_ = effect;
    return nullptr;
}

std::optional<std::nullptr_t> 
JsDropSourceAction::put_Playlist( int32_t id )
{
    playlistIdx_ = id;
    return nullptr;
}

std::optional<std::nullptr_t> 
JsDropSourceAction::put_ToSelect( bool toSelect )
{
    toSelect_ = toSelect;
    return nullptr;
}

}

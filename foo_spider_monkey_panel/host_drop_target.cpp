#include "stdafx.h"
#include "host.h"
#include "host_drop_target.h"
#include "js_panel_window.h"

#include <js_engine/js_container.h>
#include <js_objects/global_object.h>
#include <js_objects/drop_source_action.h>

// TODO: rewrite this (mb move to js container?)

HostDropTarget::HostDropTarget( JSContext * cx, HWND hWnd, mozjs::JsContainer* pJsContainer )
    : IDropTargetImpl( hWnd )
    , pJsCtx_( cx )
    , pJsContainer_( pJsContainer )
{
    assert( cx );
    JS::RootedObject jsObject( cx, mozjs::JsDropSourceAction::CreateJs( cx ) );
    if ( !jsObject )
    {// report in CreateJs
        throw std::runtime_error( "Failed to create JsDropSourceAction" );
    }

    pNativeAction_ = static_cast<mozjs::JsDropSourceAction*>(JS_GetInstancePrivate( cx, jsObject, &mozjs::JsDropSourceAction::JsClass, nullptr ));
    assert( pNativeAction_ );

    JS::RootedObject jsGlobal( cx, JS::CurrentGlobalOrNull( cx ) );
    assert( jsGlobal );

    pNativeGlobal_ = static_cast<mozjs::JsGlobalObject*>(JS_GetInstancePrivate( cx, jsGlobal, &mozjs::JsGlobalObject::JsClass, nullptr ));
    assert( pNativeGlobal_ );

    auto& heapMgr = pNativeGlobal_->GetHeapManager();

    heapMgr.RegisterUser( this );

    JS::RootedValue objectValue( cx, JS::ObjectValue( *jsObject ) );
    objectId_ = heapMgr.Store( objectValue );
    JS::RootedValue globalValue( cx, JS::ObjectValue( *jsGlobal ) );
    globalId_ = heapMgr.Store( globalValue );

    needsCleanup_ = true;
}

HostDropTarget::~HostDropTarget()
{
}

void HostDropTarget::FinalRelease()
{// most of the JS object might be invalid at GC time,
 // so we need to be extra careful
    if ( !needsCleanup_ )
    {
        return;
    }

    std::scoped_lock sl( cleanupLock_ );
    if ( !needsCleanup_ )
    {
        return;
    }

    auto& heapMgr = pNativeGlobal_->GetHeapManager();

    heapMgr.Remove( globalId_ );
    heapMgr.Remove( objectId_ );
    heapMgr.UnregisterUser( this );

}

void HostDropTarget::DisableHeapCleanup()
{
    std::scoped_lock sl( cleanupLock_ );
    needsCleanup_ = false;
}

HRESULT HostDropTarget::OnDragEnter( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    if ( !pdwEffect )
    {
        return E_POINTER;
    }

    pNativeAction_->Reset();

    bool native;
    HRESULT hr = ole_interaction::get()->check_dataobject( pDataObj, m_fb2kAllowedEffect, native );
    if ( !SUCCEEDED( hr ) )
    {
        m_fb2kAllowedEffect = DROPEFFECT_NONE;
    }
    else if ( native && (DROPEFFECT_MOVE & *pdwEffect) )
    {
        m_fb2kAllowedEffect |= DROPEFFECT_MOVE; // Remove check_dataobject move suppression for intra fb2k interactions
    }

    pNativeAction_->Effect() = *pdwEffect & m_fb2kAllowedEffect;

    ScreenToClient( m_hWnd, reinterpret_cast<LPPOINT>(&pt) );

    on_drag_enter( grfKeyState, pt );

    *pdwEffect = pNativeAction_->Effect();
    return S_OK;
}

HRESULT HostDropTarget::OnDragLeave()
{
    on_drag_leave();
    return S_OK;
}

HRESULT HostDropTarget::OnDragOver( DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    if ( !pdwEffect )
    {
        return E_POINTER;
    }

    pNativeAction_->Effect() = *pdwEffect & m_fb2kAllowedEffect;

    ScreenToClient( m_hWnd, reinterpret_cast<LPPOINT>(&pt) );
    on_drag_over( grfKeyState, pt );

    *pdwEffect = pNativeAction_->Effect();

    return S_OK;
}

HRESULT HostDropTarget::OnDrop( IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect )
{
    if ( !pdwEffect )
    {
        return E_POINTER;
    }

    pNativeAction_->Effect() = *pdwEffect & m_fb2kAllowedEffect;

    ScreenToClient( m_hWnd, reinterpret_cast<LPPOINT>(&pt) );
    on_drag_drop( grfKeyState, pt );

    if ( *pdwEffect == DROPEFFECT_NONE || pNativeAction_->Effect() == DROPEFFECT_NONE )
    {
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

    dropped_files_data_impl droppedData;
    HRESULT hr = ole_interaction::get()->parse_dataobject( pDataObj, droppedData );
    if ( SUCCEEDED( hr ) )
    {
        int playlist = pNativeAction_->Playlist();
        t_size base = pNativeAction_->Base();
        bool to_select = pNativeAction_->ToSelect();

        droppedData.to_handles_async_ex( playlist_incoming_item_filter_v2::op_flag_delay_ui,
                                         core_api::get_main_window(),
                                         new service_impl_t<helpers::js_process_locations>( playlist, base, to_select ) );
    }

    *pdwEffect = pNativeAction_->Effect();

    return S_OK;
}

void HostDropTarget::on_drag_enter( unsigned keyState, POINTL& pt )
{
    auto& heapMgr = pNativeGlobal_->GetHeapManager();

    JSAutoRequest ar( pJsCtx_ );
    JS::RootedObject jsGlobal( pJsCtx_, heapMgr.Get( globalId_ ).toObjectOrNull() );
    assert( jsGlobal );
    JSAutoCompartment ac( pJsCtx_, jsGlobal );

    JS::RootedValue jsValue( pJsCtx_, heapMgr.Get( objectId_ ) );
    assert( jsValue.isObject() );
    JS::RootedObject jsObject( pJsCtx_, &jsValue.toObject() );

    pJsContainer_->InvokeJsCallback( "on_drag_enter",
                                     static_cast<JS::HandleObject>(jsObject),
                                     static_cast<int32_t>(pt.x),
                                     static_cast<int32_t>(pt.y),
                                     static_cast<uint32_t>(keyState) );
}

void HostDropTarget::on_drag_leave()
{
    pJsContainer_->InvokeJsCallback( "on_drag_leave" );
}

void HostDropTarget::on_drag_over( unsigned keyState, POINTL& pt )
{
    auto& heapMgr = pNativeGlobal_->GetHeapManager();

    JSAutoRequest ar( pJsCtx_ );
    JS::RootedObject jsGlobal( pJsCtx_, heapMgr.Get( globalId_ ).toObjectOrNull() );
    assert( jsGlobal );
    JSAutoCompartment ac( pJsCtx_, jsGlobal );

    JS::RootedValue jsValue( pJsCtx_, heapMgr.Get( objectId_ ) );
    assert( jsValue.isObject() );
    JS::RootedObject jsObject( pJsCtx_, &jsValue.toObject() );

    pJsContainer_->InvokeJsCallback( "on_drag_over",
                                     static_cast<JS::HandleObject>(jsObject),
                                     static_cast<int32_t>(pt.x),
                                     static_cast<int32_t>(pt.y),
                                     static_cast<uint32_t>(keyState) );
}

void HostDropTarget::on_drag_drop( unsigned keyState, POINTL& pt )
{
    auto& heapMgr = pNativeGlobal_->GetHeapManager();

    JSAutoRequest ar( pJsCtx_ );
    JS::RootedObject jsGlobal( pJsCtx_, heapMgr.Get( globalId_ ).toObjectOrNull() );
    assert( jsGlobal );
    JSAutoCompartment ac( pJsCtx_, jsGlobal );

    JS::RootedValue jsValue( pJsCtx_, heapMgr.Get( objectId_ ) );
    assert( jsValue.isObject() );
    JS::RootedObject jsObject( pJsCtx_, &jsValue.toObject() );

    pJsContainer_->InvokeJsCallback( "on_drag_drop",
                                     static_cast<JS::HandleObject>(jsObject),
                                     static_cast<int32_t>(pt.x),
                                     static_cast<int32_t>(pt.y),
                                     static_cast<uint32_t>(keyState) );
}

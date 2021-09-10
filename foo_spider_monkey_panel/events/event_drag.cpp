#include <stdafx.h>

#include "event_drag.h"

#include <com_utils/com_destruction_handler.h>
#include <js_engine/js_container.h>
#include <panel/js_panel_window.h>

// TODO: store IDataObjectPtr in some panel object instead

namespace smp
{

Event_Drag::Event_Drag( EventId id, int32_t x, int32_t y, uint32_t mask, uint32_t modifiers, const panel::DragActionParams& dragParams, IDataObjectPtr pData )
    : Event_Mouse( id, x, y, mask, modifiers )
    , dragParams_( dragParams )
    , pDataObject_( pData )
    , pStorage_( com::GetNewStoredObject() )
{
    assert( core_api::is_main_thread() );
    assert( pDataObject_ );
    assert( pStorage_ );

    pDataObject_->AddRef();
    pStorage_->pUnknown = pDataObject_;
}

Event_Drag::~Event_Drag()
{
    assert( !pDataObject_ || core_api::is_main_thread() );
}

Event_Drag* Event_Drag::AsDragEvent()
{
    return this;
}

std::optional<bool> Event_Drag::JsExecute( mozjs::JsContainer& /*jsContainer*/ )
{
    assert( false );
    return std::nullopt;
}

const panel::DragActionParams& Event_Drag::GetDragParams() const
{
    return dragParams_;
}

IDataObjectPtr Event_Drag::GetStoredData() const
{
    assert( pDataObject_ );
    return pDataObject_;
}

void Event_Drag::DisposeStoredData()
{
    assert( core_api::is_main_thread() );
    if ( pStorage_ )
    {
        pDataObject_ = nullptr;
        com::MarkStoredObjectAsToBeDeleted( pStorage_ );
        pStorage_ = nullptr;
    }
}

} // namespace smp

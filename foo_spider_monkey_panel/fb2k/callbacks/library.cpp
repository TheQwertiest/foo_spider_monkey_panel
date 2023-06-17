#include <stdafx.h>

#include <tasks/dispatcher/event_dispatcher.h>
#include <tasks/events/panel_event.h>

using namespace smp;

namespace
{

class LibraryCallbackImpl : public library_callback
{
public:
    void on_items_added( metadb_handle_list_cref p_data ) override;
    void on_items_modified( metadb_handle_list_cref p_data ) override;
    void on_items_removed( metadb_handle_list_cref p_data ) override;
};

} // namespace

namespace
{

void LibraryCallbackImpl::on_items_added( metadb_handle_list_cref p_data )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbLibraryItemsAdded ) );
}

void LibraryCallbackImpl::on_items_modified( metadb_handle_list_cref p_data )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbLibraryItemsModified ) );
}

void LibraryCallbackImpl::on_items_removed( metadb_handle_list_cref p_data )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbLibraryItemsRemoved ) );
}

} // namespace

namespace
{

FB2K_SERVICE_FACTORY( LibraryCallbackImpl );

} // namespace

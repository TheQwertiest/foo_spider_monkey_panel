#include <stdafx.h>

#include <tasks/dispatcher/event_dispatcher.h>
#include <tasks/events/library_item_event.h>

using namespace smp;

namespace
{

class LibraryCallbackImpl : public library_callback
{
public:
    void on_items_added( metadb_handle_list_cref p_data ) final;
    void on_items_modified( metadb_handle_list_cref p_data ) final;
    void on_items_removed( metadb_handle_list_cref p_data ) final;
};

} // namespace

namespace
{

void LibraryCallbackImpl::on_items_added( metadb_handle_list_cref p_data )
{
    EventDispatcher::Get().PutEventToAll(
        std::make_unique<LibraryItemEvent>( EventId::kNew_FbLibraryItemsAdded,
                                            smp::make_not_null_shared<const metadb_handle_list>( p_data ) ) );
}

void LibraryCallbackImpl::on_items_modified( metadb_handle_list_cref p_data )
{
    EventDispatcher::Get().PutEventToAll(
        std::make_unique<LibraryItemEvent>( EventId::kNew_FbLibraryItemsModified,
                                            smp::make_not_null_shared<const metadb_handle_list>( p_data ) ) );
}

void LibraryCallbackImpl::on_items_removed( metadb_handle_list_cref p_data )
{
    EventDispatcher::Get().PutEventToAll(
        std::make_unique<LibraryItemEvent>( EventId::kNew_FbLibraryItemsRemoved,
                                            smp::make_not_null_shared<const metadb_handle_list>( p_data ) ) );
}

} // namespace

namespace
{

FB2K_SERVICE_FACTORY( LibraryCallbackImpl );

} // namespace

#include <stdafx.h>

#include <tasks/dispatcher/event_dispatcher.h>
#include <tasks/events/panel_event.h>

using namespace smp;

namespace
{

class UiSelectionCallbackImpl
    : public ui_selection_callback
{
public:
    UiSelectionCallbackImpl( EventId eventId );

    void on_selection_changed( metadb_handle_list_cref p_selection ) final;

private:
    EventId eventId_;
};

class InitQuitImpl
    : public initquit
{
public:
    InitQuitImpl();

    void on_init() final;
    void on_quit() final;

private:
    UiSelectionCallbackImpl implWithAny_;
    UiSelectionCallbackImpl implWithPrefered_;
};

} // namespace

namespace
{

UiSelectionCallbackImpl::UiSelectionCallbackImpl( EventId eventId )
    : eventId_( eventId )
{
}

void UiSelectionCallbackImpl::on_selection_changed( metadb_handle_list_cref /*p_selection*/ )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( eventId_ ) );
}

InitQuitImpl::InitQuitImpl()
    : implWithAny_( EventId::kNew_FbAnySelectionChange )
    , implWithPrefered_( EventId::kNew_FbPreferedSelectionChange )
{
}

void InitQuitImpl::on_init()
{
    ui_selection_manager_v2::get()->register_callback( &implWithAny_, ui_selection_manager_v2::flag_no_now_playing );
    ui_selection_manager_v2::get()->register_callback( &implWithPrefered_, 0 );
}

void InitQuitImpl::on_quit()
{
    ui_selection_manager_v2::get()->unregister_callback( &implWithAny_ );
    ui_selection_manager_v2::get()->unregister_callback( &implWithPrefered_ );
}

} // namespace

namespace
{

FB2K_SERVICE_FACTORY( InitQuitImpl );

} // namespace

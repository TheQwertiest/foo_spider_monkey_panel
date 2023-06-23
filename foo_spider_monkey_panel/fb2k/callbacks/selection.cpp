#include <stdafx.h>

#include <tasks/dispatcher/event_dispatcher.h>
#include <tasks/events/panel_event.h>

using namespace smp;

namespace
{

class InitQuitImpl
    : public initquit
    , public ui_selection_callback
{
public:
    // initquit
    void on_init() final;
    void on_quit() final;

    // ui_selection_callback
    void on_selection_changed( metadb_handle_list_cref p_selection ) final;
};

} // namespace

namespace
{

void InitQuitImpl::on_init()
{
    ui_selection_manager_v2::get()->register_callback( this, 0 );
}

void InitQuitImpl::on_quit()
{
    ui_selection_manager_v2::get()->unregister_callback( this );
}

void InitQuitImpl::on_selection_changed( metadb_handle_list_cref /*p_selection*/ )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbSelectionChange ) );
}

} // namespace

namespace
{

FB2K_SERVICE_FACTORY( InitQuitImpl );

} // namespace

#include <stdafx.h>

#include <tasks/dispatcher/event_dispatcher.h>
#include <tasks/events/panel_event.h>

using namespace smp;

namespace
{

class ReplayGainCoreSettingsNotifyImpl : public replaygain_core_settings_notify
{
    void on_changed( const t_replaygain_config& cfg ) override;
};

class InitQuitImpl
    : public initquit
{
public:
    void on_init() override;
    void on_quit() override;

private:
    ReplayGainCoreSettingsNotifyImpl impl_;
};

} // namespace

namespace
{

void ReplayGainCoreSettingsNotifyImpl::on_changed( const t_replaygain_config& /*cfg*/ )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbReplayGainCfgChanged ) );
}

void InitQuitImpl::on_init()
{
    replaygain_manager_v2::get()->add_notify( &impl_ );
}

void InitQuitImpl::on_quit()
{
    replaygain_manager_v2::get()->remove_notify( &impl_ );
}

} // namespace

namespace
{

FB2K_SERVICE_FACTORY( InitQuitImpl );

} // namespace

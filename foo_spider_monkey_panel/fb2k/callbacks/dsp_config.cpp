#include <stdafx.h>

#include <tasks/dispatcher/event_dispatcher.h>
#include <tasks/events/library_item_event.h>

using namespace smp;

namespace
{

class DspConfigImpl : public dsp_config_callback
{
public:
    void on_core_settings_change( const dsp_chain_config& p_newdata ) final;
};

} // namespace

namespace
{

void DspConfigImpl::on_core_settings_change( const dsp_chain_config& /*p_newdata*/ )
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbDspCoreSettingsChange ) );
}

} // namespace

namespace
{

FB2K_SERVICE_FACTORY( DspConfigImpl );

} // namespace

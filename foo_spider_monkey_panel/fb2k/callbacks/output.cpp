#include <stdafx.h>

#include <tasks/dispatcher/event_dispatcher.h>
#include <tasks/events/panel_event.h>

using namespace smp;

namespace
{

class OutputConfigChangeCallbackImpl
    : public output_config_change_callback
{
public:
    OutputConfigChangeCallbackImpl() = default;

    void outputConfigChanged() final;
};

class InitQuitImpl
    : public initquit
{
public:
    InitQuitImpl() = default;

    void on_init() final;
    void on_quit() final;

private:
    OutputConfigChangeCallbackImpl impl_;
};

} // namespace

namespace
{

void OutputConfigChangeCallbackImpl::outputConfigChanged()
{
    EventDispatcher::Get().PutEventToAll( std::make_unique<PanelEvent>( EventId::kNew_FbOutputConfigChange ) );
}

void InitQuitImpl::on_init()
{
    output_manager_v2::get()->addCallback( &impl_ );
}

void InitQuitImpl::on_quit()
{
    output_manager_v2::get()->removeCallback( &impl_ );
}

} // namespace

namespace
{

FB2K_SERVICE_FACTORY( InitQuitImpl );

} // namespace

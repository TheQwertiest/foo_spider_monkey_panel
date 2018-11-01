#pragma once

#include <user_message.h>

#include <unordered_map>
#include <vector>
#include <memory>

namespace smp
{

namespace panel
{
class CallBackDataBase;
}

class MessageDataHolder
{
public:
    ~MessageDataHolder() = default;
    MessageDataHolder( const MessageDataHolder& ) = delete;
    MessageDataHolder& operator=( const MessageDataHolder& ) = delete;

    static MessageDataHolder& GetInstance();

    void StoreData( CallbackMessage messageId, const std::vector<HWND>& recievers, std::shared_ptr<panel::CallBackDataBase> pData );
    std::shared_ptr<panel::CallBackDataBase> ClaimData( CallbackMessage messageId, HWND hWnd );

    void FlushDataForHwnd( HWND hWnd, const panel::CallBackDataBase* pDataToRemove );
    void FlushAllDataForHwnd( HWND hWnd );

private:
    MessageDataHolder() = default;

    void FlushDataInternal( HWND hWnd, const panel::CallBackDataBase* pDataToRemove );

private:
    std::unordered_multimap<CallbackMessage,
                            std::pair<
                                std::shared_ptr<panel::CallBackDataBase>,
                                std::vector<HWND>>> dataStorage_;
};

} // namespace smp

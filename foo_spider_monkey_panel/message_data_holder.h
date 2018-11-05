#pragma once

#include <user_message.h>

#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

namespace smp
{

namespace panel
{
class CallbackData;
}

class MessageDataHolder
{
public:
    ~MessageDataHolder() = default;
    MessageDataHolder( const MessageDataHolder& ) = delete;
    MessageDataHolder& operator=( const MessageDataHolder& ) = delete;

    static MessageDataHolder& GetInstance();

    void StoreData( CallbackMessage messageId, const std::vector<HWND>& recievers, std::shared_ptr<panel::CallbackData> pData );
    std::shared_ptr<panel::CallbackData> ClaimData( CallbackMessage messageId, HWND hWnd );

    void FlushDataForHwnd( HWND hWnd, const panel::CallbackData* pDataToRemove );
    void FlushAllDataForHwnd( HWND hWnd );

private:
    MessageDataHolder() = default;

    void FlushDataInternal( HWND hWnd, const panel::CallbackData* pDataToRemove );

private:
    using MessageData = std::pair<std::vector<HWND>, std::shared_ptr<panel::CallbackData>>;
    using MessageDataList = std::vector<MessageData>;

    std::mutex dataLock_;
    std::unordered_map<CallbackMessage, MessageDataList> dataStorage_;
};

} // namespace smp

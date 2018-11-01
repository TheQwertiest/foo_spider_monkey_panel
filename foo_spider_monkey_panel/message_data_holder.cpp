#include <stdafx.h>
#include "message_data_holder.h"

#include <panel_manager.h>

namespace smp
{

MessageDataHolder& MessageDataHolder::GetInstance()
{
    static MessageDataHolder storage;
    return storage;
}

void MessageDataHolder::StoreData( CallbackMessage messageId, const std::vector<HWND>& recievers, std::shared_ptr<panel::CallBackDataBase> pData )
{
    assert( pData );
    dataStorage_.emplace( messageId, std::make_pair( pData, recievers ) );
}

std::shared_ptr<panel::CallBackDataBase> MessageDataHolder::ClaimData( CallbackMessage messageId, HWND hWnd )
{
    auto dataRange = dataStorage_.equal_range( messageId );
    for ( auto dataElemIt = dataRange.first; dataElemIt != dataRange.second; ++dataElemIt )
    {
        auto& [data, recievers] = dataElemIt->second;
        const auto it = std::find( recievers.begin(), recievers.end(), hWnd );
        if ( recievers.end() != it )
        {
            auto dataCopy = data; ///< need copy in case current element is destroyed

            recievers.erase( it );
            if ( !recievers.size() )
            {
                dataElemIt = dataStorage_.erase( dataElemIt );
            }
            else
            {
                ++dataElemIt;
            }

            return dataCopy;
        }
    }

    assert( 0 );
    uBugCheck();
}

void MessageDataHolder::FlushDataForHwnd( HWND hWnd, const panel::CallBackDataBase* pDataToRemove )
{
    FlushDataInternal( hWnd, pDataToRemove );
}

void MessageDataHolder::FlushAllDataForHwnd( HWND hWnd )
{
    FlushDataInternal( hWnd, nullptr );
}

void MessageDataHolder::FlushDataInternal( HWND hWnd, const panel::CallBackDataBase* pDataToRemove )
{
    const bool shouldRemoveAll = !pDataToRemove;
    for ( auto dataElemIt = dataStorage_.begin(); dataElemIt != dataStorage_.end(); )
    {
        auto& [pData, recievers] = dataElemIt->second;
        if ( !shouldRemoveAll && pData.get() != pDataToRemove )
        {
            continue;
        }

        const auto it = std::find( recievers.cbegin(), recievers.cend(), hWnd );
        if ( recievers.cend() != it )
        {
            recievers.erase( it );
        }

        if ( !recievers.size() )
        {
            dataElemIt = dataStorage_.erase( dataElemIt );
        }
        else
        {
            ++dataElemIt;
        }
    }
}

} // namespace smp

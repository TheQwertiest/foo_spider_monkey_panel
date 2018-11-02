#include <stdafx.h>
#include "message_data_holder.h"

#include <callback_data.h>

namespace smp
{

MessageDataHolder& MessageDataHolder::GetInstance()
{
    static MessageDataHolder storage;
    return storage;
}

void MessageDataHolder::StoreData( CallbackMessage messageId, const std::vector<HWND>& recievers, std::shared_ptr<panel::CallBackDataBase> pData )
{
    std::scoped_lock sl( dataLock_ );

    assert( pData );
    
    if ( dataStorage_.count( messageId ) )
    {
        dataStorage_[messageId].emplace_back( std::make_pair( recievers, pData ) );
    }
    else
    {
        dataStorage_.emplace( messageId, std::vector{ std::make_pair( recievers, pData ) } );
    }
}

std::shared_ptr<panel::CallBackDataBase> MessageDataHolder::ClaimData( CallbackMessage messageId, HWND hWnd )
{
    std::scoped_lock sl( dataLock_ );

    assert( dataStorage_.count( messageId ) );
    
    auto& msgDataList = dataStorage_[messageId];
    for ( auto msgDataIt = msgDataList.begin(); msgDataIt != msgDataList.end(); ++msgDataIt )
    {
        auto& [recievers, pData] = *msgDataIt;
        if ( const auto recieverIt = std::find( recievers.begin(), recievers.end(), hWnd );
             recievers.end() != recieverIt )
        {
            auto pDataCopy = pData; ///< need copy in case current element is destroyed

            recievers.erase( recieverIt );
            if ( !recievers.size() )
            {
                msgDataList.erase( msgDataIt );
            }
            if ( !msgDataList.size() )
            {
                dataStorage_.erase( messageId );
            }

            return pDataCopy;
        }
    }

    assert( 0 );
    uBugCheck();
}

void MessageDataHolder::FlushDataForHwnd( HWND hWnd, const panel::CallBackDataBase* pDataToRemove )
{
    std::scoped_lock sl( dataLock_ );

    FlushDataInternal( hWnd, pDataToRemove );
}

void MessageDataHolder::FlushAllDataForHwnd( HWND hWnd )
{
    std::scoped_lock sl( dataLock_ );

    FlushDataInternal( hWnd, nullptr );
}

void MessageDataHolder::FlushDataInternal( HWND hWnd, const panel::CallBackDataBase* pDataToRemove )
{
    const bool shouldRemoveAll = !pDataToRemove;
    for ( auto it = dataStorage_.begin(); it != dataStorage_.end(); )
    {
        auto& msgDataList = it->second;
        for ( auto msgDataIt = msgDataList.begin(); msgDataIt != msgDataList.end(); )
        {
            auto& [recievers, pData] = *msgDataIt;
            if ( !shouldRemoveAll && pData.get() != pDataToRemove )
            {
                continue;
            }

            const auto recieverIt = std::find( recievers.cbegin(), recievers.cend(), hWnd );
            if ( recievers.cend() != recieverIt )
            {
                recievers.erase( recieverIt );
            }
            if ( !recievers.size() )
            {
                msgDataIt = msgDataList.erase( msgDataIt );
            }
            else
            {
                ++msgDataIt;
            }
        }
        if ( !msgDataList.size() )
        {
            it = dataStorage_.erase( it );
        }
        else
        {
            ++it;
        }
    }
}

} // namespace smp

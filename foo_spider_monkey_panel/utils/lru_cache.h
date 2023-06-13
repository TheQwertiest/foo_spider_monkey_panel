#pragma once

#include <list>
#include <unordered_map>

namespace smp
{

template <typename KeyT, typename ValueT>
class LruCache
{
    struct ListElem
    {
        KeyT key;
        ValueT value;
    };

    using ListIteratorT = std::list<ListElem>::iterator;

public:
    LruCache( size_t capacity )
        : capacity_( capacity )
    {
    }

    template <typename KeyU, typename ValueU>
    void Put( KeyU&& key, ValueU& value )
    {
        auto it = keyToItemIt_.find( key );
        if ( it == keyToItemIt_.end() )
        {
            if ( items_.size() == capacity_ )
            {
                keyToItemIt_.erase( items_.back().key );
                items_.pop_back();
            }

            const auto& item = items_.emplace_front( std::forward<KeyU>( key ), std::forward<ValueU>( value ) );
            keyToItemIt_.try_emplace( item.key, items_.begin() );
        }
        else
        {
            items_.splice( items_.begin(), items_, it->second );
        }
    }

    const ValueT& Get( const KeyT& key )
    {
        auto it = keyToItemIt_.at( key );
        items_.splice( items_.begin(), items_, it );
        return it->value;
    }

    void Clear()
    {
        keyToItemIt_.clear();
        items_.clear();
    }

    bool Contains( const KeyT& key ) const
    {
        return keyToItemIt_.contains( key );
    }

    size_t Size() const
    {
        return keyToItemIt_.size();
    }

    size_t Capacity() const
    {
        return keyToItemIt_.size();
    }

    void SetCapacity( size_t capacity )
    {
        while ( items_.size() > capacity )
        {
            keyToItemIt_.erase( items_.back().key );
            items_.pop_back();
        }
        capacity_ = capacity;
    }

private:

private:
    std::list<ListElem> items_;
    std::unordered_map<KeyT, ListIteratorT> keyToItemIt_;
    size_t capacity_;
};

} // namespace smp

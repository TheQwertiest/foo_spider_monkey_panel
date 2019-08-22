#pragma once

#include <memory>

namespace smp::pfc_x
{

/// @brief STL wrapper for objects derived from pfc::list_base_const_t
template <typename T>
class Stl
{
public:
    using pfc_container_type = typename std::decay_t<T>;
    using is_pfc_list_const = typename std::conditional_t<
        std::is_same_v<pfc_container_type, pfc::list_base_const_t<typename pfc_container_type::t_item>>,
        std::true_type,
        std::false_type>;

    using value_type = typename pfc_container_type::t_item;
    using difference_type = ptrdiff_t;
    using size_type = size_t;
    using reference = value_type&;
    // because list_base_const_t does not return a reference...
    using const_reference = typename std::conditional_t<
        is_pfc_list_const::value,
        value_type,
        const value_type&>;

    static_assert( std::is_base_of_v<pfc::list_base_const_t<value_type>, pfc_container_type>, "Must be derived from pfc::list_base_const_t" );

    class const_iterator
    {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = Stl::value_type;
        using difference_type = Stl::difference_type;
        using pointer = const value_type*;
        using reference = Stl::const_reference;

        const_iterator() = default;
        const_iterator( difference_type idx, const pfc_container_type* pPfc )
            : pPfc_( pPfc )
            , curIdx_( idx )
        {
        }

        const_iterator( const const_iterator& other )
        {
            pPfc_ = other.pPfc_;
            curIdx_ = other.curIdx_;
        }
        ~const_iterator() = default;

        const_iterator& operator=( const const_iterator& other )
        {
            if ( this != &other )
            {
                pPfc_ = other.pPfc_;
                curIdx_ = other.curIdx_;
            }

            return *this;
        }

        bool operator==( const const_iterator& other ) const
        {
            return ( pPfc_ == other.pPfc_
                     && curIdx_ == other.curIdx_ );
        }
        bool operator!=( const const_iterator& other ) const
        {
            return ( !( *this == other ) );
        }
        bool operator<( const const_iterator& other ) const
        {
            return ( curIdx_ == other.curIdx_ );
        }
        bool operator>( const const_iterator& other ) const
        {
            return ( other < *this );
        }
        bool operator<=( const const_iterator& other ) const
        {
            return ( !( other < *this ) );
        }
        bool operator>=( const const_iterator& other ) const
        {
            return ( !( *this < other ) );
        }

        const_iterator& operator++()
        {
            ++curIdx_;
            return ( *this );
        }
        const_iterator operator++( int )
        {
            const_iterator tmp = *this;
            ++*this;
            return ( tmp );
        }
        const_iterator& operator--()
        {
            --curIdx_;
            return ( *this );
        }
        const_iterator operator--( int )
        {
            const_iterator tmp = *this;
            --*this;
            return ( tmp );
        }
        const_iterator& operator+=( difference_type offset )
        {
            curIdx_ += offset;
            return ( *this );
        }
        const_iterator operator+( difference_type offset ) const
        {
            const_iterator tmp = *this;
            return ( tmp += offset );
        }
        const_iterator& operator-=( difference_type offset )
        {
            return ( *this += -offset );
        }
        const_iterator operator-( difference_type offset ) const
        {
            const_iterator tmp = *this;
            return ( tmp -= offset );
        }
        difference_type operator-( const_iterator other ) const
        {
            return ( curIdx_ - other.curIdx_ );
        }

        reference operator*() const
        {
            return ( *pPfc_ )[curIdx_];
        }
        template <typename = typename std::enable_if_t<!is_pfc_list_const::value>>
        pointer operator->() const
        {
            return &( **this );
        }
        reference operator[]( difference_type offset ) const
        {
            return ( *( *this + offset ) );
        }

    protected:
        const pfc_container_type* pPfc_ = nullptr;
        difference_type curIdx_ = 0;
    };

    class iterator
        : public const_iterator
    {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = Stl::value_type;
        using difference_type = Stl::difference_type;
        using pointer = value_type*;
        using reference = Stl::reference;

        iterator() = default;
        iterator( difference_type idx, pfc_container_type* pPfc )
            : const_iterator( idx, pPfc )
        {
        }

        iterator( const iterator& other )
            : const_iterator( other )
        {
        }
        ~iterator() = default;

        iterator& operator=( const iterator& other )
        {
            if ( this != &other )
            {
                this->pPfc_ = other.pPfc_;
                this->curIdx_ = other.curIdx_;
            }

            return *this;
        }
        iterator& operator++()
        {
            ++( this->curIdx_ );
            return *this;
        }
        iterator operator++( int )
        {
            iterator tmp = *this;
            ++*this;
            return ( tmp );
        }
        iterator& operator--()
        {
            --( this->curIdx_ );
            return *this;
        }
        iterator operator--( int )
        {
            iterator tmp = *this;
            --*this;
            return ( tmp );
        }
        iterator& operator+=( difference_type offset )
        {
            *static_cast<const_iterator*>( this ) += offset;
            return ( *this );
        }
        iterator operator+( difference_type offset ) const
        {
            iterator tmp = *this;
            return ( tmp += offset );
        }
        iterator& operator-=( difference_type offset )
        {
            return ( *this += -offset );
        }
        iterator operator-( difference_type offset ) const
        {
            iterator tmp = *this;
            return ( tmp -= offset );
        }
        difference_type operator-( iterator other ) const
        {
            return ( *static_cast<const const_iterator*>( this ) - other );
        }

        reference operator*() const
        {
            return ( const_cast<reference>( const_iterator::operator*() ) );
        }
        template <typename = typename std::enable_if_t<!is_pfc_list_const::value>>
        pointer operator->() const
        {
            return ( const_cast<pointer>( const_iterator::operator->() ) );
        }
        reference operator[]( difference_type offset ) const
        {
            return ( *( *this + offset ) );
        }
    };

    // typedef std::reverse_iterator<iterator> reverse_iterator;             //optional
    // typedef std::reverse_iterator<const_iterator> const_reverse_iterator; //optional

    template <typename = typename std::enable_if_t<std::is_reference_v<T>>>
    Stl( T& base )
        : pfc_( base ){};

    template <typename... Args, typename = typename std::enable_if_t<!std::is_reference_v<T>>>
    Stl( Args&&... args )
        : pfc_( std::forward<Args>( args )... ){};

    Stl( const Stl& ) = delete;
    ~Stl() = default;

    Stl& operator=( const Stl& other ) = delete;

    // bool operator==( const Stl& ) const;
    // bool operator!=( const Stl& ) const;
    // bool operator<( const Stl& ) const;  //optional
    // bool operator>( const Stl& ) const;  //optional
    // bool operator<=( const Stl& ) const; //optional
    // bool operator>=( const Stl& ) const; //optional

    template <typename = typename std::enable_if_t<!std::is_const_v<std::remove_reference_t<T>>>>
    iterator begin()
    {
        return iterator( 0, &pfc_ );
    }
    const_iterator begin() const
    {
        return const_iterator( 0, &pfc_ );
    }

    const_iterator cbegin() const
    {
        return begin();
    }

    template <typename = typename std::enable_if_t<!std::is_const_v<std ::remove_reference_t<T>>>>
    iterator end()
    {
        return iterator( size(), &pfc_ );
    }
    const_iterator end() const
    {
        return const_iterator( size(), &pfc_ );
    }
    const_iterator cend() const
    {
        return end();
    }

    /*
    reverse_iterator rbegin();              //optional
    const_reverse_iterator rbegin() const;  //optional
    const_reverse_iterator crbegin() const; //optional
    reverse_iterator rend();                //optional
    const_reverse_iterator rend() const;    //optional
    const_reverse_iterator crend() const;   //optional
    */

    reference front()
    {
        return this->operator[]( 0 );
    }
    const_reference front() const
    {
        return this->operator[]( 0 );
    }
    reference back()
    {
        return this->operator[]( this->size() - 1 );
    }
    const_reference back() const
    {
        return this->operator[]( this->size() - 1 );
    }

    /*
    template <class... Args>
    void emplace_front( Args&&... ); //optional
    template <class... Args>
    void emplace_back( Args&&... );                //optional
    void push_front( const T& );                   //optional
    void push_front( T&& );                        //optional
    */
    void push_back( const value_type& item )
    {
        pfc_.add_item( item );
    }

    /*
    void push_back( T&& );                         //optional
    void pop_front();                              //optional
    void pop_back();                               //optional
    */

    reference operator[]( size_type idx )
    {
        return pfc_[idx];
    }
    const_reference operator[]( size_type idx ) const
    {
        return pfc_[idx];
    }
    reference at( size_type idx )
    {
        if ( idx >= this->size() )
        {
            throw std::out_of_range;
        }
        return this->operator[]( idx );
    }
    const_reference at( size_type idx ) const
    {
        if ( idx >= this->size() )
        {
            throw std::out_of_range;
        }
        return this->operator[]( idx );
    }

    /*
    template <class... Args>
    iterator emplace( const_iterator, Args&&... );    //optional
    iterator insert( const_iterator, const T& );      //optional
    iterator insert( const_iterator, T&& );           //optional
    iterator insert( const_iterator, size_type, T& ); //optional
    template <class iter>
    iterator insert( const_iterator, iter, iter );               //optional
    iterator insert( const_iterator, std::initializer_list<T> ); //optional
    iterator erase( const_iterator );                            //optional
    iterator erase( const_iterator, const_iterator );            //optional
    */

    void clear()
    {
        pfc_.remove_all();
    }

    /*
    template <class iter>
    void assign( iter, iter );               //optional
    void assign( std::initializer_list<T> ); //optional
    void assign( size_type, const T& );      //optional

    void swap( Stl& );
    */

    size_type size() const
    {
        return pfc_.get_size();
    }
    size_type max_size() const
    {
        return std::numeric_limits<uint32_t>::max();
    }
    bool empty() const
    {
        return !this->size();
    }

public:
    template <typename = typename std::enable_if_t<!std::is_const_v<std::remove_reference_t<T>>>>
    pfc_container_type& Pfc()
    {
        return pfc_;
    }

    const pfc_container_type& Pfc() const
    {
        return pfc_;
    }

private:
    T pfc_;
};

template <typename T>
using Stl_Ref = typename smp::pfc_x::Stl<T&>;

template <typename T>
using Stl_CRef = typename smp::pfc_x::Stl<const T&>;

template <typename T>
Stl_Ref<T> Make_Stl_Ref( T& base )
{
    return Stl_Ref<T>( base );
}

template <typename T>
Stl_CRef<T> Make_Stl_CRef( const T& base )
{
    return Stl_CRef<T>( base );
}

} // namespace smp::pfc_x

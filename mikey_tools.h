#pragma once
#include <iostream>
#include <string>
#include <exception>
#include <stdexcept>
#include <memory>
#include <cstddef>

namespace mhy
{
template <typename T>
class RangeT
{
  public:
    typedef T * PtrT;
    typedef T   value_type;

  public:
    PtrT first = 0;
    PtrT last  = 0;

  public:
    RangeT()                    = default;
    RangeT( const RangeT<T> & ) = default;
    // RangeT( RangeT<T> && )      = default;

    template <typename U>
    RangeT( const RangeT<U> & u ) : first( (PtrT)u.first ), last( (PtrT)u.last )
    {
    }

    template <typename U>
    RangeT<T> & operator=(const RangeT<U> & u)
    {
        first = (PtrT)u.first;
        last = (PtrT)u.last;
        return *this;
    }

    //-------
    RangeT( PtrT a, PtrT b ) : first( a ), last( b ) {}

    RangeT( PtrT a, size_t count ) : first( a ), last( a + count ) {}

  public:
    bool empty() const
    {
        return !size();
    }

    bool operator!() const
    {
        return empty();
    }

    T & operator*()
    {
        return *begin();
    }

    PtrT operator[]( size_t idx )
    {
        return begin() + idx;  // !! not range checked.
    }

    constexpr PtrT begin() const
    {
        return first;
    }

    constexpr PtrT end() const
    {
        return last;
    }

    constexpr size_t size() const
    {
        return last - first;
    }

    constexpr PtrT data() const
    {
        return first;
    }
};

template <typename T>
constexpr auto range( T * first, size_t count )
{
    return RangeT<T>( first, count );
}

template <typename T>
constexpr auto range( T * first, T * last )
{
    return RangeT<T>( first, last );
}

//========================
template <class T>
class ListT
{
  public:
    typedef T   value_type;
    typedef T * PtrT;

  private:
    RangeT<T> buf;
    PtrT      here = 0;

  public:
    ListT() = default;

    ListT( RangeT<T> buf ) : buf( buf ), here( buf.begin() ) {}

    ListT( PtrT first, size_t count ) : buf{ first, count }, here( buf.begin() ) {}

    ~ListT() {}

    //-- Load from pre-initialized memory, such as from a data file.
    // This differs from normal assigment by setting our `here`
    // to the end() of the buf, indicating it is full, rather than the beginning,
    // an empty() list.
    template <class U>
    ListT<T> & load_from( RangeT<U> u )
    {
        buf  = u;
        here = buf.end();

        return *this;
    }

    template <class U>
    ListT<T> & operator=( RangeT<U> u )
    {
        buf  = u;
        here = buf.begin();

        return *this;
    }

  public:
    bool operator!() const
    {
        return !remain();
    }

    bool empty() const
    {
        return here == begin();
    }

    constexpr PtrT begin() const
    {
        return buf.begin();
    }

    PtrT end() const
    {
        return here;
    }

    size_t size() const
    {
        return here - begin();
    }

    size_t remain() const
    {
        return end() - here;
    }

    constexpr PtrT data() const
    {
        return buf.data();
    }
    //-----
    template <typename... Vars>
    void push_back( Vars... args )
    {
        if ( here == buf.end() )
        {
            throw std::logic_error( "List<T> insert past end of buffer." );
        }
        *here++ = T{ &args... };
    }

    void push_back( T && val )
    {
        if ( here == buf.end() )
        {
            throw std::logic_error( "List<T> insert past end of buffer." );
        }
        *here++ = val;
    }

    void push_back( const T & val )
    {
        if ( here == buf.end() )
        {
            throw std::logic_error( "List<T> insert past end of buffer." );
        }
        *here++ = val;
    }

    T & front()
    {
        return *begin();
    }

    const T & front() const
    {
        return *begin();
    }

    T & back()
    {
        //-- NO range checking.
        // if (empty())
        // { throw std::logic_error("List<T> read from empty list");
        // }
        return *( here - 1 );
    }

    const T & back() const
    {
        return *( here - 1 );
    }

    T & operator[]( size_t idx )
    {
        return *( begin() + idx );
    }

    const T & operator[]( size_t idx ) const
    {
        return *( begin() + idx );
    }
};

}  // namespace mhy

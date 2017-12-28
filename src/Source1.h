#pragma once

#include <deque>
#include <mutex>
#include <condition_variable>

namespace concurrency
{

template<class T, class Container = std::deque<T> >
class queue
{
    explicit queue( const Container& );
    explicit queue( const Container&& = Container( ) );

    template<class Alloc>
    explicit queue( const Alloc& );

    template<class Alloc>
    explicit queue( const Container&, const Alloc& );

    template<class Alloc>
    explicit queue( Container&&, const Alloc& );

    template<class Alloc>
    explicit queue( queue&&, const Alloc& );

    void swap( queue& q );

    bool empty( ) const;
    size_t size( ) const;

    T& front( );
    const T& front( );

    T& back( );
    const T& back( );

    void push( const T& );
    void push( T&& );

    void pop( );
    template<class... Args> void emplace( Args&&... args );
};


template<class T >
class threadsafe_queue
{
public:
    threadsafe_queue( );
    threadsafe_queue( const threadsafe_queue& );
    threadsafe_queue& operator=( const threadsafe_queue& ) = delete;

    void push( T value )
    {
        std::lock_guard<std::mutex> lk( mut );
        data_queue.push( value );
        cond.notify_one( );
    }

    bool try_pop( T& out);
    std::shared_ptr<T> try_pop( );

    void wait_and_pop( T& out )
    {
        std::unique_lock<std::mutex> ulk( mut );
        cond.wait( ulk, [ this ] () -> bool {
            return !data_queue.empty( );
        } );
        out = data_queue.front( );

        data_queue.pop( );
    }

    std::shared_ptr<T> wait_and_pop( );

    bool empty( ) const;
private:
    std::mutex mut;
    std::queue<T> data_queue;
    std::condition_variable cond;
};

};
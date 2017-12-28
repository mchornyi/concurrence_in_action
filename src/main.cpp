// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "Samples.h"
#include "stdafx.h"

#include <thread>
#include <iostream>
#include <vector>

using namespace std;

template < typename Iterator, typename T >
struct accumulate_block
{
    void
    operator( )( Iterator first, Iterator last, T& result )
    {
        result = std::accumulate( first, last, result );
    }
};

template < typename Iterator, typename T >
T
parallel_accumulate( Iterator first, Iterator last, T init )
{
    unsigned long const length = std::distance( first, last );
    if ( !length )
        return init;

    unsigned long const min_per_thread = 25;
    unsigned long const max_threads = ( length + min_per_thread - 1 ) / min_per_thread;

    unsigned long const hardware_threads = std::thread::hardware_concurrency( );
    unsigned long const num_threads
        = std::min( hardware_threads != 0 ? hardware_threads : 2, max_threads );

    unsigned long const block_size = length / num_threads;
    std::vector< T > results( num_threads );
    std::vector< std::thread > threads( num_threads - 1 );

    Iterator block_start = first;
    for ( unsigned long i = 0; i < ( num_threads - 1 ); ++i )
    {
        Iterator block_end = block_start;
        std::advance( block_end, block_size );
        threads[ i ] = std::thread( accumulate_block< Iterator, T >( ), block_start, block_end,
                                    std::ref( results[ i ] ) );
        block_start = block_end;
    }
    accumulate_block< Iterator, T >( )( block_start, last, results[ num_threads - 1 ] );

    std::for_each( threads.begin( ), threads.end( ), std::mem_fn( &std::thread::join ) );
    return std::accumulate( results.begin( ), results.end( ), init );
}

//////////////////////////////////////////////////////////////////////////

struct empty_stack : std::exception
{
    const char* what( ) const throw( );
};

template < typename T >
class threadsafe_stack
{
private:
    std::stack< T > data;
    mutable std::mutex m;

public:
    threadsafe_stack( )
    {
    }
    threadsafe_stack( const threadsafe_stack& other )
    {
        std::lock_guard< std::mutex > lock( other.m );
        data = other.data;
    }
    threadsafe_stack& operator=( const threadsafe_stack& ) = delete;
    void
    push( T new_value )
    {
        std::lock_guard< std::mutex > lock( m );
        data.push( new_value );
    }
    std::shared_ptr< T >
    pop( )
    {
        std::lock_guard< std::mutex > lock( m );
        if ( data.empty( ) )
            throw empty_stack( );
        std::shared_ptr< T > const res( std::make_shared< T >( data.top( ) ) );
        data.pop( );
        return res;
    }
    void
    pop( T& value )
    {
        std::lock_guard< std::mutex > lock( m );
        if ( data.empty( ) )
            throw empty_stack( );
        value = data.top( );
        data.pop( );
    }
    bool
    empty( ) const
    {
        std::lock_guard< std::mutex > lock( m );
        return data.empty( );
    }
};

//////////////////////////////////////////////////////////////////////////

class hierarchical_mutex
{
    std::mutex internal_mutex;
    unsigned long const hierarchy_value;
    unsigned long previous_hierarchy_value;
    static thread_local unsigned long this_thread_hierarchy_value;  // 1

    void
    check_for_hierarchy_violation( )
    {
        if ( this_thread_hierarchy_value <= hierarchy_value )  // 2
        {
            throw std::logic_error( "mutex hierarchy violated" );
        }
    }
    void
    update_hierarchy_value( )
    {
        previous_hierarchy_value = this_thread_hierarchy_value;  // 3
        this_thread_hierarchy_value = hierarchy_value;
    }

public:
    explicit hierarchical_mutex( unsigned long value )
        : hierarchy_value( value ), previous_hierarchy_value( 0 )
    {
    }
    void
    lock( )
    {
        check_for_hierarchy_violation( );
        internal_mutex.lock( );     // 4
        update_hierarchy_value( );  // 5
    }
    void
    unlock( )
    {
        this_thread_hierarchy_value = previous_hierarchy_value;  // 6
        internal_mutex.unlock( );
    }
    bool
    try_lock( )
    {
        check_for_hierarchy_violation( );
        if ( !internal_mutex.try_lock( ) )  // 7
            return false;
        update_hierarchy_value( );
        return true;
    }
};

thread_local unsigned long hierarchical_mutex::this_thread_hierarchy_value( ULONG_MAX );  // 8

//////////////////////////////////////////////////////////////////////////
// Tail recursion
int
factorial_ext( int n, int accumulate )
{
    if ( n == 0 )
    {
        return accumulate;
    }

    return factorial_ext( n - 1, accumulate * n );
}
//////////////////////////////////////////////////////////////////////////
struct some_resource
{
    void
    do_something( )
    {
    }
};

std::shared_ptr< some_resource > resource_ptr;

std::once_flag resource_flag;

void
init_resource( )
{
    resource_ptr.reset( new some_resource );
}

void
foo( )
{
    std::call_once( resource_flag, init_resource );
    resource_ptr->do_something( );
}

//////////////////////////////////////////////////////////////////////////

template < class Fn, class... Args >
void
call( Fn&& f, Args... args )
{
    f( args... );
}

template < class T, int size >
int get_arr_size( T ( & )[ size ] )
{
    return size;
}

class A
{
public:
    A( )
    {
        x = 0;
        cout << "A ctr. ref=" << this << std::endl;
    }

    A( const A& rV)
    {
        x = rV.x;
        cout << "A copy ctr. ref=" << this << std::endl;
    }
    ~A( )
    {
        cout << "A dest. ref="<< this<< ", id =" << x<< std::endl;
    }
public:
    int x;
};


A f( bool f)
{
    if ( f )
    {
        A a;
        a.x = 1;

        return a;
    }
    else
    {
        A a;
        a.x = 11;

        return a;
    }

    A a;
    a.x = 111;

    return a;
}

template<class T, class ...Ts>
T add( T t, Ts ...args )
{
    t + add( args );
}

template<class T>
T add( T t)
{
    return t;
}


unsigned const increment_count = 2000000;
unsigned const thread_count = 2;

//unsigned i = 0;
std::atomic<unsigned> i = 0;

void func( )
{
    for ( unsigned c = 0; c<increment_count; ++c )
    {
        ++i;
    }
}

int
main( )
{
    std::vector<std::thread> threads;
    for ( unsigned c = 0; c<thread_count; ++c )
    {
        threads.push_back( std::thread( func ) );
    }
    for ( unsigned c = 0; c<threads.size( ); ++c )
    {
        threads[ c ].join( );
    }

    std::cout << thread_count << " threads, Final i=" << i
        << ", increments=" << ( thread_count*increment_count ) << std::endl;
    return 0;
    {
        cout << "---start block" << endl;


            time_t lt, gt;

            const time_t t = ::time( NULL );

            tm l_utc_time, g_utc_time;
#if defined( _WIN32 )
            localtime_s( &l_utc_time, &t );
            gmtime_s( &g_utc_time, &t );
#else
            localtime_r( &t, &l_utc_time );
            gmtime_r( &t, &g_utc_time );
#endif  // G_PLATFORM_WIN

            lt = mktime( &l_utc_time );
            gt = mktime( &g_utc_time );

            const int time_diff_sec = static_cast< int >( difftime( gt, lt ) );



        cout << "----end block" << endl;
    }

    // run_sample_1( );
    run_sample_2( );

    cout << "Press Enter to exit...";

    getchar( );

    return 0;
}

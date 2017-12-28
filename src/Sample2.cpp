#pragma once
#include "stdafx.h"

#define _ENABLE_ATOMIC_ALIGNMENT_FIX

//////////////////////////////////////////////////////////////////////////
// Description: Parallel Quicksort using futures. FP-style programming
//////////////////////////////////////////////////////////////////////////
using namespace std;
namespace
{
};

template < typename T >
list< T >
sequential_quick_sort( std::list< T > input )
{
    if ( input.empty( ) )
    {
        return input;
    }
    std::list< T > result;

    result.splice( result.begin( ), input, input.begin( ) );

    T const& pivot = *result.begin( );

    auto divide_point
        = std::partition( input.begin( ), input.end( ), [&]( T const& t ) { return t < pivot; } );

    std::list< T > lower_part;

    lower_part.splice( lower_part.end( ), input, input.begin( ), divide_point );

    auto new_lower( sequential_quick_sort( std::move( lower_part ) ) );

    auto new_higher( sequential_quick_sort( std::move( input ) ) );

    result.splice( result.end( ), new_higher );

    result.splice( result.begin( ), new_lower );

    return result;
}

template < typename T >
list< T >
parallel_quick_sort( list< T > input )
{
    if ( input.empty( ) )
    {
        return input;
    }
    list< T > result;

    result.splice( result.begin( ), input, input.begin( ) );

    T const& pivot = *result.begin( );

    auto divide_point
        = std::partition( input.begin( ), input.end( ), [ & ]( T const& t )
    {
        return t < pivot;
    } );

    std::list< T > lower_part;

    lower_part.splice( lower_part.end( ), input, input.begin( ), divide_point );

    auto new_lower( std::async( &parallel_quick_sort< T >, std::move( lower_part ) ) );

    auto new_higher( parallel_quick_sort( std::move( input ) ) );

    result.splice( result.end( ), new_higher );

    result.splice( result.begin( ), new_lower.get() );

    return result;
}

//Doesn't compile
//template<typename F, typename... Args>
//future< result_of<F( Args... )>::type >
//spaw_task(F&& f, Args&&... args )
//{
//    using result_type = std::result_of<F( Args... )>::type;
//    packaged_task<result_type> task(forward<F>(f));
//
//    future< result_type > res_future = task.get_future( );
//
//    thread t( move(task), forward<Args...>( args... ));
//
//    t.detach( );
//
//    return res_future;
//}

void
run_sample_2( )
{
    std::list< int > input{1, 12, 32, 43, 1, 0, 4, 66, 7, 5, 34, 55, 34, 78, 8, 1};


    //spaw_task( &sequential_quick_sort<int>, input );

#if 0
    {
        auto result = sequential_quick_sort( input );

        for ( auto item : result )
        {
            cout << item << " ";
        }

        cout << endl;
    }
#endif

    auto result2 = parallel_quick_sort( input );
    for ( auto item : result2 )
    {
        cout << item << " ";
    }

    cout << endl;
}

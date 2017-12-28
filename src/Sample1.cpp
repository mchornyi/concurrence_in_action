#include "stdafx.h"
#include "Samples.h"
//////////////////////////////////////////////////////////////////////////
// Description:
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
namespace
{
using namespace std;

void
sleep_for( int sec )
{
    stringstream ss;
    ss << "sleep_for " << sec << " sec" << endl;
    cout << ss.str( );

    this_thread::sleep_for( chrono::seconds( sec ) );
}

int
find_the_answer_to_ltuae( )
{
    stringstream ss;
    ss << __FUNCSIG__ << "\n"
       << " thread id = " << this_thread::get_id( ) << "\n";
    cout << ss.str( );

    return 0;
}

void
do_other_stuff( )
{
}

struct X
{
    void
    foo( int, string const& )
    {
        stringstream ss;
        ss << __FUNCSIG__ << "\n"
           << " thread id = " << this_thread::get_id( ) << "\n";
        cout << ss.str( );
    }

    void
    bar( string const& )
    {
        stringstream ss;
        ss << __FUNCSIG__ << "\n"
           << " thread id = " << this_thread::get_id( ) << "\n";
        cout << ss.str( );
    }
};

struct Y
{
    double
    operator( )( double )
    {
        stringstream ss;
        ss << __FUNCSIG__ << "\n"
           << " thread id = " << this_thread::get_id( ) << "\n";
        cout << ss.str( );

        return 0.0;
    }
};

class MoveOnly
{
public:
    MoveOnly( )
    {
    }
    MoveOnly( const MoveOnly& ) = delete;
    MoveOnly& operator=( const MoveOnly& ) = delete;
    MoveOnly( MoveOnly&& ) = default;
    MoveOnly& operator=( MoveOnly&& ) = default;

    void
    operator( )( )
    {
        stringstream ss;
        ss << __FUNCSIG__ << "\n"
           << " thread id = " << this_thread::get_id( ) << "\n";
        cout << ss.str( );
    }
};

//////////////////////////////////////////////////////////////////////////

mutex deque_mutex;
using gui_packaged_task = packaged_task< void( void ) >;
deque< gui_packaged_task > tasks;
using gui_exit_promise = promise< bool >;
using gui_exit_future = future< bool >;

void
gui_thread( gui_exit_future&& exit_future )
{
    while ( !exit_future._Is_ready( ) )
    {
        if ( tasks.empty( ) )
        {
            sleep_for( 1 );
            continue;
        }

        gui_packaged_task task;
        {
            lock_guard< mutex > guard( deque_mutex );
            task = move( tasks.front( ) );
            tasks.pop_front( );
        }

        task( );
    }
}

template < typename Func >
future< void >
add_task( Func&& func )
{
    auto task = gui_packaged_task( func );
    auto result = task.get_future( );
    {
        lock_guard< mutex > guard( deque_mutex );
        tasks.push_back( move( task ) );
    }

    return result;
}

void
run_task_1( )
{
    stringstream ss;
    ss << __FUNCSIG__ << "\n"
       << " thread id = " << this_thread::get_id( ) << "\n";
    cout << ss.str( );
}

//////////////////////////////////////////////////////////////////////////

template < typename T >
T
factorial_tail( T result, T number )
{
    if ( number == 1 )
    {
        return result;
    }
    --number;
    return factorial_tail( result * number, number );
}

template < typename T >
T
factorial( T number )
{
    return factorial_tail( number, number );
}
};

void
run_sample_1( )
{

    X mX;
    Y mY;
    MoveOnly mOnly;

    future< int > the_answer1 = async( find_the_answer_to_ltuae );
    future< void > the_answer2
        = async( &X::foo, &mX, 42, string( "hello" ) );  // Calls p->foo(42,"hello") where p is &x
    future< void > the_answer3
        = async( &X::bar, mX,
                 string( "goodbye" ) );  // Calls tmpx.bar("goodbye") where tmpx is a copy of x
    future< double > the_answer4
        = async( Y( ), 1.1 );  // Calls tmpy(1.1) where tmpy is move - constructed from Y( )
    future< double > the_answer5 = async( ref( mY ), 1.1 );  // Calls mY(1.1)
    future< void > the_answer6
        = async( move( mOnly ) );  // Calls tmp() where tmp is constructed from move( mOnly )

    cout << "The answer is(1) " << the_answer1.get( ) << endl;
    cout << "The answer is(4) " << the_answer4.get( ) << endl;
    cout << "The answer is(5) " << the_answer5.get( ) << endl;

    gui_exit_promise exit_promise;

    auto gui_future = async( launch::async, gui_thread, exit_promise.get_future( ) );

    sleep_for( 2 );

    add_task( run_task_1 );

    add_task( [] {
        stringstream ss;
        ss << __FUNCSIG__ << "\n"
           << " thread id = " << this_thread::get_id( ) << "\n";
        cout << ss.str( );
    } );

#pragma region EXCEPTION

    auto exception_future = add_task( [] {
        stringstream ss;
        ss << __FUNCSIG__ << "\n"
           << " thread id = " << this_thread::get_id( ) << "\n";
        cout << ss.str( );

        throw out_of_range( "This is exception task!" );
    } );

    auto wait_for_gui_func = []( gui_exit_promise&& promise ) {
        sleep_for( 5 );
        promise.set_value( true );
    };

    future< void > wait_for_gui_exit
        = async( launch::async, wait_for_gui_func, move( exit_promise ) );

    try
    {
        exception_future.get( );
    }
    catch ( const exception& ex )
    {
        cout << ex.what( ) << endl;
    }
#pragma endregion EXCEPTION
/**/
#pragma region Waiting from multiple threads

    promise< int > input_promise;
    auto input_future_shared = input_promise.get_future( ).share( );
    list< future< void > > future_list;
    for ( int i = 0; i < 10; ++i )
    {
        auto future_local = async( launch::async, [input_future_shared, &future_list ]( ) {
            assert( input_future_shared.valid( ) );
            auto in_value = input_future_shared.get( );
            auto value = factorial( in_value );
            stringstream ss;
            ss << __FUNCSIG__ << "\n"
               << " thread id = " << this_thread::get_id( ) << " Factorial result is: " << value
               << "\n";
            cout << ss.str( );
        } );

        future_list.push_back( move(future_local) );
    }

    cout << "Enter the value[1..6] to calculate supper factorial: ";
    int input_value = 0;
    cin >> input_value;
    input_value = input_value <= 0 ? 1 : input_value;
    input_value = input_value > 6 ? 6 : input_value;

    input_promise.set_value( input_value );

    // wait and block all executors
    future_list.clear( );

#pragma endregion Waiting from multiple threads
    gui_future.wait( );
}
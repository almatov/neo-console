/* neo-console.cc
****************************************************************************************************************
****************************************************************************************************************

    Copyright (C) 2021, 2022, 2023 Askar Almatov

    This program is free software: you can redistribute it and/or modify it under the terms of the GNU General
    Public License as published by the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License along with this program.  If not, see
    <https://www.gnu.org/licenses/>.
*/

#include <atomic>
#include <csignal>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <ifaddrs.h>
#include <netdb.h>
#include <unistd.h>

#include "nanohat.h"

using std::atomic_flag;
using std::string;

constexpr const int     WAIT_TIMEOUT = 1000;    // milliseconds

enum class State
{
    IP,
    CLOCK,
    BLANK
};

static atomic_flag      stopFlag( ATOMIC_FLAG_INIT );

/**************************************************************************************************************/
void
signalCatcher( int signalNumber )
{
    stopFlag.test_and_set();
}

/**************************************************************************************************************/
string
getIp()
{
    string          ipAddresses;
    struct ifaddrs* ifaHead;

    getifaddrs( &ifaHead );

    for ( struct ifaddrs* ifa = ifaHead; ifa != nullptr; ifa = ifa->ifa_next )
    {
        if ( ifa->ifa_addr != nullptr && ifa->ifa_addr->sa_family == AF_INET )
        {
            char    host[ NI_MAXHOST ];

            getnameinfo( ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST );

            if ( strncmp(host, "127.0", 5) != 0 )
            {
                ipAddresses += host;
                ipAddresses += '\n';
            }
        }
    }

    freeifaddrs( ifaHead );
    return ipAddresses.empty()? "no IP" : move( ipAddresses );
}

/**************************************************************************************************************/
string
getClock()
{
    char    clock[ 32 ];
    time_t  now = time( nullptr );

    strftime( clock, sizeof(clock), "\n   %F\n    %T", localtime(&now) );
    return clock;
}

/**************************************************************************************************************/
int
main()
{
    struct sigaction    signalAction;

    sigemptyset( &signalAction.sa_mask );
    signalAction.sa_flags = 0;
    signalAction.sa_handler = signalCatcher;
    sigaction( SIGTERM, &signalAction, nullptr );
    sigaction( SIGINT, &signalAction, nullptr );

    NanoHat     hat;
    State       state = State::IP;

    while ( !stopFlag.test_and_set() )
    {
        stopFlag.clear();

        switch ( hat.getKey(WAIT_TIMEOUT) )
        {
            case NanoHat::Key::KEY_F1:
                state = State::IP;
                break;
            case NanoHat::Key::KEY_F2:
                state = State::CLOCK;
                break;
            case NanoHat::Key::KEY_F3:
                state = State::BLANK;
        };

        switch ( state )
        {
            case State::IP:
                hat.print( getIp() );
                break;
            case State::CLOCK:
                hat.print( getClock() );
                break;
            case State::BLANK:
            default:
                hat.print( "" );
        };
    }

    hat.print( "" );
    return EXIT_SUCCESS;
}

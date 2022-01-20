/* nanohat.cc
****************************************************************************************************************
****************************************************************************************************************

    Copyright (C) 2022 Askar Almatov

    This program is free software: you can redistribute it and/or modify it under the terms of the GNU General
    Public License as published by the Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License along with this program.  If not, see
    <https://www.gnu.org/licenses/>.
*/

#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include "nanohat.h"

using std::endl;
using std::exception;
using std::ofstream;
using std::runtime_error;
using std::string;
using std::to_string;

constexpr const int     F1_PIN_         = 0;
constexpr const int     F2_PIN_         = 2;
constexpr const int     F3_PIN_         = 3;
constexpr const int     OLED_WIDTH_     = 128;
constexpr const int     OLED_HEIGHT_    = 64;
constexpr const int     I2C_ADDRESS_    = 0x3c;
constexpr const char*   I2C_DEVICE_     = "/dev/i2c-0";
constexpr const int     BUFFER_SIZE_    = ( OLED_WIDTH_ * OLED_HEIGHT_ ) >> 3;

extern const uint8_t    FONT[][ 16 ];

/**************************************************************************************************************/
static void
oledCommand_( int fd, int command )
{
    char                        buf[] = { 0x80, static_cast<char>(command) };
    struct i2c_msg              message = { I2C_ADDRESS_, 0, sizeof(buf), buf };
    struct i2c_rdwr_ioctl_data  data = { &message, 1 };

    ioctl( fd, I2C_RDWR, &data );
}

/**************************************************************************************************************/
static void
oledFlush_( int fd, uint8_t* buffer )
{
    char*                       buf = reinterpret_cast<char*>( buffer - 1 );
    struct i2c_msg              message = { I2C_ADDRESS_, 0, BUFFER_SIZE_ + 1, buf };
    struct i2c_rdwr_ioctl_data  data = { &message, 1 };

    *buf = 0x40;
    ioctl( fd, I2C_RDWR, &data );
}

/**************************************************************************************************************/
NanoHat::NanoHat()
{
    ofstream( "/sys/class/gpio/unexport" ) << F1_PIN_ << endl << F2_PIN_ << endl << F3_PIN_ << endl;
    ofstream( "/sys/class/gpio/export" ) << F1_PIN_ << endl << F2_PIN_ << endl << F3_PIN_ << endl;

    if ( access(("/sys/class/gpio/gpio"+to_string(F3_PIN_)+"/value").c_str(), F_OK) < 0 )
    {
        throw runtime_error( "cannot export GPIO" );
    }

    const string    dir1( "/sys/class/gpio/gpio" + to_string(F1_PIN_) );
    const string    dir2( "/sys/class/gpio/gpio" + to_string(F2_PIN_) );
    const string    dir3( "/sys/class/gpio/gpio" + to_string(F3_PIN_) );

    ofstream( dir1 + "/direction" ) << "in" << endl;
    ofstream( dir2 + "/direction" ) << "in" << endl;
    ofstream( dir3 + "/direction" ) << "in" << endl;
    ofstream( dir1 + "/edge" ) << "rising" << endl;
    ofstream( dir2 + "/edge" ) << "rising" << endl;
    ofstream( dir3 + "/edge" ) << "rising" << endl;

    fd1_ = open( (dir1 + "/value").c_str(), O_RDONLY ); 
    fd2_ = open( (dir2 + "/value").c_str(), O_RDONLY ); 
    fd3_ = open( (dir3 + "/value").c_str(), O_RDONLY ); 

    struct epoll_event  event1 = { .events = EPOLLIN | EPOLLET, .data = {.fd = fd1_} };
    struct epoll_event  event2 = { .events = EPOLLIN | EPOLLET, .data = {.fd = fd2_} };
    struct epoll_event  event3 = { .events = EPOLLIN | EPOLLET, .data = {.fd = fd3_} };

    epfd_ = epoll_create( 1 );

    if
    (
        epoll_ctl( epfd_, EPOLL_CTL_ADD, fd1_, &event1 ) < 0 ||
        epoll_ctl( epfd_, EPOLL_CTL_ADD, fd2_, &event2 ) < 0 ||
        epoll_ctl( epfd_, EPOLL_CTL_ADD, fd3_, &event3 ) < 0
    )
    {
        throw runtime_error( "cannot add epoll events" );
    }

    epoll_wait( epfd_, &revent_, 10, 1 );
    i2cfd_ = open( I2C_DEVICE_, O_RDWR | O_NONBLOCK );

    int     initCommands[] = 
    {
        0xae, 0xd5, 0x80, 0xa8, 0x3f, 0xd3, 0x00, 0x40, 0x8d, 0x14, 0x20, 0x00, 0xa1, 0xc8, 0xda, 0x12,
        0x81, 0xcf, 0xd9, 0xf1, 0xdb, 0x40, 0xa4, 0xa6, 0x2e, 0xaf, 0x21, 0x00, 0x7f, 0x22, 0x00, 0x07
    };

    for ( int cmd : initCommands )
    {
        oledCommand_( i2cfd_, cmd );
    }

    buffer_ = new uint8_t[ BUFFER_SIZE_ + sizeof(void*) ] + sizeof( void* );
    memset( buffer_, 0, BUFFER_SIZE_ );
    oledFlush_( i2cfd_, buffer_ );
}

/**************************************************************************************************************/
NanoHat::~NanoHat()
{
    close( i2cfd_ );
    close( fd1_ );
    close( fd2_ );
    close( fd3_ );
    ofstream( "/sys/class/gpio/unexport" ) << F1_PIN_ << endl << F2_PIN_ << endl << F3_PIN_ << endl;
    delete[] ( buffer_ - sizeof(void*) );
}

/**************************************************************************************************************/
void
NanoHat::waitKey( int timeout )
{
    if ( epoll_wait(epfd_, &revent_, 1, timeout) <= 0 )
    {
        revent_.data.fd = -1;
    }
}

/**************************************************************************************************************/
bool
NanoHat::isKey1() const
{
    return revent_.data.fd == fd1_;
}

/**************************************************************************************************************/
bool
NanoHat::isKey2() const
{
    return revent_.data.fd == fd2_;
}

/**************************************************************************************************************/
bool
NanoHat::isKey3() const
{
    return revent_.data.fd == fd3_;
}

/**************************************************************************************************************/
void
NanoHat::print( const string& message )
{
    int     x = 0;
    int     y = 0;

    memset( buffer_, 0, BUFFER_SIZE_ );

    for ( char ch : message )
    {
        if ( y >= 4 )
        {
            break;
        }

        if ( ch == '\n' )
        {
            x = 0;
            ++y;
            continue;
        }
        else if ( ch < 32 || ch >= 128 )
        {
            ch = ' ';
        }

        uint8_t*        top = buffer_ + ( y << 8 ) + ( x << 3 );
        uint8_t*        bottom = buffer_ + ( y << 8 ) + ( x+16 << 3 );
        const uint8_t*  font = FONT[ ch - 32 ];

        memcpy( top, font, 8 );
        memcpy( bottom, font + 8, 8 );

        if ( ++x >= 16 )
        {
            x = 0;
            ++y;
        }
    }

    oledFlush_( i2cfd_, buffer_ );
}

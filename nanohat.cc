/* nanohat.cc
****************************************************************************************************************
****************************************************************************************************************

    Copyright (C) 2022, 2023 Askar Almatov

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
#include <iostream>
#include <stdexcept>
#include <string>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include "nanohat.h"

using std::endl;
using std::runtime_error;
using std::string;

constexpr const int     F1_PIN_             = 0;
constexpr const int     F2_PIN_             = 2;
constexpr const int     F3_PIN_             = 3;
constexpr const char*   GPIO_DEVICE_        = "/dev/gpiochip1";
constexpr const int     GPIO_BUFFER_SIZE_   = 32;
constexpr const char*   I2C_DEVICE_         = "/dev/i2c-0";
constexpr const int     I2C_ADDRESS_        = 0x3c;
constexpr const int     OLED_WIDTH_         = 128;
constexpr const int     OLED_HEIGHT_        = 64;
constexpr const int     OLED_BUFFER_SIZE_   = ( OLED_WIDTH_ * OLED_HEIGHT_ ) >> 3;

extern const uint8_t    FONT[][ 16 ];

/**************************************************************************************************************/
static void
oledCommand_( int fd, int command )
{
    uint8_t                     buf[] = { 0x80, static_cast<char>(command) };
    struct i2c_msg              message = { I2C_ADDRESS_, 0, sizeof(buf), buf };
    struct i2c_rdwr_ioctl_data  data = { &message, 1 };

    ioctl( fd, I2C_RDWR, &data );
}

/**************************************************************************************************************/
static void
oledFlush_( int fd, uint8_t* buffer )
{
    uint8_t*                    buf = buffer - 1;
    struct i2c_msg              message = { I2C_ADDRESS_, 0, OLED_BUFFER_SIZE_ + 1, buf };
    struct i2c_rdwr_ioctl_data  data = { &message, 1 };

    *buf = 0x40;
    ioctl( fd, I2C_RDWR, &data );
}

/**************************************************************************************************************/
NanoHat::NanoHat()
{
    struct gpiod_chip*  chip = gpiod_chip_open( GPIO_DEVICE_ );

    if ( chip == nullptr )
    {
        throw runtime_error( "cannot open GPIO device" );
    }

    extern char*                    program_invocation_short_name;
    unsigned                        offsets[] = { F1_PIN_, F2_PIN_, F3_PIN_ };
    struct gpiod_line_settings*     settings = gpiod_line_settings_new();
    struct gpiod_request_config*    reqConfig = gpiod_request_config_new();
    struct gpiod_line_config*       lineConfig = gpiod_line_config_new();

    gpiod_line_settings_set_direction( settings, GPIOD_LINE_DIRECTION_INPUT );
    gpiod_line_settings_set_edge_detection( settings, GPIOD_LINE_EDGE_RISING );
    gpiod_line_config_add_line_settings( lineConfig, offsets, sizeof(offsets)/sizeof(offsets[0]), settings );
    gpiod_request_config_set_consumer( reqConfig, program_invocation_short_name );
    gpioBuffer_ = gpiod_edge_event_buffer_new( GPIO_BUFFER_SIZE_ );
    gpioRequest_ = gpiod_chip_request_lines( chip, reqConfig, lineConfig );

    if ( gpioRequest_ == nullptr )
    {
        throw runtime_error( "failed to request GPIO lines" );
    }

    gpioFd_ = gpiod_line_request_get_fd( gpioRequest_ );
    gpiod_request_config_free( reqConfig );
    gpiod_line_config_free( lineConfig );
    gpiod_line_settings_free( settings );
    gpiod_chip_close( chip );

    static int  initCommands[] =
    {
        0xae, 0xd5, 0x80, 0xa8, 0x3f, 0xd3, 0x00, 0x40, 0x8d, 0x14, 0x20, 0x00, 0xa1, 0xc8, 0xda, 0x12,
        0x81, 0xcf, 0xd9, 0xf1, 0xdb, 0x40, 0xa4, 0xa6, 0x2e, 0xaf, 0x21, 0x00, 0x7f, 0x22, 0x00, 0x07
    };

    oledFd_ = open( I2C_DEVICE_, O_RDWR | O_NONBLOCK );

    for ( int cmd : initCommands )
    {
        oledCommand_( oledFd_, cmd );
    }

    oledBuffer_ = new uint8_t[ OLED_BUFFER_SIZE_ + sizeof(void*) ] + sizeof( void* );
    memset( oledBuffer_, 0, OLED_BUFFER_SIZE_ );
    oledFlush_( oledFd_, oledBuffer_ );
}

/**************************************************************************************************************/
NanoHat::~NanoHat()
{
    close( oledFd_ );
    delete[] ( oledBuffer_ - sizeof(void*) );
    gpiod_line_request_release( gpioRequest_ );
    gpiod_edge_event_buffer_free( gpioBuffer_ );
}

/**************************************************************************************************************/
NanoHat::Key
NanoHat::getKey( int timeout )
{
    if ( keyQueue_.empty() )
    {
        struct pollfd   pollFd = { .fd = gpioFd_, .events = POLLIN };

        if ( poll(&pollFd, 1, timeout) > 0 )
        {
            int     nEvents = gpiod_line_request_read_edge_events( gpioRequest_, gpioBuffer_, GPIO_BUFFER_SIZE_ );

            for ( int i = 0; i < nEvents; ++i )
            {
                struct gpiod_edge_event*    event = gpiod_edge_event_buffer_get_event( gpioBuffer_, i );
                unsigned                    offset = gpiod_edge_event_get_line_offset( event );

                switch ( offset )
                {
                    case F1_PIN_:
                        keyQueue_.push( Key::F1 );
                        break;
                    case F2_PIN_:
                        keyQueue_.push( Key::F2 );
                        break;
                    case F3_PIN_:
                        keyQueue_.push( Key::F3 );
                }
            }
        }

        if ( keyQueue_.empty() )
        {
            return Key::NIL;
        }
    }

    Key     key = keyQueue_.front();

    keyQueue_.pop();
    return key;
}

/**************************************************************************************************************/
void
NanoHat::print( const string& message )
{
    int     x = 0;
    int     y = 0;

    memset( oledBuffer_, 0, OLED_BUFFER_SIZE_ );

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

        uint8_t*        top = oledBuffer_ + ( y << 8 ) + ( x << 3 );
        uint8_t*        bottom = oledBuffer_ + ( y << 8 ) + ( x+16 << 3 );
        const uint8_t*  font = FONT[ ch - 32 ];

        memcpy( top, font, 8 );
        memcpy( bottom, font + 8, 8 );

        if ( ++x >= 16 )
        {
            x = 0;
            ++y;
        }
    }

    oledFlush_( oledFd_, oledBuffer_ );
}

/* nanohat.h
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

#include <cstdint>
#include <queue>
#include <string>
#include <gpiod.h>
#include <sys/epoll.h>

/**************************************************************************************************************/
class NanoHat
{
public:
    enum class Key
    {
                                        NO_KEY,
                                        KEY_F1,
                                        KEY_F2,
                                        KEY_F3
    };

                                        NanoHat();
                                        ~NanoHat();

    Key                                 getKey();
    void                                print( const std::string& message );

protected:
    std::queue<Key>                     keyQueue_;
    struct gpiod_line_request*          gpioRequest_;
    struct gpiod_edge_event_buffer*     gpioBuffer_;
    uint8_t*                            oledBuffer_;
    int                                 i2cfd_;
};

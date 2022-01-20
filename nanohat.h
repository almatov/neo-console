/* nanohat.h
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

#include <cstdint>
#include <string>
#include <sys/epoll.h>

/**************************************************************************************************************/
class NanoHat
{
public:
                                        NanoHat();
                                        ~NanoHat();

    void                                waitKey( int timeout );
    bool                                isKey1() const;
    bool                                isKey2() const;
    bool                                isKey3() const;
    void                                print( const std::string& message );

protected:
    int                                 i2cfd_;
    int                                 epfd_;
    int                                 fd1_;
    int                                 fd2_;
    int                                 fd3_;
    struct epoll_event                  revent_;
    uint8_t*                            buffer_;
};

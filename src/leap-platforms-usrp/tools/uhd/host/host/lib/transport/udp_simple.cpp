//
// Copyright 2010-2011 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "udp_common.hpp"
#include <uhd/transport/udp_simple.hpp>
#include <boost/format.hpp>
#include <iostream>

using namespace uhd::transport;
namespace asio = boost::asio;

/***********************************************************************
 * UDP simple implementation: connected and broadcast
 **********************************************************************/
class udp_simple_impl : public udp_simple{
public:
    udp_simple_impl(
        const std::string &addr, const std::string &port, bool bcast, bool connect
    ):_connected(connect){
        //std::cout << boost::format("Creating udp transport for %s %s") % addr % port << std::endl;

        //resolve the address
        asio::ip::udp::resolver resolver(_io_service);
        asio::ip::udp::resolver::query query(asio::ip::udp::v4(), addr, port);
        _receiver_endpoint = *resolver.resolve(query);

        //create and open the socket
        _socket = socket_sptr(new asio::ip::udp::socket(_io_service));
        _socket->open(asio::ip::udp::v4());

        //allow broadcasting
        _socket->set_option(asio::socket_base::broadcast(bcast));

        //connect the socket
        if (connect) _socket->connect(_receiver_endpoint);

    }

    size_t send(const asio::const_buffer &buff){
        if (_connected) return _socket->send(asio::buffer(buff));
        return _socket->send_to(asio::buffer(buff), _receiver_endpoint);
    }

    size_t recv(const asio::mutable_buffer &buff, double timeout){
        if (not wait_for_recv_ready(_socket->native(), timeout)) return 0;
        return _socket->receive(asio::buffer(buff));
    }

private:
    bool                    _connected;
    asio::io_service        _io_service;
    socket_sptr             _socket;
    asio::ip::udp::endpoint _receiver_endpoint;
};

/***********************************************************************
 * UDP public make functions
 **********************************************************************/
udp_simple::sptr udp_simple::make_connected(
    const std::string &addr, const std::string &port
){
    return sptr(new udp_simple_impl(addr, port, false, true /* no bcast, connect */));
}

udp_simple::sptr udp_simple::make_broadcast(
    const std::string &addr, const std::string &port
){
    return sptr(new udp_simple_impl(addr, port, true, false /* bcast, no connect */));
}

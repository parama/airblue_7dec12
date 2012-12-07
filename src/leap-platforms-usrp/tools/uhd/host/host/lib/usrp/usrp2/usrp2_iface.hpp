//
// Copyright 2010 Ettus Research LLC
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

#ifndef INCLUDED_USRP2_IFACE_HPP
#define INCLUDED_USRP2_IFACE_HPP

#include <uhd/transport/udp_simple.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <utility>
#include <string>
#include "usrp2_regs.hpp"


//TODO: kill this crap when you have the top level GPS include file
typedef boost::function<void(std::string)> gps_send_fn_t;
typedef boost::function<std::string(void)> gps_recv_fn_t;

/*!
 * The usrp2 interface class:
 * Provides a set of functions to implementation layer.
 * Including spi, peek, poke, control...
 */
class usrp2_iface : public uhd::i2c_iface, boost::noncopyable{
public:
    typedef boost::shared_ptr<usrp2_iface> sptr;

    /*!
     * Make a new usrp2 interface with the control transport.
     * \param ctrl_transport the udp transport object
     * \return a new usrp2 interface object
     */
    static sptr make(uhd::transport::udp_simple::sptr ctrl_transport);

    /*!
     * Write a register (32 bits)
     * \param addr the address
     * \param data the 32bit data
     */
    virtual void poke32(boost::uint32_t addr, boost::uint32_t data) = 0;

    /*!
     * Read a register (32 bits)
     * \param addr the address
     * \return the 32bit data
     */
    virtual boost::uint32_t peek32(boost::uint32_t addr) = 0;

    /*!
     * Write a register (16 bits)
     * \param addr the address
     * \param data the 16bit data
     */
    virtual void poke16(boost::uint32_t addr, boost::uint16_t data) = 0;

    /*!
     * Read a register (16 bits)
     * \param addr the address
     * \return the 16bit data
     */
    virtual boost::uint16_t peek16(boost::uint32_t addr) = 0;

    /*!
     * Perform an spi transaction.
     * \param which_slave the slave device number
     * \param config spi config args
     * \param data the bits to write
     * \param num_bits how many bits in data
     * \param readback true to readback a value
     * \return spi data if readback set
     */
    virtual boost::uint32_t transact_spi(
        int which_slave,
        const uhd::spi_config_t &config,
        boost::uint32_t data,
        size_t num_bits,
        bool readback
    ) = 0;

    virtual void write_uart(boost::uint8_t dev, const std::string &buf) = 0;

    virtual std::string read_uart(boost::uint8_t dev) = 0;
    
    virtual gps_recv_fn_t get_gps_read_fn(void) = 0;
    virtual gps_send_fn_t get_gps_write_fn(void) = 0;

    //! The list of possible revision types
    enum rev_type {
        USRP2_REV3 = 3,
        USRP2_REV4 = 4,
        USRP_N200 = 200,
        USRP_N210 = 210,
        USRP_NXXX = 0
    };

    //! Get the revision type for this device
    virtual rev_type get_rev(void) = 0;

    //! Get the canonical name for this device
    virtual const std::string get_cname(void) = 0;

    /*!
     * Register map selected from USRP2/USRP2+.
     */
    usrp2_regs_t regs;

    //motherboard eeprom map structure
    uhd::usrp::mboard_eeprom_t mb_eeprom;
};

#endif /* INCLUDED_USRP2_IFACE_HPP */

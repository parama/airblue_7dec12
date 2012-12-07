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

#ifndef INCLUDED_UHD_TYPES_CLOCK_CONFIG_HPP
#define INCLUDED_UHD_TYPES_CLOCK_CONFIG_HPP

#include <uhd/config.hpp>

namespace uhd{

    /*!
     * Clock configuration settings:
     * The source for the 10MHz reference clock.
     * The source and polarity for the PPS clock.
     */
    struct UHD_API clock_config_t{
        //------ simple usage --------//
        static clock_config_t external(void);
        static clock_config_t internal(void);

        //------ advanced usage --------//
        enum ref_source_t {
            REF_AUTO = 'a', //automatic (device specific)
            REF_INT  = 'i', //internal reference
            REF_SMA  = 's', //external sma port
        } ref_source;
        enum pps_source_t {
            PPS_INT  = 'i', //there is no internal
            PPS_SMA  = 's', //external sma port
        } pps_source;
        enum pps_polarity_t {
            PPS_NEG = 'n', //negative edge
            PPS_POS = 'p'  //positive edge
        } pps_polarity;
        clock_config_t(void);
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_CLOCK_CONFIG_HPP */

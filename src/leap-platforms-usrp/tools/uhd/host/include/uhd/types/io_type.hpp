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

#ifndef INCLUDED_UHD_TYPES_IO_TYPE_HPP
#define INCLUDED_UHD_TYPES_IO_TYPE_HPP

#include <uhd/config.hpp>

namespace uhd{

    /*!
     * The Input/Output configuration struct:
     * Used to specify the IO type with device send/recv.
     */
    class UHD_API io_type_t{
    public:

        /*!
         * Built in IO types known to the system.
         */
        enum tid_t{
            //! Custom type (technically unsupported by implementation)
            CUSTOM_TYPE =     '?',
            //! Complex floating point (64-bit floats) range [-1.0, +1.0]
            COMPLEX_FLOAT64 = 'd',
            //! Complex floating point (32-bit floats) range [-1.0, +1.0]
            COMPLEX_FLOAT32 = 'f',
            //! Complex signed integer (16-bit integers) range [-32768, +32767]
            COMPLEX_INT16 =   's',
            //! Complex signed integer (8-bit integers) range [-128, 127]
            COMPLEX_INT8 =    'b'
        };

        /*!
         * The size of this io type in bytes.
         */
        const size_t size;

        /*!
         * The type id of this io type.
         * Good for using with switch statements.
         */
        const tid_t tid;

        /*!
         * Create an io type from a built-in type id.
         * \param tid a type id known to the system
         */
        io_type_t(tid_t tid);

        /*!
         * Create an io type from attributes.
         * The tid will be set to custom.
         * \param size the size in bytes
         */
        io_type_t(size_t size);

    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_IO_TYPE_HPP */

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

#ifndef INCLUDED_WAX_HPP
#define INCLUDED_WAX_HPP

#include <uhd/config.hpp>
#include <boost/any.hpp>

/*!
 * WAX - it's a metaphor!
 *
 * The WAX framework allows an object to have generic/anyobj properties.
 * These properties can be addressed through generic/anyobj identifiers.
 *
 * The WAX object itself is an anytype container much like boost::any.
 * To retrieve the value of the appropriate type, use my_obj.as<type>().
 *
 * Proprties may be referenced though the [] overloaded operator.
 * The [] operator returns a special proxy that allows for assigment.
 * Also, the [] operators may be chained as in the folowing examples:
 *   my_obj[prop1][prop2][prop3] = value;
 *   value = my_obj[prop1][prop2][prop3].as<type>();
 *
 * Property nesting occurs when a WAX object gets another object's link.
 * This special link is obtained through a call to my_obj.get_link().
 *
 * Note: Do not put a class derived from wax::obj into an stl container.
 * MSVC will compile the code, but the binaries will crash at runtime.
 * Rather, use pointers or smart pointers to instances of the derived class.
 */

namespace wax{

    /*!
     * The wax::bad cast will be thrown when
     * cast is called with the wrong typeid.
     */
    typedef boost::bad_any_cast bad_cast;

    /*!
     * WAX object base class:
     *
     * A wax obj has two major purposes:
     *   1) to act as a polymorphic container, just like boost any
     *   2) to provide a nested set/get properties interface
     *
     * Internally, the polymorphic container is handled by a boost any.
     * For properties, a subclass should override the set and get methods.
     * For property nesting, wax obj subclasses return special links
     * to other wax obj subclasses, and the api handles the magic.
     */
    class UHD_API obj{
    public:

        /*!
         * Default constructor:
         * The contents will be empty.
         */
        obj(void);

        /*!
         * Copy constructor:
         * The contents will be cloned.
         * \param o another wax::obj
         */
        obj(const obj &o);

        /*!
         * Templated any type constructor:
         * The contents can be anything.
         * Uses the boost::any to handle the magic.
         * \param o an object of any type
         */
        template<class T> obj(const T &o){
            _contents = o;
        }

        /*!
         * Destructor.
         */
        virtual ~obj(void);

        /*!
         * The chaining operator:
         * This operator allows access objs with properties.
         * A call to the [] operator will return a new proxy obj.
         * The proxy object is an obj with special proxy contents.
         * Assignment and casting can be used on this special object
         * to access the property referenced by the obj key.
         * \param key a key to identify a property within this obj
         * \return a special wax obj that proxies the obj and key
         */
        obj operator[](const obj &key);

        /*!
         * The assignment operator:
         * This operator allows for assignment of new contents.
         * In the special case where this obj contains a proxy,
         * the value will be set to the proxy's property reference.
         * \param val the new value to assign to the wax obj
         * \return a reference to this obj (*this)
         */
        obj & operator=(const obj &val);

        /*!
         * Get a link in the chain:
         * When a wax obj returns another wax obj as part of a get call,
         * the return value should be set to the result of this method.
         * Doing so will ensure chain-ability of the returned object.
         * \return an obj containing a valid link to a wax obj
         */
        obj get_link(void) const;

        /*!
         * Get the type of the contents of this obj.
         * \return a reference to the type_info
         */
        const std::type_info & type(void) const;

        /*!
         * Cast this obj into the desired type.
         * Usage: myobj.as<type>()
         *
         * \return an object of the desired type
         * \throw wax::bad_cast when the cast fails
         */
        template<class T> T as(void) const{
            return boost::any_cast<T>(resolve());
        }

    private:
        //private interface (override in subclasses)
        virtual void get(const obj &, obj &);
        virtual void set(const obj &, const obj &);

        /*!
         * Resolve the contents of this obj.
         * In the case where this obj is a proxy,
         * the referenced property will be resolved.
         * Otherwise, just get the private contents.
         * \return a boost any type with contents
         */
        boost::any resolve(void) const;

        //private contents of this obj
        boost::any _contents;

    };

} //namespace wax

#endif /* INCLUDED_WAX_HPP */

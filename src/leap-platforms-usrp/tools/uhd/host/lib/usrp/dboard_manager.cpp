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

#include "dboard_ctor_args.hpp"
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <uhd/utils/warning.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/types/dict.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/assign/list_of.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * storage and registering for dboards
 **********************************************************************/
//dboard registry tuple: dboard constructor, canonical name, subdev names
typedef boost::tuple<dboard_manager::dboard_ctor_t, std::string, prop_names_t> args_t;

//map a dboard id to a dboard constructor
typedef uhd::dict<dboard_id_t, args_t> id_to_args_map_t;
UHD_SINGLETON_FCN(id_to_args_map_t, get_id_to_args_map)

void dboard_manager::register_dboard(
    const dboard_id_t &dboard_id,
    dboard_ctor_t dboard_ctor,
    const std::string &name,
    const prop_names_t &subdev_names
){
    //std::cout << "registering: " << name << std::endl;
    if (get_id_to_args_map().has_key(dboard_id)){
        throw std::runtime_error(str(boost::format(
            "The dboard id %s is already registered to %s."
        ) % dboard_id.to_string() % dboard_id.to_pp_string()));
    }
    get_id_to_args_map()[dboard_id] = args_t(dboard_ctor, name, subdev_names);
}

//map an xcvr dboard id to its partner dboard id
typedef uhd::dict<dboard_id_t, dboard_id_t> xcvr_id_to_id_map_t;
UHD_SINGLETON_FCN(xcvr_id_to_id_map_t, get_xcvr_id_to_id_map)

void dboard_manager::register_dboard(
    const dboard_id_t &rx_dboard_id,
    const dboard_id_t &tx_dboard_id,
    dboard_ctor_t dboard_ctor,
    const std::string &name,
    const prop_names_t &subdev_names
){
    //regular registration for ids
    register_dboard(rx_dboard_id, dboard_ctor, name, subdev_names);
    register_dboard(tx_dboard_id, dboard_ctor, name, subdev_names);

    //register xcvr mapping for ids
    get_xcvr_id_to_id_map()[rx_dboard_id] = tx_dboard_id;
    get_xcvr_id_to_id_map()[tx_dboard_id] = rx_dboard_id;
}

std::string dboard_id_t::to_cname(void) const{
    if (not get_id_to_args_map().has_key(*this)) return "Unknown";
    return get_id_to_args_map()[*this].get<1>();
}

std::string dboard_id_t::to_pp_string(void) const{
    return str(boost::format("%s (%s)") % this->to_cname() % this->to_string());
}

/***********************************************************************
 * internal helper classes
 **********************************************************************/
/*!
 * A special wax proxy object that forwards calls to a subdev.
 * A sptr to an instance will be used in the properties structure. 
 */
class subdev_proxy : boost::noncopyable, public wax::obj{
public:
    typedef boost::shared_ptr<subdev_proxy> sptr;
    enum type_t{RX_TYPE, TX_TYPE};

    //structors
    subdev_proxy(dboard_base::sptr subdev, type_t type):
        _subdev(subdev), _type(type)
    {
        /* NOP */
    }

private:
    dboard_base::sptr   _subdev;
    type_t              _type;

    //forward the get calls to the rx or tx
    void get(const wax::obj &key, wax::obj &val){
        switch(_type){
        case RX_TYPE: return _subdev->rx_get(key, val);
        case TX_TYPE: return _subdev->tx_get(key, val);
        }
    }

    //forward the set calls to the rx or tx
    void set(const wax::obj &key, const wax::obj &val){
        switch(_type){
        case RX_TYPE: return _subdev->rx_set(key, val);
        case TX_TYPE: return _subdev->tx_set(key, val);
        }
    }
};

/***********************************************************************
 * dboard manager implementation class
 **********************************************************************/
class dboard_manager_impl : public dboard_manager{

public:
    dboard_manager_impl(
        dboard_id_t rx_dboard_id,
        dboard_id_t tx_dboard_id,
        dboard_iface::sptr iface
    );
    ~dboard_manager_impl(void);

    //dboard_iface
    prop_names_t get_rx_subdev_names(void);
    prop_names_t get_tx_subdev_names(void);
    wax::obj get_rx_subdev(const std::string &subdev_name);
    wax::obj get_tx_subdev(const std::string &subdev_name);

private:
    void init(dboard_id_t, dboard_id_t);
    //list of rx and tx dboards in this dboard_manager
    //each dboard here is actually a subdevice proxy
    //the subdevice proxy is internal to the cpp file
    uhd::dict<std::string, subdev_proxy::sptr> _rx_dboards;
    uhd::dict<std::string, subdev_proxy::sptr> _tx_dboards;
    dboard_iface::sptr _iface;
    void set_nice_dboard_if(void);
};

/***********************************************************************
 * make routine for dboard manager
 **********************************************************************/
dboard_manager::sptr dboard_manager::make(
    dboard_id_t rx_dboard_id,
    dboard_id_t tx_dboard_id,
    dboard_iface::sptr iface
){
    return dboard_manager::sptr(
        new dboard_manager_impl(rx_dboard_id, tx_dboard_id, iface)
    );
}

/***********************************************************************
 * implementation class methods
 **********************************************************************/
static args_t get_dboard_args(
    dboard_iface::unit_t unit,
    dboard_id_t dboard_id,
    bool force_to_unknown = false
){
    //special case, the none id was provided, use the following ids
    if (dboard_id == dboard_id_t::none() or force_to_unknown){
        UHD_ASSERT_THROW(get_id_to_args_map().has_key(0xfff1));
        UHD_ASSERT_THROW(get_id_to_args_map().has_key(0xfff0));
        switch(unit){
        case dboard_iface::UNIT_RX: return get_dboard_args(unit, 0xfff1);
        case dboard_iface::UNIT_TX: return get_dboard_args(unit, 0xfff0);
        default: UHD_THROW_INVALID_CODE_PATH();
        }
    }

    //verify that there is a registered constructor for this id
    if (not get_id_to_args_map().has_key(dboard_id)){
        uhd::warning::post(str(boost::format(
            "Unknown dboard ID: %s.\n"
        ) % dboard_id.to_pp_string()));
        return get_dboard_args(unit, dboard_id, true);
    }

    //return the dboard args for this id
    return get_id_to_args_map()[dboard_id];
}

dboard_manager_impl::dboard_manager_impl(
    dboard_id_t rx_dboard_id,
    dboard_id_t tx_dboard_id,
    dboard_iface::sptr iface
):
    _iface(iface)
{
    try{
        this->init(rx_dboard_id, tx_dboard_id);
    }
    catch(const std::exception &e){
        uhd::warning::post(e.what());
        this->init(dboard_id_t::none(), dboard_id_t::none());
    }
}

void dboard_manager_impl::init(
    dboard_id_t rx_dboard_id, dboard_id_t tx_dboard_id
){
    //determine xcvr status
    bool rx_dboard_is_xcvr = get_xcvr_id_to_id_map().has_key(rx_dboard_id);
    bool tx_dboard_is_xcvr = get_xcvr_id_to_id_map().has_key(tx_dboard_id);
    bool this_dboard_is_xcvr = (
        rx_dboard_is_xcvr and tx_dboard_is_xcvr and
        (get_xcvr_id_to_id_map()[rx_dboard_id] == tx_dboard_id) and
        (get_xcvr_id_to_id_map()[tx_dboard_id] == rx_dboard_id)
    );

    //warn for invalid dboard id xcvr combinations
    if (rx_dboard_is_xcvr != this_dboard_is_xcvr or tx_dboard_is_xcvr != this_dboard_is_xcvr){
        uhd::warning::post(str(boost::format(
            "Unknown transceiver board ID combination...\n"
            "RX dboard ID: %s\n"
            "TX dboard ID: %s\n"
        ) % rx_dboard_id.to_pp_string() % tx_dboard_id.to_pp_string()));
    }

    //extract dboard constructor and settings (force to unknown for messed up xcvr status)
    dboard_ctor_t rx_dboard_ctor; std::string rx_name; prop_names_t rx_subdevs;
    boost::tie(rx_dboard_ctor, rx_name, rx_subdevs) = get_dboard_args(
        dboard_iface::UNIT_RX, rx_dboard_id, rx_dboard_is_xcvr != this_dboard_is_xcvr
    );

    dboard_ctor_t tx_dboard_ctor; std::string tx_name; prop_names_t tx_subdevs;
    boost::tie(tx_dboard_ctor, tx_name, tx_subdevs) = get_dboard_args(
        dboard_iface::UNIT_TX, tx_dboard_id, tx_dboard_is_xcvr != this_dboard_is_xcvr
    );

    //initialize the gpio pins before creating subdevs
    set_nice_dboard_if();

    //dboard constructor args
    dboard_ctor_args_t db_ctor_args;
    db_ctor_args.db_iface = _iface;

    //make xcvr subdevs (make one subdev for both rx and tx dboards)
    if (this_dboard_is_xcvr){
        UHD_ASSERT_THROW(rx_dboard_ctor == tx_dboard_ctor);
        UHD_ASSERT_THROW(rx_subdevs == tx_subdevs);
        BOOST_FOREACH(const std::string &subdev, rx_subdevs){
            db_ctor_args.sd_name = subdev;
            db_ctor_args.rx_id = rx_dboard_id;
            db_ctor_args.tx_id = tx_dboard_id;
            dboard_base::sptr xcvr_dboard = rx_dboard_ctor(&db_ctor_args);
            //create a rx proxy for this xcvr board
            _rx_dboards[subdev] = subdev_proxy::sptr(
                new subdev_proxy(xcvr_dboard, subdev_proxy::RX_TYPE)
            );
            //create a tx proxy for this xcvr board
            _tx_dboards[subdev] = subdev_proxy::sptr(
                new subdev_proxy(xcvr_dboard, subdev_proxy::TX_TYPE)
            );
        }
    }

    //make tx and rx subdevs (separate subdevs for rx and tx dboards)
    else{
        //make the rx subdevs
        BOOST_FOREACH(const std::string &subdev, rx_subdevs){
            db_ctor_args.sd_name = subdev;
            db_ctor_args.rx_id = rx_dboard_id;
            db_ctor_args.tx_id = dboard_id_t::none();
            dboard_base::sptr rx_dboard = rx_dboard_ctor(&db_ctor_args);
            //create a rx proxy for this rx board
            _rx_dboards[subdev] = subdev_proxy::sptr(
                new subdev_proxy(rx_dboard, subdev_proxy::RX_TYPE)
            );
        }
        //make the tx subdevs
        BOOST_FOREACH(const std::string &subdev, tx_subdevs){
            db_ctor_args.sd_name = subdev;
            db_ctor_args.rx_id = dboard_id_t::none();
            db_ctor_args.tx_id = tx_dboard_id;
            dboard_base::sptr tx_dboard = tx_dboard_ctor(&db_ctor_args);
            //create a tx proxy for this tx board
            _tx_dboards[subdev] = subdev_proxy::sptr(
                new subdev_proxy(tx_dboard, subdev_proxy::TX_TYPE)
            );
        }
    }
}

dboard_manager_impl::~dboard_manager_impl(void){
    set_nice_dboard_if();
}

prop_names_t dboard_manager_impl::get_rx_subdev_names(void){
    return _rx_dboards.keys();
}

prop_names_t dboard_manager_impl::get_tx_subdev_names(void){
    return _tx_dboards.keys();
}

wax::obj dboard_manager_impl::get_rx_subdev(const std::string &subdev_name){
    if (not _rx_dboards.has_key(subdev_name)) throw std::invalid_argument(
        str(boost::format("Unknown rx subdev name %s") % subdev_name)
    );
    //get a link to the rx subdev proxy
    return _rx_dboards[subdev_name]->get_link();
}

wax::obj dboard_manager_impl::get_tx_subdev(const std::string &subdev_name){
    if (not _tx_dboards.has_key(subdev_name)) throw std::invalid_argument(
        str(boost::format("Unknown tx subdev name %s") % subdev_name)
    );
    //get a link to the tx subdev proxy
    return _tx_dboards[subdev_name]->get_link();
}

void dboard_manager_impl::set_nice_dboard_if(void){
    //make a list of possible unit types
    std::vector<dboard_iface::unit_t> units = boost::assign::list_of
        (dboard_iface::UNIT_RX)
        (dboard_iface::UNIT_TX)
    ;

    //set nice settings on each unit
    BOOST_FOREACH(dboard_iface::unit_t unit, units){
        _iface->set_gpio_ddr(unit, 0x0000); //all inputs
        _iface->set_gpio_out(unit, 0x0000); //all low
        _iface->set_pin_ctrl(unit, 0x0000); //all gpio
        _iface->set_clock_enabled(unit, false); //clock off
    }

    //disable all rx subdevices
    BOOST_FOREACH(const std::string &sd_name, this->get_rx_subdev_names()){
        this->get_rx_subdev(sd_name)[SUBDEV_PROP_ENABLED] = false;
    }

    //disable all tx subdevices
    BOOST_FOREACH(const std::string &sd_name, this->get_tx_subdev_names()){
        this->get_tx_subdev(sd_name)[SUBDEV_PROP_ENABLED] = false;
    }
}

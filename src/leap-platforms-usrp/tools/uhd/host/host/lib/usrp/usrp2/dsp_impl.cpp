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

#include "usrp2_impl.hpp"
#include "usrp2_regs.hpp"
#include <uhd/usrp/dsp_utils.hpp>
#include <uhd/usrp/dsp_props.hpp>
#include <boost/bind.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/math/special_functions/sign.hpp>
#include <algorithm>
#include <cmath>

using namespace uhd;
using namespace uhd::usrp;

static const size_t default_decim = 16;
static const size_t default_interp = 16;

/***********************************************************************
 * DDC Helper Methods
 **********************************************************************/
template <typename rate_type>
static rate_type pick_closest_rate(double exact_rate, const std::vector<rate_type> &rates){
    unsigned closest_match = rates.front();
    BOOST_FOREACH(rate_type possible_rate, rates){
        if(std::abs(exact_rate - possible_rate) < std::abs(exact_rate - closest_match))
            closest_match = possible_rate;
    }
    return closest_match;
}

void usrp2_mboard_impl::init_ddc_config(void){
    //create the ddc in the rx dsp dict
    _rx_dsp_proxy = wax_obj_proxy::make(
        boost::bind(&usrp2_mboard_impl::ddc_get, this, _1, _2),
        boost::bind(&usrp2_mboard_impl::ddc_set, this, _1, _2)
    );

    //initial config and update
    ddc_set(DSP_PROP_FREQ_SHIFT, double(0));
    ddc_set(DSP_PROP_HOST_RATE, double(get_master_clock_freq()/default_decim));
}

/***********************************************************************
 * DDC Properties
 **********************************************************************/
void usrp2_mboard_impl::ddc_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    switch(key.as<dsp_prop_t>()){
    case DSP_PROP_NAME:
        val = _iface->get_cname() + " ddc0";
        return;

    case DSP_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case DSP_PROP_FREQ_SHIFT:
        val = _ddc_freq;
        return;

    case DSP_PROP_FREQ_SHIFT_NAMES:
        val = prop_names_t(1, "");
        return;

    case DSP_PROP_CODEC_RATE:
        val = get_master_clock_freq();
        return;

    case DSP_PROP_HOST_RATE:
        val = get_master_clock_freq()/_ddc_decim;
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void usrp2_mboard_impl::ddc_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    switch(key.as<dsp_prop_t>()){

    case DSP_PROP_FREQ_SHIFT:{
            double new_freq = val.as<double>();
            _iface->poke32(_iface->regs.dsp_rx_freq,
                dsp_type1::calc_cordic_word_and_update(new_freq, get_master_clock_freq())
            );
            _ddc_freq = new_freq; //shadow
        }
        return;

    case DSP_PROP_HOST_RATE:{
            double extact_rate = get_master_clock_freq()/val.as<double>();
            _ddc_decim = pick_closest_rate(extact_rate, _allowed_decim_and_interp_rates);

            //set the decimation
            _iface->poke32(_iface->regs.dsp_rx_decim_rate, dsp_type1::calc_cic_filter_word(_ddc_decim));

            //set the scaling
            static const boost::int16_t default_rx_scale_iq = 1<<10;
            _iface->poke32(_iface->regs.dsp_rx_scale_iq,
                dsp_type1::calc_iq_scale_word(default_rx_scale_iq, default_rx_scale_iq)
            );
        }
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

/***********************************************************************
 * DUC Helper Methods
 **********************************************************************/
void usrp2_mboard_impl::init_duc_config(void){
    //create the duc in the tx dsp dict
    _tx_dsp_proxy = wax_obj_proxy::make(
        boost::bind(&usrp2_mboard_impl::duc_get, this, _1, _2),
        boost::bind(&usrp2_mboard_impl::duc_set, this, _1, _2)
    );

    //initial config and update
    duc_set(DSP_PROP_FREQ_SHIFT, double(0));
    duc_set(DSP_PROP_HOST_RATE, double(get_master_clock_freq()/default_interp));
}

/***********************************************************************
 * DUC Properties
 **********************************************************************/
void usrp2_mboard_impl::duc_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    switch(key.as<dsp_prop_t>()){
    case DSP_PROP_NAME:
        val = _iface->get_cname() + " duc0";
        return;

    case DSP_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case DSP_PROP_FREQ_SHIFT:
        val = _duc_freq;
        return;

    case DSP_PROP_FREQ_SHIFT_NAMES:
        val = prop_names_t(1, "");
        return;

    case DSP_PROP_CODEC_RATE:
        val = get_master_clock_freq();
        return;

    case DSP_PROP_HOST_RATE:
        val = get_master_clock_freq()/_duc_interp;
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void usrp2_mboard_impl::duc_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    switch(key.as<dsp_prop_t>()){

    case DSP_PROP_FREQ_SHIFT:{
            const double codec_rate = get_master_clock_freq();
            double new_freq = val.as<double>();

            //calculate the DAC shift (multiples of rate)
            const int sign = boost::math::sign(new_freq);
            const int zone = std::min(boost::math::iround(new_freq/codec_rate), 2);
            const double dac_shift = sign*zone*codec_rate;
            new_freq -= dac_shift; //update FPGA DSP target freq

            //set the DAC shift (modulation mode)
            if (zone == 0) _codec_ctrl->set_tx_mod_mode(0); //no shift
            else _codec_ctrl->set_tx_mod_mode(sign*4/zone); //DAC interp = 4

            _iface->poke32(_iface->regs.dsp_tx_freq,
                dsp_type1::calc_cordic_word_and_update(new_freq, codec_rate)
            );
            _duc_freq = new_freq + dac_shift; //shadow
        }
        return;

    case DSP_PROP_HOST_RATE:{
            double extact_rate = get_master_clock_freq()/val.as<double>();
            _duc_interp = pick_closest_rate(extact_rate, _allowed_decim_and_interp_rates);

            //set the interpolation
            _iface->poke32(_iface->regs.dsp_tx_interp_rate, dsp_type1::calc_cic_filter_word(_duc_interp));

            //set the scaling
            _iface->poke32(_iface->regs.dsp_tx_scale_iq, dsp_type1::calc_iq_scale_word(_duc_interp));
        }
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

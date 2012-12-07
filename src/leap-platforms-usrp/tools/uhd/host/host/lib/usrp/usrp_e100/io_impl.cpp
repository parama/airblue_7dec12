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

#include "usrp_e100_impl.hpp"
#include "usrp_e100_regs.hpp"
#include <uhd/usrp/dsp_utils.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include "../../transport/vrt_packet_handler.hpp"
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

zero_copy_if::sptr usrp_e100_make_mmap_zero_copy(usrp_e100_iface::sptr iface);

/***********************************************************************
 * Constants
 **********************************************************************/
static const size_t rx_data_inline_sid = 1;
static const size_t tx_async_report_sid = 2;
static const int underflow_flags = async_metadata_t::EVENT_CODE_UNDERFLOW | async_metadata_t::EVENT_CODE_UNDERFLOW_IN_PACKET;
static const bool recv_debug = false;

/***********************************************************************
 * io impl details (internal to this file)
 * - pirate crew of 1
 * - bounded buffer
 * - thread loop
 * - vrt packet handler states
 **********************************************************************/
struct usrp_e100_impl::io_impl{
    io_impl(usrp_e100_iface::sptr iface):
        data_xport(usrp_e100_make_mmap_zero_copy(iface)),
        get_recv_buffs_fcn(boost::bind(&usrp_e100_impl::io_impl::get_recv_buffs, this, _1)),
        get_send_buffs_fcn(boost::bind(&usrp_e100_impl::io_impl::get_send_buffs, this, _1)),
        recv_pirate_booty(data_xport->get_num_recv_frames()),
        async_msg_fifo(100/*messages deep*/)
    {
        /* NOP */
    }

    ~io_impl(void){
        recv_pirate_crew_raiding = false;
        recv_pirate_crew.interrupt_all();
        recv_pirate_crew.join_all();
    }

    bool get_recv_buffs(vrt_packet_handler::managed_recv_buffs_t &buffs){
        UHD_ASSERT_THROW(buffs.size() == 1);
        boost::this_thread::disable_interruption di; //disable because the wait can throw
        return recv_pirate_booty.pop_with_timed_wait(buffs.front(), recv_timeout);
    }

    bool get_send_buffs(vrt_packet_handler::managed_send_buffs_t &buffs){
        UHD_ASSERT_THROW(buffs.size() == 1);
        buffs[0] = data_xport->get_send_buff(send_timeout);
        return buffs[0].get() != NULL;
    }

    //The data transport is listed first so that it is deconstructed last,
    //which is after the states and booty which may hold managed buffers.
    zero_copy_if::sptr data_xport;

    //bound callbacks for get buffs (bound once here, not in fast-path)
    vrt_packet_handler::get_recv_buffs_t get_recv_buffs_fcn;
    vrt_packet_handler::get_send_buffs_t get_send_buffs_fcn;

    //timeouts set on calls to recv/send (passed into get buffs methods)
    double recv_timeout, send_timeout;

    //state management for the vrt packet handler code
    vrt_packet_handler::recv_state packet_handler_recv_state;
    vrt_packet_handler::send_state packet_handler_send_state;
    bool continuous_streaming;

    //a pirate's life is the life for me!
    void recv_pirate_loop(usrp_e100_clock_ctrl::sptr);
    bounded_buffer<managed_recv_buffer::sptr> recv_pirate_booty;
    bounded_buffer<async_metadata_t> async_msg_fifo;
    boost::thread_group recv_pirate_crew;
    bool recv_pirate_crew_raiding;
};

/***********************************************************************
 * Receive Pirate Loop
 * - while raiding, loot for recv buffers
 * - put booty into the alignment buffer
 **********************************************************************/
void usrp_e100_impl::io_impl::recv_pirate_loop(usrp_e100_clock_ctrl::sptr clock_ctrl)
{
    set_thread_priority_safe();
    recv_pirate_crew_raiding = true;

    while(recv_pirate_crew_raiding){
        managed_recv_buffer::sptr buff = this->data_xport->get_recv_buff();
        if (not buff.get()) continue; //ignore timeout/error buffers

        if (recv_debug){
            std::cout << "len " << buff->size() << std::endl;
            for (size_t i = 0; i < 9; i++){
                std::cout << boost::format("    0x%08x") % buff->cast<const boost::uint32_t *>()[i] << std::endl;
            }
            std::cout << std::endl << std::endl;
        }

        try{
            //extract the vrt header packet info
            vrt::if_packet_info_t if_packet_info;
            if_packet_info.num_packet_words32 = buff->size()/sizeof(boost::uint32_t);
            const boost::uint32_t *vrt_hdr = buff->cast<const boost::uint32_t *>();
            vrt::if_hdr_unpack_le(vrt_hdr, if_packet_info);

            //handle an rx data packet or inline message
            if (if_packet_info.sid == rx_data_inline_sid){
                if (recv_debug) std::cout << "this is rx_data_inline_sid\n";
                //same number of frames as the data transport -> always immediate
                recv_pirate_booty.push_with_wait(buff);
                continue;
            }

            //handle a tx async report message
            if (if_packet_info.sid == tx_async_report_sid and if_packet_info.packet_type != vrt::if_packet_info_t::PACKET_TYPE_DATA){
                if (recv_debug) std::cout << "this is tx_async_report_sid\n";

                //fill in the async metadata
                async_metadata_t metadata;
                metadata.channel = 0;
                metadata.has_time_spec = if_packet_info.has_tsi and if_packet_info.has_tsf;
                metadata.time_spec = time_spec_t(
                    time_t(if_packet_info.tsi), size_t(if_packet_info.tsf), clock_ctrl->get_fpga_clock_rate()
                );
                metadata.event_code = vrt_packet_handler::get_context_code<async_metadata_t::event_code_t>(vrt_hdr, if_packet_info);

                //print the famous U, and push the metadata into the message queue
                if (metadata.event_code & underflow_flags) std::cerr << "U" << std::flush;
                async_msg_fifo.push_with_pop_on_full(metadata);
                continue;
            }

            if (recv_debug) std::cout << "this is unknown packet\n";

        }catch(const std::exception &e){
            std::cerr << "Error (usrp-e recv pirate loop): " << e.what() << std::endl;
        }
    }
}

/***********************************************************************
 * Helper Functions
 **********************************************************************/
void usrp_e100_impl::io_init(void){
    //setup otw types
    _send_otw_type.width = 16;
    _send_otw_type.shift = 0;
    _send_otw_type.byteorder = otw_type_t::BO_LITTLE_ENDIAN;

    _recv_otw_type.width = 16;
    _recv_otw_type.shift = 0;
    _recv_otw_type.byteorder = otw_type_t::BO_LITTLE_ENDIAN;

    //setup before the registers (transport called to calculate max spp)
    _io_impl = UHD_PIMPL_MAKE(io_impl, (_iface));

    //clear state machines
    _iface->poke32(UE_REG_CTRL_RX_CLEAR, 0);
    _iface->poke32(UE_REG_CTRL_TX_CLEAR, 0);

    //setup rx data path
    _iface->poke32(UE_REG_CTRL_RX_NSAMPS_PER_PKT, get_max_recv_samps_per_packet());
    _iface->poke32(UE_REG_CTRL_RX_NCHANNELS, 1);
    _iface->poke32(UE_REG_CTRL_RX_VRT_HEADER, 0
        | (0x1 << 28) //if data with stream id
        | (0x1 << 26) //has trailer
        | (0x3 << 22) //integer time other
        | (0x1 << 20) //fractional time sample count
    );
    _iface->poke32(UE_REG_CTRL_RX_VRT_STREAM_ID, rx_data_inline_sid);
    _iface->poke32(UE_REG_CTRL_RX_VRT_TRAILER, 0);

    //setup the tx policy
    _iface->poke32(UE_REG_CTRL_TX_REPORT_SID, tx_async_report_sid);
    _iface->poke32(UE_REG_CTRL_TX_POLICY, UE_FLAG_CTRL_TX_POLICY_NEXT_PACKET);

    //spawn a pirate, yarrr!
    _io_impl->recv_pirate_crew.create_thread(boost::bind(
        &usrp_e100_impl::io_impl::recv_pirate_loop, _io_impl.get(), _clock_ctrl
    ));
}

void usrp_e100_impl::issue_stream_cmd(const stream_cmd_t &stream_cmd){
    _io_impl->continuous_streaming = (stream_cmd.stream_mode == stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    _iface->poke32(UE_REG_CTRL_RX_STREAM_CMD, dsp_type1::calc_stream_cmd_word(stream_cmd));
    _iface->poke32(UE_REG_CTRL_RX_TIME_SECS,  boost::uint32_t(stream_cmd.time_spec.get_full_secs()));
    _iface->poke32(UE_REG_CTRL_RX_TIME_TICKS, stream_cmd.time_spec.get_tick_count(_clock_ctrl->get_fpga_clock_rate()));
}

void usrp_e100_impl::handle_overrun(size_t){
    std::cerr << "O"; //the famous OOOOOOOOOOO
    if (_io_impl->continuous_streaming){
        this->issue_stream_cmd(stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    }
}

/***********************************************************************
 * Data Send
 **********************************************************************/
size_t usrp_e100_impl::get_max_send_samps_per_packet(void) const{
    static const size_t hdr_size = 0
        + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
        - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
    ;
    size_t bpp = _io_impl->data_xport->get_send_frame_size() - hdr_size;
    return bpp/_send_otw_type.get_sample_size();
}

size_t usrp_e100_impl::send(
    const send_buffs_type &buffs, size_t num_samps,
    const tx_metadata_t &metadata, const io_type_t &io_type,
    send_mode_t send_mode, double timeout
){
    _io_impl->send_timeout = timeout;
    return vrt_packet_handler::send(
        _io_impl->packet_handler_send_state,       //last state of the send handler
        buffs, num_samps,                          //buffer to fill
        metadata, send_mode,                       //samples metadata
        io_type, _send_otw_type,                   //input and output types to convert
        _clock_ctrl->get_fpga_clock_rate(),        //master clock tick rate
        uhd::transport::vrt::if_hdr_pack_le,
        _io_impl->get_send_buffs_fcn,
        get_max_send_samps_per_packet()
    );
}

/***********************************************************************
 * Data Recv
 **********************************************************************/
size_t usrp_e100_impl::get_max_recv_samps_per_packet(void) const{
    static const size_t hdr_size = 0
        + vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
        + sizeof(vrt::if_packet_info_t().tlr) //forced to have trailer
        - sizeof(vrt::if_packet_info_t().cid) //no class id ever used
    ;
    size_t bpp = _io_impl->data_xport->get_recv_frame_size() - hdr_size;
    return bpp/_recv_otw_type.get_sample_size();
}

size_t usrp_e100_impl::recv(
    const recv_buffs_type &buffs, size_t num_samps,
    rx_metadata_t &metadata, const io_type_t &io_type,
    recv_mode_t recv_mode, double timeout
){
    _io_impl->recv_timeout = timeout;
    return vrt_packet_handler::recv(
        _io_impl->packet_handler_recv_state,       //last state of the recv handler
        buffs, num_samps,                          //buffer to fill
        metadata, recv_mode,                       //samples metadata
        io_type, _recv_otw_type,                   //input and output types to convert
        _clock_ctrl->get_fpga_clock_rate(),        //master clock tick rate
        uhd::transport::vrt::if_hdr_unpack_le,
        _io_impl->get_recv_buffs_fcn,
        boost::bind(&usrp_e100_impl::handle_overrun, this, _1)
    );
}

/***********************************************************************
 * Async Recv
 **********************************************************************/
bool usrp_e100_impl::recv_async_msg(
    async_metadata_t &async_metadata, double timeout
){
    boost::this_thread::disable_interruption di; //disable because the wait can throw
    return _io_impl->async_msg_fifo.pop_with_timed_wait(async_metadata, timeout);
}

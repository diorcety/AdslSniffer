/* -*- c++ -*- */
/* 
 * Copyright 2014 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "as_source_impl.h"

namespace gr {
  namespace adslsniffer {

    as_source::sptr
    as_source::make()
    {
      return gnuradio::get_initial_sptr
        (new as_source_impl());
    }
    
    const uint16_t as_source_impl::sVendorId = 0x04b4;
    const uint16_t as_source_impl::sDeviceId = 0x1004;
    const uint8_t as_source_impl::sEndpoint = 0x82;

    /*
     * The private constructor
     */
    as_source_impl::as_source_impl()
      : gr::sync_block("as_source",
              gr::io_signature::make(0, 0, 0),
              gr::io_signature::make(1, 1, sizeof(float)))
    {
    }

    /*
     * Our virtual destructor.
     */
    as_source_impl::~as_source_impl()
    {
    }

    int
    as_source_impl::work(int noutput_items,
			  gr_vector_const_void_star &input_items,
			  gr_vector_void_star &output_items)
    {
        float *out = (float *) output_items[0];

        // Do <+signal processing+>

        // Tell runtime system how many output items we produced.
        return noutput_items;
    }

  } /* namespace adslsniffer */
} /* namespace gr */


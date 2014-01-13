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

#ifndef INCLUDED_ADSLSNIFFER_AS_SOURCE_IMPL_H
#define INCLUDED_ADSLSNIFFER_AS_SOURCE_IMPL_H

#include <adslsniffer/as_source.h>
#include <USB.h>

namespace gr {
  namespace adslsniffer {

    class as_source_impl : public as_source
    {
     private:
      static const uint16_t sVendorId;
      static const uint16_t sDeviceId;
      static const uint8_t sEndpoint;
      USBContext mContext;
      USBDevice::Ptr mDevice; 

     public:
      as_source_impl();
      ~as_source_impl();

      double get_samp_rate(void);

      void flush(void);
      bool start(void);
      bool stop(void);

      int work(int noutput_items,
	       gr_vector_const_void_star &input_items,
	       gr_vector_void_star &output_items);
    };

  } // namespace adslsniffer
} // namespace gr

#endif /* INCLUDED_ADSLSNIFFER_AS_SOURCE_IMPL_H */


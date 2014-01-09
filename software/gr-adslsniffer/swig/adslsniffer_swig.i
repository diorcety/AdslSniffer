/* -*- c++ -*- */

#define ADSLSNIFFER_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "adslsniffer_swig_doc.i"

%{
#include "adslsniffer/as_source.h"
%}


%include "adslsniffer/as_source.h"
GR_SWIG_BLOCK_MAGIC2(adslsniffer, as_source);

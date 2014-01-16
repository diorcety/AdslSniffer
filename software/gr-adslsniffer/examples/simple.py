#!/usr/bin/env python
##################################################
# Gnuradio Python Flow Graph
# Title: Simple
# Author: Yann Diorcet
# Generated: Thu Jan 16 22:22:58 2014
##################################################

from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from optparse import OptionParser
import adslsniffer

class simple(gr.top_block):

    def __init__(self):
        gr.top_block.__init__(self, "Simple")

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 8832000

        ##################################################
        # Blocks
        ##################################################
        self.blocks_file_sink_0 = blocks.file_sink(gr.sizeof_float*1, "/tmp/aaa", False)
        self.blocks_file_sink_0.set_unbuffered(False)
        self.adslsniffer_as_source_0 = adslsniffer.as_source()

        ##################################################
        # Connections
        ##################################################
        self.connect((self.adslsniffer_as_source_0, 0), (self.blocks_file_sink_0, 0))


# QT sink close method reimplementation

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate

if __name__ == '__main__':
    parser = OptionParser(option_class=eng_option, usage="%prog: [options]")
    (options, args) = parser.parse_args()
    tb = simple()
    tb.start()
    raw_input('Press Enter to quit: ')
    tb.stop()
    tb.wait()


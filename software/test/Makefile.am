bin_PROGRAMS = adslsniffer_test

adslsniffer_test_CPPFLAGS = \
        -I$(top_srcdir)/lib/include \
	$(LIBUSB_CFLAGS)

adslsniffer_test_SOURCES = \
	adslsniffer_test.cpp

adslsniffer_test_LDADD = \
	$(top_builddir)/lib/libAdslSniffer.la \
	$(LIBUSB_LIBS)

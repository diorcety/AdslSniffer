/*
AdslSniffer
Copyright (C) 2013  Yann Diorcet <diorcet.yann@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cppunit/TextTestRunner.h>
#include <cppunit/XmlOutputter.h>

#include <gnuradio/unittests.h>
#include "qa_adslsniffer.h"
#include <iostream>

int
main (int argc, char **argv)
{
	CppUnit::TextTestRunner runner;
	std::ofstream xmlfile(get_unittest_path("adslsniffer.xml").c_str());
	CppUnit::XmlOutputter *xmlout = new CppUnit::XmlOutputter(&runner.result(), xmlfile);

	runner.addTest(qa_adslsniffer::suite());
	runner.setOutputter(xmlout);

	bool was_successful = runner.run("", false);

	return was_successful ? 0 : 1;
}

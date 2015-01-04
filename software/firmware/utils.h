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

#ifndef UTILS__
#define UTILS__

#include <delay.h>
#include <fx2macros.h>

// EPxCFG bits
#define bmBULK bmBIT5
#define bmISO bmBIT4
#define bmINT (bmBIT5 | bmBIT4)
#define bmBUF1024 bmBIT3
#define bmBUF2X bmBIT1
#define bmBUF4X 0
#define bmBUF3X (bmBIT0 | bmBIT1)

#define SYNCDELAY() SYNCDELAY4

#define bmEPxFULL(x) bmEP##x##FULL
#define bmEPxEMPTY(x) bmEP##x##EMPTY
#define EPxFIFOBUF(x) EP##x##FIFOBUF
#define EPxBUF(x) EP##x##BUF
#define EPxBCH(x) EP##x##BCH
#define EPxBCL(x) EP##x##BCL

#endif //UTILS__

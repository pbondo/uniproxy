//====================================================================
//
// Universal Proxy
//
// GateHouse Library for handling PGHP NMEA messages
//--------------------------------------------------------------------
//
// This version is released as part of the European Union sponsored
// project Mona Lisa work package 4 for the Universal Proxy Application
//
// This version is released under the GNU General Public License with restrictions.
// See the doc/license.txt file.
//
// Copyright (C) 2004-2012 by GateHouse A/S
// All Rights Reserved.
// http://www.gatehouse.dk
// mailto:gh@gatehouse.dk
//====================================================================
#ifndef _pghputils_h
#define _pghputils_h

#include <string>

std::string TrimNmeaString(const std::string& _s);

unsigned char CalcNmeaCheckSum(const std::string& _s, int _size = -1);

std::string FormatNmeaCheckSum(int _checksum);

#endif

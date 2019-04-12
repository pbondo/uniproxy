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
#include "pghputils.h"
#include "pghpgeneral.h"

#include <math.h>

using namespace std;


std::string TrimNmeaString(const std::string& _s)
{
   std::string s(_s);
   s = mylib::strip(s);
   
   int iPos = s.find('@');
   if (iPos >= 0)
   {
	  s = s.substr(iPos);
   }
   return s;
}

unsigned char CalcNmeaCheckSum(const std::string& _s, int _size)
{
	if (_size < 0)
	{
		_size = _s.size();
	}

	int offset = 0;
	if (_s[0] == '!' || _s[0] == '$')
	{
		offset = 1;
	}

	unsigned char sum = 0;
	for (int i = offset; i < _size && _s[i] != '*'; i++ ) 
	{
		sum ^= static_cast<unsigned char>(_s[i]);
	}
	return sum;
}

std::string FormatNmeaCheckSum(int _checksum)
{
	char buf[10];
	sprintf(buf, "%02X", _checksum);
	return std::string(buf);
}


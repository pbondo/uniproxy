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
// This version is released under the GPL version 3 open source License:
// http://www.gnu.org/copyleft/gpl.html
//
// Copyright (C) 2004-2012 by GateHouse A/S
// All Rights Reserved.
// http://www.gatehouse.dk
// mailto:gh@gatehouse.dk
//====================================================================
#include "pghpstartdatarequest.h"

using namespace std;

TclAISMessageStartDataRequest::TclAISMessageStartDataRequest()
: TclAISMessageInternalBase(AISMESGINT_STARTDATA_REQUEST)
{
}


TclAISMessageStartDataRequest::~TclAISMessageStartDataRequest()
{
}


bool TclAISMessageStartDataRequest::Decode(const std::string &clData)
{
	vector<uint8_t> clDataBuffer;
	int16_t iOffset=0;
	Ascii2Byte(clDataBuffer, clData);
	DecodeHeader(clDataBuffer, iOffset);
	return true;
}


bool TclAISMessageStartDataRequest::Encode(std::string &clData)
{
	vector<uint8_t> clDataBuffer;
	EncodeHeader(clDataBuffer);
	Byte2Ascii(clDataBuffer, clData);
	return true;
}

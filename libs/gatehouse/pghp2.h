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
#ifndef _pghp2_h
#define _pghp2_h

#include <cmath>
#include <sstream>

#include "pghpnmeamsg.h"

//------------------------------------------------------
//  class TclPGHP2Message
//------------------------------------------------------
/// Encode and decode a PGHP2 message
/**
   This class decodes/encodes the internal data format PGHP2.

   $PGHP,2,<Total number of sentences>,<Sentence number>,<Sequential message identifier>,<GH mail>*hh <CR><LF>
   
   Example: $PGHP,2,9,9,9,zxcvb12345*BE<CR><LF>
*/

class TclPGHP2Message : public TclNMEAMessage
{
public:
	TclPGHP2Message();
	bool Decode(const std::string &_msg);
	void EncodePart(std::string &_msg, const std::string &_clData);

	bool VerifyData();

	inline std::string GetGHMail() { return GetPayload(); }
	inline void SetGHMail(const std::string& _aszGHMail) { SetReceivedMessageModified(true); SetPayload(_aszGHMail); }

private:
	TclPGHP2Message(const TclPGHP2Message& source);
	TclPGHP2Message& operator = (const TclPGHP2Message& source);

	std::string m_clHeader;

};

#endif

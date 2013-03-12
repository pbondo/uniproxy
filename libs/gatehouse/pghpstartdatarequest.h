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
#ifndef _pghpstartdatarequest_h
#define _pghpstartdatarequest_h

#include "pghpinternalbase.h"


//------------------------------------------------------
//  class TclAISMessageStartDataRequest
//------------------------------------------------------
class TclAISMessageStartDataRequest: public TclAISMessageInternalBase
{
public:
   TclAISMessageStartDataRequest();
   virtual ~TclAISMessageStartDataRequest();
private:
   TclAISMessageStartDataRequest(const TclAISMessageStartDataRequest& source);
   TclAISMessageStartDataRequest& operator = (const TclAISMessageStartDataRequest& source);

public:
   bool Decode(const std::string &clData);
   bool Encode(std::string &clData);

private:

};

#endif

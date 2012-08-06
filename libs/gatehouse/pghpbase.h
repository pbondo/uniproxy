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
#ifndef _pghpbase_h
#define _pghpbase_h

#include "pghpgeneral.h"

#define _AISBASE_ENUM_TYPE_META(_mac) \
   _mac( AISBASE_UNDEFINED ) \
   _mac( AISBASE_IEC ) \
   _mac( AISBASE_GH_IEC_HEADER ) \
   _mac( AISBASE_GH_DATA )
CREATE_ENUM(AisBaseType, _AISBASE_ENUM_TYPE_META);


/**
TclAISMessageBase description
*/
class TclAISMessageBase 
{
public:
   TclAISMessageBase(TenAisBaseType enType);
   virtual ~TclAISMessageBase();
private:
   TclAISMessageBase(const TclAISMessageBase& source);
   TclAISMessageBase& operator = (const TclAISMessageBase& source);

   TenAisBaseType enAisBaseType;
};

#endif

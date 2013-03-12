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
#ifndef _pghplogoffrequest_h
#define _pghplogoffrequest_h

#include "pghpinternalbase.h"

/**
\file aismessagelogoffrequest.h
\brief missing comment
*/

#define _AISMESG_ENUM_LOGOFF_REASON(_mac) \
         _mac( LOGOFF_REASON_NO_REASON ) \
         _mac( LOGOFF_REASON_SERVER_BUSY )
CREATE_ENUM(AisLogoffReason, _AISMESG_ENUM_LOGOFF_REASON);


//------------------------------------------------------
//  class TclAISMessageLogoffRequest
//------------------------------------------------------
/// Short description
/**
No detailed description
*/
class TclAISMessageLogoffRequest: public TclAISMessageInternalBase
{
public:
   TclAISMessageLogoffRequest();
   virtual ~TclAISMessageLogoffRequest();

private:
   TclAISMessageLogoffRequest(const TclAISMessageLogoffRequest& source);
   TclAISMessageLogoffRequest& operator = (const TclAISMessageLogoffRequest& source);

public:
   bool Decode(const std::string &clData);
   bool Encode(std::string &clData);

   int GetReason() const { return m_iReason; } 
   void SetReason(int _iReason) { m_iReason = _iReason; }

private:
   int m_iReason;

};

#endif

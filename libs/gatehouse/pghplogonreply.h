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
#ifndef _pghplogonreply_h
#define _pghplogonreply_h

#include "pghpgeneral.h"
#include "pghpinternalbase.h"


#define _AISMESG_ENUM_LOGON_REPLY(_mac) \
   _mac( LOGON_REPLY_OK ) \
   _mac( LOGON_REPLY_ALLREADY_CONNECTED ) \
   _mac( LOGON_REPLY_USER_INVALID ) \
   _mac( LOGON_REPLY_PASSWORD_INVALID ) \
   _mac( LOGON_REPLY_PASSWORD_EXPIRED ) \
   _mac( LOGON_REPLY_LOCKED ) \
   _mac( LOGON_REPLY_DISABLED ) \
   _mac( LOGON_REPLY_NO_LSS_GROUP ) \
   _mac( LOGON_REPLY_LOAD_BALANCE_MOVE ) \
   _mac( LOGON_REPLY_SERVER_BUSY )   
CREATE_ENUM(AisLogonReply, _AISMESG_ENUM_LOGON_REPLY);



class TclAISMessageLogonReply : public TclAISMessageInternalBase
{
public:
   TclAISMessageLogonReply();
   virtual ~TclAISMessageLogonReply();
private:
   TclAISMessageLogonReply(const TclAISMessageLogonReply& source);
   TclAISMessageLogonReply& operator = (const TclAISMessageLogonReply& source);

public:
   bool Decode(const std::string &clData);
   bool Encode(std::string &clData);

   const TenAisLogonReply& GetLogonReply() const { return enLogonReply; } 
   void SetLogonReply(const TenAisLogonReply& _enLogonReply) { enLogonReply = _enLogonReply; }

   const std::string& GetNewHost() const { return m_clNewHost; } 
   void SetNewHost(const std::string& _clNewHost) { m_clNewHost = _clNewHost; }

   int GetPort() const { return m_iPort; } 
   void SetPort(int _iPort) { m_iPort = _iPort; }

private:
   TenAisLogonReply enLogonReply;
   std::string m_clNewHost;
   int m_iPort;
};

#endif

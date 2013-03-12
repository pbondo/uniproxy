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
#ifndef _pghplogonrequest_h
#define _pghplogonrequest_h

#include "pghpinternalbase.h"

#define _AISMESG_ENUM_LOGON_VERSION(_mac) \
   _mac( LOGON_VERSION_1 ) \
   _mac( LOGON_VERSION_2 ) \
   _mac( LOGON_VERSION_3 ) 
CREATE_ENUM(AisLogonVersion, _AISMESG_ENUM_LOGON_VERSION);

#define _AISMESG_ENUM_PROXY_TYPE(_mac) \
   _mac( SUBSCRIBER_PROXY ) \
   _mac( PROVIDER_PROXY ) 
CREATE_ENUM(AisProxyType, _AISMESG_ENUM_PROXY_TYPE);


class TclAISMessageLogonRequest : public TclAISMessageInternalBase
{
public:
   TclAISMessageLogonRequest();
   virtual ~TclAISMessageLogonRequest();
private:
   TclAISMessageLogonRequest(const TclAISMessageLogonRequest& source);
   TclAISMessageLogonRequest& operator = (const TclAISMessageLogonRequest& source);

public:
   bool Decode(const std::string &clData);
   bool Encode(std::string &clData);

   const std::string GetName() const { return m_clName; } 
   void SetName(const std::string& _clName) { m_clName = _clName; }

   const std::string GetPassword() const { return m_clPassword; } 
   void SetPassword(const std::string& _clPassword) { m_clPassword = _clPassword; }

   int GetVersion() const { return m_iVersion; } 
   void SetVersion(int _iVersion) { m_iVersion = _iVersion; }

   int GetRelogon() const { return m_iRelogon; } 
   void SetRelogon(int _iRelogon) { m_iRelogon = _iRelogon; }

   const std::string& GetVersionString() const { return m_clVersionString; } 
   void SetVersionString(const std::string& _clVersionString) { m_clVersionString = _clVersionString; }

   const std::string& GetLocalUser() const { return m_clLocalUser; } 
   void SetLocalUser(const std::string& _clLocalUser) { m_clLocalUser = _clLocalUser; }

   int GetProxyType() const { return m_iProxyType; } 
   void SetProxyType(int _iProxyType) { m_iProxyType = _iProxyType; }

   int GetRetryCounter() const { return m_iRetryCounter; } 
   void SetRetryCounter(int _iRetryCounter) { m_iRetryCounter = _iRetryCounter; }

private:
   std::string m_clName;
   std::string m_clPassword;
   std::string m_clVersionString;
   std::string m_clLocalUser;
   int m_iVersion;
   int m_iRelogon;
   int m_iProxyType;
   int m_iRetryCounter;

};

#endif


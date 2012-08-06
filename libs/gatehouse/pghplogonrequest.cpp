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
#include "pghplogonrequest.h"

using namespace std;

TclAISMessageLogonRequest::TclAISMessageLogonRequest()
   : TclAISMessageInternalBase(AISMESGINT_LOGON_REQUEST)
{
   m_iVersion=0;
   m_iRelogon=0;
   m_iProxyType=0;
   m_iRetryCounter=0;
}


TclAISMessageLogonRequest::~TclAISMessageLogonRequest()
{

}


bool TclAISMessageLogonRequest::Decode(const std::string &clData)
{
   vector<uint8_t> clDataBuffer;
   int16_t iOffset=0; 
   Ascii2Byte(clDataBuffer, clData);
   
	for (int i = 0; i < clDataBuffer.size(); i++ )
	{
		std::cout << clDataBuffer[i] ;
	}
	std::cout << std::endl;
	
   DecodeHeader(clDataBuffer, iOffset);
   DecodeString(clDataBuffer, iOffset, "NAME", m_clName);
   to_lower( m_clName );
   
   DecodeString(clDataBuffer, iOffset, "PWD", m_clPassword);
   DecodeInteger(clDataBuffer, iOffset, "VER", m_iVersion);
   DecodeInteger(clDataBuffer, iOffset, "RLOG", m_iRelogon);
   DecodeString(clDataBuffer, iOffset, "LUSER", m_clLocalUser);
   DecodeString(clDataBuffer, iOffset, "VERSTR", m_clVersionString);
   DecodeInteger(clDataBuffer, iOffset, "TYPE", m_iProxyType);
   DecodeInteger(clDataBuffer, iOffset, "RETRY", m_iRetryCounter);

   return true;
}


bool TclAISMessageLogonRequest::Encode(std::string &clData)
{
   vector<uint8_t> clDataBuffer;
   EncodeHeader(clDataBuffer);
   to_lower( m_clName );
   EncodeString(clDataBuffer, "NAME", m_clName);
   EncodeString(clDataBuffer, "PWD", m_clPassword);
   EncodeInteger(clDataBuffer, "VER", m_iVersion);
   EncodeInteger(clDataBuffer, "RLOG", m_iRelogon);
   EncodeString(clDataBuffer, "LUSER", m_clLocalUser);
   EncodeString(clDataBuffer, "VERSTR", m_clVersionString);
   EncodeInteger(clDataBuffer, "TYPE", m_iProxyType);
   EncodeInteger(clDataBuffer, "RETRY", m_iRetryCounter);

   Byte2Ascii(clDataBuffer, clData);

   return true;
}

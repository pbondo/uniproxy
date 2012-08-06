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
#include "pghplogonreply.h"

using namespace std;

TclAISMessageLogonReply::TclAISMessageLogonReply()
   : TclAISMessageInternalBase(AISMESGINT_LOGON_REPLY)
{
   enLogonReply=LOGON_REPLY_OK;
   m_iPort=0;
}

TclAISMessageLogonReply::~TclAISMessageLogonReply()
{

}


bool TclAISMessageLogonReply::Decode(const std::string &clData)
{
   vector<uint8_t> clDataBuffer;
   int16_t iOffset=0; 
   Ascii2Byte(clDataBuffer, clData);

   DecodeHeader(clDataBuffer, iOffset);

   int32_t iTmp=0;
   DecodeInteger(clDataBuffer, iOffset, "RESULT", iTmp);
   DecodeString(clDataBuffer, iOffset, "NEWHOST", m_clNewHost);
   DecodeInteger(clDataBuffer, iOffset, "PORT", m_iPort);

   enLogonReply = (TenAisLogonReply) iTmp;

   return true;
}

bool TclAISMessageLogonReply::Encode(std::string &clData)
{
   vector<uint8_t> clDataBuffer;
   EncodeHeader(clDataBuffer);
   EncodeInteger(clDataBuffer, "RESULT", enLogonReply);
   EncodeString(clDataBuffer, "NEWHOST", m_clNewHost);
   EncodeInteger(clDataBuffer, "PORT", m_iPort);

   Byte2Ascii(clDataBuffer, clData);

   return true;
}

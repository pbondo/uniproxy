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
#include "pghpnmeamsg.h"
#include "pghputils.h"

#include <math.h>

using namespace std;

ENUM_TO_STR_ARRAY(AISNMEAType, _AISNMEATYPE_ENUM_TYPE_META);

STR_FROM_ENUM(AISNMEAType)


//------------------------------------------------------
//  class TclNMEAMessage
//------------------------------------------------------

TclNMEAMessage::TclNMEAMessage(TenAISNMEAType _enSentenceType)
{
   m_fHasError=false;
   m_enSentenceType = _enSentenceType;
   m_fReceivedMessageModified=false;
   m_iMaxContent=0;

   m_iTotalNumberOfMessages=1;
   m_iSentenceNumber=1;
   m_iSequentialMessageIdentifier=-1;
   m_iCheckSum=0;

   m_fSeqMesIdIsNull = false;
}

TclNMEAMessage::~TclNMEAMessage()
{
   m_iCheckSum = 0xDEADBEEF;
}


bool TclNMEAMessage::Verify(const std::string &_msg)
{

   // Find the first NMEA sentence in input paramenter 
   // (and verify that sentence delimmiters "!"/"$" and "\r\n" are present)
   size_t end = _msg.find("\r\n");
   if(end == std::string::npos)
   {
      return false;
   }      

   size_t start = _msg.find("!");
   if(start == std::string::npos || start > end)
   {
      start = _msg.find("$");
      if(start == std::string::npos || start > end)
      {
         return false;
      }      
   }
   std::string sentence = _msg.substr(static_cast<int>(start), end-start+2);
   if(!VerifyCheckSum(sentence))
   {
      return false;
   }
   return true;
}


std::string TclNMEAMessage::CalcCheckSumAsString(const std::string& _s, int _size)
{
	m_iCheckSum = CalcNmeaCheckSum(_s, _size);
	return FormatNmeaCheckSum(m_iCheckSum);
	std::string sz;
	return sz;
}


int TclNMEAMessage::CalcCheckSum(const std::string& _s, int _size)
{
   m_iCheckSum = CalcNmeaCheckSum(_s, _size);
   m_iCheckSum = 0;

   return m_iCheckSum;
}


bool TclNMEAMessage::VerifyCheckSum(const std::string &_msg)
{
   int i = static_cast<int>(_msg.find("*"));

   if (i<=0 || i == string::npos)
   {
      return false;
   }
   
   std::string chksum = CalcCheckSumAsString(_msg, i);
   std::string chksum1 = _msg.substr(i+1, 2);
   if (chksum != chksum1)
   {
      AISERR("Wrong checksum: Calculated " << chksum << ", actual " << chksum1);
      return false;
   }

   return true;
}


/// Assumes that _msg holds only a single message terminated by <CR><LF>
/// and that everything before <CR><LF> and after the last '*' before <CR><LF>
/// is the checksum. If the checksum consists of more than 2 characters this method
/// will return an empty string
std::string TclNMEAMessage::ExtractChecksumFromMessage(const std::string& _msg) const
{
   int i =static_cast<int>(_msg.find_last_of("*"));
   int j = static_cast<int>(_msg.find_last_of("\r\n"));

   if (i<=0 || i == string::npos || i >= j)
   {
      return "";
   }
   
   return _msg.substr(i+1, 2);
}


// Segments the payload stored in m_aszPayload into chucks with the maximum size m_iMaxContent.
// If the payload size is <= m_iMaxContent only a single segment will be generated
// if the payload size is > m_iMaxContent 2 or more segments will be generated with a maximum of m_iMaxContent
// bytes payload per segment.
// Expects the subclass to implement EncodePart(...) which will build the next segment using the payload transfered as one
// of the parameters. That is, EncodePart(...) must keep track of sequence numbers. Also the subclass must set m_iMaxContent!
void TclNMEAMessage::Encode(std::string &_msg)
{
   m_iTotalNumberOfMessages = 0;
   m_iSentenceNumber        = 1;

   int iLen = static_cast<int>(m_aszPayload.size());
   int iFullSizeCount = iLen/m_iMaxContent;

   m_iTotalNumberOfMessages=iFullSizeCount;
   if (iLen%m_iMaxContent!=0)
   {
      m_iTotalNumberOfMessages+=1;
   }

   if (m_iTotalNumberOfMessages>1 && m_iSequentialMessageIdentifier<0)
   {
      m_iSequentialMessageIdentifier=0;
   }

   int i;
   int iMaxPayloadLength = m_iMaxContent;

   for (i=0;i<iFullSizeCount;i++)
   {
      EncodePart(_msg, m_aszPayload.substr(i*iMaxPayloadLength, iMaxPayloadLength));
      m_iSentenceNumber++;
   }

   if (iLen%m_iMaxContent!=0)
   {
       EncodePart(_msg, m_aszPayload.substr(i*iMaxPayloadLength, iLen - (i*iMaxPayloadLength)));
   }

}


std::string TclNMEAMessage::GetDoubleAsString( const double _flVal )
{
   if(_flVal != 0.0)
   {
      return mylib::to_string(_flVal);
   }
   return "";
}


void TclNMEAMessage::EncodePart(std::string &_msg, const std::string &_clData)
{
}

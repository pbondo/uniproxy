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
#include "pghp2.h"

//------------------------------------------------------
//  class TclPGHP2Message
//------------------------------------------------------
TclPGHP2Message::TclPGHP2Message()
   : TclNMEAMessage(AISNMEATYPE_PGHP2)
{
   m_clHeader="$PGHP,2";
   m_iMaxContent = 512; //!!PCH changed from 63
}

void TclPGHP2Message::EncodePart(std::string &_msg, const std::string &_clData)
{
   std::string clSubMesg;

   clSubMesg = m_clHeader + ',';
   clSubMesg += (char) m_iTotalNumberOfMessages + '0';
   clSubMesg += ',';
   clSubMesg += (char) m_iSentenceNumber + '0';
   clSubMesg += ',';

   if (m_iSequentialMessageIdentifier>=0)
   {
      clSubMesg += (char) m_iSequentialMessageIdentifier + '0';
   }

   clSubMesg += ',';
   clSubMesg += _clData;
   clSubMesg += '*';

   clSubMesg += CalcCheckSumAsString(clSubMesg);
   clSubMesg += "\r\n";

   _msg += clSubMesg;
}



bool TclPGHP2Message::Decode(const std::string &_msg)
{

   // the message may contain several segments
   std::string buf = _msg;
   int pos;
   std::string clNewReceivedMessage;
   std::string clNewHeader;
   int iNewTotalNumberOfMessages = 0;
   int iNewSentenceNumber = 0;
   int iNewSequentialMessageIdentifier = 0;
   std::string aszNewPayload;
   uint32_t uiSegCount = 0;
   while( (pos = buf.find_first_of('\n')) != -1)	// .Index
   {
      std::string clMessage = buf.substr(0,pos+1); //.Left
      buf = buf.substr(pos+1,buf.length());

      if (!VerifyCheckSum(clMessage))
      {
         return false;
      }

      uiSegCount++; // For keeping track of the number of segments received

      int iLen=static_cast<int>(clMessage.length());
      clNewReceivedMessage += clMessage;

      clNewHeader = clMessage.substr(0,7);
      int iStart=8;
      if(clNewHeader == m_clHeader)
      {
         const char *pclData = clMessage.c_str();
         GetSmallInt(iNewTotalNumberOfMessages, pclData, iLen, iStart);
         GetSmallInt(iNewSentenceNumber, pclData, iLen, iStart);
         GetSmallInt(iNewSequentialMessageIdentifier, pclData, iLen, iStart);
         std::string clTmpPayload;
         GetString(clTmpPayload, pclData, iLen, iStart);
         aszNewPayload += clTmpPayload;
      }
      else
      {
         return false;
      }
   }

   // Set member variables taken from the last decoded segment
   m_iTotalNumberOfMessages         = iNewTotalNumberOfMessages;
   m_iSentenceNumber                = iNewSentenceNumber;
   m_iSequentialMessageIdentifier   = iNewSequentialMessageIdentifier;

   // Set the received message built from all of the segments.
   m_clReceivedMessage              = clNewReceivedMessage;
   m_aszPayload                     = aszNewPayload;
   return true;

}

bool TclPGHP2Message::VerifyData()
{

   if (GetTotalNumberOfMessages()<1 || GetTotalNumberOfMessages()>9)
   {
      return false;
   }

   if (GetSentenceNumber()<1 || GetSentenceNumber()>9)
   {
      return false;
   }

   if (GetSequentialMessageIdentifier()<0 || GetSequentialMessageIdentifier()>9)
   {
      return false;
   }

   //if (GetGHMail().size() > 84) 
   //{
   //   return false;
   //}

   return true;
}



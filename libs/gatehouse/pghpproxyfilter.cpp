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
#include "pghpproxyfilter.h"
#include "pghpxmlutil.h"

using namespace std;


TclAISMessageProxyFilter::TclAISMessageProxyFilter()
: TclAISMessageInternalBase(AISMESGINT_PROXY_FILTER)
{
}

TclAISMessageProxyFilter::~TclAISMessageProxyFilter()
{
}

bool TclAISMessageProxyFilter::Decode(const std::string &clData)
{
   vector<uint8_t> clDataBuffer;
   int16_t iOffset=0; 
   Ascii2Byte(clDataBuffer, clData);
   DecodeHeader(clDataBuffer, iOffset);
   
   int iBool = 0;
   // Time stamp
   DecodeInteger(clDataBuffer, iOffset, "TS", iBool);
   m_fTimeStamp = iBool != 0;
   DecodeInteger(clDataBuffer, iOffset, "UI", m_iUpdateInterval);
    
   // Message types
   std::string clMsgVector;
   DecodeString(clDataBuffer, iOffset, "MSGS", clMsgVector);
   SplitVectorValue(clMsgVector, m_clMessageTypes);
   DecodeInteger(clDataBuffer, iOffset, "MSGSD", iBool);
   m_fMessageTypesDefined = iBool != 0;

   // MMSI List
   std::string clMMSIVector;
   DecodeString(clDataBuffer, iOffset, "ML", clMMSIVector);
   SplitVectorValue(clMMSIVector, m_clMMSIList);
   DecodeInteger(clDataBuffer, iOffset, "MLD", iBool);
   m_fMMSIListDefined = iBool != 0;
   DecodeInteger(clDataBuffer, iOffset, "MLC", iBool);
   m_fIncludeMMSIListContent = iBool != 0;

   // User Defined areas
   std::string clUAreaVector;
   DecodeString(clDataBuffer, iOffset, "UAREA", clUAreaVector);
   SplitVectorValue(clUAreaVector, m_clUserDefinedArea);
   DecodeInteger(clDataBuffer, iOffset, "UAREAD", iBool);
   m_fUserDefinedAreaDefined = iBool != 0;

   return true;
}

bool TclAISMessageProxyFilter::Encode(std::string &clData)
{
   vector<uint8_t> clDataBuffer;
   EncodeHeader(clDataBuffer);

   // Time stamp
   EncodeInteger(clDataBuffer, "TS", m_fTimeStamp);
   EncodeInteger(clDataBuffer, "UI", m_iUpdateInterval);
   
   // Message types
   std::string clMsgVector;
   CreateVectorValue(clMsgVector, m_clMessageTypes);
   EncodeString(clDataBuffer, "MSGS", clMsgVector);
   EncodeInteger(clDataBuffer, "MESGD", m_fMessageTypesDefined);

   // MMSI List
   std::string clMMSIVector;
   CreateVectorValue(clMMSIVector, m_clMMSIList);
   EncodeString(clDataBuffer, "ML", clMMSIVector);
   EncodeInteger(clDataBuffer, "MLD", m_fMMSIListDefined);
   EncodeInteger(clDataBuffer, "MLC", m_fIncludeMMSIListContent);

   // User defined areas
   std::string clUAreaVector;
   CreateVectorValue(clUAreaVector, m_clUserDefinedArea);
   EncodeString(clDataBuffer, "UAREA", clUAreaVector);
   EncodeInteger(clDataBuffer, "UAREAD", m_fUserDefinedAreaDefined);

   Byte2Ascii(clDataBuffer, clData);
   return true;
}




//======================================================
//                    TclAISMessageLogoffRequest
//
//                       GH\AIS
//------------------------------------------------------
//   $Author: tma $
// $Revision: 1.3 $
//     $Date: 2006/10/06 14:48:42 $
//------------------------------------------------------
// Confidential
// 
// Copyright © 2004 by GateHouse A/S Denmark
// All Rights Reserved.
// http://www.gatehouse.dk
//======================================================

//#include "stdafx.h"
#include "pghplogoffrequest.h"

using namespace std;

TclAISMessageLogoffRequest::TclAISMessageLogoffRequest()
   :  TclAISMessageInternalBase(AISMESGINT_LOGOFF_REQUEST),
      m_iReason(LOGOFF_REASON_NO_REASON)
{
}

TclAISMessageLogoffRequest::~TclAISMessageLogoffRequest()
{
}

bool TclAISMessageLogoffRequest::Decode(const std::string &clData)
{
   vector<uint8_t> clDataBuffer;
   int16_t iOffset=0; 
   Ascii2Byte(clDataBuffer, clData);

   DecodeHeader(clDataBuffer, iOffset);
 
   DecodeInteger(clDataBuffer, iOffset, "R", m_iReason);

   return true;
}

bool TclAISMessageLogoffRequest::Encode(std::string &clData)
{
   vector<uint8_t> clDataBuffer;
   EncodeHeader(clDataBuffer);

   EncodeInteger(clDataBuffer, "R", m_iReason);
   Byte2Ascii(clDataBuffer, clData);

   return true;
}





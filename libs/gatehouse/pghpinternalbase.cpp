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
#include "pghpinternalbase.h"


TclAISMessageInternalBase::TclAISMessageInternalBase(TenAisMesgInternalType _enType)
   : TclAISMessageBase(AISBASE_GH_DATA), enType(_enType)
{

}

TclAISMessageInternalBase::~TclAISMessageInternalBase()
{

}

void TclAISMessageInternalBase::EncodeString( std::vector<uint8_t> &clDataBuffer, const std::string &clName,
                                             const std::string &clData)
{
   BuildString(clDataBuffer, ASN_STR, clName);
   BuildString(clDataBuffer, ASN_STR, clData);
}

void TclAISMessageInternalBase::EncodeHeader(std::vector<uint8_t> &clDataBuffer)
{
   BuildInt(clDataBuffer, ASN_INTEGER, GetType());
}

void TclAISMessageInternalBase::DecodeHeader(std::vector<uint8_t> &clDataBuffer,
                                             int16_t &iOffset)
{
   uint8_t iType=0;
   int32_t iMesgType=0;

   ParseInt(clDataBuffer, iOffset, iType, iMesgType);

   if (GetType() == AISMESGINT_UNKNOWN)
   {
      SetType((TenAisMesgInternalType)iMesgType);
   }
}

void TclAISMessageInternalBase::EncodeInteger(std::vector<uint8_t> &clDataBuffer,
                                              const std::string &clName, 
                                              int32_t iData)
{
   BuildString(clDataBuffer, ASN_STR, clName);
   BuildInt(clDataBuffer, ASN_INTEGER, iData);
}


void TclAISMessageInternalBase::EncodeInteger(std::vector<uint8_t>& _dataBuffer, int32_t _data)
{
   BuildInt(_dataBuffer, ASN_INTEGER, _data);
}


void TclAISMessageInternalBase::EncodeDouble(std::vector<uint8_t>& _dataBuffer, double _data)
{
   BuildDouble(_dataBuffer, ASN_DOUBLE, _data);
}


void TclAISMessageInternalBase::EncodeDateTime(std::vector<uint8_t> &clDataBuffer, const std::string &clName, const boost::posix_time::ptime &clDT)
{
   BuildString(clDataBuffer, ASN_DATETIME, clName);
   BuildInt(clDataBuffer, ASN_INTEGER, clDT.date().year() );
   BuildInt(clDataBuffer, ASN_INTEGER, clDT.date().month() );
   BuildInt(clDataBuffer, ASN_INTEGER, clDT.date().day() );
   BuildInt(clDataBuffer, ASN_INTEGER, clDT.time_of_day().hours() );
   BuildInt(clDataBuffer, ASN_INTEGER, clDT.time_of_day().minutes() );
   BuildInt(clDataBuffer, ASN_INTEGER, clDT.time_of_day().seconds() );
   BuildInt(clDataBuffer, ASN_INTEGER, 0 ); // milliseconds
}


void TclAISMessageInternalBase::DecodeInteger(std::vector<uint8_t> &clDataBuffer, int16_t &iOffset, const std::string &, int32_t &iData)
{
   std::string clTmpName;
   uint8_t iType=0;

   ParseString(clDataBuffer, iOffset, iType, clTmpName);
   ParseInt(clDataBuffer, iOffset, iType, iData);
}


void TclAISMessageInternalBase::DecodeInteger(std::vector<uint8_t>& _dataBuffer, int16_t& _offset, int32_t& _data)
{
   uint8_t type = 0;
   ParseInt(_dataBuffer, _offset, type, _data);
}


void TclAISMessageInternalBase::DecodeDouble(std::vector<uint8_t>& _dataBuffer, int16_t& _offset, double& _data)
{
   uint8_t type = 0;
   ParseDouble(_dataBuffer, _offset, type, _data);
}


void TclAISMessageInternalBase::DecodeString(std::vector<uint8_t> &clDataBuffer,
                                             int16_t &iOffset, 
                                             const std::string &clName, 
                                             std::string &clData)
{
   std::string clTmpName;
   uint8_t iType=0;

   ParseString(clDataBuffer, iOffset, iType, clTmpName);

   if (clName != clTmpName)
   {
      AISERR(clName << "!="  << clTmpName);
   }

   ParseString(clDataBuffer, iOffset, iType, clData);
}


void TclAISMessageInternalBase::BuildInt(std::vector<uint8_t> &clData, uint8_t iType, int32_t iIntData)
{
   int32_t iRes=0;
   uint32_t iMask=0;
   uint8_t iSize = sizeof(int32_t);

   iRes = iIntData;
   
   // Truncate "unnecessary" bytes off of the most significant end of this
   // 2's complement integer.  There should be no sequence of 9
   // consecutive 1's or 0's at the most significant end of the
   // integer.
   
   iMask = 0x1FF << ((8 * (sizeof(int32_t) - 1)) - 1);

   // iMask is 0xFF800000 on a big-endian machine 
   while((((iRes & iMask) == 0) || ((iRes & iMask) == iMask)) && iSize > 1)
   {
      iSize--;
      iRes &= ~iMask;
      iRes <<= 8;
   }

   BuildHeader(clData, iType, iSize);

   iMask = 0xFF << (8 * (sizeof(int32_t) - 1));

   // iMask is 0xFF000000 on a big-endian machine 
   while(iSize--)
   {
      clData.push_back( (uint8_t)((iRes & iMask) >> (8 * (sizeof(int32_t) - 1))));
      iRes &= ~iMask;
      iRes <<= 8;
   }
}


void TclAISMessageInternalBase::BuildDouble(std::vector<uint8_t>& _data, uint8_t _type, double _val)
{
   // No need to store size, as it is fixed
   _data.push_back(_type);

   const unsigned char* p = reinterpret_cast<const unsigned char*>(&_val);

   // iMask is 0xFF000000 on a big-endian machine 
   int size = sizeof(double);
   while (size--)
   {
      _data.push_back(*p++);
   }
}


void TclAISMessageInternalBase::BuildHeader(std::vector<uint8_t> &clData, uint8_t iType, uint32_t iLength)
{
  clData.push_back(iType);
  BuildLength(clData, iLength);
} 

void TclAISMessageInternalBase::BuildLength(std::vector<uint8_t> &clData, uint32_t iLength)
{
   // no indefinite lengths sent 
   if (iLength < 0x80)
   {
      clData.push_back((uint8_t)iLength);
   } 
   else if (iLength <= 0xFF)
   {
      clData.push_back((uint8_t)(0x01 | ASN_LONG_LEN));
      clData.push_back((uint8_t)iLength);
   }
   else  
   {
      // 0xFF < length <= 0xFFFF 
      clData.push_back((uint8_t)(0x02 | ASN_LONG_LEN));
      clData.push_back((uint8_t)((iLength >> 8) & 0xFF));
      clData.push_back((uint8_t)(iLength & 0xFF));
   }
} 

void TclAISMessageInternalBase::BuildString(std::vector<uint8_t> &clData, uint8_t iType, const std::string &clStr)
{
   // ASN.1 octet string ::= primstring | cmpdstring
   // primstring ::= 0x04 asnlength byte {byte}*
   // cmpdstring ::= 0x24 asnlength string {string}*
   // This code will never send a compound string.
   
   BuildHeader(clData, iType, (uint32_t)clStr.length());

   for (int i=0;i<clStr.length();++i)
   {   
      clData.push_back(clStr[i]);
   }
}

bool TclAISMessageInternalBase::ParseHeader(std::vector<uint8_t>& clData, int16_t& iOffset, uint8_t& iType, uint32_t& iLength)
{
   if (iOffset >= clData.size())
   {
      AISERR("Exceeded data size: Offset " << iOffset << " Size " << clData.size());
      return false;
   }
   iType = clData[iOffset++];   
   return ParseLength( clData, iOffset, iLength);
}


bool TclAISMessageInternalBase::ParseLength(const std::vector<uint8_t>& _clData, int16_t& _iOffset, uint32_t& _iLength)
{
   if (_iOffset >= _clData.size())
   {
      AISERR("Exceeded data size: Offset " << _iOffset << " Size " << _clData.size());
      return false;
   }
   uint8_t bLength = _clData[_iOffset];

   if (bLength & ASN_LONG_LEN)
   {
      bLength &= ~ASN_LONG_LEN;
      if (bLength == 0)
      {
         AISERR("Indefinite length not supported");
         return false;
      }
      if (bLength > sizeof(long))
      {
         AISERR("Length too high " << bLength);
         return false;
      }

      if (_iOffset + bLength >= _clData.size())
      {
         AISERR("Overflow: Data size " << _clData.size() << ", offset " << _iOffset << ", length size " << bLength);
         return false;
      }
      _iLength=0;
      int iCnt=0;
      for (int i=bLength-1; i>=0; i--)
      {
         _iLength |= (_clData[_iOffset + 1 + i] << (8*iCnt++));
      }

      _iOffset += bLength + 1;
      return true;
   }
   _iLength = (uint32_t) bLength;
   _iOffset += 1;
   return true;
}; 


bool TclAISMessageInternalBase::ParseString(const std::vector<uint8_t>& _clData,
                                            int16_t& _iOffset,
                                            uint8_t& _iType,
                                            std::string& _clString)
{
   if (_iOffset >= _clData.size())
   {
      AISERR("Exceeded data size: Offset " << _iOffset << " Size " << _clData.size());
      return false;
   }

   _iType = _clData[_iOffset++];
   uint32_t iParsedLength = 0;
   if (!ParseLength(_clData, _iOffset, iParsedLength))
   {
      return false;
   }

   if (_iOffset+iParsedLength > _clData.size())
   {
      AISERR("Overflow: Data size " << _clData.size() << ", offset " << _iOffset << ", parsed length " << iParsedLength);
      return false;
   }

   while (iParsedLength--)
   {
      _clString.append(1, (char) _clData[_iOffset++]);
   }

   return true; 
}


bool TclAISMessageInternalBase::ParseInt(std::vector<uint8_t> &clData,
                                         int16_t &iOffset, 
                                         uint8_t &iType,
                                         int32_t &iInt)
{
   if (iOffset >= clData.size())
   {
      AISERR("Exceeded data size: Offset " << iOffset << " Size " << clData.size());
      return false;
   }

   iType = clData[iOffset++];
   uint32_t iParsedLength=0;
   if (!ParseLength(clData, iOffset, iParsedLength))
   {
      return false;
   }

   if (iParsedLength==0 || iParsedLength>4)
   {
      AISERR("Wrong length " << iParsedLength);
      return false;
   }

   if (iOffset+iParsedLength > clData.size())
   {
      AISERR("Overflow: Data size " << clData.size() << ", offset " << iOffset << ", parsed length " << iParsedLength);
      return false;
   }

   iInt=0;
   if (clData[iOffset] & 0x80)
   {
      iInt = -1; // integer is negative 
   }
   
   while(iParsedLength--)
   {
      iInt = (iInt << 8) | clData[iOffset++];
   }

   return true;
}


bool TclAISMessageInternalBase::ParseDouble(std::vector<uint8_t>& _data, int16_t& _offset, uint8_t& _type, double& _val)
{
   if (_offset >= _data.size())
   {
      AISERR("Exceeded data size: Offset " << _offset << " Size " << _data.size());
      return false;
   }

   _type = _data[_offset++];
   uint32_t parsedLength = sizeof(double);  // Fixed size

   if (_offset+parsedLength > _data.size())
   {
      AISERR("Overflow: Data size " << _data.size() << ", offset " << _offset << ", required " << parsedLength);
      return false;
   }

   _val = *reinterpret_cast<double*>(&(_data[_offset]));

   _offset += parsedLength;

   return true;
}


bool TclAISMessageInternalBase::Decode(const std::string &clData)
{
   std::vector<uint8_t> clDataBuffer;
   int16_t iOffset=0; 
   Ascii2Byte(clDataBuffer, clData);

   DecodeHeader(clDataBuffer, iOffset);
   return true;
}

bool TclAISMessageInternalBase::Encode(std::string &)
{
   return false;
}

void TclAISMessageInternalBase::Ascii2Byte(std::vector<uint8_t> &clDataBuffer, const std::string &clData)
{
   for (int i=0;i<clData.length();i+=2)
   {
      int iData=0; 
      char buf[3] = {clData[i], clData[i+1], 0};
      sscanf(buf, "%02X", &iData);
      clDataBuffer.push_back(iData);
   }
}

void TclAISMessageInternalBase::Byte2Ascii(const std::vector<uint8_t> &clDataBuffer, std::string &clData)
{
   char buf[10];
   for (int i=0;i<clDataBuffer.size();i++)
   {
      sprintf(buf, "%02X", clDataBuffer[i]);
      clData += buf;
   }
}

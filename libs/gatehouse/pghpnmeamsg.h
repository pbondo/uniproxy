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
#ifndef _pghpnmeamsg_h
#define _pghpnmeamsg_h

#include <boost/shared_ptr.hpp>
#include <gatehouse/pghpgeneral.h>

#define _AISNMEATYPE_ENUM_TYPE_META(_mac) \
   _mac( AISNMEATYPE_PGHP1 ) \
   _mac( AISNMEATYPE_PGHP2 ) \
   _mac( AISNMEATYPE_PGHP3 ) \
   _mac( AISNMEATYPE_PGHP4 ) \
   _mac( AISNMEATYPE_PGHP5 ) \
   _mac( AISNMEATYPE_PGHP6 ) \
   _mac( AISNMEATYPE_PGHP7 ) \
   _mac( AISNMEATYPE_PGHP8 ) \
   _mac( AISNMEATYPE_PGHP9 ) \
   _mac( AISNMEATYPE_PGHP10 ) \
   _mac( AISNMEATYPE_PGHP11 ) \
   _mac( AISNMEATYPE_PGHP12 ) \
   _mac( AISNMEATYPE_PGHP13 ) \
   _mac( AISNMEATYPE_PGHP14 ) \
   _mac( AISNMEATYPE_PGHP15 ) \
   _mac( AISNMEATYPE_PGHP16 ) \
   _mac( AISNMEATYPE_PGHP17 ) \
   _mac( AISNMEATYPE_PGHP18 ) \
   _mac( AISNMEATYPE_PGHP19 ) \
   _mac( AISNMEATYPE_PGHP20 ) \
   _mac( AISNMEATYPE_PGHP21 ) \
   _mac( AISNMEATYPE_PGHP22 ) \
   _mac( AISNMEATYPE_PGHP23 ) \
   _mac( AISNMEATYPE_PGHP24 ) \
   _mac( AISNMEATYPE_PGHP25 ) \
   _mac( AISNMEATYPE_PGHP26 ) \
   _mac( AISNMEATYPE_PGHP27 ) \
   _mac( AISNMEATYPE_PGHP28 ) \
   _mac( AISNMEATYPE_PGHP29 ) \
   _mac( AISNMEATYPE_PGHP30 ) \
   _mac( AISNMEATYPE_PGHP31 ) \
   _mac( AISNMEATYPE_PGHP32 ) \
   _mac( AISNMEATYPE_PGHP33 ) \
   _mac( AISNMEATYPE_PGHP34 ) \
   _mac( AISNMEATYPE_PGHP35 ) \
   _mac( AISNMEATYPE_PGHP36 ) \
   _mac( AISNMEATYPE_PGHP37 ) \
   _mac( AISNMEATYPE_PGHP38 ) \
   _mac( AISNMEATYPE_PGHP39 ) \
   _mac( AISNMEATYPE_PGHP40 ) \
   _mac( AISNMEATYPE_PGHP41 ) \
   _mac( AISNMEATYPE_PGHP42 ) \
   _mac( AISNMEATYPE_PGHP43 ) \
   _mac( AISNMEATYPE_PGHP44 ) \
   _mac( AISNMEATYPE_PGHP45 ) \
   _mac( AISNMEATYPE_PGHP46 ) \
   _mac( AISNMEATYPE_PGHP47 ) \
   _mac( AISNMEATYPE_PGHP48 ) \
   _mac( AISNMEATYPE_PGHP49 ) \
   _mac( AISNMEATYPE_PGHP50 ) \
   _mac( AISNMEATYPE_PGHP51 ) \
   _mac( AISNMEATYPE_UNDEFINED ) \
   _mac( AISNMEATYPE_VDM ) \
   _mac( AISNMEATYPE_VDO ) \
   _mac( AISNMEATYPE_ABM ) \
   _mac( AISNMEATYPE_BBM ) \
   _mac( AISNMEATYPE_ALR ) \
   _mac( AISNMEATYPE_AIR ) \
   _mac( AISNMEATYPE_UNKNOWN ) \
   _mac( AISNMEATYPE_TEXT ) \
   _mac( AISNMEATYPE_VDO_DESEGMENTED ) \
   _mac( AISNMEATYPE_VDM_DESEGMENTED ) \
   _mac( AISNMEATYPE_ABM_DESEGMENTED) \
   _mac( AISNMEATYPE_BBM_DESEGMENTED) \
   _mac( AISNMEATYPE_PGHP2_DESEGMENTED) \
   _mac( AISNMEATYPE_PGHP20_DESEGMENTED) \
   _mac( AISNMEATYPE_PGHP21_DESEGMENTED) \
   _mac( AISNMEATYPE_PGHP36_DESEGMENTED) \
   _mac( AISNMEATYPE_PGHP48_DESEGMENTED) \
   _mac( AISNMEATYPE_PGHP20_COMBINED) \
   _mac( AISNMEATYPE_PGHP21_COMBINED) \
   _mac( AISNMEATYPE_PGHP22_COMBINED) \
   _mac( AISNMEATYPE_PGHP30_COMBINED) \
   _mac( AISNMEATYPE_PGHP33_COMBINED) \
   _mac( AISNMEATYPE_CAB ) \
   _mac( AISNMEATYPE_CBM ) \
   _mac( AISNMEATYPE_BCF ) \
   _mac( AISNMEATYPE_DLM ) \
   _mac( AISNMEATYPE_QUERY ) \
   _mac( AISNMEATYPE_ABK ) \
   _mac( AISNMEATYPE_WRONGFORMAT ) \
   _mac( AISNMEATYPE_SCANTER_TRACK ) \
   _mac( AISNMEATYPE_SCANTER_TRACK_CREATE ) \
   _mac( AISNMEATYPE_XML ) \
   _mac( AISNMEATYPE_XML_USER_CMD ) \
   _mac( AISNMEATYPE_GPG_GGA ) \
   _mac( AISNMEATYPE_GPG_GSA ) \
   _mac( AISNMEATYPE_GPG_GSV ) \
   _mac( AISNMEATYPE_GPG_GSV_DESEGMENTED ) \
   _mac( AISNMEATYPE_GPG_RMC ) \
   _mac( AISNMEATYPE_GPG_VTG ) \
   _mac( AISNMEATYPE_PORB ) \
   _mac( AISNMEATYPE_VSI ) \
   _mac( AISNMEATYPE_TLB ) \
   _mac( AISNMEATYPE_TLL ) \
   _mac( AISNMEATYPE_TTM ) \
   _mac( AISNMEATYPE_RSD ) \
   _mac( AISNMEATYPE_OSD ) \
   _mac( AISNMEATYPE_COMMENT_BLOCK ) \
   _mac( AISNMEATYPE_ECB ) \
   _mac(AISNMEATYPE_VER) \
   _mac(AISNMEATYPE_PSHI)
CREATE_ENUM(AISNMEAType, _AISNMEATYPE_ENUM_TYPE_META);
ENUM_TO_STR_ARRAY_H(AISNMEAType, _AISNMEATYPE_ENUM_TYPE_META);

//------------------------------------------------------
//  class TclNMEAMessage
//------------------------------------------------------
/// Base class for NMEA message classes.
class TclNMEAMessage
{
public:
   TclNMEAMessage(TenAISNMEAType _enSentenceType = AISNMEATYPE_UNDEFINED);
   virtual ~TclNMEAMessage();

   bool HasError() const { return m_fHasError; }
   void SetHasError(bool _fHasError) { m_fHasError = _fHasError; }

   /// Encode sentence(s). This always appends to \a _msg.
   virtual void Encode(std::string& _msg);

   virtual void EncodePart(std::string &_msg, const std::string &_clData);

   /// Decode sentence. If false is returned the decoded message will contain bogus data.
   virtual bool Decode(const std::string &_msg ///< Input message.
                       ) = 0;

   /// Verify if message format is is legal.
   virtual bool Verify(const std::string &_msg    ///< Input message.
                       );

   /// Get one char int from data
   static bool GetSmallInt(int &iData, const char *pclMesgData, int iLen, int &iStart)
   {
      if (iStart<iLen)
      {
         char ch = pclMesgData[iStart];

         if (ch != ',' && ch != '*')
         {
            iData = ch -'0';

            iStart++;

            ch = pclMesgData[iStart];
            if (ch != ',' && ch != '*')
            {
               return false;
            }

            iStart++;
         }
         else
         {
            iStart++;
         }

         return true;
      }
      else
      {
         return false;
      }
   }

   /// Get one char from data
   static bool GetChar(char &iData, const char *pclMesgData, int iLen, int &iStart)
   {
      if (iStart<iLen)
      {
         char ch = pclMesgData[iStart];

         if (ch != ',' && ch != '*')
         {
            iData = ch;
            iStart+=2;
         }
         else
         {
            iStart++;
         }

         return true;
      }
      else
      {
         return false;
      }
   }

   // get string from data
   static bool GetString(std::string &clData, const char *pclMesgData, int iLen, int &iStart)
   {
      int i=iStart;
      int iLenMinusOne = iLen-1;
      while (iStart<iLen && i<iLen)
      {
         char ch = pclMesgData[i];
         if (ch == ',' || ch == '*' || ch == '\r' || i==iLenMinusOne)
         {
            if (i-iStart>0)
            {
               clData.assign(&pclMesgData[iStart], i-iStart);
            } else
            {
               clData="";
            }
            iStart=i+1;
            break;
         }
         i++;
      }

      return true;
   }

   // get int from data
   static bool GetInt(int &iData, const char *pclMesgData, int iLen, int &iStart)
   {
      int i=iStart;
      while (iStart<iLen && i<iLen)
      {
         char ch = pclMesgData[i];
         if (ch == ',' || ch == ':' || ch == 'G' || ch == '*' || ch == '\r')
         {
            if (i-iStart>0)
            {
               std::string clStr;
               clStr.assign(&pclMesgData[iStart], i-iStart);
					mylib::from_string( clStr, iData );
            }
            iStart=i+1;
            break;
         }
         i++;
      }

      return true;
   }


   // get double from data
   static bool GetDouble(double &dData, const char *pclMesgData, int iLen, int &iStart)
   {
      int i=iStart;
      while (iStart<iLen && i<iLen)
      {
         char ch = pclMesgData[i];
         if (ch == ',' || ch == '*' || ch == '\r')
         {
            if (i-iStart>0)
            {
               std::string clStr;
               clStr.assign(&pclMesgData[iStart], i-iStart);
					mylib::from_string( clStr, dData );
            }
            iStart=i+1;
            break;
         }
         i++;
      }

      return true;
   }


   // get checksum from data
   static bool GetCheckSumWord(int &iData, const char *pclMesgData, int iLen, int &iStart)
   {
      int i=iStart;
      while (iStart<iLen && i<iLen)
      {
         char ch = pclMesgData[i];
         if (ch == ',' || ch == '*' || i-iStart==2)
         {
            if (i-iStart>0)
            {
               std::string clStr;
               clStr.assign(&pclMesgData[iStart], i-iStart);
               sscanf(clStr.c_str(), "%X", &iData);
            }
            iStart=i+1;
            break;
         }
         i++;
      }

      return true;
   }


   static std::string EnsureWidth(char chFill, std::string clStr, int iLen)
   {
      if (static_cast<int>(clStr.size()) < iLen)
      {
         int missing = iLen - static_cast<int>(clStr.size());
         while (missing-- > 0)
         {
            clStr = chFill + clStr;
         }
      }

      return clStr;
   }

   bool GetReceivedMessageModified() const { return m_fReceivedMessageModified; }
   void SetReceivedMessageModified(bool _fReceivedMessageModified) { m_fReceivedMessageModified = _fReceivedMessageModified; }

   virtual std::string GetReceivedMessage() const { return m_clReceivedMessage; }


   /// Get Total number  sentences needed to transfer the message (1 to 9).
   int GetTotalNumberOfMessages() const { return m_iTotalNumberOfMessages; }
   /// Get Sentence number (1 to 9)
   int GetSentenceNumber() const { return m_iSentenceNumber; }
   
   /// Get sequential message identifier (0 to 9)
   virtual int GetSequentialMessageIdentifier() const { return m_iSequentialMessageIdentifier; }

   /// Set total number of sentences needed to transfer the message (1 to 9).
   void SetTotalNumberOfMessages(int _iTotalNumberOfMessages, bool _fMarkAsModified = true)
   {
      if(_fMarkAsModified)
      {
         SetReceivedMessageModified(_fMarkAsModified);
      }
      m_iTotalNumberOfMessages = _iTotalNumberOfMessages;
   }

   /// Set Sentence number (1 to 9)
   void SetSentenceNumber(int _iSentenceNumber, bool _fMarkAsModified = true)
   {
      if(_fMarkAsModified)
      {
         SetReceivedMessageModified(_fMarkAsModified);
      }
      m_iSentenceNumber = _iSentenceNumber;
   }

   /// Set sequential message identifier (0 to 9)
   void SetSequentialMessageIdentifier(int _iSequentialMessageIdentifier, bool _fMarkAsModified = true)
   {
      if(_fMarkAsModified)
      {
         SetReceivedMessageModified(_fMarkAsModified);
      }
      m_iSequentialMessageIdentifier = _iSequentialMessageIdentifier;
   }

   /// Get payload from messages VDM, VDO, and PGH2.
   /// Using this method on other sentences will have no effect.
   virtual const std::string& GetPayload()  { return m_aszPayload; }
   void SetPayload(const std::string& _aszPayload, bool _fMarkAsModified = true)
   {
      if(_fMarkAsModified)
      {
         SetReceivedMessageModified(_fMarkAsModified);
      }
      m_aszPayload = _aszPayload;
   }

   virtual TenAISNMEAType GetSentenceType() const {return m_enSentenceType;}
   
   void SetSentenceType(TenAISNMEAType _enSentenceType, bool _fMarkAsModified = true)
   {
      if(_fMarkAsModified)
      {
         SetReceivedMessageModified(_fMarkAsModified);
      }
      m_enSentenceType = _enSentenceType;
   }

   inline int GetChecksum() const {return m_iCheckSum;}
   
   inline std::string GetChecksumAsString() const
   {
      std::string s;
      char buf[5];
      sprintf(buf,"%02X", m_iCheckSum);
      s = buf;
      return s;
   }

   /// Returns double formatted as a string
   std::string GetDoubleAsString( const double _flVal );

   /// Sets the message checksum from a string of hexadecimal characters.
   virtual inline void SetChecksum(const std::string& _clChecksum, bool _fMarkAsModification = true)
   {
      if(_fMarkAsModification)
      {
         SetReceivedMessageModified(true);
      }
      sscanf(_clChecksum.c_str(), "%X", &m_iCheckSum);
   }

   /// Sets the message checksum
   virtual inline void SetChecksum(int _uiChecksum, bool _fMarkAsModification = true)
   {
      if(_fMarkAsModification)
      {
         SetReceivedMessageModified(true);
      }
      m_iCheckSum = _uiChecksum;
   }

   /// Generate checksum for one NMEA message.
   /**
      The checksum value shall be computed on all received characters between, but not including,
      "$"/"!" and "*".

      Example of a NMEA message with checksum:
       !AIVDM,1,1,,A,102=a??P?w<tSF0l4Q@>4?v00001,0*46
      , where "46" is the checksum.
   */
   /// The computed check sum is returned, and as a nice side effect it is also stored in m_iCheckSum.
   int CalcCheckSum(const std::string& _msg, int _size);

   /// The computed check sum is returned, and as a nice side effect it is also stored in m_iCheckSum.
   std::string CalcCheckSumAsString(const std::string& s, int iLength = -1);

   int GetMaxContent() const { return m_iMaxContent; }

protected:
   void AppendPayload(const  std::string& _clAdditionalPayload )
   {
      m_aszPayload += _clAdditionalPayload;
   }

   /// Verify if checksum of the message is correct.
   bool VerifyCheckSum(const std::string &_msg);

   /// Extract the checksum substring from the message. Returns an empty string if no checksum was found.
   std::string ExtractChecksumFromMessage(const std::string& _msg) const;

   TclNMEAMessage& operator = (const TclNMEAMessage& source)
   {
      m_iTotalNumberOfMessages         = source.m_iTotalNumberOfMessages;
      m_iSentenceNumber                = source.m_iSentenceNumber;
      m_iSequentialMessageIdentifier   = source.m_iSequentialMessageIdentifier;
      m_aszPayload                     = source.m_aszPayload;
      m_clReceivedMessage              = source.m_clReceivedMessage;
      m_fReceivedMessageModified       = source.m_fReceivedMessageModified;
      m_iMaxContent                    = source.m_iMaxContent;
      m_iCheckSum                      = source.m_iCheckSum;
      m_fSeqMesIdIsNull                = source.m_fSeqMesIdIsNull;
      return *this;
   }

private:
   TclNMEAMessage(const TclNMEAMessage& source);

protected:
   int m_iTotalNumberOfMessages;
   int m_iSentenceNumber;
   int m_iSequentialMessageIdentifier;

   std::string   m_aszPayload;
   std::string   m_clReceivedMessage;
   bool        m_fReceivedMessageModified;
   int         m_iMaxContent;
   int         m_iCheckSum;

   TenAISNMEAType m_enSentenceType;
   bool m_fSeqMesIdIsNull; // Indicates that the decoded message held a NULL value instead of a Sequential Message Identifier
   bool m_fHasError;

};

#endif

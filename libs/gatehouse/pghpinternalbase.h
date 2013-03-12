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
#ifndef _pghpinternalbase_h
#define _pghpinternalbase_h

#include <sstream>
#include <vector>

#include "pghpbase.h"

const int ASN_INTEGER = 0x02;
const int ASN_DOUBLE = 0x03;
const int ASN_STR = 0x06;
const int ASN_DATETIME = 0x07;
const int ASN_LONG_LEN = 0x80; 


#define _AISMESG_INTERNAL_ENUM_TYPE_META(_mac) \
   _mac( AISMESGINT_UNKNOWN ) \
   _mac( AISMESGINT_LOGON_REQUEST ) \
   _mac( AISMESGINT_LOGON_REPLY ) \
   _mac( AISMESGINT_LOGOFF_REQUEST ) \
   _mac( AISMESGINT_STARTDATA_REQUEST ) \
   _mac( AISMESGINT_STOPDATA_REQUEST ) \
   _mac( AISMESGINT_UPDATE_INTERVAL ) \
   _mac( AISMESGINT_DROP_SSL_CONNECTION ) \
   _mac( AISMESGINT_ADD_SERVER ) \
   _mac( AISMESGINT_CLEAR_SERVERLIST ) \
   _mac( AISMESGINT_STATUS_SET_OBJECT_STATE ) \
   _mac( AISMESGINT_STATUS_REMOVE_OBJECT ) \
   _mac( AISMESGINT_STATUS_ADD_TEXTMSG ) \
   _mac( AISMESGINT_STATUS_REMOVE_TEXTMSG ) \
   _mac( AISMESGINT_CHANGE_PASSWORD_REQUEST ) \
   _mac( AISMESGINT_CHANGE_PASSWORD_REPLY ) \
   _mac( AISMESGINT_CHANGE_PASSWORD_COMMAND ) \
   _mac( AISMESGINT_HTMLSTAT_REPORT ) \
   _mac( AISMESGINT_LSSUSER_STATUS ) \
   _mac( AISMESGINT_LSS_STATUS ) \
   _mac( AISMESGINT_INTERNAL_CONNECTION ) \
   _mac( AISMESGINT_HTMLSTAT_CLIENTSTATE ) \
   _mac( AISMESGINT_PROXY_FILTER ) \
   _mac( AISMESGINT_PROXY_ACCESS_RIGHTS ) \
   _mac( AISMESGINT_AREA_LIST ) \
   _mac( AISMESGINT_LB_COMMAND ) \
   _mac( AISMESGINT_MOVE_PROXY ) \
   _mac( AISMESGINT_GAD_ACCESS_RIGHTS ) \
   _mac( AISMESGINT_GAD_PARAMETERS ) \
   _mac( AISMESGINT_WDOG_DATA ) \
   _mac( AISMESGINT_WDOG_CONTROL ) \
   _mac( AISMESGINT_ADD_SERVER_LIST ) \
   _mac( AISMESGINT_STATUS_MESSAGE ) \
   _mac( AISMESGINT_GAD_SEND_INFO ) \
   _mac( AISMESGINT_CLOCK_SYNC ) \
   _mac( AISMESGINT_CLOCK_SYNC_INTERVAL ) \
   _mac( AISMESGINT_TIME_STAMP_METHOD ) \
   _mac( AISMESGINT_DOWNSAMPLE_INTERVAL ) \
   _mac( AISMESGINT_MAPSERVER_STATUS ) \
   _mac( AISMESGINT_RADAR_HEADER ) \
   _mac(AISMESGINT_TRACK) 
CREATE_ENUM(AisMesgInternalType, _AISMESG_INTERNAL_ENUM_TYPE_META);


class TclAISMessageInternalBase : public TclAISMessageBase
{
public:
	TclAISMessageInternalBase(TenAisMesgInternalType _enType=AISMESGINT_UNKNOWN);
	virtual ~TclAISMessageInternalBase();
private:
	TclAISMessageInternalBase(const TclAISMessageInternalBase& source);
	TclAISMessageInternalBase& operator = (const TclAISMessageInternalBase& source);

public:
	virtual bool Decode(const std::string &clData);
	virtual bool Encode(std::string &clData);

	const TenAisMesgInternalType& GetType() const { return enType; }
	void SetType(const TenAisMesgInternalType& _enType) { enType = _enType; }

	void EncodeString(std::vector<uint8_t> &clDataBuffer, const std::string &clName, const std::string &clData);
	void DecodeString(std::vector<uint8_t> &clDataBuffer, int16_t &iOffset, const std::string &clName, std::string &clData);

	void EncodeInteger(std::vector<uint8_t> &clDataBuffer, const std::string &clName, int32_t iData);
	/// Space-optimized version
	void EncodeInteger(std::vector<uint8_t>& _dataBuffer, int32_t _data);

	void EncodeDouble(std::vector<uint8_t>& _dataBuffer, double _data);
	void DecodeInteger(std::vector<uint8_t> &clDataBuffer, int16_t &iOffset, const std::string &clName, int32_t &iData);

	/// Space-optimized version
	void DecodeInteger(std::vector<uint8_t>& _dataBuffer, int16_t& _offset, int32_t& _data);
	void DecodeDouble(std::vector<uint8_t>& _dataBuffer, int16_t& _offset, double& _data);

	void EncodeDateTime(std::vector<uint8_t> &clDataBuffer, const std::string &clName, const boost::posix_time::ptime &clDT);
	void DecodeDateTime(std::vector<uint8_t> &clDataBuffer, int16_t &iOffset, const std::string &clName, boost::posix_time::ptime &clDT);

	void DecodeHeader(std::vector<uint8_t> &clDataBuffer, int16_t &iOffset);
	void EncodeHeader(std::vector<uint8_t> &clDataBuffer);

private:
	void BuildHeader(std::vector<uint8_t> &clData, uint8_t iType, uint32_t iLength);
	void BuildLength(std::vector<uint8_t> &clData, uint32_t iLength);
	void BuildString(std::vector<uint8_t> &clData, uint8_t iType, const std::string &clStr);
	void BuildInt(std::vector<uint8_t> &clData, uint8_t iType, int32_t iIntData);
	void BuildDouble(std::vector<uint8_t>& _data, uint8_t _type, double _val);

	bool ParseLength(const std::vector<uint8_t>& _clData, int16_t& _iOffset, uint32_t& _iLength);
	bool ParseHeader(std::vector<uint8_t> &clData, int16_t &iOffset, uint8_t &iType, uint32_t &iLength);
	bool ParseString(const std::vector<uint8_t>& clData, int16_t& iOffset, uint8_t& iType, std::string& clString);
	bool ParseInt(std::vector<uint8_t> &clData, int16_t &iOffset, uint8_t &iType, int32_t &iInt);
	bool ParseDouble(std::vector<uint8_t>& _data, int16_t& _offset, uint8_t& _type, double& _val);

protected:
	void Ascii2Byte(std::vector<uint8_t> &clDataBuffer, const std::string &clData);
	void Byte2Ascii(const std::vector<uint8_t> &clDataBuffer, std::string &clData);

private:
	TenAisMesgInternalType enType;
};

#endif

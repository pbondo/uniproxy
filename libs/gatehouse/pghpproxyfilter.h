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
#ifndef _pghpproxyfilter_h
#define _pghpproxyfilter_h

#include "pghpinternalbase.h"

//------------------------------------------------------
//  class TclAISMessageProxyFilter
//------------------------------------------------------
/// Proxy filter object containing the data the user can choose to filter in the proxy (the actual filtering will be done in the LSS)
/**
No detailed description
*/
class TclAISMessageProxyFilter: public TclAISMessageInternalBase
{
public:
   TclAISMessageProxyFilter();
   virtual ~TclAISMessageProxyFilter();
private:
   TclAISMessageProxyFilter(const TclAISMessageProxyFilter& source);
   TclAISMessageProxyFilter& operator = (const TclAISMessageProxyFilter& source);

public:
   bool Decode(const std::string &clData);
   bool Encode(std::string &clData);

public:
   bool m_fTimeStamp;
   bool GetTimeStamp() const { return m_fTimeStamp; } 
   void SetTimeStamp(bool _fTimeStamp) { m_fTimeStamp = _fTimeStamp; }

   int m_iUpdateInterval;
   int GetUpdateInterval() const { return m_iUpdateInterval; } 
   void SetUpdateInterval(int _iUpdateInterval) { m_iUpdateInterval = _iUpdateInterval; }

   std::vector<int> m_clMessageTypes;           // Messages from 1 to 22
   const std::vector<int>& GetMessageTypes() const { return m_clMessageTypes; } 
   void SetMessageTypes(const std::vector<int>& _clMessageTypes) { m_clMessageTypes = _clMessageTypes; }

   bool m_fMessageTypesDefined;
   bool GetMessageTypesDefined() const { return m_fMessageTypesDefined; } 
   void SetMessageTypesDefined(bool _fMessageTypesDefined) { m_fMessageTypesDefined = _fMessageTypesDefined; }

   std::vector<std::string> m_clPredefinedAreas;
   const std::vector<std::string>& GetPredefinedAreas() const { return m_clPredefinedAreas; } 
   void SetPredefinedAreas(const std::vector<std::string>& _clPredefinedAreas) { m_clPredefinedAreas = _clPredefinedAreas; }

   bool m_fPredefinedAreasDefined;
   bool GetPredefinedAreasDefined() const { return m_fPredefinedAreasDefined; } 
   void SetPredefinedAreasDefined(bool _fPredefinedAreasDefined) { m_fPredefinedAreasDefined = _fPredefinedAreasDefined; }

   std::vector<double> m_clUserDefinedArea;     // Upper left corner (lat,lon) - Lower right corner (lat,lon)
   const std::vector<double>& GetUserDefinedArea() const { return m_clUserDefinedArea; } 
   void SetUserDefinedArea(const std::vector<double>& _clUserDefinedArea) { m_clUserDefinedArea = _clUserDefinedArea; }

   bool m_fUserDefinedAreaDefined;
   bool GetUserDefinedAreaDefined() const { return m_fUserDefinedAreaDefined; } 
   void SetUserDefinedAreaDefined(bool _fUserDefinedAreaDefined) { m_fUserDefinedAreaDefined = _fUserDefinedAreaDefined; }

   std::vector<int> m_clMMSIList;            
   const std::vector<int>& GetMMSIList() const { return m_clMMSIList; } 
   void SetMMSIList(const std::vector<int>& _clMMSIList) { m_clMMSIList = _clMMSIList; }

   bool m_fIncludeMMSIListContent;    // true = include
   bool GetIncludeMMSIListContent() const { return m_fIncludeMMSIListContent; } 
   void SetIncludeMMSIListContent(bool _fIncludeMMSIListContent) { m_fIncludeMMSIListContent = _fIncludeMMSIListContent; }

   bool m_fMMSIListDefined;
   bool GetMMSIListDefined() const { return m_fMMSIListDefined; } 
   void SetMMSIListDefined(bool _fMMSIListDefined) { m_fMMSIListDefined = _fMMSIListDefined; }

};

#endif

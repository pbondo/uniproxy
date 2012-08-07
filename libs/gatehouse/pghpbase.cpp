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
#include "pghpbase.h"

/**
  \file AISMessageBase.cpp
    \brief missing comment
    */

TclAISMessageBase::TclAISMessageBase(TenAisBaseType enType)
   : enAisBaseType(enType)
{

}

TclAISMessageBase::~TclAISMessageBase()
{

}

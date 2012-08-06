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
#ifndef _pghpgeneral_h
#define _pghpgeneral_h

#include <applutil.h> // Utilities that must be provided.

#define AISERR( xx ) DERR( xx )
#define to_lower( xx ) std::transform(xx.begin(), xx.end(), xx.begin(), (int(*)(int)) std::tolower );

// Macros for creating enums

#undef GH_ENUM_META
#undef GH_STR_META

#undef CREATE_ENUM
#undef ENUM_TO_STR_ARRAY
#undef ENUM_TO_STR_ARRAY_H
#undef STR_FROM_ENUM
#undef ENUM_FROM_STR


#define GH_ENUM_META(Name) Name ,
#define GH_STR_META(Name) #Name ,


#define CREATE_ENUM(name, values) enum Ten##name { values(GH_ENUM_META) ENUM_##name##_MAX }

#define ENUM_TO_STR_ARRAY(name, values) const char* apch##name[] = { values(GH_STR_META) }

/// Note that GetXxxFromString(enum) is misnamed (kept for compatibility)
#define ENUM_TO_STR_ARRAY_H(enumname, values)                           \
        extern const char* apch##enumname[];                            \
        std::string Get##enumname##AsString(Ten##enumname value);       \
        std::string Get##enumname##FromString(Ten##enumname value);     \
        std::string Get##enumname##FromString(const char*, Ten##enumname value)

#define STR_FROM_ENUM(enumname)                                         \
        std::string Get##enumname##AsString(Ten##enumname value)        \
        {                                                               \
           if (value < ENUM_##enumname##_MAX)                           \
           {                                                            \
              return apch##enumname[value];                             \
           }                                                            \
           return "STR_FROM_ENUM error unknown value";                  \
        }                                                               \
        std::string Get##enumname##FromString(Ten##enumname value)      \
        {                                                               \
           return Get##enumname##AsString(value);                       \
        }


#define ENUM_FROM_STR(classname, enumname)                                              \
        bool Get##enumname##FromString(const char* buf, classname##::##enumname &value) \
        {                                                                               \
           for (int i = 0; i < classname##::ENUM_##enumname##_MAX; i++)                 \
           {                                                                            \
              if (strcmp(str_##enumname##[i], buf) == 0)                                \
              {                                                                         \
                  value = (classname##::##enumname) i;                                  \
                  return false;                                                         \
              }                                                                         \
           }                                                                            \
           return true;                                                                 \
        }

#endif

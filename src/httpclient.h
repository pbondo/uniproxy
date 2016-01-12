//
//
#ifndef _httpclient_h
#define _httpclient_h

#include "cppcms_util.h"

class httpclient
{
public:

   // hostnameport:  "localhost:8085"
   // url:           "/json/status/"
   static bool sync(const std::string &hostnameport, const std::string &message, std::string &result);

};


#endif


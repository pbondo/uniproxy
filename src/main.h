//====================================================================
//
// Universal Proxy
//
// Core application
//--------------------------------------------------------------------
//
// This version is released as part of the European Union sponsored
// project Mona Lisa work package 4 for the Universal Proxy Application
//
// This version is released under the GNU General Public License with restrictions.
// See the doc/license.txt file.
//
// Copyright (C) 2011-2012 by GateHouse A/S
// All Rights Reserved.
// http://www.gatehouse.dk
// mailto:gh@gatehouse.dk
//====================================================================
#ifndef _main_h
#define _main_h

#include <cppcms/application.h>
#include <cppcms/json.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/service.h>

#include "remoteclient.h"
#include "localclient.h"

#include <boost/asio.hpp>

enum
{
	PORT_WEBSERVER = 8080,

	PORT_IALA_NMEA = 8750,
	PORT_IALA_IVEF = 8751
};






// Notice this class will not necessarily be created in one global instance. At least it will not be easily globally accesible.
//
class proxy_app : public cppcms::application 
{
public:

	proxy_app(cppcms::service &srv);

	void main(std::string url);

	void my404( std::string url );

	std::string status_get_json();

	void config_reload();
	void config_upload();
	void shutdown();
	void logger_get();
	void status_get();
	void index();
	void script(const std::string);
	void client_activate(const std::string);
	void host_activate(const std::string);
	void certificate_delete(const std::string);

	static void setup_config( cppcms::json::value &_settings );
	static bool execute_openssl();

};

namespace cppcms
{

class signal
{
public:

	signal(cppcms::service &_service)
	{
		signal::m_reload = false;
		signal::m_psrv = &_service;
	#ifdef __linux__
		::signal(SIGHUP, &signal::handler);
	#endif
	}

	~signal()
	{
	#ifdef __linux__
		::signal(SIGHUP, SIG_DFL);
	#endif
		signal::m_psrv = null_ptr;
	}

	// Notice this is handled in the OS space, so we are limited in capabilities.
	// It appears to be ok to call cppcms::service.shutdown();
	static void handler(int signum)
	{
		#ifdef __linux__
		signal::m_reload = (signum == SIGHUP);
		#endif
		if ( signal::m_psrv )
		{
			signal::m_psrv->shutdown();
		}
	}

	static void reset_reload()
	{
		m_reload = false;
	}

	static void set_reload()
	{
		m_reload = true;
	}

	static bool reload()
	{
		return m_reload;
	}

private:

	static bool m_reload;
	static cppcms::service *m_psrv;

};

} // namespaces

#endif

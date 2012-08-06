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
// This version is released under the GPL version 3 open source License:
// http://www.gnu.org/copyleft/gpl.html
//
// Copyright (C) 2004-2012 by GateHouse A/S
// All Rights Reserved.
// http://www.gatehouse.dk
// mailto:gh@gatehouse.dk
//====================================================================
#ifndef _content_h
#define _content_h

#include <cppcms/view.h>
#include <cppcms/form.h>
#include <booster/function.h>
#include <string>
#include <iostream>

#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>


namespace content
{
	class host_status
	{
	public:

		bool connected;		//
		int port;			//
		std::string hostname;	// [ip or hostname]
		int flow;			// [msg/min]
	};

	class client_status
	{
	public:

		bool connected;
		int port;
		std::string hostname;
		int flow;			// [msg/min]
	};

	class master : public cppcms::base_content
	{
	public:

		std::string title;
		
		std::string javascript;

		int client_handler_port;

		std::vector<host_status> hosts;
		std::vector<client_status> clients;
	};

	class content_config : public master
	{
	public:

		content_config( )
		{		
		}

	};

	class content_status : public master
	{
	public:

		content_status( )
		{		
		}

	};

	struct buttons_form : public cppcms::form
	{
		cppcms::widgets::submit stop;

		buttons_form()
		{
			using cppcms::locale::translate;
			stop.value(translate("Stop"));
		}
	};

}

#endif

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
// Copyright (C) 2011-2013 by GateHouse A/S
// All Rights Reserved.
// http://www.gatehouse.dk
// mailto:gh@gatehouse.dk
//====================================================================
#ifndef _providerclient_h
#define _providerclient_h

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "baseclient.h"


class ProviderClient : public BaseClient
{
public:

	ProviderClient(bool _active, mylib::port_type _activate_port,
			const std::vector<LocalEndpoint> &_local_endpoints, // Local data provider
			const std::vector<RemoteEndpoint> &_proxy_endpoints, // Remote proxy
			PluginHandler &_plugin);

	void start();
	void stop();

	void threadproc();
	void interrupt();

	bool is_local_connected() const;
	std::string local_hostname() const;
	std::string local_portname() const;

	std::vector<std::string> local_hostnames() const;

	void lock();
	void unlock();

protected:

	std::vector<LocalEndpoint> m_local_endpoints; // The list of local data providers to connect to in a round robin fashion.
	int m_local_connected_index;

	boost::asio::ip::tcp::socket *mp_local_socket;

};

#endif

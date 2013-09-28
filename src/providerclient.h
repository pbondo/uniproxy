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

//typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;


class ProviderClient : public BaseClient
{
public:

	ProviderClient( bool _active, 
		const std::vector<ProxyEndpoint> &_local_endpoints, // Local data provider
		const std::vector<ProxyEndpoint> &_proxy_endpoints, // Remote proxy
		PluginHandler &_plugin );

	void start();
	void stop();

	void threadproc();
	void interrupt();

	//ssl_socket &remote_socket();

	bool is_local_connected() const;
	//bool is_remote_connected() const;

	//std::string remote_hostname() const;
	std::string local_hostname() const;
	//int remote_port() const;
	int local_port() const;

	std::vector<std::string> local_hostnames();

	//const std::string dolog();
	//void dolog( const std::string &_line );

	void lock();
	void unlock();

	// The following are used in the status_get
	//bool m_active;
	//int m_id;
	//boost::posix_time::ptime m_activate_stamp;

protected:

	//std::vector<ProxyEndpoint> m_proxy_endpoints; // The list of remote proxies to connect to in a round robin fashion.
	std::vector<ProxyEndpoint> m_local_endpoints; // The list of local data providers to connect to in a round robin fashion.
	//int m_proxy_connected_index; // These should be declared atomic as soon as it is supported by the compilers.
	int m_local_connected_index;

	// Should these move to ProxyEndpoint ?
	//data_flow m_count_out;

	//std::string get_password() const;

	// It is difficult to handle the mutexes, so keep this one protected and use local getter function to retrieve information.
	//mutable stdt::mutex m_mutex;

	//ssl_socket *mp_remote_socket;
	boost::asio::ip::tcp::socket *mp_local_socket;

	//mylib::thread m_thread;
	//PluginHandler &m_plugin;

	enum { max_length = 1024 };
	char m_local_data[max_length];
	//char m_remote_data[max_length];

	//std::string m_log;

};


#endif

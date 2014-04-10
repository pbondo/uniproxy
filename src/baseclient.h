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
#ifndef _baseclient_h
#define _baseclient_h

#include "applutil.h"

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

class BaseClient
{
public:

	BaseClient(bool _active, int _local_port, const std::vector<ProxyEndpoint> &_proxy_endpoints, const int _max_connections, PluginHandler &_plugin);

	virtual void start() = 0;
	virtual void stop() = 0;
	virtual void interrupt() = 0;
	virtual bool is_local_connected() const = 0;
	virtual std::vector<std::string> local_hostnames() const = 0;
	virtual std::string local_portname() const;

	// The remote connections does not need to be virtual.
	std::string remote_hostname() const;
	int remote_port() const;
	bool is_remote_connected(int index = -1) const;

	ssl_socket &remote_socket();

	const std::string dolog();
	void dolog( const std::string &_line );

	std::string get_password() const;
	
	void threadproc_activate(int _index);
	void start_activate(int _index);
	void stop_activate();
	void ssl_prepare(boost::asio::ssl::context &_ctx) const;

	std::vector<ProxyEndpoint> m_proxy_endpoints; // The list of remote proxies to connect to in a round robin fashion.

	int m_id;
	int m_proxy_index;
	bool m_active;
	ssl_socket *mp_remote_socket;
	data_flow m_count_in, m_count_out;
	
	int port() const { return this->m_local_port; }

protected:

	int m_local_port;

public:

	int m_max_connections;
	boost::posix_time::ptime m_activate_stamp;

protected:

	std::string m_log;
	mylib::thread m_thread_activate;
	mylib::thread m_thread;
	PluginHandler &m_plugin;

	mutable stdt::mutex m_mutex;

	enum { max_length = 1024 };
	char m_local_data[max_length];
	char m_remote_data[max_length];

};

#endif

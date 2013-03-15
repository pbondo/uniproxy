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
#ifndef _remoteclient_h
#define _remoteclient_h

#include "applutil.h"

class RemoteProxyHost;
	
class Address
{
public:

	Address( const std::string &_hostname, const int _port )
	: m_hostname( _hostname ), m_port( _port )
	{
	}

	std::string m_hostname;
	int m_port;

};


//
// This class handles connection from remote proxy clients
//
class RemoteProxyClient
{
public:

	RemoteProxyClient(boost::asio::io_service& io_service, boost::asio::ssl::context& context, RemoteProxyHost &_host );
	~RemoteProxyClient();

	ssl_socket::lowest_layer_type& socket();

	void start( std::vector<Address> &_local_ep );
	void stop();

	// When terminating these threads e.g. due to lost connections, just leave them as zoombies
	// Clean up will be done when starting new threads
	void local_threadproc();
	void remote_threadproc();
	
	bool is_active();
	bool is_local_connected();
	bool is_remote_connected();
	
	boost::asio::ip::tcp::endpoint remote_endpoint();
	boost::asio::ip::tcp::endpoint local_endpoint();

	void dolog( const std::string &_line );

	RemoteEndpoint m_endpoint;
	boost::asio::ip::tcp::socket m_local_socket;
	ssl_socket m_remote_socket;

	data_flow m_count_in, m_count_out;

protected:

	void interrupt();

	unsigned char *m_remote_read_buffer;
	unsigned char *m_local_read_buffer;

	bool m_local_connected, m_remote_connected;

	mylib::thread m_remote_thread, m_local_thread;
	std::vector<Address> m_local_ep;
	boost::asio::io_service& m_io_service;

	RemoteProxyHost &m_host;
};


//
//
class RemoteProxyHost
{
public:

	RemoteProxyHost( unsigned short _local_port, std::vector<RemoteEndpoint> &_remote_ep, std::vector<Address> &_local_ep, PluginHandler &_plugin ); //= NULL );

	void handle_accept( RemoteProxyClient* new_session, const boost::system::error_code& error);

	void start();
	void stop();

	std::string get_password() const;

	void interrupt();
	void threadproc();

	void dolog( const std::string &_line );

	int m_id;
	boost::asio::io_service m_io_service;
	boost::asio::ssl::context m_context;
	boost::asio::ip::tcp::acceptor m_acceptor;

	stdt::mutex m_mutex;
	std::vector<RemoteProxyClient*> m_clients; // NB!! This should be some shared_ptr or so ??
	std::vector<RemoteEndpoint> m_remote_ep;
	std::vector<Address> m_local_ep;
	PluginHandler &m_plugin;

	boost::posix_time::ptime m_activate_stamp;
	std::string m_activate_name;
	int m_local_port;

protected:

	mylib::thread m_thread;

	std::string m_log;

};

#endif

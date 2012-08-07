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
#ifndef _localclient_h
#define _localclient_h

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "applutil.h"

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

class LocalHost;


class LocalHostSocket
{
public:

	LocalHostSocket(LocalHost &_host, boost::asio::ip::tcp::socket *_socket);

	void handle_local_read(const boost::system::error_code& error,size_t bytes_transferred);

	boost::asio::ip::tcp::socket &socket();

	friend bool operator==( const LocalHostSocket&, const LocalHostSocket&);

protected:

	boost::asio::ip::tcp::socket *m_socket;

	LocalHost &m_host;
};


class LocalHost
{
public:

	LocalHost( bool _active, const std::string &_name, int _local_port, const std::string &_remote_host, const int _remote_port, const int _max_connections, PluginHandler &_plugin );

	void start();
	void stop();

	void threadproc();
	void interrupt();

	ssl_socket &remote_socket();

	void handle_local_read( LocalHostSocket &_hostsocket, const boost::system::error_code& error,size_t bytes_transferred);
	void handle_remote_read(const boost::system::error_code& error,size_t bytes_transferred);

	void handle_local_write(boost::asio::ip::tcp::socket *_socket, const boost::system::error_code& error);
	void handle_remote_write(const boost::system::error_code& error);

	void remove_socket( boost::asio::ip::tcp::socket &_socket );
//	bool SetupCertificates( boost::asio::ip::tcp::socket &_remote_socket );

	bool m_active;
	std::string m_remote_hostname;
	int m_remote_port;
	int m_local_port;
	int m_max_connections;
	std::string m_name;

	bool is_local_connected();
	bool is_remote_connected();
	void handle_accept( boost::asio::ip::tcp::socket *_socket, const boost::system::error_code& error );
	void cleanup();

	data_flow m_count_in, m_count_out;

	int m_id;

	//proxy_log &log();
	void dolog( const std::string &_line );
	const std::string dolog();


	std::string get_password() const;

	boost::posix_time::ptime m_activate_stamp;
	//std::string m_activate_name;

	bool remote_hostname( int index, std::string &result );
	std::vector<std::string> local_hostnames();
	
protected:

	// It is difficult to handle the mutexes, so keep this one protected and use local getter function to retrieve information.
	stdt::mutex m_mutex;
	//std::vector<std::shared_ptr<boost::asio::ip::tcp::socket> > m_local_sockets;
	
	std::vector<std::shared_ptr<LocalHostSocket> > m_local_sockets; // Elements are inserted here, once they have been accepted.

	// These are used for RAII handling. They do not own anything and should not be assigned by new.
	boost::asio::io_service *mp_io_service;
	boost::asio::ip::tcp::acceptor *mp_acceptor;
	ssl_socket *mp_remote_socket;

	mylib::thread m_thread;
	PluginHandler &m_plugin;
	int m_write_count;

	bool m_local_connected, m_remote_connected;

	enum { max_length = 1024 };
	char m_local_data[max_length];
	char m_remote_data[max_length];
	//proxy_log m_log;
	std::string m_log;
};


#endif

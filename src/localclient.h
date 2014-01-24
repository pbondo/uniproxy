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
#ifndef _localclient_h
#define _localclient_h

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/deadline_timer.hpp>

#include "baseclient.h"


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


class LocalHost : public BaseClient
{
public:

	LocalHost(bool _active, int _local_port, const std::vector<ProxyEndpoint> &_proxy_endpoints, const int _max_connections, PluginHandler &_plugin, const boost::posix_time::time_duration &_read_timeout);

	void start();
	void stop();

	void threadproc();
	void interrupt();

	void handle_local_read( LocalHostSocket &_hostsocket, const boost::system::error_code& error,size_t bytes_transferred);
	void handle_remote_read(const boost::system::error_code& error,size_t bytes_transferred);

	void handle_local_write(boost::asio::ip::tcp::socket *_socket, const boost::system::error_code& error);
	void handle_remote_write(const boost::system::error_code& error);

	void remove_socket( boost::asio::ip::tcp::socket &_socket );
	bool is_local_connected() const;
	void handle_accept( boost::asio::ip::tcp::socket *_socket, const boost::system::error_code& error );
	void cleanup();

	std::vector<std::string> local_hostnames() const;

	void check_deadline();

protected:

	std::vector<std::shared_ptr<LocalHostSocket> > m_local_sockets; // Elements are inserted here, once they have been accepted.

	// These are used for RAII handling. They do not own anything and should not be assigned by new.
	boost::asio::io_service *mp_io_service;
	boost::asio::ip::tcp::acceptor *mp_acceptor;
	boost::asio::deadline_timer *m_pdeadline;
	boost::posix_time::time_duration m_read_timeout;
	int m_write_count;

	bool m_local_connected;

};

#endif

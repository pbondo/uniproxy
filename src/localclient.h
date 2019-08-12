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
// Copyright (C) 2011-2019 by GateHouse A/S
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

   int id;
   
   static int id_gen;

protected:

   boost::asio::ip::tcp::socket *m_socket;

   LocalHost &m_host;
};


class LocalHost : public BaseClient
{
public:

   LocalHost(bool _active, mylib::port_type _local_port, mylib::port_type _activate_port, const std::vector<RemoteEndpoint> &_proxy_endpoints, const int _max_connections, PluginHandler &_plugin, const boost::posix_time::time_duration &_read_timeout, bool auto_reconnect);
   virtual ~LocalHost(){}

protected:

   void start();
   void stop();

   void threadproc();
   void interrupt();

   void handle_local_read( LocalHostSocket &_hostsocket, const boost::system::error_code& error,size_t bytes_transferred);
   void handle_remote_read(const boost::system::error_code& error,size_t bytes_transferred);

   void handle_local_write(boost::asio::ip::tcp::socket *_socket, const boost::system::error_code& error);
   void handle_remote_write(int id, const boost::system::error_code& error);
   void handle_handshake(const boost::system::error_code &err);

   void remove_socket( boost::asio::ip::tcp::socket &_socket );
   bool is_local_connected() const;
   int local_user_count() const;
   void handle_accept( boost::asio::ip::tcp::socket *_socket, const boost::system::error_code& error );
   void cleanup();

   void go_out(boost::asio::io_service &io_service);

   std::vector<std::string> local_hostnames() const;

   void check_deadline(const boost::system::error_code& error);

protected:

   std::string m_last_incoming_msg, m_last_outgoing_msg;
   boost::posix_time::ptime m_last_incoming_stamp, m_last_outgoing_stamp;


   // These are used for RAII handling. They do not own anything and should not be assigned by new.
   boost::asio::io_service *mp_io_service = nullptr;
   boost::asio::deadline_timer *m_pdeadline = nullptr;
   boost::posix_time::time_duration m_read_timeout;
   int m_write_count;

   bool m_local_connected = false;
   bool m_auto_reconnect = false; // If set the client UP will attempt to reconnect to server automatically.
   mylib::thread m_thread;

   // The following section is protected by the base gate.
   std::vector<std::shared_ptr<LocalHostSocket> > m_local_sockets; // Elements are inserted here, once they have been accepted.
   boost::asio::ip::tcp::acceptor *mp_acceptor = nullptr;

   friend class LocalHostSocket;
};

#endif

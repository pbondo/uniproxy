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
#ifndef _remoteclient_h
#define _remoteclient_h

#include "applutil.h"

class RemoteProxyHost;
   

//
// This class handles connection from remote proxy clients
//
class RemoteProxyClient
{
public:

   RemoteProxyClient(boost::asio::io_service& io_service, boost::asio::ssl::context& context, RemoteProxyHost &_host );
   ~RemoteProxyClient();

   ssl_socket::lowest_layer_type& socket();

   void start( std::vector<LocalEndpoint> &_local_ep );
   void stop();

   // When terminating these threads e.g. due to lost connections, just leave them as zoombies
   // Clean up will be done when starting new threads
   void local_threadproc();
   void remote_threadproc();
   
   bool is_active();
   bool is_local_connected();
   bool is_remote_connected();

   /// Attempt a local connection and report back with the result;
   int test_local_connection(const std::string& name, const std::vector<LocalEndpoint> &_local_ep);

   boost::asio::ip::tcp::endpoint remote_endpoint() const;
   boost::asio::ip::tcp::endpoint local_endpoint() const;

   void dolog( const std::string &_line );

   RemoteEndpoint m_endpoint;
   boost::asio::ip::tcp::socket m_local_socket;
   ssl_socket m_remote_socket;
   std::mutex m_mutex;

   data_flow m_count_in, m_count_out;

   std::string m_last_incoming_msg, m_last_outgoing_msg;
   boost::posix_time::ptime m_last_incoming_stamp, m_last_outgoing_stamp;

   std::string dinfo();

protected:

   void interrupt();

   unsigned char *m_remote_read_buffer;
   unsigned char *m_local_read_buffer;

   bool m_local_connected, m_remote_connected;

   mylib::thread m_remote_thread, m_local_thread;
   std::vector<LocalEndpoint> m_local_ep;
   boost::asio::io_service& m_io_service;

   boost::asio::ip::tcp::endpoint m_remote_cache, m_local_cache;

   RemoteProxyHost &m_host;
};


//
//
class RemoteProxyHost
{
public:

   RemoteProxyHost( mylib::port_type _local_port, const std::vector<RemoteEndpoint> &_remote_ep, const std::vector<LocalEndpoint> &_local_ep, PluginHandler &_plugin );

   void lock();
   void unlock();

   void start();
   void stop();

   bool remove_any(const std::vector<RemoteEndpoint>& removed);

   mylib::port_type port() const { return this->m_local_port; }
   int test_local_connection(const std::string& name);


   int id() const { return this->m_id;}

   void add_remotes(const std::vector<RemoteEndpoint> &_remote_ep);
   void remove_remotes(const std::vector<RemoteEndpoint> &_remote_ep);
   void stop_by_name(const std::string& certname);

   cppcms::json::value save_json_status() const;
   cppcms::json::value save_json_config() const;

   void dolog(const std::string &_line);

protected:

   void handle_accept( RemoteProxyClient* new_session, const boost::system::error_code& error);

   std::string get_password() const;


   void interrupt();
   void threadproc();


   int m_id;
   boost::asio::io_service m_io_service;
   boost::asio::ssl::context m_context;
   boost::asio::ip::tcp::acceptor m_acceptor;

   std::string dinfo() const;

public: // NB!! These should be private

   PluginHandler &m_plugin;
   bool m_active;
   std::vector<RemoteEndpoint> m_remote_ep; // static list loaded at start
   std::vector<LocalEndpoint> m_local_ep;

protected:

   mylib::port_type m_local_port;

   mylib::thread m_thread;


   // The following sections shall be protected by a gate
   mutable stdt::mutex m_mutex;
   std::vector<RemoteProxyClient*> m_clients; // NB!! This should be some shared_ptr or so ??

   mutable stdt::mutex m_mutex_log;
   std::string m_log;

};

std::ostream & operator << (std::ostream & os, const RemoteProxyHost &host);

#endif

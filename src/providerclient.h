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
         PluginHandler &_plugin, const cppcms::json::value &_json);

protected:

   void start();
   void stop();

   void threadproc_reader();
   void threadproc_writer();
   void interrupt();
   void interrupt_writer();

   void connect_remote(boost::asio::io_service &io_service, ssl_socket &remote_socket);

   bool is_local_connected() const;
   int local_user_count() const;
   std::string local_hostname() const;
   std::string local_portname() const;

   std::vector<std::string> local_hostnames() const;

public:

   void lock();
   void unlock();

protected:


   std::mutex m_buffer_mutex;
   std::condition_variable m_buffer_condition;
   std::vector<std::string> m_buffer;
   std::size_t m_buffer_size = 1000000;
   std::size_t m_buffer_count = 0;
   bool m_use_buffer = false;

   std::vector<LocalEndpoint> m_local_endpoints; // The list of local data providers to connect to in a round robin fashion.
   int m_local_connected_index;

   mylib::thread m_thread_write;
   boost::asio::ip::tcp::socket *mp_local_socket;
   const cppcms::json::value json;
};

bool is_provider(const BaseClient &client);

std::ostream &operator << (std::ostream &os, const ProviderClient &client);

#endif

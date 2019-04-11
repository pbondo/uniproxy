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

   BaseClient(bool _active, mylib::port_type _local_port, mylib::port_type _activate_port, const std::vector<RemoteEndpoint> &_proxy_endpoints, const int _max_connections, PluginHandler &_plugin);

   virtual cppcms::json::value save_json_status();
   virtual cppcms::json::value save_json_config() const;

   virtual void start() = 0;
   virtual void stop() = 0;
   virtual void interrupt() = 0;

   bool is_active() const
   {
      return this->m_active;
   }

   // Return true if the name if found.
   bool activate(const std::string& name);

   virtual std::string local_portname() const;

   bool set_active(const std::string& param, int id, bool active);

   bool certificate_exists(const std::string& certname) const;

   int id() const
   {
      return this->m_id;
   }

   friend std::ostream &operator << (std::ostream &os, const BaseClient &client);

   mylib::port_type port() const { return this->m_local_port; }

   mylib::port_type activate_port() const { return this->m_activate_port; }

protected:

   virtual std::string info() const;

   virtual bool is_local_connected() const = 0;
   virtual int local_user_count() const = 0;
   virtual std::vector<std::string> local_hostnames() const = 0;

   // The remote connections does not need to be virtual.
   std::string remote_hostname() const;
   int remote_port() const;
   bool is_remote_connected(int index = -1) const;

   ssl_socket &remote_socket();

   const std::string dolog() const;
   void dolog( const std::string &_line );

   std::string get_password() const;
   
   void threadproc_activate(int _index);
   void start_activate(int _index);
   void stop_activate();
   void ssl_prepare(boost::asio::ssl::context &_ctx) const;

   std::vector<RemoteEndpoint> m_proxy_endpoints; // The list of remote proxies to connect to in a round robin fashion.

   int m_id;
   int m_proxy_index;
   bool m_active;
   ssl_socket *mp_remote_socket;
   data_flow m_count_in, m_count_out;
   

   mylib::port_type m_local_port;
   mylib::port_type m_activate_port;

   int m_max_connections;
   boost::posix_time::ptime m_activate_stamp;

   std::string m_log;
   mylib::thread m_thread_activate;
   mylib::thread m_thread;
   PluginHandler &m_plugin;

   mutable std::mutex m_mutex_base;

   enum { max_length = 1024 };
   char m_local_data[max_length+1];
   char m_remote_data[max_length+1];

};

std::ostream &operator << (std::ostream &os, const BaseClient &client);

#endif

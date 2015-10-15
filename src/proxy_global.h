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
#ifndef _proxy_global_h
#define _proxy_global_h

#include "applutil.h"
#include "remoteclient.h"
#include "localclient.h"
#include "providerclient.h"
#include <cppcms/application.h>


typedef std::shared_ptr<BaseClient> baseclient_ptr;
typedef std::shared_ptr<RemoteProxyHost> remotehost_ptr;


class client_certificate_exchange : public mylib::thread
{
public:

   client_certificate_exchange( );

   void start( const std::vector<LocalEndpoint> &eps );

   void lock();
   void unlock();

   // Since this is a thread we want our own copy.
   void thread_proc( const std::vector<LocalEndpoint> eps );

};


class activate_host : public mylib::thread
{
public:

   class activate_t
   {
   public:
      boost::posix_time::ptime m_activate_stamp;
      std::string m_activate_name;
   };

   activate_host();
   void start(int _port);
   void interrupt();
   void threadproc(int _port);
   void add(std::string _certname);
   bool is_in_list(const std::string &_certname, boost::posix_time::ptime &_timeout);
   bool remove(const std::string &_certname);

   size_t size() const
   {
      std::lock_guard<std::mutex> lock(this->m_mutex);
      return this->m_activate.size();
   }

protected:

   void cleanup();

   mutable std::mutex m_mutex;
   std::vector<activate_t> m_activate;
   boost::asio::ip::tcp::socket *mp_socket = nullptr;
   boost::asio::ip::tcp::acceptor *mp_acceptor = nullptr;

};


class session_data
{
public:

   session_data( int _id );
   void update_timestamp();

   boost::posix_time::ptime m_timestamp;
   int m_id;
   int m_logger_read_index;
};


class proxy_global
{
public:

   proxy_global();
   void lock();
   void unlock();
   void stop( const std::string &_name );
   void stopall(); // Make a hard stop of all.

   typedef enum 
   {
      hosts    = 0x01,
      clients  = 0x02,
      web      = 0x04,
      config   = 0x08,
      all      = hosts|clients|web|config
   } json_acl;

   // These may throw exceptions.
   void populate_json(cppcms::json::value &obj, int _json_acl);
   void unpopulate_json(cppcms::json::value obj);

   std::string save_json_status( bool readable );
   std::string save_json_config( bool readable );

   //
   bool load_configuration();
   bool is_new_configuration(cppcms::json::value &obj) const;

   // lists with associated mutex
   stdt::mutex m_mutex_list;
   std::vector<remotehost_ptr> remotehosts;
   std::vector<baseclient_ptr> localclients;
   std::vector<LocalEndpoint> uniproxies;

   mylib::port_type m_web_port = 8085;
   std::string m_ip4_mask;
   bool m_debug;
   cppcms::json::value m_new_setup; // Stop and start services to match this.
   boost::posix_time::seconds m_activate_timeout = boost::posix_time::seconds(60);
   mylib::port_type m_activate_port = 25500;

   // Own name to be used for generating own certificate.
   std::string m_name;

   // The returned value here can be used for the duration of this transaction without being removed or moved.
   session_data &get_session_data( cppcms::session_interface &_session );
   void clean_session();

   // (common) Names from the certificates loaded from the imported certs.pem file.
   stdt::mutex m_mutex_certificates;
   std::vector<std::string> m_cert_names;
   bool load_certificate_names( const std::string & _filename );

   bool SetupCertificatesClient( boost::asio::ip::tcp::socket &_remote_socket, const std::string &_connection_name);
   std::string SetupCertificatesServer( boost::asio::ip::tcp::socket &_remote_socket, const std::vector<std::string> &_connection_names);

   bool certificate_available( const std::string &_cert_name);
   bool execute_openssl();

   bool is_same( const BaseClient &client, cppcms::json::value &obj, bool &param_changes, bool &client_changes ) const;
   bool is_same(const RemoteProxyHost &host, cppcms::json::value &obj, bool &param_changed, bool &locals_changed, std::vector<RemoteEndpoint> &rem_added, std::vector<RemoteEndpoint> &rem_removed) const;

   activate_host m_activate_host;

   bool m_log_all_data = false;

   std::ofstream m_out_data_log_file;
   std::ofstream m_in_data_log_file;

   std::string m_log_path = "log/";
   

protected:

   stdt::mutex m_session_data_mutex;
   std::vector< std::shared_ptr<session_data>> m_sessions;
};

extern proxy_global global;

#endif

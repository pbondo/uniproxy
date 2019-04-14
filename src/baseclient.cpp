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
#include "baseclient.h"
#include <boost/bind.hpp>
#include "proxy_global.h"

static int static_local_id = 0;

BaseClient::BaseClient(bool _active, mylib::port_type _local_port, mylib::port_type _activate_port, const std::vector<RemoteEndpoint> &_proxy_endpoints, const int _max_connections, PluginHandler &_plugin)
:  m_active(_active),
   m_count_in(true), m_count_out(true),
   m_local_port(_local_port),
   m_activate_port(_activate_port),
   m_max_connections(_max_connections),
   m_thread_activate([]{}),
   m_plugin(_plugin)
{
   this->m_proxy_endpoints = _proxy_endpoints;
   this->m_activate_stamp = boost::get_system_time();
   this->m_id = ++static_local_id;
   DOUT(info() << "Client port: " << this->m_local_port << " max con: " << this->m_max_connections << " active? " << (int)this->m_active);
}


ssl_socket &BaseClient::remote_socket()
{
   ASSERTE(this->mp_remote_socket, uniproxy::error::socket_invalid, "");
   return *this->mp_remote_socket;
}


std::string BaseClient::info() const
{
   return this->remote_hostname() + ":" + mylib::to_string(this->remote_port()) + " => :" + mylib::to_string(this->port()) + " ";
}


void BaseClient::dolog( const std::string &_line )
{
   {
      std::lock_guard<std::mutex> l(this->m_mutex_base);
      this->m_log = _line;
   }
   log().add(_line);
}


const std::string BaseClient::dolog() const
{
   return this->m_log;
}


int BaseClient::remote_port() const
{
   return this->m_proxy_endpoints[this->m_proxy_index].m_port;
}


std::string BaseClient::remote_hostname() const
{
   return this->m_proxy_endpoints[this->m_proxy_index].m_hostname;
}


bool BaseClient::is_remote_connected(int index) const
{
   return this->mp_remote_socket != nullptr
      && this->mp_remote_socket->lowest_layer().is_open()
      && is_connected(this->mp_remote_socket->lowest_layer())
      && (this->m_proxy_index == index || index == -1);
}


std::string BaseClient::get_password() const
{
   return "1234";
}


void BaseClient::ssl_prepare(boost::asio::ssl::context &ssl_context) const
{
   ssl_context.set_password_callback(boost::bind(&BaseClient::get_password,this));
   ssl_context.set_verify_mode(boost::asio::ssl::context::verify_peer|boost::asio::ssl::context::verify_fail_if_no_peer_cert);
   ssl_context.load_verify_file(my_certs_name);
   ssl_context.use_certificate_chain_file(my_public_cert_name);
   ssl_context.use_private_key_file(my_private_key_name, boost::asio::ssl::context::pem);
}


void BaseClient::threadproc_activate(int index)
{
   if ( boost::get_system_time() < this->m_activate_stamp )
   {
      this->m_proxy_index = index; // For activation we do not seek randomness, so we pick the particular.
      std::string epz = this->remote_hostname() + ":" + mylib::to_string(this->remote_port()) + "(" + mylib::to_string(this->m_activate_port) + ")";
      try
      {
         DOUT(info() << "Activate: " << index << " starting");
         boost::asio::io_service io_service;
         boost::asio::io_service::work session_work(io_service);
         boost::asio::ip::tcp::socket socket(io_service);

         this->dolog("Activate Performing remote connection to: " + epz );
         boost::asio::sockect_connect(socket, io_service, this->remote_hostname(), this->m_activate_port);

         RemoteEndpoint &ep = this->m_proxy_endpoints[index];
         std::error_code err;
         this->dolog("Activate Attempting to perform certificate exchange with " + ep.m_name );
         this->m_activate_stamp = boost::get_system_time(); // Reset to current time to avoid double attempts.
         std::vector<std::string> certnames;
         certnames.push_back(ep.m_name);
         if (global.SetupCertificatesClient(socket, certnames.front()) &&
            !global.SetupCertificatesServer( socket, certnames).empty() )
         {
            this->dolog("Succeeded in exchanging certificates with " + certnames.front());
         }
         else
         {
            this->dolog("Failed to exchange certificate: " + err.message() );
         }
      }        
      catch(std::exception &exc)
      {
         DERR("Failed to connect for activation: " << epz << " " << exc.what());
      }
   }
}


void BaseClient::stop_activate()
{
   this->m_thread_activate.stop();
}


void BaseClient::start_activate(int _index)
{
   this->m_thread_activate.start([=](){this->threadproc_activate(_index);});  
}


std::string BaseClient::local_portname() const
{
   return mylib::to_string(this->m_local_port);
}

bool BaseClient::activate(const std::string& name)
{
   int index = 0;
   for (auto& r : this->m_proxy_endpoints)
   {
      if (r.m_name == name)
      {
         this->m_activate_stamp = boost::get_system_time() + boost::posix_time::seconds(30); // The client should timeout quickly
         this->stop_activate();
         this->start_activate(index);
         return true;
      }
      index++;
   }
   return false;
}

bool BaseClient::set_active(const std::string& param, int id, bool active)
{
   if (this->m_id == id)
   {
      this->m_active = active;
      if (this->m_active)
      {
         this->start();
      }
      else
      {
         this->stop();
      }
      DOUT("Updated active state for: " << param << " new value: " << this->m_active);
      return true;
   }
   return false;
}

bool BaseClient::certificate_exists(const std::string& certname) const
{
   for (auto& r : this->m_proxy_endpoints)
   {
      if (r.m_name == certname)
      {
         return true;
      }
   }
   return false;
}

cppcms::json::value BaseClient::save_json_status()
{
   std::lock_guard<std::mutex> l(this->m_mutex_base);

   cppcms::json::object obj;
   obj["id"] = this->m_id;
   obj["active"] = this->m_active;
   obj["port"] = this->local_portname();
   obj["activate"]["port"] = this->activate_port();
   obj["connected_local"] = this->is_local_connected();
   obj["log"] = this->dolog();

   // Fill in the list of attached local hosts.
   std::vector<std::string> hostnames = this->local_hostnames();
   for (int index3 = 0; index3 < hostnames.size(); index3++)
   {
      std::string sz = hostnames[index3];
      if (sz.length())
      {
         obj["local_hostname"][index3] = sz;
      }
   }

   bool remote_conn = false;
   for (int index2 = 0; index2 < this->m_proxy_endpoints.size(); index2++)
   {
      if (this->is_remote_connected(index2))
      {
         remote_conn = true;
      }
   }
   for (int index2 = 0; index2 < this->m_proxy_endpoints.size(); index2++)
   {
      auto &r = this->m_proxy_endpoints[index2];
      cppcms::json::object obj2;
      obj2["name"] = r.m_name;
      obj2["hostname"] = r.m_hostname;
      obj2["port"] = r.m_port;
      if ((this->is_local_connected() && this->is_remote_connected(index2)) || !remote_conn)
      {
         obj2["connected_remote"] = this->is_remote_connected(index2);
         if (this->is_remote_connected(index2))
         {
            obj2["count_in"] = this->m_count_in.get();
            obj2["count_out"] = this->m_count_out.get();
         }
         obj["users"] = this->local_user_count();
      }
      if (global.certificate_available(r.m_name))
      {
         obj2["cert"] = true;
      }
      if (this->m_proxy_index == index2 && this->m_activate_stamp > boost::get_system_time())
      {
         auto div = this->m_activate_stamp - boost::get_system_time();
         obj2["activate"] = div.total_seconds();
      }
      obj["remotes"][index2] = obj2;
   }
   return obj;
}

cppcms::json::value BaseClient::save_json_config() const
{
   cppcms::json::value obj;
   obj["id"] = this->m_id;
   obj["active"] = this->m_active;
   obj["local_port"] = this->local_portname();
   obj["remote_hostname"] = this->remote_hostname();
   obj["remote_port"] = this->remote_port();
   obj["max_connections"] = this->m_max_connections;
   return obj;
}

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
// Copyright (C) 2011-2015 by GateHouse A/S
// All Rights Reserved.
// http://www.gatehouse.dk
// mailto:gh@gatehouse.dk
//====================================================================
#include "providerclient.h"
#include <boost/bind.hpp>
#include "proxy_global.h"
#include "cppcms_util.h"
#include <random>

using boost::asio::ip::tcp;


ProviderClient::ProviderClient(bool _active, mylib::port_type _activate_port, const std::vector<LocalEndpoint> &_local_endpoints, 
   const std::vector<RemoteEndpoint> &_proxy_endpoints, PluginHandler &_plugin, const cppcms::json::value &_json)
   : BaseClient(_active, 0, _activate_port, _proxy_endpoints, 1, _plugin),
   m_local_connected_index(0),
   m_thread_write([this]{this->interrupt_writer();}),
   mp_local_socket( nullptr ),
   json(_json)
{
   this->m_local_endpoints = _local_endpoints;
   int i;
   if (cppcms::utils::check_int(this->json, "buffer_size", i))
   {
      this->m_buffer_size = i*1024*1024;
   }
}


bool ProviderClient::is_local_connected() const
{
   stdt::lock_guard<stdt::mutex> l(this->m_mutex);
   return this->mp_local_socket != nullptr && is_connected(this->mp_local_socket->lowest_layer());
}


int ProviderClient::local_user_count() const
{
   return this->is_local_connected();
}


void ProviderClient::start()
{
   if (this->m_thread.is_running())
   {
      DOUT("Provider client is already running on port: " << this->port());
      return;
   }
   this->m_thread.start( [this]{ this->threadproc_reader(); } );
}


void ProviderClient::stop()
{
   {
      std::lock_guard<std::mutex> lock(this->m_buffer_mutex);
      this->m_buffer_condition.notify_one();
   }
   this->stop_activate();
   this->m_thread_write.stop();
   this->m_thread.stop();
}


void ProviderClient::interrupt_writer()
{
   std::lock_guard<std::mutex> lovk(this->m_buffer_mutex);
   this->m_buffer_condition.notify_one();
}


// The interrupt function may happen in local or other (main) thread context.
void ProviderClient::interrupt()
{
   try
   {
      stdt::lock_guard<stdt::mutex> l(this->m_mutex);

      if ( this->mp_local_socket != nullptr )
      {
         boost::system::error_code ec;
         this->mp_local_socket->lowest_layer().shutdown( boost::asio::socket_base::shutdown_both, ec );
      }
      if ( this->mp_remote_socket != nullptr )
      {
         boost::system::error_code ec;
         this->mp_remote_socket->lowest_layer().shutdown( boost::asio::socket_base::shutdown_both, ec );
      }
   }
   catch( std::exception &exc )
   {
      this->dolog( exc.what() );
   }
   DOUT(__FUNCTION__ << ":" << __LINE__ );
}


void ProviderClient::lock()
{
}


void ProviderClient::unlock()
{
   if (this->m_thread.is_running())
   {
//      this->m_thread.stop();
   }
   this->interrupt();
}



void ProviderClient::threadproc_writer()
{
   int timeout = 3000;
   while(this->m_thread_write.check_run())
   {
      try
      {
         DOUT("Provider writer start");
         this->m_count_out.clear();
         while(timeout < 10000)
            timeout += 1000;
         boost::asio::io_service io_service;
         boost::asio::ssl::context ssl_context( io_service, boost::asio::ssl::context::tlsv12);
         ssl_context.set_password_callback(boost::bind(&ProviderClient::get_password,this));
         ssl_context.set_verify_mode(boost::asio::ssl::context::verify_peer|boost::asio::ssl::context::verify_fail_if_no_peer_cert);
         ssl_context.load_verify_file(my_certs_name);
         ssl_context.use_certificate_chain_file(my_public_cert_name);
         ssl_context.use_private_key_file(my_private_key_name, boost::asio::ssl::context::pem);

         ssl_socket remote_socket( io_service, ssl_context );
         mylib::protect_pointer<ssl_socket> p2( this->mp_remote_socket, remote_socket, this->m_mutex );
         std::vector<int> indexes(this->m_proxy_endpoints.size());
         for (int index = 0; index < indexes.size(); index++)
         {
            indexes[index] = index;
         }
         std::shuffle(std::begin(indexes), std::end(indexes), std::default_random_engine(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count())));
         for (int index = 0; index < indexes.size(); index++)
         {
            this->m_thread_write.check_run();
            this->m_proxy_index = indexes[index]; // Get the next random connection.
            std::string ep = this->remote_hostname() + ":" + mylib::to_string(this->remote_port());
            if (global.certificate_available(this->m_proxy_endpoints[this->m_proxy_index].m_name))
            {
               try
               {
                  this->dolog("Performing remote connection to: " + ep );
                  boost::asio::sockect_connect( remote_socket.lowest_layer(), io_service, remote_hostname(), this->remote_port());
                  break;
               }
               catch( std::exception &exc )
               {
                  this->dolog("Failed connection to remote: " + ep);
               }
            }
            else
            {
               DOUT("Ignored due to missing certificate: " << ep);
            }
         }
         ASSERTE(this->is_remote_connected(), uniproxy::error::socket_invalid,"Provider failed connection to remote host");

         this->dolog("Provider connected to remote host: " + this->remote_hostname() + ":" + mylib::to_string(this->remote_port()) + " Attempting SSL handshake" );
         remote_socket.handshake(boost::asio::ssl::stream_base::client);
         this->dolog("Succesfull SSL handshake to remote host: " + this->remote_hostname() + ":" + mylib::to_string(this->remote_port()));

         boost::asio::socket_set_keepalive_to( remote_socket.lowest_layer(), std::chrono::seconds(20) );

         // This implementation uses the naive read some data and then write some data.
         // Since the TCP stack is using buffers internally this should run reasonably efficient.
         for ( ; this->m_thread_write.check_run(); )
         {
            {
               std::unique_lock<std::mutex> lk(this->m_buffer_mutex);
               if (this->m_buffer_condition.wait_for(lk, std::chrono::seconds(60)) == std::cv_status::timeout)
               {
                  DOUT("Condition timed out");
               }
            }
            std::string data;
            do
            {
               data.clear();
               {
                  std::lock_guard<std::mutex> lk(this->m_buffer_mutex);
                  if (!this->m_buffer.empty())
                  {
                     data = this->m_buffer.front();
                     this->m_buffer.erase(this->m_buffer.begin());
                  }
               }
               if (!data.empty())
               {
                  Buffer buffer((void*)data.data(), data.size());
                  if ( this->m_plugin.message_filter_local2remote( buffer ) ) //, full ) )
                  {
                     // The plugin is allowed to modify the buffer, thus we need to recalculate size
                     int length = remote_socket.write_some(boost::asio::buffer( buffer.m_buffer, buffer.m_size));
                     
                     this->m_count_out.add(length);
                     //DOUT("data size sent: " << length);
                  }
               }               
            }
            while (!data.empty());
         }
      }
      catch(std::exception &exc)
      {
         DERR("Exception: " << exc.what());
      }
      this->m_thread_write.sleep(timeout);
   }
}


void ProviderClient::threadproc_reader()
{
   int timeout = 5000;
   for ( ; this->m_thread.check_run() ; )
   {
      // On start and after each lost connection we end up here.
      try
      {
         if (timeout < 60000)
         {
            timeout += 5000;
         }
         this->m_thread.sleep( timeout );

         bool found = false;
         for ( int index = 0; index < this->m_proxy_endpoints.size(); index++ )
         {
            if (global.certificate_available(this->m_proxy_endpoints[index].m_name))
            {
               found = true;
            }
         }
         if (!found) continue;

         this->m_proxy_index = 0;
         this->m_local_connected_index = 0;

         boost::asio::io_service io_service;

         stdt::lock_guard<ProviderClient> l(*this);

         boost::asio::io_service::work session_work(io_service);

         boost::asio::ip::tcp::socket local_socket(io_service);
         mylib::protect_pointer<boost::asio::ip::tcp::socket> p1( this->mp_local_socket, local_socket, this->m_mutex );
         std::string ep;
         for ( int index = 0; index < this->m_local_endpoints.size(); index++ )
         {
            ep = this->m_local_endpoints[index].m_hostname + ":" + mylib::to_string(this->m_local_endpoints[index].m_port);
            try
            {
               this->dolog("Provider connecting to local: " + ep );
               boost::asio::sockect_connect( local_socket, io_service, this->m_local_endpoints[index].m_hostname, this->m_local_endpoints[index].m_port );
               this->m_local_connected_index = index;
               break;
            }
            catch( std::exception &exc )
            {
               DOUT(  __FUNCTION__ << ":" << __LINE__ << " Failed connection to: " << ep << " " << exc.what() );
            }
         }
         ASSERTE(this->is_local_connected(), uniproxy::error::socket_invalid,"Provider failed connection to local host: " + ep);
         boost::asio::socket_set_keepalive_to( local_socket, std::chrono::seconds(20) );
         
         this->m_thread_write.start([this]{this->threadproc_writer();});

         for ( ; this->m_thread.check_run(); )
         {
            int length;
            length = local_socket.read_some( boost::asio::buffer( this->m_local_data, max_length-1 ) );
            
            if (length > 0)
            {
               this->m_local_data[length] = 0;
               std::lock_guard<std::mutex> lk(this->m_buffer_mutex);
               while (!this->m_buffer.empty() && this->m_buffer_count > this->m_buffer_size)
               {
                  this->m_buffer_count -= this->m_buffer.front().size();
                  this->m_buffer.erase(this->m_buffer.begin());
               }
               if (this->m_buffer.empty())
               {
                  this->m_buffer_count = 0; // Just reset when we can.
               }
               this->m_buffer_count += length;
               this->m_buffer.push_back(this->m_local_data);
               this->m_buffer_condition.notify_one();
            }
         }
      }
      catch( std::exception &exc )
      {
         this->dolog(exc.what());
      }
   }
}


std::string ProviderClient::local_hostname() const
{
   return this->m_local_endpoints[this->m_local_connected_index].m_hostname;
}


std::vector<std::string> ProviderClient::local_hostnames() const
{
   std::vector<std::string> result;
   for ( auto &ep : this->m_local_endpoints )
   {
      result.push_back(ep.m_hostname);
   }
   return result;
}


std::string ProviderClient::local_portname() const
{
   return "Provider";
}


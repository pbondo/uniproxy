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
#include "localclient.h"

#include <boost/bind.hpp>
#include "proxy_global.h"

using boost::asio::ip::tcp;
using boost::asio::deadline_timer;


int LocalHostSocket::id_gen = 0;

LocalHostSocket::LocalHostSocket(LocalHost &_host, boost::asio::ip::tcp::socket *_socket)
: m_host(_host)
{
   this->id = ++id_gen;
   this->m_socket = _socket;
}


void LocalHostSocket::handle_local_read(const boost::system::error_code& _error,size_t _bytes_transferred)
{
   this->m_host.handle_local_read(*this, _error, _bytes_transferred);
}


boost::asio::ip::tcp::socket &LocalHostSocket::socket()
{
   ASSERTE(this->m_socket,uniproxy::error::socket_invalid,"Fatal Invalid socket");
   return *this->m_socket;
}


LocalHost::LocalHost(bool _active, mylib::port_type _local_port, mylib::port_type _activate_port, const std::vector<RemoteEndpoint> &_proxy_endpoints, const int _max_connections, PluginHandler &_plugin, const boost::posix_time::time_duration &_read_timeout)
   : BaseClient(_active, _local_port, _activate_port, _proxy_endpoints, _max_connections, _plugin),
   mp_io_service( nullptr ),
   mp_acceptor( nullptr ),
   m_pdeadline( nullptr ),
   m_read_timeout(_read_timeout)
{
   this->m_local_connected = false; 
}


bool LocalHost::is_local_connected() const
{
   bool result = false;
   std::lock_guard<std::mutex> l(this->m_mutex);
   for ( auto iter = this->m_local_sockets.begin(); iter != this->m_local_sockets.end(); iter++ )
   {
      if ( (*iter)->socket().is_open() && this->m_local_connected )
      {
         result = true;
      }
   }
   return result;
}


int LocalHost::local_user_count() const
{
   int result = 0;
   std::lock_guard<std::mutex> l(this->m_mutex);
   for ( auto iter = this->m_local_sockets.begin(); iter != this->m_local_sockets.end(); iter++ )
   {
      if ( (*iter)->socket().is_open() && this->m_local_connected )
      {
         result++;
      }
   }
   return result;
}


std::vector<std::string> LocalHost::local_hostnames() const
{
   std::vector<std::string> result;
   std::lock_guard<std::mutex> l(this->m_mutex);
   for ( auto iter = this->m_local_sockets.begin(); iter != this->m_local_sockets.end(); iter++ )
   {
      boost::asio::ip::tcp::socket &socket((*iter)->socket());
      boost::system::error_code ec;
      std::string sz = socket.remote_endpoint(ec).address().to_string(); // This one should not throw exceptions
      result.push_back(sz);
   }
   return result;
}


void LocalHost::start()
{
   if (this->m_thread.is_running())
   {
      DOUT("LocalHost already running on port: " << this->port());
      return;
   }
   this->m_thread.start( [this]{ this->threadproc(); } );
}


void LocalHost::stop()
{
   this->stop_activate();
   this->m_thread.stop();
}


// This will always happen in local thread context.
void LocalHost::cleanup()
{
   try
   {
      std::lock_guard<std::mutex> l(this->m_mutex);
      while ( this->m_local_sockets.size() > 0 )
      {
         boost::system::error_code ec;
         TRY_CATCH( this->m_local_sockets.front()->socket().close(ec) );
         this->m_local_sockets.erase( this->m_local_sockets.begin() ); // Since we use shared_ptr it should autodelete.
      }
      this->mp_acceptor = NULL;
   }
   catch( std::exception &exc )
   {
      this->dolog( exc.what() );
   }
}


// The interrupt function may happen in local or other (main) thread context.
void LocalHost::interrupt()
{
   try
   {
      DOUT("Enter");
      {
         std::lock_guard<std::mutex> l(this->m_mutex);
         if ( this->mp_acceptor != nullptr )
         {
            boost::system::error_code ec;
            TRY_CATCH( (*this->mp_acceptor).cancel(ec) );
            TRY_CATCH( boost::asio::socket_shutdown( *this->mp_acceptor,ec ) );
         }
      }
      {
         std::lock_guard<std::mutex> l(this->m_mutex);

         for ( int index = 0; index < this->m_local_sockets.size(); index++ )
         {
            boost::system::error_code ec;
            TRY_CATCH( this->m_local_sockets[index]->socket().shutdown( boost::asio::socket_base::shutdown_both, ec ) );
         }
      }
      {
         std::lock_guard<std::mutex> l(this->m_mutex);
         if ( this->mp_remote_socket != nullptr )
         {
            boost::system::error_code ec;
            TRY_CATCH( (*this->mp_remote_socket).lowest_layer().shutdown( boost::asio::socket_base::shutdown_both, ec ) );
         }
      }
      DOUT("Completed");
   }
   catch( std::exception &exc )
   {
      this->dolog( exc.what() );
   }
}


void LocalHost::remove_socket( boost::asio::ip::tcp::socket &_socket )
{
   std::lock_guard<std::mutex> l(this->m_mutex);
   for ( auto iter = this->m_local_sockets.begin(); iter != this->m_local_sockets.end(); iter++ )
   {
      if ( (*iter)->socket() == _socket )
      {
         boost::system::error_code ec;
         TRY_CATCH( _socket.shutdown(boost::asio::socket_base::shutdown_both,ec) );
         TRY_CATCH( _socket.close(ec) );
         this->m_local_sockets.erase(iter);
         break;
      }
   }
}


void LocalHost::handle_local_read( LocalHostSocket &_hostsocket, const boost::system::error_code& error,size_t bytes_transferred)
{
   if (!error)
   {
      this->m_count_out.add( bytes_transferred );
      Buffer buffer( this->m_local_data, bytes_transferred );
      this->m_last_outgoing_msg = this->m_local_data;
      this->m_last_outgoing_stamp = boost::get_system_time();
      if (global.m_out_data_log_file.is_open())
      {
         this->m_local_data[bytes_transferred] = 0;
         global.m_out_data_log_file << "[" << mylib::to_string(boost::get_system_time()) << "]" << this->m_local_data;
      }
      boost::asio::async_write( this->remote_socket(), boost::asio::buffer( this->m_local_data, bytes_transferred), boost::bind(&LocalHost::handle_remote_write, this, _hostsocket.id, boost::asio::placeholders::error));
   }
   else
   {
      DERR(local_address_port(_hostsocket.socket()) << " Error: " << error << " connections: " << this->m_local_sockets.size());
      this->remove_socket(_hostsocket.socket());
      DOUT(" Last outgoing msg: " << this->m_last_outgoing_stamp << ":" << this->m_last_outgoing_msg << " connections: " << this->m_local_sockets.size());
      if (this->m_local_sockets.empty())
      {
         throw std::runtime_error( "Local connection closed for " + mylib::to_string(this->m_local_port) );
      }
   }
}


void LocalHost::handle_local_write( boost::asio::ip::tcp::socket *_socket, const boost::system::error_code& error)
{
   this->m_write_count--;
   if (!error)
   {
   }
   else
   {
      DOUT(local_address_port(*_socket) << " One of the attached sockets disconnected: " << remote_address_port(*_socket));
      this->remove_socket(*_socket);
   }
   if ( this->m_write_count == 0 && this->m_local_sockets.size() > 0 )
   {
      this->m_pdeadline->expires_from_now(this->m_read_timeout);

      this->remote_socket().async_read_some( boost::asio::buffer( this->m_remote_data, max_length), boost::bind(&LocalHost::handle_remote_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
   }
   if ( this->m_local_sockets.size() == 0 )
   {
      throw std::runtime_error( __FUNCTION__ );       
   }
}


void LocalHost::handle_remote_read(const boost::system::error_code& error,size_t bytes_transferred)
{
   if (!error)
   {
      this->m_count_in.add( bytes_transferred );
      std::lock_guard<std::mutex> l(this->m_mutex);
      this->m_remote_data[bytes_transferred] = 0;
      this->m_last_incoming_msg = this->m_remote_data;
      this->m_last_incoming_stamp = boost::get_system_time();
      if (global.m_in_data_log_file.is_open())
      {
         global.m_in_data_log_file << "[" << mylib::to_string(boost::get_system_time()) << "]" << this->m_remote_data;
      }
      this->m_write_count = this->m_local_sockets.size();
      for ( int index = 0; index < this->m_write_count; index++ )
      {
         boost::asio::ip::tcp::socket *psocket = &this->m_local_sockets[index]->socket();
         boost::asio::async_write( *psocket, boost::asio::buffer( this->m_remote_data, bytes_transferred), boost::bind(&LocalHost::handle_local_write, this, psocket, boost::asio::placeholders::error));
      }
   }
   else
   {
      DOUT("Error: " << error << ": " << error.message() << " bytes transferred: " << bytes_transferred);
      DOUT("Last incoming msg: " << this->m_last_incoming_stamp << ":" << this->m_last_incoming_msg);
      throw boost::system::system_error( error );
   }
}


void LocalHost::handle_remote_write(int id, const boost::system::error_code& error)
{
   if (!error)
   {
      for (int i = 0; i < this->m_local_sockets.size(); i++)
      {
         LocalHostSocket *ls = this->m_local_sockets[i].get();
         if (ls->id == id)
         {
            ls->socket().async_read_some( boost::asio::buffer( this->m_local_data, max_length), boost::bind(&LocalHostSocket::handle_local_read, ls, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
            break;
         }
      }
   }
   else
   {
      DOUT( "Error: " << error << " in " << __FUNCTION__ << ":" <<__LINE__);
      throw boost::system::system_error( error );
   }
}


void LocalHost::handle_accept( boost::asio::ip::tcp::socket *_socket, const boost::system::error_code& error )
{
   int count = this->m_local_sockets.size();
   DOUT(local_address_port(*_socket) << " error?: " << error << " Added extra local socket " << remote_address_port(*_socket) << " now " << this->m_local_sockets.size() << " connections");
   if (!error && _socket)
   {
      if (count < this->m_max_connections)
      {
         auto p = std::make_shared<LocalHostSocket>(*this, _socket);
         this->m_local_sockets.push_back(p);

         // NB!! Currently all will read to the same buffer, so it will crash eventually.
         _socket->async_read_some(boost::asio::buffer(m_local_data, max_length), boost::bind(&LocalHostSocket::handle_local_read, p.get(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
      }
      else
      {
         DOUT("Already too many connections: " << count << " vs. " << this->m_max_connections);
         boost::system::error_code ec;
         _socket->close(ec);
         delete _socket;
      }
   }
   else
   {
      DOUT(local_address_port(*_socket) << " Local removed, now " << this->m_local_sockets.size() << " connections");
      delete _socket; // NB!! This is not really the right way. Explore passing the shared_ptr as a parameter instead.
   }

   // Unconditionally we start looking for the next socket.
   if ( this->mp_io_service != NULL && this->mp_acceptor != NULL && this->m_local_sockets.size() > 0)
   {
      
      boost::asio::ip::tcp::socket *new_socket = new boost::asio::ip::tcp::socket( *this->mp_io_service);
      this->mp_acceptor->async_accept( *new_socket, boost::bind(&LocalHost::handle_accept, this, new_socket, boost::asio::placeholders::error));
   }
}


// Read from the remote socket did timeout.
// Oddly this is also called whenever a succesfull read was performed.
void LocalHost::check_deadline()
{
   bool call_interrupt = false;
   {
      std::lock_guard<std::mutex> l(this->m_mutex);
      if (this->m_pdeadline == nullptr || this->m_pdeadline->expires_at() <= deadline_timer::traits_type::now())
      {
         DERR(":" << this->port() << " Timeout read from remote socket");
         if (this->m_pdeadline != nullptr)
         {
            this->m_pdeadline->expires_at(boost::posix_time::pos_infin);
         }
         boost::system::error_code ec;
         this->remote_socket().shutdown(ec);
         this->remote_socket().lowest_layer().close();
         // This call should stop all local connections.
         call_interrupt = true;
      }
      // Put the actor back to sleep.
      if (this->m_pdeadline != nullptr)
      {  // This is called for every transmission.
         this->m_pdeadline->async_wait(boost::bind(&LocalHost::check_deadline, this));
      }
   }
   if (call_interrupt)
   {  // Must happen outside the mutex
      this->interrupt();
   }
}


void LocalHost::go_out(boost::asio::io_service &io_service)
{
   try
   {
      boost::asio::deadline_timer deadline(io_service);
      mylib::protect_pointer<boost::asio::deadline_timer> p_deadline( this->m_pdeadline, deadline, this->m_mutex );
      boost::asio::ssl::context ssl_context( io_service, boost::asio::ssl::context::tlsv12);
      ssl_context.set_password_callback(boost::bind(&LocalHost::get_password,this));
      ssl_context.set_verify_mode(boost::asio::ssl::context::verify_peer|boost::asio::ssl::context::verify_fail_if_no_peer_cert);
      ssl_context.load_verify_file(my_certs_name);
      ssl_context.use_certificate_chain_file(my_public_cert_name);
      ssl_context.use_private_key_file(my_private_key_name, boost::asio::ssl::context::pem);
      ssl_socket remote_socket( io_service, ssl_context );
      mylib::protect_pointer<ssl_socket> p2( this->mp_remote_socket, remote_socket, this->m_mutex );

      this->dolog("Connecting to remote host: " + this->remote_hostname() + ":" + mylib::to_string(this->remote_port()) );
      boost::asio::sockect_connect( remote_socket.lowest_layer(), io_service, this->remote_hostname(), this->remote_port() );

      this->dolog("Connected to remote host: " + this->remote_hostname() + ":" + mylib::to_string(this->remote_port()) + " Attempting SSL handshake" );
      DOUT( "handles: " << remote_socket.next_layer().native_handle() << " / " << remote_socket.lowest_layer().native_handle() );

      boost::asio::socket_set_keepalive_to(remote_socket.lowest_layer(), std::chrono::seconds(20));

      remote_socket.handshake( boost::asio::ssl::stream_base::client );
      this->dolog("Succesfull SSL handshake to remote host: " + this->remote_hostname() + ":" + mylib::to_string(this->remote_port()) );

      DOUT(" Prepare timeout at: " << this->m_read_timeout)
      deadline.async_wait(boost::bind(&LocalHost::check_deadline, this));
      deadline.expires_from_now(this->m_read_timeout);
      remote_socket.async_read_some( boost::asio::buffer( m_remote_data, max_length), boost::bind(&LocalHost::handle_remote_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred) );

      // Now let the io_service handle the session.
      io_service.run();

      deadline.cancel();
   }
   catch (std::exception &exc)
   {
      this->dolog(exc.what());
      if (this->m_local_sockets.empty())
      {
         throw;
      }
   }
}


void LocalHost::threadproc()
{
   for ( ; this->m_thread.check_run() ; )
   {
      // On start and after each lost connection we end up here.
      try
      {
         DOUT(__FUNCTION__ << ":" << __LINE__);
         boost::asio::io_service io_service;
         mylib::protect_pointer<boost::asio::io_service> p_io_service( this->mp_io_service, io_service, this->m_mutex );

         // We make this as a pointer because there may be more than one.
         boost::asio::ip::tcp::socket *local_socket = new boost::asio::ip::tcp::socket(io_service);

         // NB!! This will only support ipv4
         boost::asio::ip::tcp::acceptor acceptor( io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), this->m_local_port) );

         mylib::protect_pointer<boost::asio::ip::tcp::acceptor> p3( this->mp_acceptor, acceptor, this->m_mutex );
         boost::asio::io_service::work session_work(io_service);

         std::shared_ptr<void> ptr( NULL, [this](void*){ DOUT("local exit loop"); this->interrupt(); this->cleanup();} );
         this->dolog("Waiting for local connection" );

         // Synchronous wait for connection from local TCP socket. Must be handled by the interrupt function
         acceptor.accept( *local_socket );
         auto p = std::make_shared<LocalHostSocket>(*this, local_socket);
         this->m_local_sockets.push_back( p );  // NB!! redo by inserting line above directly
         this->m_local_connected = true;

         boost::asio::socket_set_keepalive_to( *local_socket, std::chrono::seconds(20) );
         local_socket->async_read_some( boost::asio::buffer( m_local_data, max_length), boost::bind(&LocalHostSocket::handle_local_read, p.get(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred) );

         // NB!! The following line may leak.... but only very slow.
         boost::asio::ip::tcp::socket *new_socket = new boost::asio::ip::tcp::socket(io_service);
         acceptor.async_accept( *new_socket, boost::bind(&LocalHost::handle_accept, this, new_socket, boost::asio::placeholders::error));

         while(this->m_thread.check_run() && !this->m_local_sockets.empty())
         {
            if (!this->m_proxy_endpoints.empty()) // Pick a new random access value.
            {
               this->m_proxy_index = std::rand() % this->m_proxy_endpoints.size();
            }
            this->go_out(io_service);
            this->m_thread.sleep(2000);
         }
      }
      catch( std::exception &exc )
      {
         this->dolog(exc.what());
      }
      this->m_local_connected = false;
      this->m_thread.sleep( 1500 ); //We will need some time to ensure the remote end has settled. May need to be investigated.
   }
}


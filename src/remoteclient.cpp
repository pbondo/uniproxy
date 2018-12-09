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
#include "remoteclient.h"
#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/regex.hpp>
#include "proxy_global.h"
#include <random>

static int static_remote_count = 0;


std::ostream & operator << (std::ostream & os, const RemoteProxyHost &host)
{
   os << "[";
   for (auto item : host.m_remote_ep)
   {
      os << " port: " << item.m_port << " hostname " << item.m_hostname << " name " << item.m_name << ",";
   }
   os << "][";
   for (auto item : host.m_local_ep)
   {
      os << " hostname " << item.m_hostname << " port " << item.m_port << ",";
   }
   os << "]";
   return os;
}


/*
When a connection is established a an instance of RemoteProxyClient is started.
It will initiate 2 threads:
 * 1 to handle reading from the connection to the local host
 * 1 to handle reading from the connection to the remote proxy
 *
 * When one of threads detects a socket disconnect it simply disconnect both sockets.
 *
 */
RemoteProxyClient::RemoteProxyClient(boost::asio::io_service& io_service, boost::asio::ssl::context& context, RemoteProxyHost &_host )
:  m_local_socket(io_service), m_remote_socket(io_service, context), 
   m_remote_thread( [&]{ this->interrupt(); } ),
   m_local_thread( [&]{ this->interrupt(); } ),
   m_io_service(io_service),
   m_host(_host)
{
   this->m_local_connected = this->m_remote_connected = false;
   this->m_remote_read_buffer = new unsigned char[ this->m_host.m_plugin.max_buffer_size() + 1 ];
   this->m_local_read_buffer = new unsigned char[ this->m_host.m_plugin.max_buffer_size() + 1 ];
}


RemoteProxyClient::~RemoteProxyClient()
{
   DOUT(this->dinfo());
   delete[] this->m_remote_read_buffer;
   delete[] this->m_local_read_buffer;
}


std::string RemoteProxyClient::dinfo()
{
   std::string cached;
   boost::system::error_code ec;
   boost::asio::ip::tcp::endpoint ep_local = this->m_local_socket.lowest_layer().remote_endpoint(ec);
   if (!ec)
   {
      this->m_local_cache = ep_local;
   }
   boost::asio::ip::tcp::endpoint ep_remote = this->m_remote_socket.lowest_layer().remote_endpoint(ec);
   if (!ec)
   {
      this->m_remote_cache = ep_remote;
   }
   else
   {
      cached = " Cached.";
   }

   std::ostringstream oss;
   oss << this->m_remote_cache << " => " << this->m_local_cache << cached << " ";
   return oss.str();
}


void RemoteProxyClient::dolog( const std::string &_line )
{
   this->m_host.dolog(_line);
}


bool RemoteProxyClient::is_local_connected()
{
   return this->m_local_socket.is_open() && this->m_local_connected;
}


bool RemoteProxyClient::is_remote_connected()
{
   return this->m_remote_socket.lowest_layer().is_open() && this->m_remote_connected;
}


boost::asio::ip::tcp::endpoint RemoteProxyClient::local_endpoint() const
{
   boost::system::error_code ec;
   boost::asio::ip::tcp::endpoint ep = this->m_local_socket.remote_endpoint(ec);
   return ep;
}


boost::asio::ip::tcp::endpoint RemoteProxyClient::remote_endpoint() const
{
   boost::system::error_code ec;
   boost::asio::ip::tcp::endpoint ep = this->m_remote_socket.lowest_layer().remote_endpoint(ec);
   return ep;
}


ssl_socket::lowest_layer_type& RemoteProxyClient::socket()
{
   return this->m_remote_socket.lowest_layer();
}


void RemoteProxyClient::start( std::vector<LocalEndpoint> &_local_ep )
{
   this->m_local_ep = _local_ep;
   this->m_remote_thread.start( [&]{ this->remote_threadproc(); } );
}


bool RemoteProxyClient::is_active()
{
   return this->m_local_thread.is_running() || this->m_remote_thread.is_running();
}


void RemoteProxyClient::stop()
{
   // The threads are stopped and joined.
   this->m_local_thread.stop();
   this->m_remote_thread.stop();
}


void RemoteProxyClient::interrupt()
{
   boost::system::error_code ec;
   DOUT(this->dinfo());
   if ( this->m_local_socket.is_open() )
   {
      TRY_CATCH( this->m_local_socket.shutdown( boost::asio::socket_base::shutdown_both, ec ) );
      TRY_CATCH( this->m_local_socket.close(ec) );
   }
   if ( this->m_remote_socket.lowest_layer().is_open() )
   {
      this->m_remote_socket.lowest_layer().cancel(ec);
   }
   this->m_local_connected = this->m_remote_connected = false;
}


// This thread is the primary data mover for e.g. radar and AIS tracks.
// It serves data sent from the host system to the client system.
void RemoteProxyClient::local_threadproc()
{
   try
   {
      DOUT(this->dinfo());
      boost::asio::socket_set_keepalive_to( this->m_local_socket, std::chrono::seconds(20) );

      if ( this->m_host.m_plugin.stream_local2remote(this->m_local_socket, this->m_remote_socket, this->m_local_thread ) )
      {
         // The plugin does handle all the data streaming itself, so nothing left for us to do.
      }
      else
      {
         // This implementation uses the naive read some data and then write some data.
         // Since the TCP stack is using buffers internally this should run reasonably efficient.
         for ( ; this->m_local_thread.check_run(); )
         {
            int length;
            boost::system::error_code ec;
            length = this->m_local_socket.read_some( boost::asio::buffer( this->m_local_read_buffer, this->m_host.m_plugin.max_buffer_size() ), ec );
            if (ec.value() != 0 || length == 0)
            {
               DOUT(this->dinfo() << "Local read socket Failed reading data " << ec.category().name() << " val: " << (int)ec.value() << " msg: " << ec.category().message(ec.value()) << " length: " << length);
               DOUT(this->dinfo() << "Last outgoing message: " << this->m_last_outgoing_stamp << ":" << this->m_last_outgoing_msg);
               break;
            }
            this->m_local_read_buffer[length] = 0;
            this->m_last_outgoing_stamp = boost::get_system_time();
            this->m_last_outgoing_msg = (const char*)this->m_local_read_buffer;
            if (global.m_out_data_log_file.is_open())
            {
               std::ofstream ofs(global.m_log_path + "out_" + this->m_endpoint.m_name + ".log", std::ios::ate | std::ios::app | std::ios::binary);
               ofs << "[" << mylib::to_string(boost::get_system_time()) << "]" << this->m_local_read_buffer;
            }
            Buffer buffer( this->m_local_read_buffer, length );
            if ( this->m_host.m_plugin.message_filter_local2remote( buffer ) )
            {
               // The plugin is allowed to modify the buffer, thus we need to recalculate size
               length = this->m_remote_socket.write_some( boost::asio::buffer( buffer.m_buffer, buffer.m_size ) );
               this->m_count_out.add(length);
            }
         }
      }
   }
   catch( std::exception &exc )
   {
      this->dolog(dinfo() + exc.what());
   }
   DOUT(this->dinfo() << "Thread stopping");
   this->interrupt();
   boost::system::error_code ec;
   this->m_remote_socket.shutdown(ec);
   this->m_remote_socket.lowest_layer().close(ec);
   DOUT(this->dinfo() << "Thread stopped");
}


int RemoteProxyClient::test_local_connection(const std::string& name, const std::vector<LocalEndpoint> &_local_ep)
{
   int result = 500;
   try
   {
      this->m_local_ep = _local_ep;
      for (auto rep : this->m_host.m_remote_ep)
      {
         if (rep.m_name == name)
         {
            this->m_endpoint = rep;
            break;
         }
      }
      DOUT(this->dinfo() << "Test host, found remote connection: " << this->m_endpoint.m_name << " is connected locally? " << this->m_local_connected);
      if (this->m_local_connected)
      {
         return 429;
      }
      this->m_local_connected = false;
      std::vector<int> indexes(this->m_local_ep.size());
      for (int index = 0; index < indexes.size(); index++)
      {
         indexes[index] = index;
      }
      std::shuffle(std::begin(indexes), std::end(indexes), std::default_random_engine(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count())));
      for (int i = 0; i < indexes.size(); i++)
      {
         DOUT(this->dinfo() << "Test Host Random i: " << i << " index " << indexes[i] << " size: " << indexes.size());
      }
      std::string ep;
      for (int index = 0; index < this->m_local_ep.size(); index++)
      {
         int proxy_index = indexes[index];
         ep = this->m_local_ep[proxy_index].m_hostname + ":" + mylib::to_string(this->m_local_ep[proxy_index].m_port);
         try
         {
            this->dolog(this->dinfo() + "Test Host Performing local connection to: " + ep );
            boost::asio::sockect_connect(this->m_local_socket, this->m_io_service, this->m_local_ep[proxy_index].m_hostname, this->m_local_ep[proxy_index].m_port);
            this->m_local_connected = true;
            break;
         }
         catch( std::exception &exc )
         {
            DOUT(this->dinfo() << " Failed connection to: " << ep << " " << exc.what() );
         }
      }
      if (!this->m_local_connected)
      {
         return 422;
      }
      try
      {
         this->dolog(this->dinfo() + "Performing logon procedure to " + ep);
         if (this->m_host.m_plugin.connect_handler(this->m_local_socket, this->m_endpoint))
         {
            result = 200;
            this->dolog(this->dinfo() + "Test Host Completed successfully " + ep);
         }
      }
      catch(std::exception& exc)
      {
         this->dolog(this->dinfo() + exc.what());
         result = 401;
      }
      this->dolog(this->dinfo() + "Test Host done test logon procedure to " + ep);
      this->m_local_connected = false;
      if (this->m_local_socket.is_open())
      {
         boost::system::error_code ec;
         TRY_CATCH(this->m_local_socket.shutdown(boost::asio::socket_base::shutdown_both, ec));
         TRY_CATCH(this->m_local_socket.close(ec));
      }
   }
   catch(boost::system::system_error &boost_error)
   {
      std::ostringstream oss;
      oss << boost_error.code() << " what: " << boost_error.what();
      DOUT(this->dinfo() << "Test Host Boost or system error " + oss.str());
      result = 500;
   }
   catch(std::exception &exc)
   {
      this->dolog(this->dinfo() + exc.what());
      result = 500;
   }
   return result;
}


// Handle the remote SSL connection.
void RemoteProxyClient::remote_threadproc()
{
   try
   {
      DOUT(this->dinfo());
      if ( this->m_local_ep.size() == 0 )
      {
         throw std::runtime_error("No local endpoints found");
      }
      this->m_remote_connected = true; // Should perhaps be after the SSL handshake.
      this->dolog(this->dinfo() + "Performing SSL hansdshake connection");
      this->m_remote_socket.handshake( boost::asio::ssl::stream_base::server );
      this->dolog(this->dinfo() + "SSL connection ok");

      bool hit = false;
      // See if we can find the relevant
      std::string issuer, subject, common_name = "NOT VALID";
      if ( get_certificate_issuer_subject( this->m_remote_socket, issuer, subject ) && issuer.length() > 0 )
      {
         std::vector< std::string > result;
         boost::algorithm::split_regex( result, issuer, boost::regex( "/CN=" ) );
         if ( result.size() > 1 )
         {
            common_name = result[1];
         }
         DOUT(this->dinfo() << "Received certificate CN= " << common_name );
         for ( auto iter1 = this->m_host.m_remote_ep.begin(); iter1 != this->m_host.m_remote_ep.end(); iter1++ )
         {
            if ( common_name == (*iter1).m_name )
            {
               hit = true;
               this->m_endpoint = (*iter1);
               break;
            }
         }
      }
      if ( !hit )
      {
         throw std::runtime_error("Certificate valid but no active connections specified: " + common_name );
      }
      this->m_local_connected = false;
      std::vector<int> indexes(this->m_local_ep.size());
      for (int index = 0; index < indexes.size(); index++)
      {
         indexes[index] = index;
      }
      std::shuffle(std::begin(indexes), std::end(indexes), std::default_random_engine(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count())));
      for (int i = 0; i < indexes.size(); i++)
      {
         DOUT(this->dinfo() << "Random i: " << i << " index " << indexes[i] << " size: " << indexes.size());
      }
      std::string ep;
      for (int index = 0; index < this->m_local_ep.size(); index++)
      {
         int proxy_index = indexes[index];
         ep = this->m_local_ep[proxy_index].m_hostname + ":" + mylib::to_string(this->m_local_ep[proxy_index].m_port);
         try
         {
            this->dolog(this->dinfo() + "Performing local connection to: " + ep );
            boost::asio::sockect_connect( this->m_local_socket, this->m_io_service, this->m_local_ep[proxy_index].m_hostname, this->m_local_ep[proxy_index].m_port );
            this->m_local_connected = true;
            break;
         }
         catch( std::exception &exc )
         {
            DOUT(this->dinfo() << " Failed connection to: " << ep << " " << exc.what() );
         }
      }
      if ( !this->m_local_connected )
      {
         throw std::runtime_error("Failed connection to local host");
      }
      this->dolog(this->dinfo() + "Performing logon procedure to " + ep);
      if ( ! this->m_host.m_plugin.connect_handler( this->m_local_socket, this->m_endpoint ) )
      {     
         throw std::runtime_error("Failed plugin connect_handler for type: " + this->m_host.m_plugin.m_type );
      }
      this->dolog(this->dinfo() + "Completed logon procedure to " + ep);
      this->m_local_thread.start( [&]{this->local_threadproc(); } );
      boost::asio::socket_set_keepalive_to( this->m_remote_socket.lowest_layer(), std::chrono::seconds(20) );
      for ( ; this->m_remote_thread.check_run(); )
      {
         boost::system::error_code ec;
         int length = this->m_remote_socket.read_some(boost::asio::buffer(this->m_remote_read_buffer, this->m_host.m_plugin.max_buffer_size()), ec);
         if (ec.value() != 0 || length == 0)
         {
            DOUT(this->dinfo() << "Remote read socket Failed reading data " << ec.category().name() << " val: " << (int)ec.value() << " msg: " << ec.category().message(ec.value()) << " length: " << length);
            DOUT(this->dinfo() << "Last received msg: " << this->m_last_incoming_stamp << ":" << this->m_last_incoming_msg);
            break;
         }
         if (length > 0)
         {
            this->m_remote_read_buffer[length] = 0;
            this->m_last_incoming_msg = (const char*)this->m_remote_read_buffer;
            this->m_last_incoming_stamp = boost::get_system_time();
            if (global.m_in_data_log_file.is_open())
            {
               std::ofstream ofs(global.m_log_path + "in_" + common_name + ".log", std::ios::ate | std::ios::app | std::ios::binary);
               ofs << "[" << mylib::to_string(boost::get_system_time()) << "]" << this->m_remote_read_buffer;
            }
            Buffer buffer( this->m_remote_read_buffer, length );
            if ( this->m_host.m_plugin.message_filter_remote2local( buffer ) )
            {
               length = this->m_local_socket.write_some( boost::asio::buffer( buffer.m_buffer, buffer.m_size ) );
               this->m_count_in.add(length);
            }
            else
            {
               // This should not happen too often.
               this->dolog(this->dinfo() + "Data overflow");
            }
         }
      }
   }
   catch( boost::system::system_error &boost_error )
   {
      std::ostringstream oss;
      oss << "SSL: " << boost_error.code() << " what: " << boost_error.what();
      this->dolog(this->dinfo() + oss.str());
   }
   catch( std::exception &exc )
   {
      this->dolog(this->dinfo() + exc.what());
   }
   DOUT(this->dinfo() << "Thread stopping");
   this->interrupt();
   boost::system::error_code ec;
   this->m_remote_socket.shutdown(ec);
   this->m_remote_socket.lowest_layer().close(ec);
   DOUT(this->dinfo() << "Thread stopped");
}


RemoteProxyHost::RemoteProxyHost( unsigned short _local_port, const std::vector<RemoteEndpoint> &_remote_ep, const std::vector<LocalEndpoint> &_local_ep, PluginHandler &_plugin )
:  m_io_service(),
   m_context(boost::asio::ssl::context::tlsv12),
   m_acceptor(m_io_service),
   m_plugin( _plugin ),
   m_local_port(_local_port),
   m_thread( [&](){this->interrupt(); } )
{
   this->m_active = true;
   this->m_id = ++static_remote_count;
   this->m_remote_ep = _remote_ep;
   this->m_local_ep = _local_ep;

   this->m_context.set_options(boost::asio::ssl::context::default_workarounds| boost::asio::ssl::context::tlsv12);//| boost::asio::ssl::context::single_dh_use);
   this->m_context.set_password_callback(boost::bind(&RemoteProxyHost::get_password, this));
   this->m_context.set_verify_mode(boost::asio::ssl::context::verify_peer|boost::asio::ssl::context::verify_fail_if_no_peer_cert);

#ifdef _WIN32
   // #if (OPENSSL_VERSION_NUMBER < 0x00905100L)
   // By default it may be too low with a "no certificate returned" message.
   // According to google this may be the cause (indeed it was on the test setup). The 4 should at least be >= 2. The define above is also from google. But that is not good enough.
   SSL_CTX_set_verify_depth( this->m_context.native_handle(), 4 ); 
#endif

   this->m_context.load_verify_file( my_certs_name );
   this->m_context.use_certificate_chain_file(my_public_cert_name);
   this->m_context.use_private_key_file(my_private_key_name, boost::asio::ssl::context::pem);
}


std::string RemoteProxyHost::dinfo() const
{
   std::ostringstream oss;
   oss << "Port: " << m_local_port << " ";
   return oss.str();
}


void RemoteProxyHost::add_remotes(const std::vector<RemoteEndpoint> &_remote_ep)
{
   std::lock_guard<std::mutex> l(this->m_mutex);
   for (auto iter = _remote_ep.begin(); iter != _remote_ep.end(); iter++)
   {
      this->m_remote_ep.push_back(*iter);
   }
}


void RemoteProxyHost::remove_remotes(const std::vector<RemoteEndpoint> &_remote_ep)
{
   std::lock_guard<std::mutex> l(this->m_mutex);
   for (auto iter = _remote_ep.begin(); iter != _remote_ep.end(); iter++)
   {
      auto help = std::find_if(this->m_remote_ep.begin(), this->m_remote_ep.end(), [&](const RemoteEndpoint &ep){return ep == *iter;});
      if (help != this->m_remote_ep.end())
      {
         // NB!! ASSERTD NOT RUNNING!!
         this->m_remote_ep.erase(help);
      }  
   }
}


void RemoteProxyHost::stop_by_name(const std::string& certname)
{
   std::lock_guard<std::mutex> l(this->m_mutex);

   // Clean up the current client list and remove any non active clients.
   for (auto iter2 = this->m_clients.begin(); iter2 != this->m_clients.end(); )
   {
      RemoteProxyClient *pHelp = *iter2;
      if (pHelp->m_endpoint.m_name == certname)
      {
         DOUT(dinfo() << "Found and stopping host connection: " << certname);
         pHelp->stop();
         iter2++;
         DOUT(dinfo() << "Done stopping host connection: " << certname);
      }
      else
      {
         iter2++;
      }
   }
}


void RemoteProxyHost::dolog( const std::string &_line )
{
   this->m_log = _line;
   log().add( " Host port: " + mylib::to_string( this->m_local_port ) + ": " + _line );
}


std::string RemoteProxyHost::get_password() const
{
   return "1234";
}


void RemoteProxyHost::start()
{
   if (this->m_thread.is_running())
   {
      DOUT(this->dinfo() << "RemoteProxyHost already running on port: " << this->m_local_port);
      return;
   }
   // We do the following because we want it done in the main thread, so exceptions during start are propagated through.
   // In particular we want to ensure that we dont have 2 servers with the same port number.
   this->dolog(this->dinfo() + std::string("opening connection on port: ") + mylib::to_string(this->m_local_port));
   boost::asio::ip::tcp::endpoint ep(boost::asio::ip::tcp::v4(), this->m_local_port);
   this->m_acceptor.open(ep.protocol());
   this->m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(false));
   this->m_acceptor.bind(ep);
   DOUT(this->dinfo() << "Bind ok for " << ep);
   this->m_thread.start( [this]{ this->threadproc(); } );
}


void RemoteProxyHost::lock()
{

}


void RemoteProxyHost::unlock()
{
   boost::system::error_code ec;
   this->m_acceptor.cancel(ec);
   boost::asio::socket_shutdown( this->m_acceptor,ec );
   this->m_acceptor.close(ec);
}


void RemoteProxyHost::threadproc()
{
   try
   {
      this->dolog(this->dinfo() + std::string("Waiting for remote connection on port: ") + mylib::to_string(this->m_local_port));
      this->m_acceptor.listen();

      RemoteProxyClient* new_session = new RemoteProxyClient(m_io_service, m_context, *this);
      this->m_acceptor.async_accept(new_session->socket(),boost::bind(&RemoteProxyHost::handle_accept, this, new_session,boost::asio::placeholders::error));

      for ( ; this->m_thread.check_run(); )
      {
         try
         {
            this->m_io_service.run();
         }
         catch( std::exception &exc )
         {
            this->dolog(this->dinfo() + exc.what() );
         }
         this->m_thread.sleep( 1000 );
      }
   }
   catch( std::exception &exc )
   {
      this->dolog(this->dinfo() + exc.what() );
   }
}


void RemoteProxyHost::interrupt()
{
   try
   {
      this->m_io_service.stop();
   }
   catch( std::exception &exc )
   {
      this->dolog(this->dinfo() + exc.what() );
   }
}


void RemoteProxyHost::stop()
{
   DOUT(this->dinfo() << "Stopping port: " << this->port());
   this->m_thread.stop();
   std::lock_guard<stdt::mutex> l(this->m_mutex);
   // Clean up the current client list and remove any non active clients.
   for (auto item : this->m_clients)
   {
      item->stop();
   }
   DOUT(this->dinfo() << "Stopped all on port: " << this->port());
}


int RemoteProxyHost::test_local_connection(const std::string& name)
{
   RemoteProxyClient test(this->m_io_service, this->m_context, *this);
   return test.test_local_connection(name, this->m_local_ep);
}


// A new connection from a remote proxy is accepted
//
void RemoteProxyHost::handle_accept(RemoteProxyClient* new_session, const boost::system::error_code& error)
{
   try
   {
      DOUT(this->dinfo() << " error state " << error);
      if (!error)
      {
         stdt::lock_guard<stdt::mutex> l(this->m_mutex);
         // Clean up the current client list and remove any non active clients.
         for ( auto iter2 = this->m_clients.begin(); iter2 != this->m_clients.end(); )
         {
            RemoteProxyClient *pHelp = *iter2;
            if ( ! pHelp->is_active() )
            {
               pHelp->stop();
               iter2 = this->m_clients.erase( iter2 );
               delete pHelp;
            }
            else
            {
               // NB!! We should not allow an additional one here, so we should actually return from the function here ??
               iter2++;
            }
         }
         DOUT(this->dinfo());
         this->m_context.load_verify_file( my_certs_name );
         this->m_clients.push_back( new_session );
         new_session->start( this->m_local_ep );

         mylib::msleep(1000);
         DOUT(this->dinfo() << "Trying to reload verify file");
         this->m_context.load_verify_file( my_certs_name );
         // This call is not multithread safe. load_certificate_names( my_certs_name );

         // We create the next one, which is then waiting for a connection.
         new_session = new RemoteProxyClient(m_io_service, m_context,*this);
         m_acceptor.async_accept(new_session->socket(),boost::bind(&RemoteProxyHost::handle_accept, this, new_session,boost::asio::placeholders::error));
         return;
      }
      else
      {
         DOUT(this->dinfo() << "failed to accept, deleting session");
         delete new_session;
      }
   }
   catch( std::exception &exc )
   {
      this->dolog(this->dinfo() + exc.what() );
   }
}


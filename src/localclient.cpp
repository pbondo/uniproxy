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
// This version is released under the GPL version 3 open source License:
// http://www.gnu.org/copyleft/gpl.html
//
// Copyright (C) 2011-2012 by GateHouse A/S
// All Rights Reserved.
// http://www.gatehouse.dk
// mailto:gh@gatehouse.dk
//====================================================================
#include "localclient.h"

#include <boost/bind.hpp>


static int static_local_id = 0;

using boost::asio::ip::tcp;


LocalHostSocket::LocalHostSocket(LocalHost &_host, boost::asio::ip::tcp::socket *_socket)
: m_host(_host)
{
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


LocalHost::LocalHost( bool _active, const std::string &_name, int _local_port, const std::string &_remote_hostname, const int _remote_port, const int _max_connections, PluginHandler &_plugin )
:	m_active(_active),
	m_count_out(true),
	mp_io_service( null_ptr ),
	mp_acceptor( null_ptr ),
	mp_remote_socket( null_ptr ),
	m_thread( [this]{ this->interrupt(); } ),
	m_plugin(_plugin)
{
	this->m_id = ++static_local_id;
	this->m_local_connected = this->m_remote_connected = false;
	this->m_local_port = _local_port;
	this->m_remote_hostname = _remote_hostname;
	this->m_remote_port = _remote_port;
	this->m_name = _name;
	this->m_max_connections = _max_connections;
	this->m_activate_stamp = boost::get_system_time();
}


void LocalHost::dolog( const std::string &_line )
{
	this->m_log = _line;
	log().add( " name: " + this->m_name + " hostname: " + this->m_remote_hostname + " port: " + mylib::to_string(this->m_remote_port) + ": " + _line );
}


const std::string LocalHost::dolog()
{
	return this->m_log;
}


bool LocalHost::is_local_connected()
{
	bool result = false;
	stdt::lock_guard<stdt::mutex> l(this->m_mutex);
	for ( auto iter = this->m_local_sockets.begin(); iter != this->m_local_sockets.end(); iter++ )
	{
		if ( (*iter)->socket().is_open() && this->m_local_connected )
		{
			result = true;
		}
	}
	return result;
}


std::vector<std::string> LocalHost::local_hostnames()
{
	std::vector<std::string> result;
	stdt::lock_guard<stdt::mutex> l(this->m_mutex);
	for ( auto iter = this->m_local_sockets.begin(); iter != this->m_local_sockets.end(); iter++ )
	{
		boost::asio::ip::tcp::socket &socket( (*iter)->socket() ); // this->m_local_sockets[index]->socket());
		boost::system::error_code ec;
		std::string sz = socket.remote_endpoint(ec).address().to_string(); // This one should not throw exceptions
		result.push_back(sz);
	}
	return result;
}


bool LocalHost::remote_hostname( int index, std::string &result )
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex);
	if ( index < this->m_local_sockets.size() )
	{
		result.clear();
		boost::asio::ip::tcp::socket &socket(this->m_local_sockets[index]->socket());
//xx xx		if ( socket.is_open() ) //&& socket.remote_endpoint().address() )
		boost::system::error_code ec;
		std::string sz = socket.remote_endpoint(ec).address().to_string(); // This one should not throw exceptions
		if ( !ec ) result = sz;
		return true;
	}
	return false;
}


bool LocalHost::is_remote_connected()
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex);
	return this->mp_remote_socket != null_ptr && this->mp_remote_socket->lowest_layer().is_open() && this->m_remote_connected;
}


void LocalHost::start()
{
	this->m_thread.start( [this]{ this->threadproc(); } );
}


void LocalHost::stop()
{
	this->m_thread.stop();
}


// This will always happen in local thread context.
void LocalHost::cleanup()
{
	try
	{
		stdt::lock_guard<stdt::mutex> l(this->m_mutex);
		while ( this->m_local_sockets.size() > 0 )
		{
			TRY_CATCH( this->m_local_sockets.front()->socket().close() );
			this->m_local_sockets.erase( this->m_local_sockets.begin() ); // Since we use shared_ptr it should autodelete.
		}
		DOUT(__FUNCTION__ << ":" << __LINE__ );
	
		this->mp_acceptor = NULL;
	}
	catch( std::exception &exc )
	{
		this->dolog( exc.what() );
	}
	DOUT(__FUNCTION__ << ":" << __LINE__ );
}


// The interrupt function may happen in local or other (main) thread context.
void LocalHost::interrupt()
{
	try
	{
		{
			DOUT(__FUNCTION__ << ":" << __LINE__ );
			stdt::lock_guard<stdt::mutex> l(this->m_mutex);
			if ( this->mp_acceptor != null_ptr )
			{
				TRY_CATCH( (*this->mp_acceptor).cancel() );
				TRY_CATCH( boost::asio::socket_shutdown( *this->mp_acceptor ) );
			}
		}
		{
			DOUT(__FUNCTION__ << ":" << __LINE__ );
			stdt::lock_guard<stdt::mutex> l(this->m_mutex);

			for ( int index = 0; index < this->m_local_sockets.size(); index++ )
			{
				TRY_CATCH( this->m_local_sockets[index]->socket().shutdown( boost::asio::socket_base::shutdown_both ) );
			}
		}
		{
			stdt::lock_guard<stdt::mutex> l(this->m_mutex);
			DOUT(__FUNCTION__ << ":" << __LINE__ );
			if ( this->mp_remote_socket != null_ptr )
			{
				TRY_CATCH( (*this->mp_remote_socket).lowest_layer().shutdown( boost::asio::socket_base::shutdown_both ) );
			}
		}
	}
	catch( std::exception &exc )
	{
		this->dolog( exc.what() );
	}
	DOUT(__FUNCTION__ << ":" << __LINE__ );
}


ssl_socket &LocalHost::remote_socket()
{
	ASSERTE(this->mp_remote_socket, uniproxy::error::socket_invalid, "");
	return *this->mp_remote_socket;
}


void LocalHost::remove_socket( boost::asio::ip::tcp::socket &_socket )
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex);
	for ( auto iter = this->m_local_sockets.begin(); iter != this->m_local_sockets.end(); iter++ )
	{
		if ( (*iter)->socket() == _socket )
		{
			TRY_CATCH( _socket.shutdown(boost::asio::socket_base::shutdown_both) );
			TRY_CATCH( _socket.close() );
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
		boost::asio::async_write( this->remote_socket(), boost::asio::buffer( this->m_local_data, bytes_transferred), boost::bind(&LocalHost::handle_remote_write, this, boost::asio::placeholders::error));
	}
	else
	{
		DOUT( "Error: " << error << " in " << __FUNCTION__ << ":" <<__LINE__ << " connections: " << this->m_local_sockets.size() );
		if ( this->m_local_sockets.size() <= 1 )
		{
			throw std::runtime_error( "Local connection closed for " + this->m_name );
		}
		// Else we silently fail to read anything from any of the other sockets.
		// This is a side effect of allowing more than one local client to connect. Only one is to send data.
		this->remove_socket(_hostsocket.socket());
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
		DOUT( "One of the attached sockets disconnected: " << __FUNCTION__ << ":" <<__LINE__ << " : " ); //<< _socket->remote_endpoint().address() );
		this->remove_socket(*_socket);
	}
	if ( this->m_write_count == 0 && this->m_local_sockets.size() > 0 )
	{
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
		stdt::lock_guard<stdt::mutex> l(this->m_mutex);
		this->m_write_count = this->m_local_sockets.size();
		for ( int index = 0; index < this->m_write_count; index++ )
		{
			boost::asio::ip::tcp::socket *psocket = &this->m_local_sockets[index]->socket(); // .get();
			boost::asio::async_write( *psocket, boost::asio::buffer( this->m_remote_data, bytes_transferred), boost::bind(&LocalHost::handle_local_write, this, psocket, boost::asio::placeholders::error));
		}
	}
	else
	{
		DOUT( "Error: " << error << ": " << error.message() << " bytes: " << bytes_transferred << " in " << __FUNCTION__ << ":" <<__LINE__);
		throw boost::system::system_error( error );
		//throw std::runtime_error( __FUNCTION__ );
	}
}


void LocalHost::handle_remote_write(const boost::system::error_code& error)
{
	if (!error)
	{
		// Notice that if the first [0] failes, then we won't get to the next [0]. But that is also how it should be.
		if ( this->m_local_sockets.size() )
		{
			LocalHostSocket * p = this->m_local_sockets.front().get();
			p->socket().async_read_some( boost::asio::buffer( this->m_local_data, max_length), boost::bind(&LocalHostSocket::handle_local_read, p, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		}
	}
	else
	{
		DOUT( "Error: " << error << " in " << __FUNCTION__ << ":" <<__LINE__);
		throw boost::system::system_error( error );
		//throw std::runtime_error( __FUNCTION__ );
	}
}


std::string LocalHost::get_password() const
{
	return "1234";
}


void LocalHost::handle_accept( boost::asio::ip::tcp::socket *_socket, const boost::system::error_code& error )
{
	if ( !error )
	{
		auto p = std::make_shared<LocalHostSocket>( *this, _socket);
		this->m_local_sockets.push_back(p);
		DOUT("Added extra local socket: " << this->m_local_sockets.size() );
	}
	else
	{
		delete _socket; // NB!! This is not really the right way. Explore passing the shared_ptr as a parameter instead.
		DOUT("Local accept error: " << this->m_local_sockets.size() );
	}

	// Unconditionally we start looking for the next socket.
	if ( this->mp_io_service != NULL && this->mp_acceptor != NULL )
	{
		boost::asio::ip::tcp::socket *new_socket = new boost::asio::ip::tcp::socket( *this->mp_io_service);
		this->mp_acceptor->async_accept( *new_socket, boost::bind(&LocalHost::handle_accept, this, new_socket, boost::asio::placeholders::error));
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

			boost::asio::ssl::context ssl_context( io_service, boost::asio::ssl::context::sslv23);

			ssl_context.set_password_callback(boost::bind(&LocalHost::get_password,this));
			ssl_context.set_verify_mode(boost::asio::ssl::context::verify_peer|boost::asio::ssl::context::verify_fail_if_no_peer_cert);
			ssl_context.load_verify_file(my_certs_name);
			ssl_context.use_certificate_chain_file(my_public_cert_name);
			ssl_context.use_private_key_file(my_private_key_name, boost::asio::ssl::context::pem);

			boost::asio::ip::tcp::socket *local_socket = new boost::asio::ip::tcp::socket(io_service);

			ssl_socket remote_socket( io_service, ssl_context );

			mylib::protect_pointer<ssl_socket> p2( this->mp_remote_socket, remote_socket, this->m_mutex );

			// NB!! This will only support ipv4
			boost::asio::ip::tcp::acceptor acceptor( io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), this->m_local_port) );

			mylib::protect_pointer<boost::asio::ip::tcp::acceptor> p3( this->mp_acceptor, acceptor, this->m_mutex );
			boost::asio::io_service::work session_work(io_service);

			//void *pn = NULL;
			std::shared_ptr<void> ptr( NULL, [this](void*){ DOUT("local exit loop"); this->interrupt(); this->cleanup();} );

			this->dolog("Waiting for local connection" );
			// Synchronous wait for connection from local TCP socket. Must be handled by the interrupt function

			acceptor.accept( *local_socket );
			auto p = std::make_shared<LocalHostSocket>(*this, local_socket);
			this->m_local_sockets.push_back( p );	// NB!! redo by inserting line above directly


			this->m_local_connected = true;

			this->dolog("Connecting to remote host: " + this->m_remote_hostname + ":" + mylib::to_string(this->m_remote_port) );
			boost::asio::sockect_connect( remote_socket.lowest_layer(), io_service, this->m_remote_hostname, this->m_remote_port );
			this->m_remote_connected = true;

			if ( boost::get_system_time() < this->m_activate_stamp )
			{
				std::error_code err;
				this->dolog("Attempting to perform certificate exchange with " + this->m_name );
				this->m_activate_stamp = boost::get_system_time(); // Reset to current time to avoid double attempts.
				if (	!SetupCertificates( remote_socket.next_layer(), this->m_name, false, err ) &&
						!SetupCertificates( remote_socket.next_layer(), this->m_name, true, err ) )
				{
					this->dolog("Succeeded in exchanging certificates" );
				}
				else
				{
					this->dolog("Failed to exchange certificate: " + err.message() );
				}
				throw std::runtime_error("Called setup certificate, retry connection"); // Not an error
			}
			this->dolog("Connected to remote host: " + this->m_remote_hostname + ":" + mylib::to_string(this->m_remote_port) + " Attempting SSL handshake" );

			DOUT( "handles: " << remote_socket.next_layer().native_handle() << " / " << remote_socket.lowest_layer().native_handle() );

			remote_socket.handshake( boost::asio::ssl::stream_base::client );
			this->dolog("Succesfull SSL handshake to remote host: " + this->m_remote_hostname + ":" + mylib::to_string(this->m_remote_port) );

			boost::asio::socket_set_keepalive_to( *local_socket, 20 );
			boost::asio::socket_set_keepalive_to( remote_socket.lowest_layer(), 20 );

			local_socket->async_read_some( boost::asio::buffer( m_local_data, max_length), boost::bind(&LocalHostSocket::handle_local_read, p.get(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred) );
			remote_socket.async_read_some( boost::asio::buffer( m_remote_data, max_length), boost::bind(&LocalHost::handle_remote_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred) );

			// NB!! The following line may leak....
			boost::asio::ip::tcp::socket *new_socket = new boost::asio::ip::tcp::socket(io_service);
			acceptor.async_accept( *new_socket, boost::bind(&LocalHost::handle_accept, this, new_socket, boost::asio::placeholders::error));

			// Now let the io_service handle the session.
			io_service.run();
		}
		catch( std::exception &exc )
		{
			this->dolog(exc.what() );
		}
		this->m_local_connected = this->m_remote_connected = false;
		this->m_thread.sleep( 4000 );	//We will need some time to ensure the remote end has settled. May need to be investigated.
	}
}


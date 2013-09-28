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
#include "providerclient.h"
#include <boost/bind.hpp>

using boost::asio::ip::tcp;


//static int static_local_id = 0;


ProviderClient::ProviderClient( bool _active, 
		const std::vector<ProxyEndpoint> &_local_endpoints, const std::vector<ProxyEndpoint> &_proxy_endpoints, 
		PluginHandler &_plugin )
:	BaseClient(_active, -1, _proxy_endpoints, 1, _plugin),
	//m_active(_active),
	//m_proxy_index(0),
	m_local_connected_index(0),
	//m_count_out(true),
	//mp_remote_socket( nullptr ),
	mp_local_socket( nullptr )
	//m_thread( [this]{ this->interrupt(); } ),
	//m_plugin(_plugin)
{
	//this->m_id = ++static_local_id;
	//this->m_proxy_endpoints = _proxy_endpoints;
	this->m_local_endpoints = _local_endpoints;
	//this->m_activate_stamp = boost::get_system_time();
}

/*
void ProviderClient::dolog( const std::string &_line )
{
	this->m_log = _line;
	log().add( " hostname: " + this->remote_hostname() + " port: " + mylib::to_string(this->remote_port()) + ": " + _line );
}


const std::string ProviderClient::dolog()
{
	return this->m_log;
}
*/

bool ProviderClient::is_local_connected() const
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex);
	return this->mp_local_socket != nullptr && is_connected(this->mp_local_socket->lowest_layer());
}

/*
bool ProviderClient::is_remote_connected() const
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex);
	return this->mp_remote_socket != nullptr && is_connected(this->mp_remote_socket->lowest_layer());
}
*/

void ProviderClient::start()
{
	this->m_thread.start( [this]{ this->threadproc(); } );
}


void ProviderClient::stop()
{
	this->m_thread.stop();
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

/*
ssl_socket &ProviderClient::remote_socket()
{
	ASSERTE(this->mp_remote_socket, uniproxy::error::socket_invalid, "");
	return *this->mp_remote_socket;
}


std::string ProviderClient::get_password() const
{
	return "1234";
}
*/

void ProviderClient::lock()
{
}


void ProviderClient::unlock()
{
	this->interrupt();
}


void ProviderClient::threadproc()
{	
	for ( ; this->m_thread.check_run() ; )
	{
		// On start and after each lost connection we end up here.
		try
		{
			this->m_proxy_index = 0;
			this->m_local_connected_index = 0;

			boost::asio::io_service io_service;
			//std::shared_ptr<void> ptr( NULL, [this](void*){ DOUT("local exit loop"); this->interrupt(); this->cleanup();} );
			stdt::lock_guard<ProviderClient> l(*this);

			boost::asio::io_service::work session_work(io_service);

			this->dolog("Provider connecting to remote host");

			boost::asio::ip::tcp::socket local_socket(io_service);
			mylib::protect_pointer<boost::asio::ip::tcp::socket> p1( this->mp_local_socket, local_socket, this->m_mutex );
			for ( int index = 0; index < this->m_local_endpoints.size(); index++ )
			{
				std::string ep = this->m_local_endpoints[index].m_hostname + ":" + mylib::to_string(this->m_local_endpoints[index].m_port);
				try
				{
					this->dolog("Performing local connection to: " + ep );
					boost::asio::sockect_connect( local_socket, io_service, this->m_local_endpoints[index].m_hostname, this->m_local_endpoints[index].m_port );
					//DOUT( __FUNCTION__ << ":" << __LINE__ );
					this->m_local_connected_index = index;
					//this->m_local_connected = true;
					break;
				}
				catch( std::exception &exc )
				{
					DOUT(  __FUNCTION__ << ":" << __LINE__ << " Failed connection to: " << ep << " " << exc.what() );
				}
			}
			ASSERTE(this->is_local_connected(), uniproxy::error::socket_invalid,"Provider failed connection to local host");


			boost::asio::ssl::context ssl_context( io_service, boost::asio::ssl::context::sslv23);
			ssl_context.set_password_callback(boost::bind(&ProviderClient::get_password,this));
			ssl_context.set_verify_mode(boost::asio::ssl::context::verify_peer|boost::asio::ssl::context::verify_fail_if_no_peer_cert);
			ssl_context.load_verify_file(my_certs_name);
			ssl_context.use_certificate_chain_file(my_public_cert_name);
			ssl_context.use_private_key_file(my_private_key_name, boost::asio::ssl::context::pem);

			ssl_socket remote_socket( io_service, ssl_context );
			mylib::protect_pointer<ssl_socket> p2( this->mp_remote_socket, remote_socket, this->m_mutex );

			this->dolog("Connecting to remote host: " + this->remote_hostname() + ":" + mylib::to_string(this->remote_port()) );
			//boost::asio::sockect_connect( remote_socket.lowest_layer(), io_service, this->remote_hostname(), this->remote_port() );
			for ( int index = 0; index < this->m_proxy_endpoints.size(); index++ )
			{
				std::string ep = this->m_proxy_endpoints[index].m_hostname + ":" + mylib::to_string(this->m_proxy_endpoints[index].m_port);
				try
				{
					this->dolog("Performing remote connection to: " + ep );
					//boost::asio::sockect_connect( local_socket, io_service, this->m_proxy_endpoints[index].m_hostname, this-m_proxy_endpoints[index].m_port );
					boost::asio::sockect_connect( remote_socket.lowest_layer(), io_service, this->m_proxy_endpoints[index].m_hostname, this->m_proxy_endpoints[index].m_port );
					//boost::asio::sockect_connect( remote_socket.lowest_layer(), io_service, this->remote_hostname(), this->remote_port() );
					//DOUT( __FUNCTION__ << ":" << __LINE__ );
					this->m_proxy_index = index;
					break;
				}
				catch( std::exception &exc )
				{
					DOUT(  __FUNCTION__ << ":" << __LINE__ << " Failed connection to: " << ep << " " << exc.what() );
				}
			}
			ASSERTE(this->is_remote_connected(), uniproxy::error::socket_invalid,"Provider failed connection to remote host");

			if ( boost::get_system_time() < this->m_activate_stamp )
			{
				ProxyEndpoint &ep = this->m_proxy_endpoints[this->m_proxy_index];
				std::error_code err;
				this->dolog("Provider attempting to perform certificate exchange with " + ep.m_name );
				this->m_activate_stamp = boost::get_system_time(); // Reset to current time to avoid double attempts.
				if (	!SetupCertificates( remote_socket.next_layer(), ep.m_name, false, err ) &&
						!SetupCertificates( remote_socket.next_layer(), ep.m_name, true, err ) )
				{
					this->dolog("Succeeded in exchanging certificates" );
				}
				else
				{
					this->dolog("Failed to exchange certificate: " + err.message() );
				}
				throw std::runtime_error("Called setup certificate, retry connection"); // Not an error
			}
			
			this->dolog("Provider connected to remote host: " + this->remote_hostname() + ":" + mylib::to_string(this->remote_port()) + " Attempting SSL handshake" );
			remote_socket.handshake(boost::asio::ssl::stream_base::client);
			this->dolog("Succesfull SSL handshake to remote host: " + this->remote_hostname() + ":" + mylib::to_string(this->remote_port()));

			boost::asio::socket_set_keepalive_to( local_socket, 20 );
			boost::asio::socket_set_keepalive_to( remote_socket.lowest_layer(), 20 );

			// This implementation uses the naive read some data and then write some data.
			// Since the TCP stack is using buffers internally this should run reasonably efficient.
			for ( ; this->m_thread.check_run(); )
			{
				int length;
				length = local_socket.read_some( boost::asio::buffer( this->m_local_data, max_length-1 ) );
				this->m_count_out.add(length);
				this->m_local_data[length] = 0;
				Buffer buffer( this->m_local_data, length );
				if ( this->m_plugin.message_filter_local2remote( buffer ) ) //, full ) )
				{
					// The plugin is allowed to modify the buffer, thus we need to recalculate size
					length = remote_socket.write_some( boost::asio::buffer( buffer.m_buffer, buffer.m_size ) );
					this->m_count_out.add(length);
				}
			}
		}
		catch( std::exception &exc )
		{
			this->dolog(exc.what());
		}
		//We will need some time to ensure the remote end has settled. May need to be investigated.
		// Could also increase over time while unsuccessfull and reset on succesfull connection.
		this->m_thread.sleep( 4000 );
	}
}

/*
std::string ProviderClient::remote_hostname() const
{
	return this->m_proxy_endpoints[this->m_proxy_index].m_hostname;
}


int ProviderClient::remote_port() const
{
	return this->m_proxy_endpoints[this->m_proxy_index].m_port;
}
*/

std::string ProviderClient::local_hostname() const
{
	return this->m_local_endpoints[this->m_local_connected_index].m_hostname;
}

std::vector<std::string> ProviderClient::local_hostnames()
{
	std::vector<std::string> result;
	for ( auto &ep : this->m_local_endpoints )
	{
		result.push_back(ep.m_hostname);
	}
	return result;
}



int ProviderClient::local_port() const
{
	return this->m_local_endpoints[this->m_local_connected_index].m_port;
}

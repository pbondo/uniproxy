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
//typedef std::shared_ptr<LocalHost> localhost_ptr;
typedef std::shared_ptr<RemoteProxyHost> remotehost_ptr;
//typedef std::shared_ptr<ProviderClient> providerclient_ptr;

class client_certificate_exchange : public mylib::thread
{
public:

	client_certificate_exchange( ) : mylib::thread( [](){} )
	{
	}

	void start( int _port )
	{
		this->mylib::thread::start( [this,_port]( ){this->thread_proc(_port);} );
	}

	void thread_proc( int _port )
	{
		try
		{
			boost::asio::io_service io_service;
			boost::asio::ip::tcp::socket local_socket(io_service);
			boost::asio::ip::tcp::endpoint endpoint( boost::asio::ip::address::from_string( "127.0.0.1" ), _port );
			DOUT( __FUNCTION__ << " connect to " << endpoint );
			local_socket.connect( endpoint );

			const int buffer_size = 100;
			char buffer[buffer_size]; //
			local_socket.read_some( boost::asio::buffer( buffer, buffer_size ) );
		}
		catch(...)
		{
		}
	}

};



class session_data
{
public:

	session_data( int _id );
	void update_timestamp();

	boost::posix_time::ptime m_timestamp;
	int m_id;
	int m_read_index;
};


class proxy_global
{
public:

	proxy_global();
	void lock();
	void unlock();
	void stop( const std::string &_name );

	typedef enum 
	{
		hosts 	= 0x01,
		clients	= 0x02,
		web		= 0x04,
		config	= 0x08,
		all		= hosts|clients|web|config
	} json_acl;

	bool populate_json( cppcms::json::value &obj, int _json_acl );
	std::string save_json_status( bool readable );
	std::string save_json_config( bool readable );

	stdt::mutex m_mutex;

	// NB!! We need a gate / mutex for these. For now only a problem during shutdown. Also for config update
	std::vector<remotehost_ptr> remotehosts;
	std::vector<baseclient_ptr> localclients;
	//std::vector<localhost_ptr> localhosts;
	//std::vector<providerclient_ptr> providerclients;

	//bool m_reload;	// Set this before calling service().shutdown();
	int m_port;
	std::string m_ip4_mask;
	bool m_debug;

	// Own name to be used for generating own certifcate.
	std::string m_name;

	client_certificate_exchange m_thread;

	session_data &get_session_data( cppcms::session_interface &_session );

	int m_session_id_counter;
	stdt::mutex m_session_data_mutex;
	std::vector< std::shared_ptr<session_data>> m_sessions;

};

#endif

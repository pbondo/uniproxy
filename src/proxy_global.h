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
		hosts 	= 0x01,
		clients	= 0x02,
		web		= 0x04,
		config	= 0x08,
		all		= hosts|clients|web|config
	} json_acl;

	// These may throw exceptions.
	void populate_json(cppcms::json::value &obj, int _json_acl);
	void unpopulate_json(cppcms::json::value &obj);

	std::string save_json_status( bool readable );
	std::string save_json_config( bool readable );

	//
	bool load_configuration();

	// lists with associated mutex
	stdt::mutex m_mutex_list;
	std::vector<remotehost_ptr> remotehosts;
	std::vector<baseclient_ptr> localclients;
	std::vector<LocalEndpoint> uniproxies;

	mylib::port_type m_port;
	std::string m_ip4_mask;
	bool m_debug;
	cppcms::json::value m_new_setup; // Stop and start services to match this.
	int m_certificate_timeout;

	// Own name to be used for generating own certificate.
	std::string m_name;

	// The returned value here can be used for the duration of this transaction without being removed or moved.
	session_data &get_session_data( cppcms::session_interface &_session );
	void clean_session();

	// (common) Names from the certificates loaded from the imported certs.pem file.
	stdt::mutex m_mutex_certificates;
	std::vector<std::string> m_cert_names;
	bool load_certificate_names( const std::string & _filename );
	std::error_code SetupCertificates( boost::asio::ip::tcp::socket &_remote_socket, const std::string &_connection_name, bool _server, std::error_code& ec );

	bool certificate_available( const std::string &_cert_name);
	bool execute_openssl();

	bool is_provider(const BaseClient &client) const;

	bool is_same( const BaseClient &client, cppcms::json::value &obj, bool &param_changes, bool &client_changes ) const;
	bool is_same( const RemoteProxyHost &host, cppcms::json::value &obj, bool &param_changes, bool &client_changes, bool &locals_changed ) const;


protected:

	stdt::mutex m_session_data_mutex;
	std::vector< std::shared_ptr<session_data>> m_sessions;

};

extern proxy_global global;

#endif

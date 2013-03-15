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
// Copyright (C) 2011-2012 by GateHouse A/S
// All Rights Reserved.
// http://www.gatehouse.dk
// mailto:gh@gatehouse.dk
//====================================================================
#include "proxy_global.h"
#include "cppcms_util.h"
#include <cppcms/view.h>



PluginHandler standard_plugin("");


proxy_global::proxy_global()
{
	this->m_port = 8085; // Default
	this->m_ip4_mask = "0.0.0.0";
	this->m_debug = false;	
//	this->m_reload = false;
	this->m_session_id_counter = 0;
}

//-----------------------------------


void proxy_global::lock()
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex);
	for ( auto iter = this->remotehosts.begin(); iter != this->remotehosts.end(); iter++ )
	{
		remotehost_ptr p = *iter;
		DOUT("Starting remotehost at: " << p->m_local_port ); //m_acceptor.local_endpoint() );
		p->start();
	}
	for ( auto iter = this->localhosts.begin(); iter != this->localhosts.end(); iter++ )
	{
		localhost_ptr p = *iter;
		if ( p->m_active )
		{
			DOUT("Starting localhost at: " << p->m_local_port );
			p->start();
		}
	}
}


void proxy_global::unlock()
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex);
	DOUT("sockethandler.StopAll()");
	for ( auto iter = this->remotehosts.begin(); iter != this->remotehosts.end(); iter++ )
	{
		DOUT("remotehost.Stop()");
		remotehost_ptr p = *iter;
		RemoteProxyHost* p2 = p.get();
		p2->stop();
	}
	for ( auto iter = this->localhosts.begin(); iter != this->localhosts.end(); iter++ )
	{
		DOUT("localhost.Stop()");
		(*iter)->stop();
	}
	this->m_thread.stop();
	this->remotehosts.clear();
	this->localhosts.clear();
}




// The function will check for each of the main entries. If a main entry exists, then it is will overwrite, i.e. destroy any existing information.
// NB!! This should not be called while threads are active.
bool proxy_global::populate_json( cppcms::json::value &obj, int _json_acl )
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex);
	if ( (_json_acl & clients) > 0 && obj["clients"].type() == cppcms::json::is_array )
	{
		this->localhosts.clear();
		cppcms::json::array ar = obj["clients"].array();
		for ( auto iter1 = ar.begin(); iter1 != ar.end(); iter1++ )
		{
			auto &item1 = *iter1;
			bool active = cppcms::utils::check_bool( item1, "active", true, false );
			std::string name = check_string( item1, "name", "", false );
			int client_port = check_int( item1, "port", -1, true );
			int max_connections = check_int( item1, "max_connections", 1, false );
			std::vector<ProxyEndpoint> proxy_endpoints;
			if ( item1["remotes"].type() == cppcms::json::is_array )
			{
				auto ar2 = item1["remotes"].array();
				for ( auto iter2 = ar2.begin(); iter2 != ar2.end(); iter2++ )
				{
					auto &item2 = *iter2;
					ProxyEndpoint ep( cppcms::utils::check_bool( item2, "active", true, false ), 
										check_string( item2, "name", "", false ), 
										check_string( item2, "hostname", "", true),
										check_int(item2,"port",-1,true) 
										);
					if (ep.m_active)
					{
						proxy_endpoints.push_back( ep );
					}
				}
			}
			if ( active && proxy_endpoints.size() > 0 )
			{
				// NB!! Search for the correct plugin version
				localhost_ptr local_ptr( new LocalHost( active, name, client_port, proxy_endpoints, max_connections, standard_plugin ) );
				local_ptr->m_proxy_endpoints = proxy_endpoints;
				this->localhosts.push_back( local_ptr );
			} // NB!! What else if one of them is empty
		}
	}
	if ( (_json_acl & hosts) > 0 && obj["hosts"].type() == cppcms::json::is_array )
	{
		this->remotehosts.clear();
		cppcms::json::array ar1 = obj["hosts"].array();
		for ( auto iter1 = ar1.begin(); iter1 != ar1.end(); iter1++ )
		{
			auto &item1 = *iter1;
			// We must have a port number and at least one local address and one remote address
			int host_port = check_int( item1, "port", -1, true );
			std::vector<Address> local_endpoints;
			if ( item1["locals"].type() == cppcms::json::is_array )
			{
				auto ar2 = item1["locals"].array();
				for ( auto iter2 = ar2.begin(); iter2 != ar2.end(); iter2++ )
				{
					auto &item2 = *iter2;
					Address addr( 	check_string( item2, "hostname", "", true ), 
									check_int( item2, "port", -1, true ) );
					local_endpoints.push_back( addr );
				}
			}
			std::vector<RemoteEndpoint> remote_endpoints;
			if ( item1["remotes"].type() == cppcms::json::is_array )
			{
				auto ar2 = item1["remotes"].array();
				for ( auto iter2 = ar2.begin(); iter2 != ar2.end(); iter2++ )
				{
					auto &item2 = *iter2;
					RemoteEndpoint ep( cppcms::utils::check_bool( item2, "active", true, false ), 
										check_string( item2, "name", "", false ), 
										check_string( item2, "hostname", "", true),
										check_string( item2, "username", "", false), 
										check_string( item2, "password", "", false ) );						
					remote_endpoints.push_back( ep );
				}
			}
			// Check size > 0
			PluginHandler *plugin = &standard_plugin;
			std::string plugin_type = check_string( item1, "type", "", false );
			if ( plugin_type.length() > 0 )
			{
				for ( auto iter3 = PluginHandler::plugins().begin(); iter3 != PluginHandler::plugins().end(); iter3++ )
				{
					if ( (*iter3)->m_type == plugin_type )
					{
						plugin = (*iter3);
					}
				}
				// NB!! What if we don't find the right one ? now we simply default to the empty one.
			}
			if ( remote_endpoints.size() > 0 && local_endpoints.size() > 0 )
			{
				//remotehost_ptr remote_ptr( new RemoteProxyHost( host_port, remote_endpoints, local_endpoints, *plugin ) );
				remotehost_ptr remote_ptr = std::make_shared<RemoteProxyHost>( host_port, remote_endpoints, local_endpoints, *plugin );
				this->remotehosts.push_back( remote_ptr );
			} // NB!! What else if one of them is empty
		}
	}
	if ( (_json_acl & web) > 0 && obj["web"].type() == cppcms::json::is_object )
	{
		cppcms::json::value &web_obj( obj["web"] );
		cppcms::utils::check_int( web_obj, "port", this->m_port );
	}
	if ( (_json_acl & config) > 0 && obj["config"].type() == cppcms::json::is_object )
	{
		cppcms::json::value &config_obj( obj["config"] );
		cppcms::utils::check_string( config_obj, "name", this->m_name );
		cppcms::utils::check_bool( config_obj, "debug", this->m_debug );
	}
	return true;
}

	
std::string proxy_global::save_json_status( bool readable )
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex);

	cppcms::json::value glob;
	// The client part
	for ( int index = 0; index < this->localhosts.size(); index++ )
	{
		LocalHost &host( *this->localhosts[index] );
		cppcms::json::object obj;
		obj["id"] = host.m_id;
		obj["active"] = host.m_active;
		obj["name"] = host.m_name;
		obj["local_port"] = host.m_local_port;
		obj["remote_hostname"] = host.remote_hostname();
		obj["remote_port"] = host.remote_port();
		obj["connected_local"] = host.is_local_connected();
/*
//		obj["connected_remote"] = host.is_remote_connected();
		if ( host.is_remote_connected(index) )
		{
			obj["count_in"] = host.m_count_in.get();
			obj["count_out"] = host.m_count_out.get();
		}
*/
//NB!!		obj["log"] = host.log().peek();
		obj["log"] = host.dolog();

		std::vector<std::string> hostnames = host.local_hostnames();
		//for ( int index3 = 0; host.remote_hostname(index3,sz); index3++)
		for (int index3 = 0; index3 < hostnames.size(); index3++)
		{
			std::string sz = hostnames[index3];
			if ( sz.length() )
			{
				obj["local_hostname"][index3] = sz;
			}
		}
		for (int index3 = 0; index3 < m_cert_names.size(); index3++)
		{
			if ( host.m_name == m_cert_names[index3] )
			{
				obj["cert"] = true;
			}
		}

		if (host.m_activate_stamp > boost::get_system_time())
		{
			auto div = host.m_activate_stamp - boost::get_system_time();
			obj["activate"] = div.seconds();
		}
		
		for ( int index2 = 0; index2 < host.m_proxy_endpoints.size(); index2++ )
		{
			auto &r = host.m_proxy_endpoints[index2];
			cppcms::json::object obj2;
			obj2["name"] = r.m_name;
			obj2["hostname"] = r.m_hostname;
			obj2["port"] = r.m_port;
			//obj2["connected_local"] = false; //host.is_local_connected();
			obj2["connected_remote"] = host.is_remote_connected(index2);
			if ( host.is_remote_connected(index2) )
			{
				obj2["count_in"] = host.m_count_in.get();
				obj2["count_out"] = host.m_count_out.get();
			}

			obj["remotes"][index2] = obj2;
		}
		glob["clients"][index] = obj;
	}

	// The host part
	for ( int index = 0; index < this->remotehosts.size(); index++ )
	{
		RemoteProxyHost &host( *this->remotehosts[index] );
		cppcms::json::object obj_host;
		obj_host["id"] = host.m_id;
		obj_host["port"] = host.m_local_port; // .m_acceptor.local_endpoint().port();
		obj_host["type"] = host.m_plugin.m_type;
//NB!!		obj_host["log"] = host.log().peek();
		// Loop through each remote proxy
		for ( int index2 = 0; index2 < host.m_remote_ep.size(); index2++ )
		{
			cppcms::json::object obj;
			obj["active"] = host.m_remote_ep[index2].m_active;
			obj["name"] = host.m_remote_ep[index2].m_name;
			for ( int index3 = 0; index3 < m_cert_names.size(); index3++)
			{
				if ( host.m_remote_ep[index2].m_name == m_cert_names[index3] )
				{
					obj["cert"] = true;
				}
			}
			obj["hostname"] = host.m_remote_ep[index2].m_hostname;
			if ( host.m_remote_ep[index2].m_name == host.m_activate_name && host.m_activate_stamp > boost::get_system_time() )
			{
				auto div = host.m_activate_stamp - boost::get_system_time();
				obj["activate"] = div.seconds();
			}
			for ( auto iter3 = host.m_clients.begin(); iter3 != host.m_clients.end(); iter3++ )
			{
				RemoteProxyClient &client( *(*iter3) );
				if ( client.m_endpoint == host.m_remote_ep[index2] )
				{
					bool is_local_connected = client.is_local_connected();
					obj["connected_local"] = is_local_connected;
					if ( is_local_connected ) // Not thread safe
					{
						// NB!! Here we provide an IP address, but it should be a hostname.
						obj["local_hostname"] = client.local_endpoint().address().to_string();
						//client.m_local_socket.remote_endpoint().address().to_string();
					}
					bool is_remote_connected = client.is_remote_connected();
					obj["connected_remote"] = is_remote_connected;
					if ( is_remote_connected ) // Not thread safe
					{
						obj["remote_hostname"] =  client.remote_endpoint().address().to_string();
						//client.is_remote_connected();
						obj["count_in"] = client.m_count_in.get();
						obj["count_out"] = client.m_count_out.get();
					}
					break;
				}
			}
			obj_host["remotes"][index2] = obj;
		}
		glob["hosts"][index] = obj_host;
	}

	cppcms::json::object config_obj;
	//config_obj["debug"] = this->m_debug;
	config_obj["name"] = this->m_name;
	glob["global"] = config_obj;

	std::ostringstream os;
	glob.save( os, readable );
	return os.str();
}


std::string proxy_global::save_json_config( bool readable )
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex);
	cppcms::json::value glob;
	// The client part
	for ( int index = 0; index < this->localhosts.size(); index++ )
	{
		LocalHost &host( *this->localhosts[index] );
		cppcms::json::object obj;
		obj["id"] = host.m_id;
		obj["active"] = host.m_active;
		obj["name"] = host.m_name;
		obj["local_port"] = host.m_local_port;
		obj["remote_hostname"] = host.remote_hostname();
		obj["remote_port"] = host.remote_port();
		obj["max_connections"] = host.m_max_connections;
		glob["clients"][index] = obj;
	}
	// The host part
	for ( int index = 0; index < this->remotehosts.size(); index++ )
	{
		RemoteProxyHost &host( *this->remotehosts[index] );
		cppcms::json::object obj_host;
		obj_host["id"] = host.m_id;
		obj_host["port"] = host.m_local_port; // .m_acceptor.local_endpoint().port();
		obj_host["type"] = host.m_plugin.m_type;
		for ( int index2 = 0; index2 < host.m_local_ep.size(); index2++ )
		{
			cppcms::json::object obj;
			obj["hostname"] = host.m_local_ep[index2].m_hostname;
			obj["port"] = host.m_local_ep[index2].m_port;
			obj_host["locals"][index2] = obj;
		}		
		for ( int index2 = 0; index2 < host.m_remote_ep.size(); index2++ )
		{
			cppcms::json::object obj;
			obj["name"] = host.m_remote_ep[index2].m_name;
			obj["active"] = host.m_remote_ep[index2].m_active;
			obj["hostname"] = host.m_remote_ep[index2].m_hostname;
			obj["username"] = host.m_remote_ep[index2].m_username;
			obj["password"] = host.m_remote_ep[index2].m_password;
			obj_host["remotes"][index2] = obj;
		}
		glob["hosts"][index] = obj_host;
	}
	
	cppcms::json::object web_obj;
	web_obj["port"] = this->m_port;
	web_obj["ip4"] = this->m_ip4_mask; //hostname;
	glob["web"] = web_obj;

	cppcms::json::object config_obj;
	config_obj["debug"] = this->m_debug;
	config_obj["name"] = this->m_name;
	glob["global"] = config_obj;

	std::ostringstream os;
	glob.save( os, readable );
	return os.str();
}


//-----------------------------------


session_data &proxy_global::get_session_data( cppcms::session_interface &_session )
{
	int id;
	if ( _session.is_set( "id" ) )
	{
		id = mylib::from_string(_session.get( "id" ),id);
//		DOUT("Session coockie id: " << id );
	}
	else
	{
		id = ++this->m_session_id_counter;
		_session.set( "id", mylib::to_string(id) );
		DOUT("New Session coockie id: " << id );
	}

	for ( int index = 0; index < this->m_sessions.size(); index++ )
	{
		if ( this->m_sessions[index]->m_id == id )
		{
			//DOUT("Found session for id: " << id );
			this->m_sessions[index]->update_timestamp();
			return *this->m_sessions[index];
		}
	}
	DOUT("Adding new session for id: " << id );
	auto p = std::make_shared<session_data>( id );
	this->m_sessions.push_back( p );
	return *p;
}


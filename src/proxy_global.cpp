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
#include "proxy_global.h"
#include "cppcms_util.h"
#include <cppcms/view.h>


PluginHandler standard_plugin("");


proxy_global::proxy_global()
{
	this->m_port = 8085; // Default
	this->m_ip4_mask = "0.0.0.0";
	this->m_debug = false;	
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
	for ( auto iter = this->localclients.begin(); iter != this->localclients.end(); iter++ )
	{
		baseclient_ptr p = *iter;
		if ( p->m_active )
		{
			DOUT("Starting localhost at: " << p->m_local_port );
			p->start();
		}
	}
/*	
	for ( auto iter = this->providerclients.begin(); iter != this->providerclients.end(); iter++ )
	{
		providerclient_ptr p = *iter;
		if ( p->m_active )
		{
			DOUT("Starting providerclients at: " );
			p->start();
		}
	}
*/
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
	for ( auto iter = this->localclients.begin(); iter != this->localclients.end(); iter++ )
	{
		DOUT("localhost.Stop()");
		(*iter)->stop();
	}
/*	
	for ( auto iter = this->providerclients.begin(); iter != this->providerclients.end(); iter++ )
	{
		DOUT("providerclient.Stop()");
		(*iter)->stop();
	}
*/
	this->m_thread.stop();
	this->remotehosts.clear();
	this->localclients.clear();
	//this->providerclients.clear();
}




// The function will check for each of the main entries. If a main entry exists, then it is will overwrite, i.e. destroy any existing information.
// NB!! This should not be called while threads are active.
bool proxy_global::populate_json( cppcms::json::value &obj, int _json_acl )
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex);
	if ( (_json_acl & clients) > 0 && obj["clients"].type() == cppcms::json::is_array )
	{
		this->localclients.clear();
		cppcms::json::array ar = obj["clients"].array();
		for ( auto iter1 = ar.begin(); iter1 != ar.end(); iter1++ )
		{
			auto &item1 = *iter1;
			bool active = cppcms::utils::check_bool( item1, "active", true, false );
			int client_port = check_int( item1, "port", -1, true );
			bool provider = cppcms::utils::check_bool( item1, "provider", false, false );
			int max_connections = check_int( item1, "max_connections", 1, false );
			std::vector<ProxyEndpoint> proxy_endpoints, provider_endpoints;
			if ( item1["remotes"].type() == cppcms::json::is_array )
			{
				auto ar2 = item1["remotes"].array();
				for ( auto iter2 = ar2.begin(); iter2 != ar2.end(); iter2++ )
				{
					auto &item2 = *iter2;
					ProxyEndpoint ep( cppcms::utils::check_bool( item2, "active", true, false ), 
										check_string( item2, "name", "", true ), 
										check_string( item2, "hostname", "", true),
										check_int(item2,"port",-1,true) 
										);
					if (ep.m_active)
					{
						proxy_endpoints.push_back( ep );
					}
				}
			}
			if (provider)
			{
				if ( item1["locals"].type() == cppcms::json::is_array ) // Only for provider
				{
					auto ar2 = item1["locals"].array();
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
							provider_endpoints.push_back( ep );
						}
					}
				}
				if ( active && provider_endpoints.size() > 0 )
				{
					baseclient_ptr local_ptr( new ProviderClient( active, provider_endpoints, proxy_endpoints, standard_plugin ) );
					//local_ptr->m_proxy_endpoints = proxy_endpoints;
					this->localclients.push_back( local_ptr );
				}
				else
				{
					// NB!! Here should go an error if active
					DERR("Provider configuration invalid");
				}
			}
			else if ( active && proxy_endpoints.size() > 0 )
			{
				// NB!! Search for the correct plugin version
				baseclient_ptr local_ptr( new LocalHost( active, client_port, proxy_endpoints, max_connections, standard_plugin ) );
				//local_ptr->m_proxy_endpoints = proxy_endpoints;
				this->localclients.push_back( local_ptr );
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

	// ---- THE CLIENT PART ----
	for ( int index1 = 0; index1 < this->localclients.size(); index1++ )
	{
		BaseClient &local = *this->localclients[index1];
		cppcms::json::object obj;
		obj["id"] = local.m_id;
		obj["active"] = local.m_active;
		obj["port"] = local.m_local_port;
		obj["connected_local"] = local.is_local_connected();
		obj["log"] = local.dolog();

		// Fill in the list of attached local hosts.
		std::vector<std::string> hostnames = local.local_hostnames();
		for (int index3 = 0; index3 < hostnames.size(); index3++)
		{
			std::string sz = hostnames[index3];
			if ( sz.length() )
			{
				obj["local_hostname"][index3] = sz;
			}
		}

		for ( int index2 = 0; index2 < local.m_proxy_endpoints.size(); index2++ )
		{
			auto &r = local.m_proxy_endpoints[index2];
			cppcms::json::object obj2;
			obj2["name"] = r.m_name;
			obj2["hostname"] = r.m_hostname;
			obj2["port"] = r.m_port;
			obj2["connected_remote"] = local.is_remote_connected(index2);
			if ( local.is_remote_connected(index2) )
			{
				obj2["count_in"] = local.m_count_in.get();
				obj2["count_out"] = local.m_count_out.get();
			}
			for (int index3 = 0; index3 < m_cert_names.size(); index3++)
			{
				if ( r.m_name == m_cert_names[index3] )
				{
					obj2["cert"] = true;
				}
			}
			if ( local.m_proxy_index == index2 && local.m_activate_stamp > boost::get_system_time())
			{				
				auto div = local.m_activate_stamp - boost::get_system_time();
				obj2["activate"] = div.seconds();
			}

			obj["remotes"][index2] = obj2;
		}
		glob["clients"][index1] = obj;
	}
/*
	// ---- THE PROVIDER PART ----
	for ( int index = 0; index < this->providerclients.size(); index++ )
	{
		ProviderClient &prov = *this->providerclients[index];
		cppcms::json::object obj;
		obj["id"] = prov.m_id;
		obj["active"] = prov.m_active;
		obj["log"] = prov.dolog();
//		obj["name"] = prov.m_name;

		if ( prov.is_local_connected() )
		{
			obj["connected_local"] = true;
			obj["local_hostname"] = prov.local_hostname();
			obj["local_port"] = prov.local_port();
		}
		if ( prov.is_remote_connected() )
		{
			obj["connected_remote"] = true;
			obj["remote_hostname"] = prov.remote_hostname();
			obj["remote_port"] = prov.remote_port();
		}
		
		if ( prov.m_activate_stamp > boost::get_system_time())
		{				
			auto div = prov.m_activate_stamp - boost::get_system_time();
			obj["activate"] = div.total_seconds();
		}
		
		glob["providers"][index] = obj;
	}
*/
	// ---- THE HOST PART ----
	for ( int index = 0; index < this->remotehosts.size(); index++ )
	{
		RemoteProxyHost &host( *this->remotehosts[index] );
		cppcms::json::object obj_host;
		obj_host["id"] = host.m_id;
		obj_host["port"] = host.m_local_port; // .m_acceptor.local_endpoint().port();
		obj_host["type"] = host.m_plugin.m_type;

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
					}
					bool is_remote_connected = client.is_remote_connected();
					obj["connected_remote"] = is_remote_connected;
					if ( is_remote_connected ) // Not thread safe
					{
						obj["remote_hostname"] =  client.remote_endpoint().address().to_string();
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
	config_obj["name"] = this->m_name;
	glob["global"] = config_obj;

	std::ostringstream os;
	glob.save( os, readable );
	return os.str();
}


// NB!! Currently for debugging only.
std::string proxy_global::save_json_config( bool readable )
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex);
	cppcms::json::value glob;
	// The client part
	for ( int index = 0; index < this->localclients.size(); index++ )
	{
		BaseClient &host( *this->localclients[index] );
		cppcms::json::object obj;
		obj["id"] = host.m_id;
		obj["active"] = host.m_active;
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
/*	
	// The provider part
	for ( int index = 0; index < this->providerclients.size(); index++ )
	{
		ProviderClient &prov = *this->providerclients[index];
		cppcms::json::object obj;
		obj["id"] = prov.m_id;
		obj["active"] = prov.m_active;
		//obj["local_port"] = host.m_local_port;
		//obj["remote_hostname"] = prov.remote_hostname();
		//obj["remote_port"] = prov.remote_port();

		//obj["local_hostname"] = prov.local_hostname();
		//obj["local_port"] = prov.local_port();
		//obj["max_connections"] = host.m_max_connections;
		glob["providers"][index] = obj;
	}
*/
	cppcms::json::object web_obj;
	web_obj["port"] = this->m_port;
	web_obj["ip4"] = this->m_ip4_mask;
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


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
proxy_global global;



proxy_global::proxy_global()
{
	this->m_port = 8085; // Default
	this->m_ip4_mask = "0.0.0.0";
	this->m_debug = false;
}

//-----------------------------------


void proxy_global::lock()
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex_list);
	for ( auto iter = this->remotehosts.begin(); iter != this->remotehosts.end(); iter++ )
	{
		remotehost_ptr p = *iter;
		DOUT("Starting remotehost at: " << p->port() ); //m_acceptor.local_endpoint() );
		p->start();
	}
	for ( auto iter = this->localclients.begin(); iter != this->localclients.end(); iter++ )
	{
		baseclient_ptr p = *iter;
		if ( p->m_active )
		{
			DOUT("Starting localhost at: " << p->local_portname() );
			p->start();
		}
	}
}


void proxy_global::unlock()
{
	DOUT(__FUNCTION__ << ":" << __LINE__);
	//this->unpopulate_json(this->m_new_setup);
	this->stopall();
	DOUT(__FUNCTION__ << ":" << __LINE__);
}


void proxy_global::stopall()
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex_list);
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
	this->remotehosts.clear();
	this->localclients.clear();
}


bool proxy_global::is_provider(const BaseClient &client) const
{
	return dynamic_cast<const ProviderClient*>(&client) != nullptr;
}

bool has_array(const cppcms::json::value &obj, const std::string &name)
{
	cppcms::json::value res = obj.find( name );
	return res.type() == cppcms::json::is_array;
}


bool proxy_global::is_same( const BaseClient &client, cppcms::json::value &obj1, bool &param_changed, bool &remotes_changed ) const
{
	param_changed = false;
	remotes_changed = false;
	mylib::port_type client_port = 0;
	cppcms::utils::check_port( obj1, "port", client_port );
	if ( client.port() != client_port ) return false;
	if ( client_port > 0 || (client_port == 0 && this->is_provider(client)))
	{
		remotes_changed = true;
		cppcms::json::value res = obj1.find( "remotes" );
		if (res.type() == cppcms::json::is_array)
		{
			int i1 = res.array().size();
			if ( i1 == client.m_proxy_endpoints.size())
			{
				remotes_changed = false;
				for (int index = 0; index < client.m_proxy_endpoints.size(); index++)
				{				
					RemoteEndpoint ep;
					if (!ep.load(res[index]) || !(ep == client.m_proxy_endpoints[index]) )
					{
						remotes_changed = true;
					}
				}
			}
		}
		// param_changes, we should set.
		if (! (client.m_active == cppcms::utils::check_bool(obj1,"active",true,false)))
		{
			param_changed = true;
		}
		return true;
	}
	return false;
}


bool proxy_global::is_same( const RemoteProxyHost &host, cppcms::json::value &obj, bool &param_changed, bool &remotes_changed, bool &locals_changed ) const
{
	param_changed = false;
	remotes_changed = false;
	locals_changed = false;
	mylib::port_type host_port = 0;
	cppcms::utils::check_port( obj, "port", host_port );
	if ( host.port() != host_port ) return false;
	if ( host_port > 0)
	{
		remotes_changed = true;
		cppcms::json::value res = obj.find( "remotes" );
		if (res.type() == cppcms::json::is_array)
		{
			int i1 = res.array().size();
			if ( i1 == host.m_remote_ep.size())
			{
				remotes_changed = false;
				for (int index = 0; index < host.m_remote_ep.size(); index++)
				{
					RemoteEndpoint ep;
					if (!ep.load(res[index]) || !(ep == host.m_remote_ep[index]) )
					{
						remotes_changed = true;
					}
				}
			}
		}
		// Check for local connection changes.
		locals_changed = true;
		res = obj.find( "locals" );
		if (res.type() == cppcms::json::is_array)
		{
			int i1 = res.array().size();
			if (i1 == host.m_local_ep.size())
			{
				locals_changed = false;
				for (int index = 0; index < host.m_local_ep.size(); index++)
				{
					LocalEndpoint ep;
					if (!ep.load(res[index]) || !(ep == host.m_local_ep[index]) )
					{
						locals_changed = true;
					}
				}
			}
		}

		// param_changes, we should set.
		if (! (host.m_plugin.m_type == cppcms::utils::check_string(obj,"type","",false)))
		{
			param_changed = true;
		}
		return true;
	}
	return false;
}


// obj represents the new setup. So anything running that doesn't match the new object must be stopped and removed.
void proxy_global::unpopulate_json( cppcms::json::value &obj )
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex_list);

	for ( auto iter = this->localclients.begin(); iter != this->localclients.end(); )
	{
		std::vector<RemoteEndpoint> rep;
		std::vector<LocalEndpoint> lec;
		bool keep = false;
		if ( obj["clients"].type() == cppcms::json::is_array )
		{
			cppcms::json::array clients = obj["clients"].array();
			for ( auto &client : clients )
			{
				bool param = false;
				bool connections = false;
				if (this->is_same(*(*iter),client,param,connections))
				{
					keep = !(param || connections);
				}
			}
		}
		if (keep)
		{
			DOUT("keeping client on port: " << (*iter)->port() );
			iter++;
		}
		else
		{
			DOUT("Stopping and removing client on port: " << (*iter)->port() );
			(*iter)->stop();
			iter = this->localclients.erase(iter);
		}
	}
	for ( auto iter = this->remotehosts.begin(); iter != this->remotehosts.end(); )
	{
		bool keep = false;
		cppcms::json::value kobj;
		if ( obj["hosts"].type() == cppcms::json::is_array )
		{
			cppcms::json::array hosts = obj["hosts"].array();
			for ( auto &host : hosts )
			{
				bool param = false;
				bool remote_changes = false;
				bool local_changes = false;
				if (this->is_same(*(*iter),host,param,remote_changes,local_changes))
				{
					kobj = host;
					keep = !(param || local_changes); // || remote_changes  Currently any kind of changes resets
				}
			}
		}
		if (keep)
		{
			DOUT("keeping host on port: " << (*iter)->port() << " Checking and removing any cancelled remote connections" );
			std::vector<RemoteEndpoint> eps;
			load_endpoints(kobj,"remotes",eps); // If we fail to read, we will get an empty and stop all.
			for (auto it2 = (*iter)->m_clients.begin(); it2 != (*iter)->m_clients.end(); )
			{
				if (std::find_if(eps.begin(),eps.end(), [&](const RemoteEndpoint &ep){ return ep.m_name == (*it2)->m_endpoint.m_name; } ) == eps.end())
				{
					// It was not in the list of next round of valid names, so we should remove.
					DOUT("On port: " << (*iter)->port() << " Stop client with name: " << (*it2)->m_endpoint.m_name );
					(*it2)->stop();
					it2 = (*iter)->m_clients.erase(it2);
				}
				else
				{
					it2++;
				}
			}
			iter++;
		}
		else
		{
			DOUT("Stopping and removing host on port: " << (*iter)->port() );
			remotehost_ptr p = *iter;
			RemoteProxyHost* p2 = p.get();
			p2->stop();
			iter = this->remotehosts.erase(iter);
		}
		DOUT("remotehost.Stop()");
	}
}


// The function will check for each of the main entries. If a main entry exists, then it is will overwrite, i.e. destroy any existing information.
// NB!! This should not be called while threads are active.
void proxy_global::populate_json( cppcms::json::value &obj, int _json_acl )
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex_list);
	if ( (_json_acl & clients) > 0 && obj["clients"].type() == cppcms::json::is_array )
	{
		DOUT(__FUNCTION__ << " populating clients");

		cppcms::json::array ar = obj["clients"].array();
		for ( auto iter1 = ar.begin(); iter1 != ar.end(); iter1++ )
		{
			auto &item1 = *iter1;
			bool active = cppcms::utils::check_bool( item1, "active", true, false );
			mylib::port_type client_port = cppcms::utils::check_int( item1, "port", 0, true );
			auto iter_cur = std::find_if(this->localclients.begin(),this->localclients.end(),[&](const baseclient_ptr& client){ return client_port == client->port();});
			if (iter_cur != this->localclients.end())
			{
				DOUT("Found existing running client on port: " << client_port);
				continue;
			}

			int help;
			bool provider = cppcms::utils::check_bool( item1, "provider", false, false );
			boost::posix_time::time_duration read_timeout = boost::posix_time::minutes(5);
			if (cppcms::utils::check_int( item1, "timeout", help ) && help > 0)
			{
				read_timeout = boost::posix_time::minutes(help);
			}
			int max_connections = cppcms::utils::check_int( item1, "max_connections", 1, false );
			std::vector<RemoteEndpoint> proxy_endpoints;
			std::vector<LocalEndpoint> provider_endpoints;
			if ( item1["remotes"].type() == cppcms::json::is_array )
			{
				auto ar2 = item1["remotes"].array();
				for ( auto iter2 = ar2.begin(); iter2 != ar2.end(); iter2++ )
				{
					auto &item2 = *iter2;
					RemoteEndpoint ep;
					/*( //cppcms::utils::check_bool( item2, "active", true, false ), 
										check_string( item2, "name", "", true ), 
										check_string( item2, "hostname", "", true),
										check_int(item2,"port",-1,true) 
										);*/
					//if (ep.m_active)
					if (ep.load(item2))
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
						LocalEndpoint ep; /*( //cppcms::utils::check_bool( item2, "active", true, false ), 
											check_string( item2, "name", "", false ), 
											check_string( item2, "hostname", "", true),
											check_int(item2,"port",-1,true) 
											);*/
						//if (ep.m_active)
						if (ep.load(item2))
						{
							provider_endpoints.push_back( ep );
						}
					}
				}
				if ( //active && 
					provider_endpoints.size() > 0 )
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
				baseclient_ptr local_ptr( new LocalHost( active, client_port, proxy_endpoints, max_connections, standard_plugin, read_timeout ) );
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
			mylib::port_type host_port = cppcms::utils::check_int( item1, "port", 0, true );
			std::vector<LocalEndpoint> local_endpoints;
			if ( item1["locals"].type() == cppcms::json::is_array )
			{
				auto ar2 = item1["locals"].array();
				for ( auto iter2 = ar2.begin(); iter2 != ar2.end(); iter2++ )
				{
					auto &item2 = *iter2;
					LocalEndpoint addr; //( cppcms::utils::check_string( item2, "hostname", "", true ), 
									//cppcms::utils::check_int( item2, "port", 0, true ) );
					if (addr.load(item2)) local_endpoints.push_back( addr );
				}
			}
			std::vector<RemoteEndpoint> remote_endpoints;
			if ( item1["remotes"].type() == cppcms::json::is_array )
			{
				auto ar2 = item1["remotes"].array();
				for ( auto iter2 = ar2.begin(); iter2 != ar2.end(); iter2++ )
				{
					auto &item2 = *iter2;
					RemoteEndpoint ep; //( //cppcms::utils::check_bool( item2, "active", true, false ), 
										//cppcms::utils::check_string( item2, "name", "", false ), 
										//cppcms::utils::check_string( item2, "hostname", "", true),
										//cppcms::utils::check_string( item2, "username", "", false), 
										//cppcms::utils::check_string( item2, "password", "", false ) );						
					if (ep.load(item2)) remote_endpoints.push_back( ep );
				}
			}
			// Check size > 0
			PluginHandler *plugin = &standard_plugin;
			std::string plugin_type = cppcms::utils::check_string( item1, "type", "", false );
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
				remotehost_ptr remote_ptr = std::make_shared<RemoteProxyHost>( host_port, remote_endpoints, local_endpoints, *plugin );
				this->remotehosts.push_back( remote_ptr );
			} // NB!! What else if one of them is empty
		}
	}
	if ( (_json_acl & web) > 0 && obj["web"].type() == cppcms::json::is_object )
	{
		cppcms::json::value &web_obj( obj["web"] );
		cppcms::utils::check_port( web_obj, "port", this->m_port );
	}
	if ( (_json_acl & config) > 0 && obj["config"].type() == cppcms::json::is_object )
	{
		cppcms::json::value &config_obj( obj["config"] );
		cppcms::utils::check_string( config_obj, "name", this->m_name );
		cppcms::utils::check_bool( config_obj, "debug", this->m_debug );
	}
}

	
std::string proxy_global::save_json_status( bool readable )
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex_list);
	cppcms::json::value glob;

	// ---- THE CLIENT PART ----
	for ( int index1 = 0; index1 < this->localclients.size(); index1++ )
	{
		BaseClient &local = *this->localclients[index1];
		cppcms::json::object obj;
		obj["id"] = local.m_id;
		obj["active"] = local.m_active;
		obj["port"] = local.local_portname();
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
			if (this->certificate_available(r.m_name))
			{
				obj2["cert"] = true;
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


	// ---- THE HOST PART ----
	for ( int index = 0; index < this->remotehosts.size(); index++ )
	{
		RemoteProxyHost &host( *this->remotehosts[index] );
		cppcms::json::object obj_host;
		obj_host["id"] = host.m_id;
		obj_host["port"] = host.port();
		obj_host["type"] = host.m_plugin.m_type;

		// Loop through each remote proxy
		for ( int index2 = 0; index2 < host.m_remote_ep.size(); index2++ )
		{
			cppcms::json::object obj;
			//obj["active"] = host.m_remote_ep[index2].m_active;
			obj["name"] = host.m_remote_ep[index2].m_name;
			if (this->certificate_available(host.m_remote_ep[index2].m_name))
			{
					obj["cert"] = true;
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
						obj["remote_hostname"] =  client.remote_endpoint().address().to_string(); // NB!! This one is dangerous
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


bool proxy_global::execute_openssl()
{
	std::string params;
#ifdef _WIN32
	params = " -config openssl.cnf ";
#endif
	int res = process::execute_process( "openssl", " req " + params + "-x509 -nodes -days 10000 -subj /C=DK/ST=Denmark/L=GateHouse/CN=" + this->m_name + " -newkey rsa:1024 -keyout my_private_key.pem -out my_public_cert.pem "
	, [&](const std::string &_out) { DOUT(_out); }
	, [&](const std::string &_err) { DERR(_err); }
	);
	DOUT("Execute openssl result: " << res);
	return res == 0;
}


//
bool proxy_global::load_configuration()
{
	static int openssl_count = 0;
	std::string certificate_common_name;
	try
	{
		{
			int line = 0;
			cppcms::json::value my_object;
			std::ifstream ifs( config_filename );
			ASSERTE(my_object.load( ifs, false, &line ), uniproxy::error::parse_file_failed, config_filename + " on line: " + mylib::to_string(line));
			this->populate_json(my_object,proxy_global::config|proxy_global::web);
		}
		ASSERTE(!this->m_name.empty(),uniproxy::error::certificate_name_unknown, config_filename + " should contain \"config\" : { \"name\" : \"own_certificate_name\" }" );

		// NB!! For these we should not overwrite any existing files. If the files exist, but there is an error in them we need to get user interaction ??
		bool load_private = false;
		bool load_public = false;
		{
			std::ifstream ifs( my_private_key_name );
			if ( ifs.good() )
			{
				boost::system::error_code ec1,ec2;
				boost::asio::ssl::context ctx(boost::asio::ssl::context_base::sslv23);
				ctx.use_private_key_file(my_private_key_name,boost::asio::ssl::context_base::file_format::pem,ec1);
				ctx.use_certificate_file(my_public_cert_name,boost::asio::ssl::context_base::file_format::pem,ec2);
				load_private = !ec1 && !ec2;
				DOUT("Private handling: " << load_private << " 1:" << ec1 << " " << (ec1) << " " << !ec1 << " 2:" << ec2 << " " << (ec2) << " " << !ec2 );
				if (!load_private)
				{
					log().add("Found private file which does NOT contain a valid private key. Delete file: " + my_private_key_name + " and try again");
				}
			}
		}
		{
			std::vector<certificate_type> certs;
			load_public = load_certificates_file( my_public_cert_name, certs ) && certs.size() > 0;
			if ( load_public )
			{
				certificate_common_name = get_common_name(certs[0]);
				DOUT("Found own private certificate with name: \"" << certificate_common_name + "\"" );
				if ( this->m_name == certificate_common_name )
				{
					DOUT("Certificate do match own name");
				}
				else
				{
					log().add("Own name \"" + this->m_name + "\" does not match own certificate \"" + certificate_common_name + "\"");
					//throw std::runtime_error("Own name \"" + this->m_name + "\" does not match own certificate \"" + certificate_common_name + "\"" );
				}
			}
		}
		if ( ! (load_private && load_public) )
		{
			if ( this->m_name.length() == 0 )
			{
				throw std::runtime_error("certificate files not found and cannot be generated because the name is not specified in the configuration json file" );
			}
			if ( this->execute_openssl() )
			{
				// NB!! We should now be ready to try once again. Can we end up in an endless loop here ? Guard anyway.
				if ( ++openssl_count < 3 )
				{
					log().add("Succesfully executed the initial OpenSSL command");
					throw mylib::reload_exception();
				}
				log().add("Unkown fatal error, possibly endless loop!");
			}
			else
			{
				log().add("Failed to execute initial OpenSSL command. Is OpenSSL properly installed and available in the path");
			}
		}
		{	// Sort of a safe create if not exist
			std::ifstream ifs( my_certs_name );
			if ( !ifs.good() )
			{
				std::ofstream ofs( my_certs_name );
			}
		}

		// 
		if ( this->m_name.length() > 0 && this->m_name == certificate_common_name )
		{
			int line;
			this->m_new_setup.null(); // Reset to ensure we don't have undefined behaviour.
			std::ifstream ifs( config_filename );
			if (!this->m_new_setup.load( ifs, false, &line ) )
			{
				log().add("Failed to load and parse configuration file: " + config_filename + " on line: " + mylib::to_string(line));
			}
			this->populate_json(this->m_new_setup,proxy_global::all);
		}
		this->load_certificate_names( my_certs_name );
	}
	catch( std::exception exc )
	{
		
		return false;
	}
	return true;
}


// NB!! Currently for debugging only.
std::string proxy_global::save_json_config( bool readable )
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex_list);
	cppcms::json::value glob;
	// The client part
	for ( int index = 0; index < this->localclients.size(); index++ )
	{
		BaseClient &host( *this->localclients[index] );
		cppcms::json::object obj;
		obj["id"] = host.m_id;
		obj["active"] = host.m_active;
		obj["local_port"] = host.local_portname();
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
		obj_host["port"] = host.port(); // .m_acceptor.local_endpoint().port();
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
			//obj["active"] = host.m_remote_ep[index2].m_active;
			obj["hostname"] = host.m_remote_ep[index2].m_hostname;
			obj["username"] = host.m_remote_ep[index2].m_username;
			obj["password"] = host.m_remote_ep[index2].m_password;
			obj_host["remotes"][index2] = obj;
		}
		glob["hosts"][index] = obj_host;
	}

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
	stdt::lock_guard<stdt::mutex> l(this->m_session_data_mutex);
	int id;
	if ( _session.is_set( "id" ) )
	{
		id = mylib::from_string(_session.get( "id" ),id);
	}
	else
	{
		id = rand();
		_session.set( "id", mylib::to_string(id) );
	}

	for ( int index = 0; index < this->m_sessions.size(); index++ )
	{
		if ( this->m_sessions[index]->m_id == id )
		{
			this->m_sessions[index]->update_timestamp();
			return *this->m_sessions[index];
		}
	}
	DOUT("Adding new session for id: " << id );
	auto p = std::make_shared<session_data>( id );
	this->m_sessions.push_back( p );
	return *p;
}


void proxy_global::clean_session()
{
	stdt::lock_guard<stdt::mutex> l(this->m_session_data_mutex);

}


bool proxy_global::load_certificate_names( const std::string & _filename )
{
	stdt::lock_guard<stdt::mutex> lock(this->m_mutex_certificates);
   std::string names;
	bool result = false;
	std::vector< certificate_type > certs;
	m_cert_names.clear();
	if ( load_certificates_file( _filename, certs ) )
	{
		for ( auto iter = certs.begin(); iter != certs.end(); iter++ )
		{
			m_cert_names.push_back( get_common_name( *iter ) );
         names += "|" + get_common_name( *iter );
		}
		result = true;
	}
   DOUT("Load certificates from file: " << names );
	return result;
}


std::error_code proxy_global::SetupCertificates( boost::asio::ip::tcp::socket &_remote_socket, const std::string &_connection_name, bool _server, std::error_code& ec )
{
	try
	{
		ec = make_error_code( uniproxy::error::unknown_error );
		DOUT( __FUNCTION__ << " " << _server << " connection name: " << _connection_name << " server?: " << _server );
		const int buffer_size = 4000;
		char buffer[buffer_size]; //
		memset(buffer,0,buffer_size);
		ASSERTE( _connection_name.length() > 0, uniproxy::error::connection_name_unknown, "" );
		if ( _server )
		{
			int length = _remote_socket.read_some( boost::asio::buffer( buffer, buffer_size ) );
			DOUT("Received: " << length << " bytes");
			ASSERTE( length > 0 && length < buffer_size, uniproxy::error::certificate_invalid, "received" ); // NB!! Check the overflow situation.....
			DOUT("SSL Possible Certificate received: " << buffer );
			std::vector<certificate_type> remote_certs, local_certs;

			ASSERTE( load_certificates_string( buffer, remote_certs ) && remote_certs.size() == 1, uniproxy::error::certificate_invalid, "received for connection: " + _connection_name  );
			std::string remote_name = get_common_name( remote_certs[0] );
			DOUT("Received certificate name: " << remote_name << " for connection: " << _connection_name );

			ASSERTE(_connection_name == remote_name, uniproxy::error::certificate_invalid, "Received " + remote_name + " for connection: " + _connection_name + ". They must match");
			ASSERTE( load_certificates_file( my_certs_name, local_certs ), uniproxy::error::certificate_not_found, std::string( " in file ") + my_certs_name + " for connection: " + _connection_name);

			for ( auto iter = local_certs.begin(); iter != local_certs.end(); )
			{
				if ( remote_name == get_common_name( *iter ) )
				{
					DOUT("Removing old existing certificate name for replacement: " << remote_name );
					iter = local_certs.erase( iter );
				}
				else
				{
					iter++;
				}
			}
			local_certs.push_back( remote_certs[0] );
			save_certificates_file( my_certs_name, local_certs );
			this->load_certificate_names( my_certs_name );
			return ec = std::error_code();
		}
		else
		{
			std::string cert = readfile( my_public_cert_name );
			ASSERTE( cert.length() > 0, uniproxy::error::certificate_not_found, std::string( " certificate not found in file: ") + my_certs_name );
			int count = _remote_socket.write_some( boost::asio::buffer( cert, cert.length() ) );
			DOUT("Wrote certificate: " << count << " of " << cert.length() << " bytes ");
			return ec = std::error_code();
		}
	}
	catch( std::exception &exc )
	{
		DOUT( __FUNCTION__ << " exception: " << exc.what() );
	}
	return ec;
}


bool proxy_global::certificate_available( const std::string &_cert_name)
{
	stdt::lock_guard<stdt::mutex> lock(this->m_mutex_certificates);
	return std::find(this->m_cert_names.begin(), this->m_cert_names.end(), _cert_name ) != this->m_cert_names.end();
}


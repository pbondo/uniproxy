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
#include "main.h"

#include <cppcms/applications_pool.h>
#include <cppcms/mount_point.h>

#include <cppcms/http_response.h>
#include <cppcms/http_file.h>
#include "cppcms_util.h"

#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <fstream>

#include <webserver/content.h>
#include <boost/process.hpp>
#include "proxy_global.h"

namespace cppcms 
{

bool signal::m_reload;
cppcms::service *signal::m_psrv = NULL;

} // namespace

// I didn't want to bother having a file for a single line of code. The following line does not really belong here.
std::vector<PluginHandler*> *PluginHandler::m_plugins = NULL;
class cppcms::form c;
proxy_global global;
std::string config_filename = "uniproxy.json";



////////////////////////////////////////////

proxy_app::proxy_app(cppcms::service &srv) : cppcms::application(srv)
{
	dispatcher().assign("^/command/client/activate/name=(.*)&dummy=(.*)$",&proxy_app::client_activate,this,1);
	dispatcher().assign("^/command/host/activate/name=(.*)&dummy=(.*)$",&proxy_app::host_activate,this,1);
	dispatcher().assign("^/command/client/delete/name=(.*)&dummy=(.*)$",&proxy_app::certificate_delete,this,1);
	dispatcher().assign("^/command/host/delete/name=(.*)&dummy=(.*)$",&proxy_app::certificate_delete,this,1);
	dispatcher().assign("^/command/stop/(.*)$",&proxy_app::shutdown,this);
	dispatcher().assign("^/command/config/upload/(.*)$",&proxy_app::config_upload,this);
	dispatcher().assign("^/command/config/reload/(.*)$",&proxy_app::config_reload,this);
	dispatcher().assign("^/script/(.*)$",&proxy_app::script,this,1);
	dispatcher().assign("^/status/(.*)$",&proxy_app::status_get,this);
	dispatcher().assign("^/logger/(.*)$",&proxy_app::logger_get,this);
	dispatcher().assign("^/$",&proxy_app::index,this);
	//dispatcher().assign("^//(.*)$",&proxy_app::,this);
}


void proxy_app::index()
{
	auto &data = global.get_session_data( this->session() );
	//DOUT( "proxy_app::main: " << url << " resetting for id: " << data.m_id);
	data.m_read_index = 0; // reset			
	content::content_status c;
	c.title = "Maritime UniProxy";
	this->render( "status", c );
}


void proxy_app::config_reload()
{
	DOUT( __FUNCTION__ ); //"proxy_app::main: " << url );
	throw mylib::reload_exception();
}


void proxy_app::logger_get()
{
	stdt::lock_guard<stdt::mutex> l(global.m_session_data_mutex); // A long lock. Should perhaps be more fine grained.

	int log_write_index = log().m_log.size();
	auto &data = global.get_session_data( this->session() );
	for ( int index = data.m_read_index; index < log_write_index; index++ )
	{
		this->response().out() << log().m_log[index] << std::endl;
	}
	data.m_read_index = log_write_index;
}


void proxy_app::status_get()
{
	this->response().out() << this->status_get_json();
}

void proxy_app::script(const std::string)
{
	int kb = 0;
	std::string filename; // = url.substr(1); // NB! This is a security risc, therefore we start by hardcoding the single valid file to read.
	filename = "script/jquery.js";
	std::ifstream ifs(filename);
	DOUT("Reading from file: " << filename );
	while ( ifs ) // We could probably use std::getline, but it may overflow. The following will not accept \0 characters.
	{
		kb++;
		char buffer[1001];
		memset(buffer, 0, 1001 );
		ifs.read( buffer, 1000 );
		this->response().out() << buffer;
	}
	DOUT("Read: ca " << kb << " kbyte data");
}



void proxy_app::host_activate(const std::string _param)
{
	DOUT(__FUNCTION__ <<  ": " << _param );
	for ( auto iter = global.remotehosts.begin(); iter != global.remotehosts.end(); iter++ )
	{
		RemoteProxyHost *p = iter->get();
		for ( auto iter = p->m_remote_ep.begin(); iter != p->m_remote_ep.end(); iter++ )
		{
			RemoteEndpoint &ep = *iter;
			if ( ep.m_name == _param )
			{
				p->m_activate_stamp = boost::get_system_time() + boost::posix_time::seconds( 60 );
				p->m_activate_name = _param;
			}
		}
	}
}


void proxy_app::certificate_delete(const std::string _param)
{
	DOUT( __FUNCTION__ << ": " << _param );
	if ( delete_certificate_file( my_certs_name, _param ) )
	{
		throw mylib::reload_exception();
	}
}


void proxy_app::my404( std::string url )
{
	DOUT( "proxy_app::my404: " << url );
	response().out() << "404 " << mylib::time_stamp() ;
}


std::string proxy_app::status_get_json()
{
	std::string result = global.save_json_status( true );
	return result;
}


// NB!! kludge! Should be possible inline with a simple regex
bool check_remove_substr( const std::string &_input, const std::string &_pattern, std::string & _output )
{
	if ( _input.substr( 0, _pattern.length() ) == _pattern )
	{
		_output = _input.substr( _pattern.length() );
		return true;
	}
	return false;
}


bool check_url( const std::string &_input, const std::string &_pattern, std::string & _output )
{
	boost::regex re; re = _pattern; //(char*)_pattern.c_str();
	boost::cmatch matches;
	if ( boost::regex_match(_input.c_str(), matches, re) )
	{
		if ( matches.size() > 0 )
		{
			_output = std::string( matches[1].first, matches[1].second );
		}
		return true;
	}
	return false;
}




void proxy_app::config_upload()
{
	try
	{
		DOUT( __FUNCTION__);
		cppcms::widgets::file image;
		if(request().request_method()=="POST")
		{
			std::string filename = "upload.json";
			image.name( filename );
			image.load(context());
			image.name( filename );
			if(image.validate())
			{
				DOUT("Saving configuration file as " << filename );
				image.value()->save_to( filename );
				image.clear();
				// NB!! Here we should validate and reload configuration.

				int line = 0;
				std::ifstream ifs( filename );
				cppcms::json::value my_object;
				ASSERTE( my_object.load( ifs, false, &line ), uniproxy::error::parse_file_failed, filename );
				boost::system::error_code ec;
				
				if ( boost::filesystem::exists(config_filename) )
				{
					boost::filesystem::copy_file( config_filename, config_filename + ".backup", boost::filesystem::copy_option::overwrite_if_exists, ec );
					ASSERTE( ec == boost::system::errc::success, uniproxy::error::file_failed_copy, config_filename  + " to " + config_filename + ".backup" );
				}
				boost::filesystem::copy_file( filename, config_filename, boost::filesystem::copy_option::overwrite_if_exists, ec );
				ASSERTE( ec == boost::system::errc::success, uniproxy::error::file_failed_copy, filename + " to " + config_filename );

				//this->config_reload();
				this->response().set_redirect_header("/");
				throw mylib::reload_exception();
			}
		}
	}
	catch( std::system_error &exc1 )
	{
		log().add( exc1.code().message() + " " + exc1.what() );
	}
	catch( std::exception &exc2 )
	{
		log().add( exc2.what() );
	}
	this->response().set_redirect_header("/");
}


void proxy_app::shutdown()
{
	DOUT(__FUNCTION__);
	this->service().shutdown();
}


void proxy_app::client_activate(const std::string _param)
{
	DOUT(__FUNCTION__ << ": " << _param );
	for ( auto iter = global.localhosts.begin(); iter != global.localhosts.end(); iter++ )
	{
		LocalHost *p = iter->get();
		for ( int index = 0; index < p->m_proxy_endpoints.size(); index++ )
		{
			auto &r = p->m_proxy_endpoints[index];
			if ( r.m_name == _param )
			{
				p->m_activate_stamp = boost::get_system_time() + boost::posix_time::seconds( 60 );
				p->m_proxy_index = index;
				global.m_thread.stop();
				global.m_thread.start( p->m_local_port );
				break;
			}
		}
	}
}


void proxy_app::setup_config( cppcms::json::value &settings_object )
{
	settings_object["file_server"]["enable"]=false;
	if ( global.m_debug )
	{
		DOUT("Debug = TRUE");
		settings_object["file_server"]["enable"]=true;
		settings_object["http"]["script"]="/json";
	}
	#ifdef _WIN32
	std::string coockie_path = getenv("TEMP");
	if ( !coockie_path.length() )
	{
		coockie_path = "c:/temp";
	}
	#else
	std::string coockie_path = "/tmp";
	#endif
	std::string coockie = "uniproxy" + mylib::to_string(global.m_port);
	settings_object["service"]["api"]="http";
	settings_object["service"]["port"]= global.m_port;
	settings_object["service"]["ip"]= global.m_ip4_mask;
	settings_object["session"]["location"] = "client";
	settings_object["session"]["cookies"]["prefix"] = coockie;
	settings_object["session"]["server"]["storage"] = "files"; //memory";
	settings_object["session"]["server"]["dir"] = coockie_path + "/uniproxy_" + boost::posix_time::to_iso_string(boost::get_system_time().time_of_day() ).substr(0,6);
	settings_object["session"]["client"]["encryptor"] = "hmac";
	settings_object["session"]["client"]["key"] = "29b6e071ad5870228c6a2115d88d3b2e";
	DOUT( "Webserver configuration: " << settings_object );
}


bool proxy_app::execute_openssl()
{
	// openssl req -x509 -nodes -days 365 -subj "/C=DK/ST=Denmark/L=GateHouse/CN=client" -newkey rsa:1024 -keyout my_private_key.pem -out my_public_cert.pem
	std::string scriptname = "openssl req -x509 -nodes -days 365 -subj /C=DK/ST=Denmark/L=GateHouse/CN=" + global.m_name + " -newkey rsa:1024 -keyout my_private_key.pem -out my_public_cert.pem";
	boost::process::context ctx;
	ctx.environment = boost::process::self::get_environment();
	ctx.stdout_behavior = boost::process::capture_stream(); 
	ctx.stderr_behavior = boost::process::capture_stream();
	DOUT("Execute openssl: " << scriptname );
	boost::process::child c = boost::process::launch_shell( scriptname, ctx );
	std::string line; 
	boost::process::pistream &is1 = c.get_stdout(); 
	while (std::getline(is1, line)) 
		std::cout << line << std::endl; 	
	boost::process::pistream &is2 = c.get_stderr(); 
	while (std::getline(is2, line)) 
		std::cout << line << std::endl; 	

	DOUT("Waiting for openssl: " << c.get_id() );
	boost::process::status s = c.wait();
	DOUT("openssl returned: " << s.exit_status() << " " << s.exited() );
	return (s.exit_status() == 0) && (s.exited() == 1);
}


void proxy_app::main(std::string url)
{
	try
	{
		std::string url2 = url;
		//DOUT("main url: " << url );
		std::string param;
		// If we run in debug mode then the json part will already have been removed.
		if ( check_remove_substr( url, "/json", param ) )
		{
			url = param;
		}
		if ( url.find("/logger") == std::string::npos && url.find( "/status" ) == std::string::npos )
		{
			DOUT("main url: " << url );
		}
		//DOUT( "proxy_app::main: " << url << " unknown command, checking base" );
		this->cppcms::application::main(url); //"/json" + url);

	}
	catch( mylib::reload_exception &)
	{
		DOUT( __LINE__ << "Reload exception");
		//global.m_reload = true;
		cppcms::signal::set_reload();
		this->service().shutdown();
	}
	catch( std::exception &exc )
	{
		log().add( exc.what() );
	}
}


//-------------------------------------------------------



session_data::session_data( int _id )
{
	this->m_id = _id;
	this->m_read_index = 0;
	this->update_timestamp();
}


void session_data::update_timestamp()
{
	this->m_timestamp = boost::get_system_time();
}



//-----------------------------------

const char help_text[] = "Usage: uniproxy [-l/--working-dir=<working directory>]\nLog on with webbrowser http://localhost:8085/\n";



int main(int argc,char ** argv)
{
	int openssl_count = 0;

   if (check_arg( argc, argv, 'h', "help"))
   {
      std::cout << help_text << std::endl;
      return 0;
   }
   std::string workdir;
   if (check_arg( argc, argv, 'w', "working-dir", workdir) && workdir.length() > 0)
   {
      // If this fails, there is not much we can do about it anyway, so we silently fail.
      boost::system::error_code ec;
      boost::filesystem::current_path(workdir,ec);
   }

	do
	{
		try
		{
			cppcms::signal::reset_reload();
			std::string certificate_common_name;
			log().clear();
			DOUT( std::string("UniProxy starting with parameters: ") << argv[0] << std::string(" in path: ") << boost::filesystem::current_path() );
			log().add( std::string("UniProxy starting" ) ); // with parameters: ") + argv[0] + std::string(" in path: ") );

			DOUT("Loading plugins count: " << PluginHandler::plugins().size() );
			for ( int index = 0; index < PluginHandler::plugins().size(); index++ )
			{
				DOUT("Plugin loaded: " << PluginHandler::plugins()[index]->m_type );
			}

			{
				int line = 0;
				cppcms::json::value my_object;
				std::ifstream ifs( config_filename );
				if ( ! my_object.load( ifs, false, &line ) || !global.populate_json(my_object,proxy_global::config|proxy_global::web) )
				{
					log().add("Failed to load and parse configuration file: " + config_filename + " on line: " + mylib::to_string(line) );
				}
			}

			if ( global.m_name.length() > 0 ) // We have a "possibly" valid name.
			{
				// NB!! For these we should not overwrite any existing files. If the files exist, but there is an error in them we need to get user interaction ??
				bool load_private = false, load_public = false;
				{
					std::ifstream ifs( my_private_key_name );
					load_private = ifs.good(); // It is not a certificate, so we should not override!!!!
				}
				{
					std::vector<certificate_type> certs;
					load_public = load_certificates_file( my_public_cert_name, certs ) && certs.size() > 0;
					if ( load_public )
					{
						certificate_common_name = get_common_name(certs[0]);
						DOUT("Found own certificate with name: \"" << certificate_common_name + "\"" );
						if ( global.m_name != certificate_common_name )
						{
							log().add( "Own name \"" + global.m_name + "\" does not match own certificate \"" + certificate_common_name + "\"" );
						}
					}
				}
				if ( ! (load_private && load_public) )
				{
					if ( global.m_name.length() == 0 )
					{
						throw std::runtime_error("certificate files not found and cannot be generated because the name is not specified in the configuration json file" );
					}
					if ( proxy_app::execute_openssl() )
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
				{
					std::ifstream ifs( my_certs_name );
					if ( !ifs.good() )
					{
						std::ofstream ofs( my_certs_name );
					}
				}
			}
			else
			{
				log().add( "Failed to load own name from configuration file at: global : { name : value }");
			}

			// 
			if ( global.m_name.length() > 0 && global.m_name == certificate_common_name )
			{
				int line;
				cppcms::json::value my_object;
				std::ifstream ifs( config_filename );
				if (!my_object.load( ifs, false, &line ) )
				{
					log().add("Failed to load and parse configuration file: " + config_filename + " on line: " + mylib::to_string(line));
				}
				if (!global.populate_json(my_object,proxy_global::all))
				{
					log().add("Failed to populate configuration");
				}
			}
			load_certificate_names( my_certs_name );
			stdt::lock_guard<proxy_global> lock(global);

			DOUT( "Loaded config: " << global.save_json_config( true ) );
			// Create settings object data
			cppcms::json::value settings_object;
			proxy_app::setup_config( settings_object );
			cppcms::service srv(settings_object);
			if ( global.m_debug )
			{
				srv.applications_pool().mount(cppcms::applications_factory<proxy_app>());
			}
			else
			{
				srv.applications_pool().mount(cppcms::applications_factory<proxy_app>(),cppcms::mount_point(""));
			}
			cppcms::signal sig(srv);
			srv.run();
		}
		catch( std::exception &exc )
		{
			log().add( exc.what() );
		}
		catch( mylib::reload_exception &)
		{
			DOUT("Reload");
			cppcms::signal::set_reload();
		}
	}
	while( cppcms::signal::reload() );
	DOUT( "Application stopping" );
	return 0;
}


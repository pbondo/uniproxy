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

// The following unfortunately needs to be before.main.h,  otherwise we have a link error with copy_file (crap)
#ifdef _WIN32
#else
#define BOOST_NO_CXX11_SCOPED_ENUMS
#endif
#include <boost/filesystem.hpp>

#include "main.h"

#include <cppcms/applications_pool.h>
#include <cppcms/mount_point.h>
#include <cppcms/http_response.h>
#include <cppcms/http_file.h>
#include "cppcms_util.h"

#include <boost/regex.hpp>

#include <fstream>

#include <webserver/content.h>
#include <boost/process.hpp>
#include "proxy_global.h"
#include "httpclient.h"

#ifdef _WIN32
#include "win_util.h"
#endif

namespace cppcms 
{

bool signal::m_reload;
cppcms::service *signal::m_psrv = NULL;

} // namespace

// I didn't want to bother having a file for a single line of code. The following line does not really belong here.
std::vector<PluginHandler*> *PluginHandler::m_plugins = NULL;
class cppcms::form c;




////////////////////////////////////////////

proxy_app::proxy_app(cppcms::service &srv) : cppcms::application(srv), timer_(srv.get_io_service())
{
	dispatcher().assign("^/command/client/activate/name=(.*)&id=(.*)&dummy=(.*)$",&proxy_app::client_activate,this,1,2);
	dispatcher().assign("^/command/client/active/name=(.*)&id=(.*)&check=(.*)&dummy=(.*)$",&proxy_app::client_active,this,1,2,3);
	dispatcher().assign("^/command/host/activate/name=(.*)&dummy=(.*)$",&proxy_app::host_activate,this,1);
	dispatcher().assign("^/command/host/active/name=(.*)&id=(.*)&check=(.*)&dummy=(.*)$",&proxy_app::host_active,this,1,2,3);
	dispatcher().assign("^/command/client/delete/name=(.*)&dummy=(.*)$",&proxy_app::certificate_delete,this,1);
	dispatcher().assign("^/command/host/delete/name=(.*)&dummy=(.*)$",&proxy_app::certificate_delete,this,1);
	dispatcher().assign("^/command/stop/(.*)$",&proxy_app::shutdown,this);
	dispatcher().assign("^/command/config/upload/(.*)$",&proxy_app::config_upload,this);
	dispatcher().assign("^/command/config/reload/(.*)$",&proxy_app::config_reload,this);
	dispatcher().assign("^/script/(.*)$",&proxy_app::script,this,1);
	dispatcher().assign("^/status/(.*)$",&proxy_app::status_get,this);
	dispatcher().assign("^/logger/(.*)$",&proxy_app::logger_get,this);
	dispatcher().assign("^/command/certificate/get/(.*)$",&proxy_app::get_certificates,this,1);
	dispatcher().assign("^/$",&proxy_app::index,this);
	//dispatcher().assign("^//(.*)$",&proxy_app::,this);
}


void proxy_app::index()
{
	auto &data = global.get_session_data( this->session() );
	//DOUT( "proxy_app::main: " << url << " resetting for id: " << data.m_id);
	data.m_logger_read_index = 0; // reset			
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
	int log_write_index = log().count();
	auto &data = global.get_session_data( this->session() );
	for ( int index = data.m_logger_read_index; index < log_write_index; index++ )
	{
		std::string text = log().get(index);
		if (!text.empty())
		{
			this->response().out() << text << std::endl;
		}
	}
	data.m_logger_read_index = log_write_index;
}


void proxy_app::status_get()
{
	this->response().content_type("application/json");
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


void proxy_app::get_certificates(const std::string _param)
{
	std::string certs = readfile( my_certs_name );
	this->response().out() << certs;
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
				global.m_activate_host.add(_param);
				return; // No need to add it more than once.
			}
		}
	}
}


void proxy_app::host_active(const std::string _param, const std::string _id, const std::string _checked)
{
	DOUT(__FUNCTION__ << ": " << _param << " Checked: " << _checked);

	for ( auto iter = global.remotehosts.begin(); iter != global.remotehosts.end(); iter++ )
	{
		RemoteProxyHost *p = iter->get();
		int id;
		if (p->m_id == mylib::from_string(_id,id) )
		{
			if (_checked == "true")
			{
				p->m_active = true;
				p->start();
			}
			else
			{
				p->m_active = false;
				p->stop();
			}
			DOUT("Updated active state for host: " << id << " new value: " << _checked);
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
	boost::regex re; re = _pattern;
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


// Reset without stopping the application.
void proxy_app::timeout_handle()
{
	cppcms::signal::set_reload();
	this->shutdown();
}


void proxy_app::config_upload()
{
	try
	{
		DOUT( __FUNCTION__);
		if(request().request_method()=="POST")
		{
			std::string filename = "upload.json";
			if (context().request().files().size() > 0) // Indicates a file has been uploaded
			{
				DOUT("Saving configuration from file as " << filename );
				cppcms::widgets::file image;
				image.name( filename );
				image.load(context());
				image.name( filename );
				ASSERTE(image.validate(), uniproxy::error::parse_file_failed, filename );
				image.value()->save_to( filename );
				image.clear();
				DOUT("Saved configuration file as " << filename );
			}
			else
			{
				DOUT("Saving configuration from json data");
				cppcms::http::request::form_type ft = context().request().post();
				ASSERTE(ft.begin() != ft.end(), uniproxy::error::parse_file_failed, filename );
				std::ofstream ofs(filename);
				ofs << (*ft.begin()).second;
				ofs.close();
				DOUT("Saved configuration file as " << filename );
			}
			// NB!! Here we should validate and reload configuration.
			int line = 0;
			std::ifstream ifs( filename );
			cppcms::json::value newobj;
			ASSERTE( newobj.load( ifs, false, &line ), uniproxy::error::parse_file_failed, filename );

			boost::system::error_code ec;
			if ( boost::filesystem::exists(config_filename) )
			{
				boost::filesystem::copy_file( config_filename, config_filename + ".backup", boost::filesystem::copy_option::overwrite_if_exists, ec );
				ASSERTE( ec == boost::system::errc::success, uniproxy::error::file_failed_copy, config_filename  + " to " + config_filename + ".backup" );
			}
			boost::filesystem::copy_file( filename, config_filename, boost::filesystem::copy_option::overwrite_if_exists, ec );
			ASSERTE( ec == boost::system::errc::success, uniproxy::error::file_failed_copy, filename + " to " + config_filename );

			// Check if configuration has changed to warrant a restart.
			if (global.is_new_configuration(newobj))
			{
				DOUT("Configuration changed sufficiently to warrant a restart. All connections will be closed");
				this->response().set_redirect_header("/");
				throw mylib::reload_exception();
			}
			else
			{
				try
				{
					global.unpopulate_json(newobj);
					global.populate_json(newobj, proxy_global::json_acl::all);
					DOUT("Updated configuration without restart");
				}
				catch(std::exception exc)
				{
					DOUT("Configuration update failed to warrant a restart. All connections will be closed");
					throw mylib::reload_exception();
				}
			}
		}
	}
	catch( std::system_error &exc1 )
	{
		log().add( "Error: " + exc1.code().message() + " " + exc1.what() );
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


void proxy_app::client_activate(const std::string _param, const std::string _id)
{
	DOUT(__FUNCTION__ << ": " << _param << " ID: " << _id);

	for ( auto iter = global.localclients.begin(); iter != global.localclients.end(); iter++ )
	{
		BaseClient *p = iter->get();
		int id;
		if (p->m_id == mylib::from_string(_id,id) )
		{
			for ( int index = 0; index < p->m_proxy_endpoints.size(); index++ )
			{
				auto &r = p->m_proxy_endpoints[index];
				if ( r.m_name == _param )
				{
					p->m_activate_stamp = boost::get_system_time() + boost::posix_time::seconds(30); // The client should timeout quickly
					p->m_proxy_index = index;
					p->stop_activate();
					p->start_activate(index);
					break;
				}
			}
		}
	}
}


void proxy_app::client_active(const std::string _param, const std::string _id, const std::string _checked)
{
	DOUT(__FUNCTION__ << ": " << _param << " ID: " << _id << " Checked: " << _checked);

	for ( auto iter = global.localclients.begin(); iter != global.localclients.end(); iter++ )
	{
		BaseClient *p = iter->get();
		int id;
		if (p->m_id == mylib::from_string(_id,id) )
		{
			p->m_active = (_checked == "true") ? true : false;
			if (p->m_active)
			{
				p->start();
			}
			else
			{
				p->stop();
			}
			DOUT("Updated active state for: " << _param << " new value: " << p->m_active);
			return;
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
	std::string coockie = "uniproxy" + mylib::to_string(global.m_web_port);
	settings_object["service"]["api"]="http";
	settings_object["service"]["port"]= global.m_web_port;
	settings_object["service"]["ip"]= global.m_ip4_mask;
	settings_object["session"]["location"] = "client";
	settings_object["session"]["cookies"]["prefix"] = coockie;
	settings_object["session"]["server"]["storage"] = "files"; //memory";
	settings_object["session"]["server"]["dir"] = coockie_path + "/uniproxy_" + boost::posix_time::to_iso_string(boost::get_system_time().time_of_day() ).substr(0,6);
	settings_object["session"]["client"]["encryptor"] = "hmac";
	settings_object["session"]["client"]["key"] = "29b6e071ad5870228c6a2115d88d3b2e";
	DOUT( "Webserver configuration: " << settings_object );
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
		if ( url.find("/logger") == std::string::npos && url.find( "/status" ) == std::string::npos && url.find("/command/certificate/get/") == std::string::npos)
		{
			DOUT("main url: " << url );
		}
		this->cppcms::application::main(url);
	}
	catch( mylib::reload_exception &)
	{
		DOUT( __LINE__ << "Reload exception");
		this->timer_.expires_from_now(booster::ptime::milliseconds(1000));
		this->timer_.async_wait(binder(this,&proxy_app::timeout_handle));
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
	this->m_logger_read_index = 0;
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
	if (check_arg( argc, argv, 'h', "help"))
	{
		std::cout << help_text << std::endl;
		return 0;
	}
#ifdef _WIN32
   EnableFirewallRule();
#endif
	do
	{
		try // Outer loop for reload exceptions.
		{
#ifdef __linux__			
			if (cppcms::signal::reload())
			{
				cppcms::signal::reset_reload(); // Reset here to allow signal 1 or 15 while waiting
				DOUT("Reloading, we need to wait 60+ seconds before restart in order to ensure we don't hook up on the same port again");
				mylib::msleep(70000);
				DOUT("Finished waiting for restart");
			}
#endif
			srand(time(NULL));
			cppcms::signal::reset_reload();
			log().clear();
			DOUT( std::string("UniProxy starting in path: ") << boost::filesystem::current_path());
			log().add( std::string("UniProxy starting" ) );

			DOUT("Loading plugins count: " << PluginHandler::plugins().size() );
			for ( int index = 0; index < PluginHandler::plugins().size(); index++ )
			{
				DOUT("Plugin loaded: " << PluginHandler::plugins()[index]->m_type );
			}
			if (!global.load_configuration())
			{
				log().add("Failed to load configuration"); // Written in global exception handler...?
			}
			stdt::lock_guard<proxy_global> lock(global);
			client_certificate_exchange cert_exch;
			stdt::lock_guard<client_certificate_exchange> certificate_exchange_lock(cert_exch);
			if (!global.uniproxies.empty())
			{
				cert_exch.start(global.uniproxies);
			}

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
	DOUT( "Stop all connections" );
	global.stopall();
	DOUT( "Application stopping" );
	return 0;
}


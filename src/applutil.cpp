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
#include "applutil.h"
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/chrono.hpp>
#include <boost/process.hpp>
#include <boost/iostreams/stream.hpp>

#ifdef _WIN32
#include <mstcpip.h>
#endif


namespace mylib
{

std::ostream &dout()
{
   return log().m_logfile;
}


std::ostream &derr()
{
   return log().m_logfile;
}


std::string time_stamp()
{
	std::string sz;
   boost::posix_time::ptime stamp = boost::get_system_time();
   sz += boost::gregorian::to_iso_string(stamp.date()) + " ";
	boost::posix_time::time_duration d = stamp.time_of_day();
	d = boost::posix_time::time_duration( d.hours(), d.minutes(), d.seconds(), 0 );
	sz += boost::posix_time::to_simple_string( d );
	return sz;
}


void msleep(int millisec)
{
#ifdef __use_boost_thread__

	boost::posix_time::ptime abstime = boost::get_system_time();
	if ( millisec >= 0 )
	{
		abstime += boost::posix_time::millisec( millisec );
	}
	boost::thread::sleep( abstime );

#else

	std::this_thread::sleep_for( std::chrono::milliseconds(millisec) );

#endif
}

}


namespace boost {

namespace filesystem {

const std::string path_separator()
{
	return std::string("/");
}

}

namespace asio {

/// int _timeout = 0 [seconds] if <= 0 then don't change system value.
void socket_set_keepalive_to( ip::tcp::socket::lowest_layer_type &_socket, std::chrono::seconds _timeout )
{
	int result;
	std::uint32_t optval = false;
	if (_timeout >  std::chrono::seconds(0))
	{
	#ifdef _WIN32
		struct tcp_keepalive
		{
			u_long  onoff;
			u_long  keepalivetime;
			u_long  keepaliveinterval;
		};
		struct tcp_keepalive keepalive = { 0 };
		DWORD uiBytes = sizeof(keepalive);
		keepalive.onoff = 1;
		keepalive.keepalivetime = (u_long)(std::chrono::duration_cast<std::chrono::milliseconds>(_timeout)).count();
		keepalive.keepaliveinterval = 1000;
		result = WSAIoctl(_socket.native(), SIO_KEEPALIVE_VALS, &keepalive, sizeof(keepalive), NULL, 0, &uiBytes, 0, 0);
	#else
		socklen_t optlen = sizeof(optval);
		optval = _timeout.count();
		result = setsockopt(_socket.native(), SOL_TCP, TCP_KEEPIDLE, &optval, optlen);
		result = getsockopt(_socket.native(), SOL_TCP, TCP_KEEPIDLE, &optval, &optlen);
		optval = 3;
		result = setsockopt(_socket.native(), SOL_TCP, TCP_KEEPCNT, &optval, optlen);
		result = getsockopt(_socket.native(), SOL_TCP, TCP_KEEPCNT, &optval, &optlen);
		optval = 10;
		result = setsockopt(_socket.native(), SOL_TCP, TCP_KEEPINTVL, &optval, optlen);
		result = getsockopt(_socket.native(), SOL_TCP, TCP_KEEPINTVL, &optval, &optlen);
		if ( result){}
	#endif
		optval = true;
	}
	socklen_t optlen = sizeof(optval);
	result = setsockopt(_socket.native(), SOL_SOCKET, SO_KEEPALIVE, (const char*)&optval, optlen);
	//result = getsockopt(_socket.native(), SOL_SOCKET, SO_KEEPALIVE, (char*)&optval, &optlen);
}


void socket_shutdown( ip::tcp::socket::lowest_layer_type &_socket, boost::system::error_code &ec )
{
	_socket.shutdown(ip::tcp::socket::shutdown_both,ec);
}


void socket_shutdown( boost::asio::ip::tcp::acceptor &_acceptor, boost::system::error_code &ec )
{
	shutdown( _acceptor.native(), ip::tcp::socket::shutdown_both );
	ec = make_error_code(boost::system::errc::success);
}


void sockect_connect( ip::tcp::socket::lowest_layer_type &_socket, boost::asio::io_service &_io_service, const std::string &_host, int _port )
{
	// The following section uses a DNS resolver which should be valid for both IPv4 and IPv6
	boost::asio::ip::tcp::resolver resolver(_io_service);
	boost::asio::ip::tcp::resolver::query query( _host, mylib::to_string( _port ) );
	boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
	boost::asio::ip::tcp::resolver::iterator end;

	boost::system::error_code error = boost::asio::error::host_not_found;
	while (error && endpoint_iterator != end)
	{
		_socket.close();
		boost::asio::ip::tcp::endpoint ep( *endpoint_iterator++ );
		DOUT("Attempting connect to: " << ep );
		_socket.connect(ep, error);
		DOUT("Result from connect to: " << ep << " : " << error );
	}
	if (error)
	{
		throw boost::system::system_error(error);
	}
}


bool is_connected( ip::tcp::socket::lowest_layer_type &_socket )
{
	boost::system::error_code ec;
	if (_socket.is_open())
	{
		// is_open is not enough, we need to ensure we are connected. Check for the remote end_point.
		// boost::asio::ip::tcp::endpoint ep = 
		_socket.remote_endpoint(ec);
		return (!ec);
	}
	return false;
}


bool get_certificate_issuer_subject( boost::asio::ssl::stream<boost::asio::ip::tcp::socket> &_socket, std::string &_issuer, std::string &_subject )
{
	bool result = false;
	if (X509* cert = SSL_get_peer_certificate( _socket.native_handle()))
	{
		if (SSL_get_verify_result(_socket.native_handle()) == X509_V_OK)
		{
			char buf[256] = "";
			X509_NAME_oneline(X509_get_subject_name(cert), buf, 256);
			_subject = buf;
			X509_NAME_oneline(X509_get_issuer_name(cert), buf, 256);
			_issuer = buf;
			result = true;
		}
		X509_free( cert );
	}
	return result;
}

}
}


std::string get_subject_name( const certificate_type &_cert )
{
	char buf[256] = "";
	X509_NAME_oneline(X509_get_subject_name(_cert.get()), buf, 256);
	return buf;
}


std::string get_issuer_name( const certificate_type &_cert )
{
	char buf[256] = "";
	X509_NAME_oneline(X509_get_issuer_name(_cert.get()), buf, 256);
	return buf;
}


std::string get_common_name( const certificate_type &_cert )
{
	std::string common_name, issuer = get_issuer_name( _cert );
	if ( issuer.length() > 0 )
	{
		std::vector< std::string > result;
		boost::algorithm::split_regex( result, issuer, boost::regex( "/CN=" ) );
		//DOUT("Split: " << result.size() );
		if ( result.size() > 1 )
		{
			common_name = result[1];
		}
		DOUT("Received certificate CN= " << common_name );
	}
	return common_name;
}


bool delete_certificate_file( const std::string &_filename, const std::string &_name )
{
	std::vector<certificate_type> local_certs;
	if ( load_certificates_file( my_certs_name, local_certs ) )
	{
		bool hit;
		do
		{
			hit = false;
			for ( auto iter = local_certs.begin(); iter != local_certs.end(); iter++ )
			{
				if ( _name == get_common_name( *iter ) )
				{
					DOUT("Deleting: " << _name );
					hit = true;
					local_certs.erase(iter);
					break;
				}
			}
		}
		while( hit );
		return save_certificates_file( my_certs_name, local_certs );
		//return true
	}
	return false;
}


int do_clear( X509 * cert)
{
	if ( cert )
	{
		X509_free( cert );
	}
	return 0;
}


bool load_certificates_file( const std::string &_filename, std::vector<certificate_type> &_certs )
{
	bool result = false;
	FILE *file;
	file = fopen( _filename.c_str(), "r" );
	DOUT( __FUNCTION__ << " file: " << _filename << " ok: " << (file != NULL) );
	if ( file != NULL )
	{
		X509 * cert;
		while( (cert = PEM_read_X509(file, NULL, NULL, NULL)) != NULL )
		{
			std::shared_ptr<X509> scert( cert, do_clear );
			_certs.push_back( scert );
			DOUT("Read certificate");
		}
		result = true;
		fclose( file );
	}
	return result;
}


bool load_certificates_string( const std::string &_certificate_string, std::vector<certificate_type> &_certs )
{
	std::string tmpname = boost::filesystem::unique_path().string();
	DOUT("Using temp filename: " << tmpname );
	std::ofstream ofs( tmpname );
	ofs << _certificate_string;
	ofs.close();
	bool result = load_certificates_file( tmpname, _certs );
	boost::filesystem::remove(tmpname);
	return result;
}


bool save_certificates_file( const std::string &_filename, const std::vector<certificate_type> &_certs )
{
	bool result = false;
	FILE *file;
	file = fopen( _filename.c_str(), "w" );
	DOUT( __FUNCTION__ << " file: " << _filename << " ok: " << (file != NULL) );
	if ( file != NULL )
	{
		for ( int index = 0; index < _certs.size(); index++ )
		{
			PEM_write_X509(file, _certs[index].get());
		}
		result = true;
		fclose( file );
	}
	return result;
}


int check_int( cppcms::json::value &_input_obj, const std::string &_inputname, const int _default_value, bool _required )
{
	int result = _default_value;
	if ( _inputname.length() > 0 )
	{
		cppcms::json::value value = _input_obj.find( _inputname );
		if ( value.type() == cppcms::json::is_number )
		{
			result = value.get_value<int>( );
		}
		else if ( value.type() == cppcms::json::is_string )
		{
			std::string sz = value.get_value<std::string>( );
			mylib::from_string( sz, result );
		}
	}
	else if ( _required )
	{
		// NB!! Give a better description and location of the problem. And how do we show this to the user ?
		throw std::runtime_error("Failed to validate JSON input for value " + _inputname + " which is required");
	}
	return result;
}


std::string check_string( cppcms::json::value &_input_obj, const std::string &_inputname, const std::string & _default_value, bool _required )
{
	std::string result = _default_value;
	if ( _inputname.length() > 0 )
	{
		cppcms::json::value value = _input_obj.find( _inputname );
		if ( value.type() == cppcms::json::is_string )
		{
			result = value.get_value<std::string>( );
		}
	}
	else if ( _required )
	{
		// NB!! Give a better description and location of the problem. And how do we show this to the user ?
		throw std::runtime_error("Failed to validate JSON input for value " + _inputname + " which is required");
	}
	return result;
}


int check_int( const char *_input)
{
	int result;
	if ( _input != nullptr && strlen(_input) > 0 )
	{
		std::string sz = _input;
		mylib::from_string( sz, result );
		
		return result;
	}
	throw std::runtime_error("check_int failed");
}


std::string check_ip4( const std::string &_input )
{
	std::string result;
	if ( _input.length() > 0 )
	{
		// NB!! Here we should at least check for x.x.x.x
		result = _input;
		return result;
	}
	throw std::runtime_error("check_ip4 failed: ");
}


//-----------------------------------

proxy_log::proxy_log(const std::string&_name)
{
	this->m_name = _name;
	this->m_logfile.open("uniproxy.log",std::ios::app|std::ios::ate);
}

std::string proxy_log::peek()
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex);
	std::string sz = "&nbsp;";
	if ( this->m_log.size() > 0)
	{
		sz = this->m_log.back();
	}
	return sz;
}


void proxy_log::add( const std::string &_value )
{
	DOUT("Log: " << this->m_name << ": " << _value );
	stdt::lock_guard<stdt::mutex> l(this->m_mutex);
	this->m_log.push_back( mylib::time_stamp() + ": " + _value);
	this->m_logfile << mylib::time_stamp() << ": " << _value << std::endl;
}


void proxy_log::clear()
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex);
//	this->m_log.clear();
}


int64_t data_flow::timestamp()
{
	auto time = boost::chrono::steady_clock::now(); 		// get the current time
	auto since_epoch = time.time_since_epoch(); 		// get the duration since epoch
	boost::chrono::seconds secs = boost::chrono::duration_cast<boost::chrono::seconds>(since_epoch);
	return secs.count();
}


data_flow::data_flow( bool _debug )
{
	this->m_debug = _debug;
	for (int index = 0; index < data_flow_size; index++)
	{
		m_buffer[index] = 0;
	}
	this->m_stamp = this->timestamp();
}


void data_flow::cleanup( int64_t _stamp )
{
	if ( _stamp >= this->m_stamp + data_flow_size )
	{
		for (int index = 0; index < data_flow_size; index++)
		{
			this->m_buffer[index] = 0;
		}
	}
	else
	{
		int index = this->m_stamp % data_flow_size;
		int index_stop = _stamp % data_flow_size;
		while ( index != index_stop )	// for index = start +1; index <= stop
		{
			index = (index + 1) % data_flow_size;
			this->m_buffer[index] = 0;
		}
	}
}


void data_flow::add( size_t _count )
{
	int64_t stamp = this->timestamp();
	this->cleanup(stamp);
	this->m_buffer[ stamp % data_flow_size ] += _count;
	this->m_stamp = stamp;
}


size_t data_flow::get()
{
	int64_t stamp = this->timestamp();
	this->cleanup(stamp);
	size_t value = 0;
	for (int index = 0; index < data_flow_size; index++)
	{
		value += this->m_buffer[index];
	}
	return value;
}


const char* uniproxy::category_impl::name() const noexcept
{
	return "uniproxy";
}


class error_code_pair
{
public:

	uniproxy::error::error_t m_error;
	const char *m_message;
};


#define error_code_def( xx, yy ) { uniproxy::error::xx, yy }
static const error_code_pair ecp[] =
{
	#include "error_codes.h"
};
#undef error_code_def


std::string uniproxy::category_impl::message(int ev) const
{
	for ( int index = 0; ecp[index].m_message != NULL; index++ )
	{
		if ( static_cast<int>(ecp[index].m_error) == ev )
		{
			return ecp[index].m_message;
		}
	}
	return "unknown error";
}


const std::error_category& application_category()
{
  static uniproxy::category_impl instance;
  return instance;
}


std::error_code make_error_code(uniproxy::error::error_t e)
{
  return std::error_code( static_cast<int>(e), application_category() );
}


std::error_condition make_error_condition(uniproxy::error::error_t e)
{
  return std::error_condition( static_cast<int>(e), application_category());
}


std::string readfile( const std::string &_filename )
{
	std::string result;
	int length;
	std::ifstream ifs( _filename );
	ifs.seekg( 0, std::ios::end );
	length = ifs.tellg();
	ifs.seekg( 0, std::ios::beg );
	if ( length > 0 )
	{
		std::unique_ptr<char> buffer( new char[length] );
		ifs.read( buffer.get(), length );
		result = buffer.get();
	}
	ifs.close();
	return result;
}


proxy_log &log()
{
	static proxy_log uniproxy_log("uniproxy_log");
	return uniproxy_log;
}


bool operator==( boost::asio::ip::tcp::socket &p1, boost::asio::ip::tcp::socket &p2 )
{
	return p1.native() == p2.native();
}


std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems)
{
   std::stringstream ss(s);
   std::string item;
   while(std::getline(ss, item, delim))
   {
      elems.push_back(item);
   }
   return elems;
}


std::vector<std::string> split(const std::string &s, char delim)
{
   std::vector<std::string> elems;
   return split(s, delim, elems);
}


bool check_arg(int argc, char *argv[], char _short_argument, const char *_long_argument)
{
   for ( int index = 0; index < argc; index++ )
   {
      if ( _short_argument && std::string( "-" ) + _short_argument == argv[index] )
      {
         return true;
      }
      if ( _long_argument && std::string( "--" ) + std::string(_long_argument) == std::string(argv[index]) )
      {
         return true;
      }
   }
   return false;
}


bool check_arg( int argc, char *argv[], char _short_argument, const char *_long_argument, std::string &result )
{
   for ( int index = 0; index < argc; index++ )
   {
      if ( _short_argument && std::string( "-" ) + _short_argument == argv[index] )
      {
         index++;
         if (index >= argc || argv[index][0] == '-') // Check if there is an appended parameter, which is not an argument.
         {
            return false;
         }
         result = argv[index];
         return true;
      }
      if ( _long_argument && std::string(argv[index]).find("--") == 0 )
      {
         std::vector<std::string> strs = split(argv[index], '=');
         if ( strs.size() >= 2 && std::string( "--" ) + std::string(_long_argument) == strs[0])
         {			
            result = strs[1];
            return true;
         }
      }
   }
   return false;
}


int process::execute_process( const std::string& _command, const std::string& _param , std::function<void(const std::string&)> _out , std::function<void(const std::string&)> _err )
{
	namespace bp = boost::process;
	using namespace boost::iostreams;

	bp::pipe pout = bp::create_pipe();
	bp::pipe perr = bp::create_pipe();
	std::unique_ptr<bp::child> pc;
	int exit_code = -1;
	{
		std::string cmd;
		std::string param;
#ifdef _WIN32
		//cmd = "cmd.exe";
		//param = " /C \"" + _command + ".exe " + _param + "\"";
		cmd = _command + ".exe";
		param = _param;
#else
		cmd = "/bin/bash";
		param = " -c \"" + _command + " " + _param + "\"";
#endif
		DOUT("Execute process: " << cmd << param );
		file_descriptor_sink sinkout(pout.sink, close_handle);
		file_descriptor_sink sinkerr(perr.sink, close_handle);
		pc = std2::make_unique<bp::child>( bp::execute(
			bp::initializers::run_exe( cmd ), bp::initializers::set_cmd_line(param),
			bp::initializers::bind_stdout(sinkout), bp::initializers::bind_stderr(sinkerr), bp::initializers::inherit_env() ) );
	}
	file_descriptor_source sourceout(pout.source, close_handle);
	file_descriptor_source sourceerr(perr.source, close_handle);
	stream<file_descriptor_source> isout(sourceout);
	stream<file_descriptor_source> iserr(sourceerr);
	std::string line;
	while (std::getline(isout, line))
	{
		if ( _out ) _out(line);
	}
	while (std::getline(iserr, line))
	{
		if ( _err ) _err(line);
	}
	exit_code = bp::wait_for_exit(*pc);
	return exit_code;
}

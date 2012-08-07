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
#include "applutil.h"
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/regex.hpp>

#ifdef _WIN32
#include <mstcpip.h>
#endif

namespace mylib
{

std::ostream &dout()
{
	return std::cout;
}


std::ostream &derr()
{
	return std::cout;
}


std::string time_stamp()
{
	std::string sz;
	boost::posix_time::time_duration d = boost::get_system_time().time_of_day();
	d = boost::posix_time::time_duration( d.hours(), d.minutes(), d.seconds(), 0 );
	sz = boost::posix_time::to_simple_string( d );
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
/*
	boost::filesystem::path slash("/");
	std::string preferredSlash;
#ifdef _WIN32
	slash.make_preferred();
	preferredSlash = "/"; //NB!! This does not compile on Windows (std::string)slash.make_preferred().native();
	preferredSlash = slash; //.c_str();
		//.native();
#else
	preferredSlash = slash.make_preferred().native();
#endif
	return preferredSlash;
*/
	return std::string("/");
}
}

namespace asio {

/// int _timeout = 0 [seconds] if <= 0 then don't change system value.
void socket_set_keepalive_to( ip::tcp::socket::lowest_layer_type &_socket, int _timeout )
{
	if ( _timeout >  0 )
	{
	#ifdef _WIN32
		int iResult;
		int _native_socket = _socket.native();
		struct tcp_keepalive
		{
			u_long  onoff;
			u_long  keepalivetime;
			u_long  keepaliveinterval;

		};
		struct tcp_keepalive keepalive = {0};
		DWORD uiBytes=0;
		iResult=WSAIoctl(_native_socket,SIO_KEEPALIVE_VALS, NULL, 0, &keepalive,sizeof(keepalive), &uiBytes,0,0);
		if ( iResult == 0 )
		{
			keepalive.onoff=1;
			//keepalive.keepalivetime=_iTime;
			keepalive.keepaliveinterval=_timeout;
			iResult=WSAIoctl(_native_socket,SIO_KEEPALIVE_VALS, &keepalive,sizeof(keepalive), NULL, 0,&uiBytes,0,0);
		}
	#else
		int result;
		//int s = _native_socket;
		int s = _socket.native();
		int optval;
		socklen_t optlen = sizeof(optval);
		optval = _timeout;

		result = setsockopt(s, SOL_TCP, TCP_KEEPIDLE, &optval, optlen);
		result = getsockopt(s, SOL_TCP, TCP_KEEPIDLE, &optval, &optlen);
		optval = 3;
		result = setsockopt(s, SOL_TCP, TCP_KEEPCNT, &optval, optlen);
		result = getsockopt(s, SOL_TCP, TCP_KEEPCNT, &optval, &optlen);
		optval = 10;
		result = setsockopt(s, SOL_TCP, TCP_KEEPINTVL, &optval, optlen);
		result = getsockopt(s, SOL_TCP, TCP_KEEPINTVL, &optval, &optlen);
		optval = 1;
		result = setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen);
		result = getsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &optval, &optlen);
		if ( result){}
	#endif
	}
}


void socket_shutdown( ip::tcp::socket::lowest_layer_type &_socket )
{
	_socket.shutdown(ip::tcp::socket::shutdown_both);
}


void socket_shutdown( boost::asio::ip::tcp::acceptor &_acceptor )
{
	shutdown( _acceptor.native(), ip::tcp::socket::shutdown_both );
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

//int socket_read_some( boost::asio::buffer &_buffer, const boost::posix_time::time_duration &_duration )


}
}


std::vector<std::string> m_cert_names;

bool load_certificate_names( const std::string & _filename )
{
	bool result = false;
	std::vector< certificate_type > certs;
	m_cert_names.clear();
	if ( load_certificates_file( _filename, certs ) )
	{
		for ( auto iter = certs.begin(); iter != certs.end(); iter++ )
		{
			m_cert_names.push_back( get_common_name( *iter ) );
		}
		result = true;
	}
	return result;
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


//bool SetupCertificates( boost::asio::ip::tcp::socket &_remote_socket, const std::string &_connection_name, bool _server )
std::error_code SetupCertificates( boost::asio::ip::tcp::socket &_remote_socket, const std::string &_connection_name, bool _server, std::error_code& ec )
{
	try
	{
		ec = make_error_code( uniproxy::error::unknown_error );
		DOUT( __FUNCTION__ << " " << _server );
		const int buffer_size = 4000;
		char buffer[buffer_size]; //
		memset(buffer,0,buffer_size);
		//ASSERTD( _connection_name.length() > 0, "Attempting to setup a certificate without a known connection name");
		ASSERTE( _connection_name.length() > 0, uniproxy::error::connection_name_unknown, "" );
		if ( _server )
		{
			//ec = make_error_code( uniproxy::error::receive_certificate );
			int length = _remote_socket.read_some( boost::asio::buffer( buffer, buffer_size ) );
			DOUT("Received: " << length << " bytes");
			//ASSERTD( length > 0 && length < buffer_size, "Invalid data received from remote host during certificate exchange" ); // NB!! Check the overflow situation.....
			ASSERTE( length > 0 && length < buffer_size, uniproxy::error::certificate_invalid, "received" ); // NB!! Check the overflow situation.....
			DOUT("SSL Possible Certificate received: " << buffer );
			std::vector<certificate_type> remote_certs, local_certs;
			//ASSERTD( load_certificates_string( buffer, remote_certs ) && remote_certs.size() == 1, "Invalid certificate received from remote host during certificate exchange" );
			ASSERTE( load_certificates_string( buffer, remote_certs ) && remote_certs.size() == 1, uniproxy::error::certificate_invalid, "received" );

			std::string remote_name = get_common_name( remote_certs[0] );
			DOUT("Received certificate name: " << remote_name << " for connection: " << _connection_name );
			if ( _connection_name == remote_name ) //.length() > 0 )
			{
				//ASSERTD( load_certificates_file( my_certs_name, local_certs ), std::string( "Failed to load local certificates from file: " ) + my_certs_name );
				ASSERTE( load_certificates_file( my_certs_name, local_certs ), uniproxy::error::certificate_not_found, std::string( " in file ") + my_certs_name );

				for ( auto iter = local_certs.begin(); iter != local_certs.end(); iter++ )
				{
					if ( remote_name == get_common_name( *iter ) )
					{
						DOUT("Removing old existing certificate name for replacement: " << remote_name );
						local_certs.erase( iter );
						break;
					}
				}
				local_certs.push_back( remote_certs[0] );
				save_certificates_file( my_certs_name, local_certs );
				return ec = std::error_code();
			}
		}
		else
		{
			std::string cert = readfile( my_public_cert_name );
			ASSERTE( cert.length() > 0, uniproxy::error::certificate_not_found, std::string( " certificate not found in file: ") + my_certs_name );
			int count = _remote_socket.write_some( boost::asio::buffer( cert, cert.length() ) );
			DOUT("Wrote certificate: " << count << " of " << cert.length() << " bytes ");
			return ec = std::error_code();
/*

			//ec = make_error_code( uniproxy::error::send_certificate );
			//DOUT("Failed SSL handshake, will attempt to send my public certificate");
			std::ifstream ifs( my_public_cert_name ); //"my_public_cert.pem");
			ASSERTE( ifs.good(), uniproxy::error::certificate_not_found, std::string( " file not opened ") + my_certs_name );
			//int length = ifs.readsome( buffer, buffer_size );
			int length = ifs.readsome( buffer, buffer_size );
			DOUT( __FUNCTION__ << ": " << __LINE__ << " : " << length );
			//if ( length > 0 && length < buffer_size )
			ASSERTE( length > 0 && length < buffer_size, uniproxy::error::certificate_not_found, std::string( " in file ") + my_certs_name );
			int count = _remote_socket.write_some( boost::asio::buffer( buffer, length ) );
			DOUT("Wrote certificate: " << count << " of " << length << " bytes ");
				return ec = std::error_code();
*/
		}
	}
	catch( std::exception &exc )
	{
		DOUT( __FUNCTION__ << " exception: " << exc.what() );
	}
	return ec;
}


int do_clear( X509 * cert)
{
	//DOUT("Clearing X509");
	if ( cert )
	{
		X509_free( cert );
	}
	//DOUT("Cleared X509");
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
			//std::shared_ptr<X509> scert( cert, X509_free );
			std::shared_ptr<X509> scert( cert, do_clear );
			//[](){ DOUT("Clear X509"); } );
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
	//boost::filesystem::path tmpname = boost::filesystem::temp_directory_path() + boost::filesystem::unique_path();
	//std::string tmpname = boost::filesystem::temp_directory_path().string() +  boost::filesystem::path_separator() + boost::filesystem::unique_path().string();
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
	if ( _input != null_ptr && strlen(_input) > 0 )
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
	this->m_logfile.open("uniproxy.log");
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
	//this->clear();	// NB!!
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
	auto time = stdt::chrono::steady_clock::now(); 		// get the current time
	auto since_epoch = time.time_since_epoch(); 		// get the duration since epoch
	stdt::chrono::seconds secs = stdt::chrono::duration_cast<stdt::chrono::seconds>(since_epoch);
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
/*
{ uniproxy::error::success, "ok" },
{ uniproxy::error::certificate_invalid, "invalid certificate" },
{ uniproxy::error::certificate_name_invalid, "certificate name invalid" },
{ uniproxy::error::certificate_name_unknown, "certificate name unknown" },
{ uniproxy::error::certificate_names_mismatch, "certificate names mismatch" },
{ uniproxy::error::connection_name_unknown, "connection name unknown" },
{ uniproxy::error::connection_name_invalid, "connection name invalid" },
{ uniproxy::error::certificate_not_found, "certificate not found" },
{ uniproxy::error::url_unknown, "unknown URL" },
{ uniproxy::error::parse_file_failed, "parse file failed" },
{ uniproxy::error::logon_username_password_invalid, "username or password invalid" },
{ uniproxy::error::logon_account_blocked, "account blocked" },
{ uniproxy::error::logon_failed, "logon failed" },
{ uniproxy::error::logon_no_response, "no response" },
{ uniproxy::error::thread_already_running, "thread already running" },
{ uniproxy::error::socket_invalid, "invalid socket" },
{ uniproxy::error::unknown_error, NULL }
};
*/

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


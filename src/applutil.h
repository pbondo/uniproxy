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
#ifndef _applutil_h
#define _applutil_h

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <cppcms/json.h>
#include "platform.h"

#include <boost/date_time/posix_time/posix_time.hpp>	// ptime
#include <boost/thread/thread_time.hpp>					// get_system_time
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <fstream>
#include <chrono>
#include <atomic>

#define DOUT( xx ) { mylib::dout() << mylib::time_stamp() << " id:"  << std::this_thread::get_id() << " " << xx << std::endl; }
#define DERR( xx ) { mylib::derr() << mylib::time_stamp() << " "  << xx << std::endl; }
#define COUT( xx ) { std::cout << " "  << xx << std::endl; }

#define ASSERTD( xx, yy ) { if ( !(xx) ) throw std::runtime_error( yy ); };
#define ASSERTEC( xx, yy, zz ) { if ( !(xx) ) throw std::system_error( yy, std::string( zz ) ); };
#define ASSERTE( xx, yy, zz ) { if ( !(xx) ) throw std::system_error( make_error_code(yy), std::string( zz ) ); };

namespace uniproxy
{

//enum class error
namespace error
{

enum error_t
{
	#define error_code_def( xx, yy ) xx
	#include "error_codes.h"
	#undef error_code_def
};

} // namespace error

class category_impl : public std::error_category
{
public:
  virtual const char* name() const noexcept;
  virtual std::string message(int ev) const;
};

} // namespace application

std::error_code make_error_code(uniproxy::error::error_t e);
std::error_condition make_error_condition(uniproxy::error::error_t e);

namespace mylib
{
typedef unsigned short port_type;
	
std::ostream &dout();
std::ostream &derr();
std::string time_stamp();

void msleep(int millisec);

std::vector<std::string> split(const std::string &s, char delim);

template <class T> inline std::string to_string (const T& t)
{
	std::stringstream ss;
	ss << t;
	return ss.str();
}



template <class T> inline T& from_string (const std::string &sz, T& t)
{
	std::stringstream ss(sz);
	ss >> t;
	return t;
}

template <class T> class protect_pointer
{
public:

	protect_pointer( T * &_p, T&_ref, stdt::mutex &_mutex ) : m_p(_p), m_mutex( _mutex )
	{
		stdt::lock_guard<stdt::mutex> l(this->m_mutex);
		this->m_p = &_ref;
	}

	~protect_pointer()
	{
		stdt::lock_guard<stdt::mutex> l(this->m_mutex);
		this->m_p = nullptr;
	}

	T * &m_p;
	stdt::mutex &m_mutex;
};


// trim from start
static inline std::string &ltrim(std::string &s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
static inline std::string &rtrim(std::string &s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
static inline std::string &trim(std::string &s)
{
	return ltrim(rtrim(s));
}

// This exception is used for terminating a thread. 
// It is by design not inherited by std::exception because we don't want the threads to catch the exception.
class interrupt_exception { };

// This exception is designed for reloading an application and possibly also for threads.
class reload_exception : public interrupt_exception{ };


class thread
{
public:

	thread( std::function<void()> _interrupt_function ) // NULL is allowed
	{
		this->m_stop = false;
		if ( _interrupt_function != NULL )
		{
			m_interrupt_function = _interrupt_function;
		}
	}

	~thread()
	{
		if ( this->m_thread.joinable() )
		{
			this->m_thread.join();
		}
	}

	// Notice we cannot restart a thread even if it has terminated.
	// _conditional indicates that we should ignore threads already running.
	void start( std::function<void()> _thread_function )
	{
		ASSERTE( !this->m_thread.joinable(), uniproxy::error::thread_already_running, "" );
		this->m_stop = false;
		this->m_thread_function = _thread_function;
		stdt::thread t1( [&]{ try { this->m_thread_function(); } catch ( ... ) { } } );
		this->m_thread = std::move(t1);
	}

	void stop( bool _wait = true )
	{
		this->m_stop = true;
		// NB!! Check if we are self threading. then we should simply call check_run
		if ( this->m_interrupt_function != nullptr )
		{
			this->m_interrupt_function();
		}
		if ( _wait && this->m_thread.joinable() )
		{
			this->m_thread.join();
		}
	}

	// throw exception
	// return true to continue running
	bool check_run( bool _throw_exception = true )
	{
		if ( this->m_stop && _throw_exception )
		{
			throw interrupt_exception();
		}
		return !this->m_stop;
	}

	void sleep( int millisec )
	{
		const int SLEEP = 5000;
		while(millisec >= SLEEP)
		{
			this->check_run();
			msleep(SLEEP);
			millisec -= SLEEP;
		}
		msleep(millisec);
		this->check_run();
	}
	/*
	bool is_running()
	{
		bool result = false;
		#ifdef __linux__
		// With pthread we are ensured the thread handle is not reused while the thread is joinable.
		// According to the Internet :-)
		if ( this->m_thread.joinable() )
		{
			int ret = pthread_kill( this->m_thread.native_handle(), 0);
			if ( ret == ESRCH )
			{
				return false;
			}
			return true;
		}
		#else
			//NB!! #warning "thread::is_running is Not Implemented"
XX XX
		#endif
		return result;
	}
	*/
	static bool is_thread_running(stdt::thread &th);

	bool is_running()
	{
		return is_thread_running(this->m_thread);
	}

	stdt::thread  &operator() ()
	{
		return this->m_thread;
	}

	bool m_stop;
	std::function<void()> m_interrupt_function;
	stdt::thread m_thread;

	std::function<void()> m_thread_function;
};

}


namespace boost {
	
namespace filesystem {
const std::string path_separator();
}

namespace asio {

// _timeout [seconds]
void socket_set_keepalive_to( ip::tcp::socket::lowest_layer_type &_socket, std::chrono::seconds _timeout );

// Send the socket shutdown command
void socket_shutdown( ip::tcp::socket::lowest_layer_type &_socket, boost::system::error_code &ec );

// I dont understand why the above cannot be used?
void socket_shutdown( boost::asio::ip::tcp::acceptor &_acceptor, boost::system::error_code &ec );

// _host may be either a host name, or a IPv4 or IPv6 address
void sockect_connect( ip::tcp::socket::lowest_layer_type &_socket, boost::asio::io_service &_io_service, const std::string &_hostname, int _port );

// Check whether the socket is connected.
bool is_connected( ip::tcp::socket::lowest_layer_type &_socket );

bool get_certificate_issuer_subject( boost::asio::ssl::stream<boost::asio::ip::tcp::socket> &_socket, std::string &_issuer, std::string &_subject );

// This function will perform a read_some on a blocking socket. Notice it uses a thread, so it is slow.
// Notice this functionality is not supported by ASIO in itself.
template<typename MutableBufferSequence> int socket_read_some_for( boost::asio::ip::tcp::socket &_socket, const MutableBufferSequence & _buffers, const std::chronot::duration & _duration )
//int socket_read_some_for( boost::asio::ip::tcp::socket &_socket, char *_buffer, int _size, const std::chronot::duration & _duration )
{
	int length = 0;
//#ifdef _WIN32
#ifdef _MSC_VER
//	return _socket.read_some( _buffers );

	stdt::packaged_task<int> task4( [&]()->int{ return _socket.read_some( _buffers ); } );

//	boost::packaged_task<int()> task4( [&]()->int{ return _socket.read_some( boost::asio::buffer( _buffer, _size ) ); } );
	auto f4 = task4.get_future();
	stdt::thread t4( std::move(task4) );
	if ( f4.wait_for( _duration ) )
	{
		length = f4.get();
	}

#else
	return _socket.read_some( _buffers );
//	The following crash with g++ 4.5. Must check with g++ 4.6 or 4.7
/*
	std::packaged_task<int()> task4( [&]()->int{ return _socket.read_some( _buffers ); } );
	//std::packaged_task<int> task4( [&]()->int{ return _socket.read_some( _buffers ); } );
	//auto f4 = task4.get_future();
	std::thread t4( std::move(task4) );

	if ( f4.wait_for( _duration ) )
	{
		length = f4.get();
	}*/
#endif
	return length;
}

}
}

bool operator==( boost::asio::ip::tcp::socket &, boost::asio::ip::tcp::socket & );

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

#define TRY_CATCH( xxx ) try { xxx; } catch ( std::exception &exc ) { DOUT("Exception:" << exc.what()); }

const std::string my_public_cert_name = "my_public_cert.pem";
const std::string my_private_key_name = "my_private_key.pem";
const std::string my_certs_name = "certs.pem";
const std::string config_filename = "uniproxy.json";

class data_flow
{
public:

	data_flow( bool _debug = false );
	
	void add( size_t _count );
	
	size_t get();

	void cleanup( int64_t _stamp );
	
	#define data_flow_size 60
	
	// Modulate by 1 seconds.
	size_t m_buffer[data_flow_size];

	int64_t timestamp();

	int64_t m_stamp;

	bool m_debug;
};


class proxy_log
{
public:

	proxy_log( const std::string &_name );

	void add( const std::string &_value );

	void clear();
	
	std::string peek() const;

	std::string get(int _index) const;

	size_t count() const;

	std::ofstream m_logfile;

protected:

	std::vector<std::pair<int,std::string>> m_log;

	mutable stdt::mutex m_mutex;
	std::string m_name;
	std::atomic<int> m_write_index;
	int m_log_file_index;
};

proxy_log &log();



class LocalEndpoint
{
public:

	LocalEndpoint( const std::string &_hostname, const mylib::port_type _port )
	: m_hostname( _hostname ), m_port( _port )
	{
	}
	
	LocalEndpoint() : m_port(0)
	{}

	bool load(cppcms::json::value &obj);
	cppcms::json::value save() const;

	friend bool operator == (const LocalEndpoint &a1, const LocalEndpoint &a2);
	friend std::ostream &operator << (std::ostream &os, const LocalEndpoint &a);

	std::string m_hostname;
	mylib::port_type m_port;

};



// The configured data for each remote connection. Handles each of the "remotes" in the list below.
//	{	port : 8750, type : "GHP"
//		locals : [ { hostname : "127.0.0.1", port : 1234 }, { hostname : "127.0.0.1", port : 1235 } ], 
//		remotes : [ { name : "UK", name : "1.2.3.4", username : "uk", password : "cheers" }, { name : "Finland", username : "hytli", password : "sauna" } ]
//	}
class RemoteEndpoint
{
public:

	RemoteEndpoint() : m_port(0) //, m_active(true)
	{
	}

	mylib::port_type m_port;
	std::string m_name;
	std::string m_hostname;
	std::string m_username;
	std::string m_password;
	//bool m_active; // This is not stored.

	friend bool operator==( const RemoteEndpoint &ep1, const RemoteEndpoint &ep2 );

	bool load(cppcms::json::value &obj);
	cppcms::json::value save() const;

};


bool load_endpoints(const cppcms::json::value &obj, const std::string &key, std::vector<LocalEndpoint> &ep);
bool load_endpoints(const cppcms::json::value &obj, const std::string &key, std::vector<RemoteEndpoint> &ep);


/*
// Used for describing the remote proxy when defining local clients.
class ProxyEndpoint
{
public:

	ProxyEndpoint( //bool _active, 
		const std::string &_name, const std::string &_hostname, int _port )
	: //m_active(_active), 
		m_name(_name), m_hostname(_hostname), m_port(_port)
	{
	}
	
	ProxyEndpoint();

	friend bool operator == (const ProxyEndpoint &ep1, const ProxyEndpoint &ep2);

	//bool m_active = true;
	std::string m_name;
	std::string m_hostname;
	mylib::port_type m_port = 0;

	bool load(cppcms::json::value &obj);
	cppcms::json::value save() const;
	
};
*/

class Buffer
{
public:

	Buffer( void * _buffer, size_t _size ) : m_buffer(0), m_size(_size)
	{
		if ( this->m_size > 0 )
		{
			this->m_buffer = new char[this->m_size+1];
			memset( this->m_buffer, 0, this->m_size+1);		// Just add a 0 termination to enable string output
			memcpy( this->m_buffer, _buffer, this->m_size );
		}
	}

	~Buffer()
	{
		delete[] (char*)this->m_buffer;
	}

	void *m_buffer;
	size_t m_size;

};


class PluginHandler
{
public:

	std::string m_type;

	PluginHandler( const std::string & _type )
	{
		this->m_type = _type;
		plugins().push_back( this );
		std::cout << "PluginHandler: " << this->m_type << " " << plugins().size() << std::endl;
	}

	static std::vector<PluginHandler*> &plugins()
	{
		if ( m_plugins == NULL )
		{
			m_plugins = new std::vector<PluginHandler*>;
		}
		return *m_plugins;
	}

	virtual size_t max_buffer_size()
	{
		return 1000;
	}

	// Return false to indicate a connect or login failure to the host system.
	// Throw a std::runtime_error to indicate failure and log a user message that will propagate to the web page.
	virtual bool connect_handler( boost::asio::ip::tcp::socket &local_socket, RemoteEndpoint &_remote_ep )
	{
		return true;
	}

	// Used for streaming data from the local tcp server to the remote ssl client.
	// std::runtime_error may be thrown to indicate lost connection and log a user message that will propagate to the web page.
	virtual bool stream_local2remote( boost::asio::ip::tcp::socket &local_socket, boost::asio::ssl::stream<boost::asio::ip::tcp::socket> &remote_socket, mylib::thread &_thread )
	{
		return false;
	}

	// The full flag indicates that a lot of data is buffered up waiting for transmission to the remote site.
	// This typically means that the remote is slow and/or congested.
	// If the full flag is set, then the function buffer the data and return false.
	// Once the full flag is cleared again we should start sending data again.
	virtual bool message_filter_local2remote( Buffer &_buffer ) //, bool _full )
	{
		return true;
	}

	virtual bool message_filter_remote2local( Buffer &_buffer ) //, bool _full )
	{
		return true;
	}

private:

	// If this was not defined as a * then it may be constructed at the wrong type. The basic = 0 seems to always work.
	static std::vector<PluginHandler*> *m_plugins;

};


//int check_int( cppcms::json::value &_input_obj, const std::string &_inputname, const int _default_value, bool _required );
//std::string check_string( cppcms::json::value &_input_obj, const std::string &_inputname, const std::string & _default_value, bool _required );
int check_int( const char *_input);
std::string check_ip4( const std::string &_input );


// Certificate handling
//
typedef std::shared_ptr<X509> certificate_type;

// Load an array of certificates. Append to the supplied vector.
// Returns true if the filename was valid and could be read, but does not indicate count read.
bool load_certificates_string( const std::string &_certificate_string, std::vector< certificate_type > &_certs );
bool load_certificates_file( const std::string &_filename, std::vector< certificate_type > &_certs );
bool save_certificates_file( const std::string &_filename, const std::vector<certificate_type> &_certs );
bool delete_certificate_file( const std::string &_filename, const std::string &_name );

std::string get_subject_name( const certificate_type &_cert );
std::string get_issuer_name( const certificate_type &_cert );
std::string get_common_name( const certificate_type &_cert );

// Read a text (NOT binary file).
std::string readfile( const std::string &_filename );

bool check_arg(int argc, char *argv[], char _short_argument, const char *_long_argument);
bool check_arg( int argc, char *argv[], char _short_argument, const char *_long_argument, std::string &result );

namespace std2
{

#ifdef _WIN32
template<class T, class U> std::unique_ptr<T> make_unique(U&& u)
{
	 return std::unique_ptr<T>(new T(std::forward<U>(u)));
}
#else
template<typename T, typename... Args> std::unique_ptr<T> make_unique(Args&&... args)
{
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#endif

} // namespace std2


class process
{
public:

static int execute_process( const std::string& _command, const std::string& _param = "", std::function<void(const std::string&)> _out = nullptr, std::function<void(const std::string&)> _err = nullptr);

}; // class


#endif


#include "baseclient.h"

static int static_local_id = 0;


BaseClient::BaseClient(//bool _active, const std::vector<ProxyEndpoint> &_proxy_endpoints, const int _max_connections)
		bool _active, //const std::string &_name, 
			int _local_port, const std::vector<ProxyEndpoint> &_proxy_endpoints, const int _max_connections, PluginHandler &_plugin )	
:
	m_proxy_index(0),
	m_active(_active),
	mp_remote_socket( nullptr ),
	m_count_in(true), m_count_out(true),
	m_local_port(_local_port),
	m_max_connections(_max_connections),
	m_thread( [this]{ this->interrupt(); } ),
	m_plugin(_plugin)
	
/*
	data_flow m_count_in, m_count_out;
	boost::posix_time::ptime m_activate_stamp;
	int m_local_port;
	int m_id;
	int m_max_connections;
	std::string m_log;
*/	
{
	this->m_proxy_endpoints = _proxy_endpoints;
	this->m_activate_stamp = boost::get_system_time();
	this->m_id = ++static_local_id;
}


ssl_socket &BaseClient::remote_socket()
{
	ASSERTE(this->mp_remote_socket, uniproxy::error::socket_invalid, "");
	return *this->mp_remote_socket;
}


void BaseClient::dolog( const std::string &_line )
{
	this->m_log = _line;
	log().add( " hostname: " + this->remote_hostname() + " port: " + mylib::to_string(this->remote_port()) + ": " + _line );
}


const std::string BaseClient::dolog()
{
	return this->m_log;
}


int BaseClient::remote_port() const
{
	return this->m_proxy_endpoints[this->m_proxy_index].m_port;
}


std::string BaseClient::remote_hostname() const
{
	return this->m_proxy_endpoints[this->m_proxy_index].m_hostname;
}


/*
bool BaseClient::is_remote_connected() const
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex);
	return this->mp_remote_socket != nullptr && is_connected(this->mp_remote_socket->lowest_layer());
}



bool BaseClient::remote_hostname( int index, std::string &result )
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex);
	if ( index < this->m_local_sockets.size() )
	{
		result.clear();
		boost::asio::ip::tcp::socket &socket(this->m_local_sockets[index]->socket());
		boost::system::error_code ec;
		std::string sz = socket.remote_endpoint(ec).address().to_string(); // This one should not throw exceptions
		if ( !ec ) result = sz;
		return true;
	}
	return false;
}
*/

bool BaseClient::is_remote_connected(int index) const
{
	stdt::lock_guard<stdt::mutex> l(this->m_mutex);
	return this->mp_remote_socket != nullptr && this->mp_remote_socket->lowest_layer().is_open() && //this->m_remote_connected && 
		is_connected(this->mp_remote_socket->lowest_layer())
		&& (this->m_proxy_index == index || index == -1);
}


std::string BaseClient::get_password() const
{
	return "1234";
}



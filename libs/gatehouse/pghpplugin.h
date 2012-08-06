//====================================================================
//
// Universal Proxy
//
// GateHouse Library for handling PGHP NMEA messages
//--------------------------------------------------------------------
//
// This version is released as part of the European Union sponsored
// project Mona Lisa work package 4 for the Universal Proxy Application
//
// This version is released under the GPL version 3 open source License:
// http://www.gnu.org/copyleft/gpl.html
//
// Copyright (C) 2004-2012 by GateHouse A/S
// All Rights Reserved.
// http://www.gatehouse.dk
// mailto:gh@gatehouse.dk
//====================================================================
#ifndef _pghpplugin_h
#define _pghpplugin_h

#include <applutil.h>
//#include <aisparser/aiswrap.h>

class PGHPFilter : public PluginHandler
{
public:

	class track
	{
	public:

		track( unsigned int _mmsi );

		std::string json_positions( const boost::posix_time::ptime &_from, const boost::posix_time::ptime &_to  );

		//std::vector<ais_msg> m_positions;		// AIS position reports, sorted with newest first
		//std::vector<ais_msg> m_static_voyage;	// AIS voyage reports, sorted with newest first

		unsigned int m_mmsi;
	};

	PGHPFilter() : PluginHandler( "GHP" ) {}

	//virtual size_t max_buffer_size();

	virtual bool connect_handler( boost::asio::ip::tcp::socket &local_socket, RemoteEndpoint &_remote_ep );
	//virtual bool stream_local2remote( boost::asio::ip::tcp::socket &local_socket, ssl_socket &remote_socket, mylib::thread &_thread );


	//bool handle_message( const ais_msg & );
	//bool message_filter_local2remote( Buffer &_buffer ); //, bool _full );

	bool SendPGHP2Mail( boost::asio::ip::tcp::socket &local_socket, const std::string &_mail );

	bool Decode( const std::string & _input, std::string &_output );

	bool SendLogonRequest( boost::asio::ip::tcp::socket &local_socket, RemoteEndpoint &_remote_ep );
	bool SendStartMesg( boost::asio::ip::tcp::socket &local_socket );

	std::vector<track> m_tracks;

};

#endif

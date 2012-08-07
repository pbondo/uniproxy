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
// This version is released under the GNU General Public License with restrictions.
// See the doc/license.txt file.
//
// Copyright (C) 2004-2012 by GateHouse A/S
// All Rights Reserved.
// http://www.gatehouse.dk
// mailto:gh@gatehouse.dk
//====================================================================
#ifndef _pghpplugin_h
#define _pghpplugin_h

#include <applutil.h>

class PGHPFilter : public PluginHandler
{
public:

	class track
	{
	public:

		track( unsigned int _mmsi );

		std::string json_positions( const boost::posix_time::ptime &_from, const boost::posix_time::ptime &_to  );

		unsigned int m_mmsi;
	};

	PGHPFilter() : PluginHandler( "GHP" ) {}

	virtual bool connect_handler( boost::asio::ip::tcp::socket &local_socket, RemoteEndpoint &_remote_ep );

	bool SendPGHP2Mail( boost::asio::ip::tcp::socket &local_socket, const std::string &_mail );

	bool Decode( const std::string & _input, std::string &_output );

	bool SendLogonRequest( boost::asio::ip::tcp::socket &local_socket, RemoteEndpoint &_remote_ep );
	bool SendStartMesg( boost::asio::ip::tcp::socket &local_socket );

	std::vector<track> m_tracks;

};

#endif

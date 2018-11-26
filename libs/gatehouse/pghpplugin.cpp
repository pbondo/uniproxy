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
#include <gatehouse/pghpplugin.h>

#include <gatehouse/pghplogonrequest.h>
#include <gatehouse/pghplogonreply.h>
#include <gatehouse/pghp2.h>
#include <gatehouse/pghpstartdatarequest.h>

#include <boost/regex.hpp>

using namespace std;
using boost::asio::ip::tcp;

// From release.cpp
extern const char * release;
extern const char * version;

PGHPFilter m_PGHPFilter;


PGHPFilter::track::track( unsigned int _mmsi )
{
	this->m_mmsi = _mmsi;
}


bool PGHPFilter::Decode( const std::string & _input, std::string &_output )
{
	boost::regex regex_pghp2;
	boost::cmatch matches;
	// "$PGHP,2,1,1,,"
	// 2011-05-23_14-00-00.sql == "(\\d{4})-(\\d{1,2})-(\\d{1,2})_(\\d{1,2})-(\\d{1,2})-(\\d{1,2}).*";
	//regex_pghp2 = "\\$PGHP,2,1,1,,(\\d{1,40}).*";
	regex_pghp2 = "\\$PGHP,2,1,1,,([0-9a-fA-F]*).*";
	if ( _input.length() > 0 && boost::regex_match(_input.c_str(), matches, regex_pghp2 ) )
	{
		_output = std::string(matches[1].first, matches[1].second);
		if ( _output.length() > 0 )
		{
			return true;
		}
	}
	return false;
}


bool PGHPFilter::SendPGHP2Mail( boost::asio::ip::tcp::socket &local_socket, const std::string &_mail )
{
	TclPGHP2Message clPGH2;
	clPGH2.SetGHMail(_mail);
	std::string clEncoded;
	clPGH2.Encode(clEncoded);
	int length = local_socket.write_some( boost::asio::buffer( clEncoded, clEncoded.length() ) );
	return length == clEncoded.length();
}



bool PGHPFilter::SendStartMesg( boost::asio::ip::tcp::socket &local_socket )
{
	std::string clRes;
	TclAISMessageStartDataRequest pclReq;
	pclReq.Encode( clRes );

	return this->SendPGHP2Mail( local_socket, clRes );
}


bool PGHPFilter::SendLogonRequest( boost::asio::ip::tcp::socket &local_socket, RemoteEndpoint &_remote_ep )
{
	TclAISMessageLogonRequest clReq;
	clReq.SetName(  _remote_ep.m_username );
	clReq.SetPassword( _remote_ep.m_password );
	clReq.SetVersion(LOGON_VERSION_3);
	clReq.SetVersionString(string(release) + string(" - ") + string(version) + string(" - ") + string("123"));
	string clLoggedOnUser = "?";
	clReq.SetLocalUser(clLoggedOnUser);
	clReq.SetProxyType(SUBSCRIBER_PROXY);
	clReq.SetRetryCounter( 2 ); //GetMoveCount() );

	string clResult;
	clReq.Encode(clResult);
	
	return this->SendPGHP2Mail( local_socket, clResult );
}


// Upon connection after remote SSL authorise and local TCP connect. We are now connected to the local socket.
// For the GH system we need to send a logon request and a startdata request.
//
// Notice we are in a "safe" thread so we can wait for reply here.
bool PGHPFilter::connect_handler( boost::asio::ip::tcp::socket &local_socket, RemoteEndpoint &_remote_ep )
{
	if ( ! this->SendLogonRequest( local_socket, _remote_ep ) )
	{
		throw std::system_error( make_error_code( uniproxy::error::logon_failed ), "logon request" );
	}

	for ( ;; )
	{
		char buffer[201];
		int length = 0;
		length = boost::asio::socket_read_some_for( local_socket, boost::asio::buffer( buffer, 200), boost::posix_time::seconds(10) );
		if ( length == 0 )
		{
			throw std::system_error( make_error_code( uniproxy::error::logon_no_response ) );
		}
		buffer[length] = 0;
		DOUT("Read: " << length << "-" << buffer << "-");
		std::istringstream is( buffer );
		while( is )
		{
			string line, msg;
			std::getline( is, line );
			DOUT("Line: " << line );
			line += "\r\n";
			if ( this->Decode( line, msg ) )
			{
				TclAISMessageInternalBase clAISMessageData;
				clAISMessageData.Decode(msg);
				DOUT( "Decode type: " <<  clAISMessageData.GetType() );
				DOUT("Msg: " << msg );
				TclPGHP2Message pghp2;
				if(pghp2.Decode(line))
				{
					string mail = pghp2.GetGHMail();
					DOUT( "GH:" << mail );
					TclAISMessageInternalBase clAISMessageData1;
					if ( clAISMessageData1.Decode(mail) )
					{
						DOUT( "decoded Type: " << clAISMessageData1.GetType() );
						switch( clAISMessageData1.GetType() )
						{
						case AISMESGINT_LOGON_REPLY:  //Logged On
							{
								TclAISMessageLogonReply reply;
								reply.Decode( mail );
								DOUT("Got reply: " << reply.GetLogonReply() );
                        std::string info = " certificate name: " + _remote_ep.m_name + " user: " + _remote_ep.m_username + " ";
								switch( reply.GetLogonReply() )
								{
								case LOGON_REPLY_OK:
									DOUT("Logon Reply OK");
									SendStartMesg( local_socket );
									return true;
								case LOGON_REPLY_USER_INVALID:
									throw std::system_error(make_error_code(uniproxy::error::logon_username_password_invalid), info + "Username invalid");
								case LOGON_REPLY_PASSWORD_INVALID:
									throw std::system_error(make_error_code(uniproxy::error::logon_username_password_invalid), info + "Password invalid");
                        case LOGON_REPLY_NO_LSS_GROUP:
									throw std::system_error(make_error_code(uniproxy::error::logon_username_password_invalid), info + "No LSS Group Assigned");
                        case LOGON_REPLY_ALLREADY_CONNECTED:
									throw std::system_error(make_error_code(uniproxy::error::logon_username_password_invalid), info + "Already connected");
                        case LOGON_REPLY_PASSWORD_EXPIRED:
									throw std::system_error(make_error_code(uniproxy::error::logon_username_password_invalid), info + "User or group expired");
                        case LOGON_REPLY_LOCKED:
									throw std::system_error(make_error_code(uniproxy::error::logon_username_password_invalid), info + "User or group locked");
                        case LOGON_REPLY_DISABLED:
									throw std::system_error(make_error_code(uniproxy::error::logon_username_password_invalid), info + "User or group disabled");
                        case LOGON_REPLY_LOAD_BALANCE_MOVE:
									throw std::system_error(make_error_code(uniproxy::error::logon_username_password_invalid), info + "Load balancer problem");
                        case LOGON_REPLY_SERVER_BUSY:
									throw std::system_error(make_error_code(uniproxy::error::logon_username_password_invalid), info + "Server Busy");
								default:
									throw std::system_error(make_error_code( uniproxy::error::logon_failed ), OSS(info + "Unknown response: " << reply.GetLogonReply()));
								}
							}
							break;
						default:
							break;
						}
					}
				}
			}
		}
	}
	return false;
}


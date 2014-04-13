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
#ifndef _cppcms_util_h
#define _cppcms_util_h

#include <cppcms/json.h>

namespace cppcms {

class utils
{
	public:


static bool check_int( cppcms::json::value &_input_obj, const std::string &_inputname, int &result )
{
	if ( _inputname.length() > 0 )
	{
		cppcms::json::value value = _input_obj.find( _inputname );
		int type1 = value.type();
		DOUT("Type: " << type1 );
		if ( value.type() == cppcms::json::is_number )
		{
			result = value.get_value<int>( );
			return true;
		}
		else if ( value.type() == cppcms::json::is_string )
		{
			std::string sz = value.get_value<std::string>( );
			mylib::from_string( sz, result );
			return true;
		}
	}
	return false;
}


static bool check_port( cppcms::json::value &_input_obj, const std::string &_inputname, unsigned short &result )
{
	int value;
	if (check_int(_input_obj,_inputname,value))
	{
		result = value;
		return true;
	}
	return false;
}

static int check_int( cppcms::json::value &_input_obj, const std::string &_inputname, const int _default_value, bool _required )
{
	int result = _default_value;
	if ( _inputname.length() > 0 )
	{
		cppcms::json::value value = _input_obj.find( _inputname );
		int type1 = value.type();
		DOUT("Type: " << type1 );
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

static bool check_bool( cppcms::json::value &_input_obj, const std::string &_inputname, bool &result )
{
	if ( _inputname.length() > 0 )
	{
		cppcms::json::value value = _input_obj.find( _inputname );
		if ( value.type() == cppcms::json::is_number )
		{
			result = value.get_value<int>( ) > 0;	// I.e. 1,2,3,4,5.... => true
			return true;
		}
		else if ( value.type() == cppcms::json::is_string )
		{
			std::string sz = value.get_value<std::string>( );
			result = sz == "true" || sz == "on";
			return true;
		}
		if ( value.type() == cppcms::json::is_boolean )
		{
			result = value.get_value<bool>( ) ;	// I.e. 1,2,3,4,5.... => true
			return true;
		}
	}
	return false;
}



static bool check_bool( cppcms::json::value &_input_obj, const std::string &_inputname, const bool _default_value, bool _required )
{
	bool result = _default_value;
	bool found = false;
	if ( _inputname.length() > 0 )
	{
		found = true;
		cppcms::json::value value = _input_obj.find( _inputname );
		if ( value.type() == cppcms::json::is_boolean )
		{
			result = value.get_value<bool>( );	// I.e. true
		}
		else if ( value.type() == cppcms::json::is_number )
		{
			result = value.get_value<int>( ) > 0;	// I.e. 1,2,3,4,5.... => true
		}
		else if ( value.type() == cppcms::json::is_string )
		{
			std::string sz = value.get_value<std::string>( );
			result = sz == "true" || sz == "on";
		}
		else
		{
			found = false;
		}
	}
	if ( _required && !found)
	{
		// NB!! Give a better description and location of the problem. And how do we show this to the user ?
		throw std::runtime_error("Failed to validate JSON input for bool value " + _inputname + " which is required");
	}
	return result;
}

static bool check_string( cppcms::json::value &_input_obj, const std::string &_inputname, std::string & result )
{
	if ( _inputname.length() > 0 )
	{
		cppcms::json::value value = _input_obj.find( _inputname );
		if ( value.type() == cppcms::json::is_string )
		{
			result = value.get_value<std::string>( );
			return true;
		}
	}
	return false;
}


static bool check_date( cppcms::json::value &_input_obj, const std::string &_inputname, std::string & result )
{
	return check_string( _input_obj, _inputname, result );
}


static std::string check_string( cppcms::json::value &_input_obj, const std::string &_inputname, const std::string & _default_value, bool _required )
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

};
} // namespaces

#endif

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
#ifndef _pghpxmlutil_h
#define _pghpxmlutil_h

#include <iterator>
#include <ostream>
#include <vector>
#include <sstream>

#include <boost/tokenizer.hpp>
#include <boost/tokenizer.hpp>

struct TclCoord;

/// Convert a vector into a semicolon-separated list.
/// \c operator<< must be defined for type \arg T.
template <typename T> void CreateVectorValue(std::string& _rszValue, const std::vector<T>& _rclValue);

/// Convert a semicolon-separated list into a vector.
/// Type \arg T must have a SetData() function.
template <typename T> void SplitVectorValue(const std::string& _rszValue, std::vector<T>& _rclValue);

// Specialization for string
template <> void SplitVectorValue(const std::string& _rszValue, std::vector<std::string>& _rclValue);

// Specialization for int
template <> void SplitVectorValue(const std::string& _rszValue, std::vector<int>& _rclValue);

// Specialization for bool
template <> void SplitVectorValue(const std::string& _rszValue, std::vector<bool>& _rclValue);

// Specialization for float
template <> void SplitVectorValue(const std::string& _rszValue, std::vector<float>& _rclValue);

// Specialization for double
template <> void SplitVectorValue(const std::string& _rszValue, std::vector<double>& _rclValue);

template <class T> void Tokenize(const std::string& _string, std::vector<T>& _tokens, const std::string& _delimiters);

// Replace reserved XML chars (Quote, Ampersand) with XML entities.
std::string XmlEncode(const std::string& _rszUncodedString);

// Replace HTML entities ("&gt;" and "&lt;") with (">" and "<").
std::string HtmlDecode(const std::string& _encodedString);

/// Wraps XML tags  
struct XmlTag
{
   static const char* LatitudeTag;
   static const char* LongitudeTag;

   XmlTag(const char* tagName, std::ostream& ostr);

   template <typename T>
   void value(const T& val)
   {
      m_ostream << val;
   }

   ~XmlTag();

   const char*       m_tagName;
   std::ostream&     m_ostream;
};

/// Helper for creating scopes
template <typename T> void AddXmlValue(const char* tagName, std::ostream& ostr, const T& val)
{
   XmlTag(tagName, ostr).value(val);
}


// Implementation of template functions

template <class T> void CreateVectorValue(std::string& _rszValue, const std::vector<T>& _rclValue)
{
   std::ostringstream ss;
   ss.precision(10);

   std::copy(_rclValue.begin(), _rclValue.end(), std::ostream_iterator<T>(ss, ";"));
   _rszValue = ss.str();
}


template <class T> void SplitVectorValue(const std::string& _rszValue, std::vector<T>& _rclValue)
{
	// Use the fast tokenizer to split into strings
	std::vector<std::string> s;
	Tokenize(_rszValue, s, ";");
	// Then convert each string to a T
	for (std::vector<std::string>::iterator it = s.begin(); it != s.end(); ++it)
	{
		T value;
		value.SetData(*it);
		_rclValue.push_back(value);
	}
}


template<class T> void Tokenize(const std::string& _string, std::vector<T>& _tokens, const std::string& _delimiters)
{
	typedef boost::tokenizer< boost::char_separator<char> > CharTokenizer;
	boost::char_separator<char> separator(_delimiters.c_str(), "", boost::keep_empty_tokens);
	CharTokenizer tokenizer(_string, separator);
	bool lastEmpty = false;
	for (CharTokenizer::iterator tokenIter = tokenizer.begin(); tokenIter != tokenizer.end(); ++tokenIter)
	{
		const std::string& item = *tokenIter;
		lastEmpty = item.empty();
		_tokens.push_back(item);
	}
	if (lastEmpty)
	{
		// Remove last empty item
		_tokens.pop_back();
	}
}

#endif // _xmlutil_h

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
#include "pghpxmlutil.h"

#include <boost/lexical_cast.hpp>

using namespace std;


template <> void SplitVectorValue(const std::string& _rszValue, std::vector<std::string>& _rclValue)
{
   Tokenize(_rszValue, _rclValue, ";");
}


template <> void SplitVectorValue(const std::string& _string, std::vector<int>& _tokens)
{
   typedef boost::tokenizer< boost::char_separator<char> > CharTokenizer;
   boost::char_separator<char> separator(";", "", boost::keep_empty_tokens);
   CharTokenizer tokenizer(_string, separator);
   bool lastEmpty = false;
   for (CharTokenizer::iterator tokenIter = tokenizer.begin(); tokenIter != tokenizer.end(); ++tokenIter)
   {
		string item(*tokenIter);
		lastEmpty = item.empty();
		int iValue;
		iValue = boost::lexical_cast<int>( item );
		_tokens.push_back(iValue);
   }
   if (lastEmpty)
   {
		// Remove last empty item
		_tokens.pop_back();
   }
}


template <> void SplitVectorValue(const std::string& _string, std::vector<bool>& _tokens)
{
   typedef boost::tokenizer< boost::char_separator<char> > CharTokenizer;
   boost::char_separator<char> separator(";", "", boost::keep_empty_tokens);
   CharTokenizer tokenizer(_string, separator);
   bool lastEmpty = false;
   for (CharTokenizer::iterator tokenIter = tokenizer.begin(); tokenIter != tokenizer.end(); ++tokenIter)
   {
		string item(*tokenIter);
		lastEmpty = item.empty();
		int iValue;
		iValue = boost::lexical_cast<int>( item );

		_tokens.push_back(iValue != 0);
   }
   if (lastEmpty)
   {
      // Remove last empty item
      _tokens.pop_back();
   }
}


template <> void SplitVectorValue(const std::string& _string, std::vector<float>& _tokens)
{
   typedef boost::tokenizer< boost::char_separator<char> > CharTokenizer;
   boost::char_separator<char> separator(";", "", boost::keep_empty_tokens);
   CharTokenizer tokenizer(_string, separator);
   bool lastEmpty = false;
   for (CharTokenizer::iterator tokenIter = tokenizer.begin(); tokenIter != tokenizer.end(); ++tokenIter)
   {
		string item(*tokenIter);
		lastEmpty = item.empty();
		float fl;
		fl = boost::lexical_cast<float>( item );
		_tokens.push_back(fl);
   }
   if (lastEmpty)
   {
      // Remove last empty item
      _tokens.pop_back();
   }
}


template <> void SplitVectorValue(const std::string& _string, std::vector<double>& _tokens)
{
   typedef boost::tokenizer< boost::char_separator<char> > CharTokenizer;
   boost::char_separator<char> separator(";", "", boost::keep_empty_tokens);
   CharTokenizer tokenizer(_string, separator);
   bool lastEmpty = false;
   for (CharTokenizer::iterator tokenIter = tokenizer.begin();
        tokenIter != tokenizer.end();
        ++tokenIter)
   {
      string item(*tokenIter);
      lastEmpty = item.empty();
      double fl;
      //item.AsFloat(fl);
	  fl = boost::lexical_cast<float>( item );
      _tokens.push_back(fl);
   }
   if (lastEmpty)
   {
      // Remove last empty item
      _tokens.pop_back();
   }
}

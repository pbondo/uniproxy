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
#ifndef _platform_h
#define _platform_h

#ifdef __GNUC__

	// From c++0x
	#ifndef _GLIBCXX_USE_NANOSLEEP
	#define _GLIBCXX_USE_NANOSLEEP
	#endif


#else

	#define noexcept

#endif



#ifdef __use_boost_thread__

#include <boost/chrono.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>	// ptime

namespace stdt = boost;

// The following should probably be a template ?
#define wait_for timed_wait

namespace std
{
namespace chronot
{
	typedef boost::posix_time::time_duration duration;
	typedef boost::posix_time::seconds seconds;
}

namespace this_thread
{
	static unsigned int get_id() { return GetCurrentThreadId(); }
}

}

#else	// using std threads

#include <chrono>
#include <thread>
#include <mutex>
#include <future>
namespace stdt = std;

namespace std
{
namespace chronot
{
	typedef std::chrono::duration<double> duration;
	typedef std::chrono::seconds seconds;
}
}

#endif

#ifdef __linux__

	// The following will possibly later be incorporated into the C++ standard.
	template<typename T, typename ...A1> std::unique_ptr<T> make_unique(A1&& ...arg1)
	{
	  return std::unique_ptr<T>(new T(std::forward<A1>(arg1)... ));
	}

#else

	#pragma warning( disable : 4996 )

#endif


#endif

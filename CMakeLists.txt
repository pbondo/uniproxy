#
# cmake configuration file
#
# c:\lang\cmake\bin\cmake -G "Visual Studio 10" .
#
cmake_minimum_required(VERSION 2.4)

project(uniproxy)

include_directories(. ../libs/ ../src/ src/ libs/)

# This command assemble the cppcms templates
add_custom_command(
  OUTPUT views.cpp
  COMMAND cppcms_tmpl_cc.py ../webserver/master.tmpl ../webserver/view_config.tmpl ../webserver/view_status.tmpl -o views.cpp
  DEPENDS webserver/master.tmpl webserver/view_config.tmpl webserver/view_status.tmpl
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Precompiling views"
  VERBATIM
  )

#  COMMAND cppcms_tmpl_cc.py webserver/master.tmpl webserver/view_config.tmpl webserver/view_status.tmpl -o views.cpp


set( uniproxy_cpp_SOURCES
	../src/main.cpp
	../src/localclient.cpp
	../src/remoteclient.cpp
	../src/applutil.cpp
	../src/proxy_global.cpp
	../libs/gatehouse/pghpplugin.cpp
	../libs/gatehouse/pghpstartdatarequest.cpp
	../libs/gatehouse/pghpproxyfilter.cpp
	../libs/gatehouse/pghpxmlutil.cpp
	../libs/gatehouse/pghputils.cpp
	../libs/gatehouse/pghpinternalbase.cpp
	../libs/gatehouse/pghpbase.cpp
	../libs/gatehouse/pghplogonreply.cpp
	../libs/gatehouse/pghplogoffrequest.cpp
	../libs/gatehouse/pghplogonrequest.cpp
	../libs/gatehouse/pghpnmeamsg.cpp
	../libs/gatehouse/pghp2.cpp
)

set( uniproxy_SOURCES
	${uniproxy_cpp_SOURCES}
	views.cpp
)

set( uniproxy_SOURCES
	${uniproxy_SOURCES}
	README.md
	../src/applutil.h
	../src/cppcms_util.h
	../src/localclient.h
	../src/main.h
	../src/proxy_global.h
	../src/platform.h
	../src/remoteclient.h
	../src/error_codes.h
)

IF (WIN32)

	# VC10
	ADD_DEFINITIONS( -D_WIN32_WINNT=0x0501 /wd4250 /wd4018 )
	include_directories (${uniproxy_SOURCE_DIR} c:/lang/include c:/lang/w32/include c:/lang/boost_1_47 c:/lang/openssl/include/)
	LINK_DIRECTORIES( "c:/lang/boost_1_47/lib" "c:/lang/w32/lib" "c:/lang/openssl/lib/VC/static/" )

	# Mingw64
	#ADD_DEFINITIONS( -std=c++11 )
	#include_directories (${uniproxy_SOURCE_DIR} c:/lang/include c:/lang/mw64/include c:/lang/mw64/include/boost-1_49  c:/lang/OpenSSL-1.0.0a-bin-x64/x86_64-w64-mingw32/include )
	#LINK_DIRECTORIES( c:/lang/OpenSSL-1.0.0a-bin-x64/x86_64-w64-mingw32/lib c:/lang/mingw/x86_64-w64-mingw32/lib c:c:/lang/mingw/lib "c:/lang/mw64/lib"  )

ELSE (WIN32)

	ADD_DEFINITIONS( -g -std=c++11 -Wall -Wno-sign-compare )

	include_directories ( /usr/local/include/boost-1.49.0/ )
	LINK_DIRECTORIES( /usr/local/lib/boost-1.49.0/ )

ENDIF (WIN32)

add_executable(uniproxy ${uniproxy_SOURCES})

IF (WIN32)
	#VC
	target_link_libraries (uniproxy libcppcms-d libbooster-d SSLeay32MDd libeay32MDd zlib pcre dbghelp )

	# Mingw the order of libraries are important
	#target_link_libraries (uniproxy  libcppcms libbooster pcre boost_system-mgw47-mt-1_49 boost_regex-mgw47-mt-1_49 boost_filesystem-mgw47-mt-1_49 ssleay32 crypto ws2_32 wsock32 mswsock zlib m)

ELSE (WIN32)

	target_link_libraries(uniproxy cppcms.a booster.a boost_filesystem-mt.a boost_system-mt.a boost_regex-mt.a pthread pcre icuuc icui18n icudata gcrypt dl z ssl.a crypto.a)
	##target_link_libraries(uniproxy cppcms.a booster.a boost_filesystem-mt.a boost_system-mt.a boost_regex-mt.a pthread.a pcre.a icuuc.a icui18n.a icudata.a gcrypt.a dl z.a ssl.a crypto.a libc.a)

	INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/)
	INSTALL(DIRECTORY DESTINATION /var/lib/uniproxy)
	INSTALL(DIRECTORY DESTINATION /var/lib/uniproxy/script)
	INSTALL(FILES script/jquery.js DESTINATION /var/lib/uniproxy/script/)
	INSTALL(FILES deb/uniproxy.conf DESTINATION /etc/init/ )


ENDIF (WIN32)


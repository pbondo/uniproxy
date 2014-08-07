#
# cmake configuration file
#
#
cmake_minimum_required(VERSION 2.4)

project(uniproxy)

IF (WIN32)
	SET(VC 11)
	SET(LANG c:/lang)
	SET(BOOST d:/lang/boost_1_55_0)
ENDIF()


include_directories(. src/ libs/ ${HOME}/lang/third/include/)

# This command assemble the cppcms templates
IF (WIN32)


add_custom_command(
  OUTPUT views.cpp
  COMMAND python "${LANG}/w64-vc${VC}/bin/cppcms_tmpl_cc" ../webserver/master.tmpl ../webserver/view_config.tmpl ../webserver/view_status.tmpl -o views.cpp
  #COMMAND cppcms_tmpl_cc.py ../webserver/master.tmpl ../webserver/view_config.tmpl ../webserver/view_status.tmpl -o views.cpp
  DEPENDS webserver/master.tmpl webserver/view_config.tmpl webserver/view_status.tmpl
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Precompiling views"
  VERBATIM
  )
ELSE (WIN32)
add_custom_command(
  OUTPUT views.cpp
  COMMAND cppcms_tmpl_cc.py ../webserver/master.tmpl ../webserver/view_config.tmpl ../webserver/view_status.tmpl -o views.cpp
  DEPENDS webserver/master.tmpl webserver/view_config.tmpl webserver/view_status.tmpl
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Precompiling views"
  VERBATIM
  )
ENDIF (WIN32)


set( uniproxy_cpp_SOURCES
	../src/main.cpp
	../src/proxy_global.cpp
	../src/baseclient.cpp
	../src/localclient.cpp
	../src/remoteclient.cpp
	../src/providerclient.cpp
	../src/applutil.cpp
	../src/httpclient.cpp
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
	README.md
	../doc/uniproxy.json.sample
	views.cpp
	../webserver/content.h
	../webserver/master.tmpl
	../webserver/view_config.tmpl
	../webserver/view_status.tmpl
)

set( uniproxy_SOURCES
	${uniproxy_SOURCES}
	../src/applutil.h
	../src/baseclient.h
	../src/cppcms_util.h
	../src/localclient.h
	../src/providerclient.h
	../src/main.h
	../src/proxy_global.h
	../src/platform.h
	../src/remoteclient.h
	../src/error_codes.h
	../src/httpclient.h
)

IF (WIN32)
	set( uniproxy_SOURCES
		${uniproxy_SOURCES}
		../src/win_util.cpp
		../src/win_util.h
		)


	# VS2012
	ADD_DEFINITIONS( -D_WIN32_WINNT=0x0501 /wd4250 /wd4018  /wd4267)
	include_directories (${uniproxy_SOURCE_DIR} ${LANG}/w64-vc${VC}/include ${LANG}/third/include ${BOOST} ${LANG}/openssl-win64/include/  c:/lang/w64-vc${VC}/third/include/ )
	LINK_DIRECTORIES( "${LANG}/w64-vc${VC}/lib" "${LANG}/openssl-win64/lib/VC/static/" ${BOOST}/libs ${BOOST}/lib64-msvc-${VC}.0)
	# Use multi cores
	SET(VISUAL_STUDIO_MP_FLAG "/MP")

	# Mingw64
	#ADD_DEFINITIONS( -std=c++11 )
	#include_directories (${uniproxy_SOURCE_DIR} ${LANG}/include ${LANG}/mw64/include ${LANG}/mw64/include/boost-1_49  ${LANG}/OpenSSL-1.0.0a-bin-x64/x86_64-w64-mingw32/include )
	#LINK_DIRECTORIES( ${LANG}/OpenSSL-1.0.0a-bin-x64/x86_64-w64-mingw32/lib ${LANG}/mingw/x86_64-w64-mingw32/lib c:${LANG}/mingw/lib "${LANG}/mw64/lib"  )

ELSE (WIN32)

	ADD_DEFINITIONS( -g -std=c++11 -Wall -Wno-sign-compare )
	include_directories(${LANG}/third/include)

ENDIF (WIN32)

add_executable(${PROJECT_NAME} ${uniproxy_SOURCES})

IF (WIN32)
	# VS2012
	target_link_libraries (${PROJECT_NAME} libcppcms libbooster SSLeay32MDd libeay32MDd zlib pcre dbghelp )

	# Mingw the order of libraries are important
	#target_link_libraries (uniproxy  libcppcms libbooster pcre boost_system-mgw47-mt-1_49 boost_regex-mgw47-mt-1_49 boost_filesystem-mgw47-mt-1_49 ssleay32 crypto ws2_32 wsock32 mswsock zlib m)

ELSE (WIN32)

	#-Wl,--whole-archive -lpthread -Wl,--no-while-archive
	ADD_DEFINITIONS( -Wl,--whole-archive -Wl,--no-while-archive )

	target_link_libraries(${PROJECT_NAME} boost_filesystem.a boost_system.a boost_chrono.a boost_regex.a boost_date_time.a boost_iostreams.a cppcms.a booster.a pthread pcre.a icuuc.a icui18n.a icudata.a icuuc.a gcrypt.a gpg-error.a dl z.a ssl.a crypto.a rt.a c)

	INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/)
	INSTALL(DIRECTORY DESTINATION /var/lib/uniproxy)
	INSTALL(DIRECTORY DESTINATION /var/lib/uniproxy/script)
	INSTALL(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/uniproxy/)
	INSTALL(FILES script/jquery.js DESTINATION /var/lib/uniproxy/script/)
	INSTALL(FILES doc/license.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/uniproxy/)
	INSTALL(FILES doc/uniproxy.json.sample DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/uniproxy/)
	INSTALL(FILES deb/uniproxy.conf DESTINATION /etc/init/ )

ENDIF (WIN32)


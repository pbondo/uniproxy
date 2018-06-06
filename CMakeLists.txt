#
# cmake configuration file
#
#
cmake_minimum_required(VERSION 2.8)

project(uniproxy)

IF (WIN32)
	SET(VC 14)
	SET(LANG c:/lang)
ELSE()
	SET(LANG $ENV{HOME}/lang)
ENDIF()


include_directories(. src/ libs/)

# This command assemble the cppcms templates
IF (WIN32)

add_custom_command(
  OUTPUT views.cpp
  COMMAND python "${LANG}/cppcms-1.2.0/bin/cppcms_tmpl_cc" ../webserver/master.tmpl ../webserver/view_config.tmpl ../webserver/view_status.tmpl -o views.cpp
  #COMMAND cppcms_tmpl_cc.py ../webserver/master.tmpl ../webserver/view_config.tmpl ../webserver/view_status.tmpl -o views.cpp
  DEPENDS webserver/master.tmpl webserver/view_config.tmpl webserver/view_status.tmpl
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Precompiling views"
  VERBATIM
  )

ELSE (WIN32)

add_custom_command(
  OUTPUT views.cpp
  COMMAND /opt/cppcms/1.2.0/bin/cppcms_tmpl_cc ../webserver/master.tmpl ../webserver/view_config.tmpl ../webserver/view_status.tmpl -o views.cpp
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
	../release.cpp
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
                ${LANG}/openssl-1.0.2d/ms/applink.c
		../src/win_util.cpp
		../src/win_util.h
		)


	ADD_DEFINITIONS( -D_WIN32_WINNT=0x0501 /wd4250 /wd4018  /wd4267 /wd4996)
	include_directories (${uniproxy_SOURCE_DIR} ${LANG}/w64-vc${VC}/include ${LANG}/cppcms-1.0.5/include ${LANG}/boost/1_64_0  ${LANG}/openssl-1.0.2d/include/)
	LINK_DIRECTORIES( "${LANG}/w64-vc${VC}/lib" "${LANG}/openssl-1.0.2d/lib/vc${VC}x64/" ${LANG}/boost/1_64_0/lib64-msvc-${VC}.0/ ${LANG}/cppcms-1.0.5/lib/vc${VC}x64/release)
	# Use multi cores
	SET(VISUAL_STUDIO_MP_FLAG "/MP")

ELSE (WIN32)

	ADD_DEFINITIONS( -g -std=c++11 -Wall -Wno-sign-compare )

	include_directories(/opt/boost/1.67.0/include/ /opt/cppcms/1.2.0/include)

	LINK_DIRECTORIES(/opt/boost/1.67.0/lib/ /opt/cppcms/1.2.0/lib)

ENDIF (WIN32)

add_executable(${PROJECT_NAME} ${uniproxy_SOURCES})

IF (WIN32)

	target_link_libraries (${PROJECT_NAME} cppcms booster libeay64vc14 ssleay64vc14 ) ##zlib pcre dbghelp )

ELSE (WIN32)

	#-Wl,--whole-archive -lpthread -Wl,--no-while-archive
	ADD_DEFINITIONS( -Wl,--whole-archive -Wl,--no-while-archive, -Wno-deprecated-declarations )


	target_link_libraries(${PROJECT_NAME} boost_filesystem.a boost_system.a boost_chrono.a boost_regex.a boost_date_time.a boost_iostreams.a cppcms.a booster.a pthread pcre.a icuuc.a icui18n.a icudata.a icuuc.a gcrypt.a gpg-error.a dl z.a ssl.a crypto.a rt.a systemd c)

	INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/)
	#INSTALL(DIRECTORY DESTINATION /var/lib/uniproxy)
	#INSTALL(DIRECTORY DESTINATION /var/lib/uniproxy/script)
	INSTALL(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/uniproxy/)
	#INSTALL(FILES script/jquery.js DESTINATION /var/lib/uniproxy/script/)
	INSTALL(FILES script/jquery.js DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/uniproxy/)
	INSTALL(FILES doc/license.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/uniproxy/)
	INSTALL(FILES doc/uniproxy.json.sample DESTINATION ${CMAKE_INSTALL_PREFIX}/share/doc/uniproxy/)
	#INSTALL(FILES deb/uniproxy.conf DESTINATION /etc/init/ )
	INSTALL(FILES deb/uniproxy.service DESTINATION /etc/systemd/system/ )

ENDIF (WIN32)


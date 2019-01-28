#
# cmake configuration file
#
#
project(uniproxy)

# This command assemble the cppcms templates
IF (WIN32)

add_custom_command(
  OUTPUT views.cpp
  COMMAND python "${LANG}/cppcms-1.2.0/bin/cppcms_tmpl_cc" ../../webserver/master.tmpl ../../webserver/view_config.tmpl ../../webserver/view_status.tmpl -o views.cpp
  #COMMAND cppcms_tmpl_cc.py ../webserver/master.tmpl ../webserver/view_config.tmpl ../webserver/view_status.tmpl -o views.cpp
  DEPENDS ../webserver/master.tmpl ../webserver/view_config.tmpl ../webserver/view_status.tmpl
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Precompiling views"
  VERBATIM
  )

ELSE (WIN32)

add_custom_command(
  OUTPUT views.cpp
  COMMAND cppcms_tmpl_cc ../../webserver/master.tmpl ../../webserver/view_config.tmpl ../../webserver/view_status.tmpl -o views.cpp
  DEPENDS ../webserver/master.tmpl ../webserver/view_config.tmpl ../webserver/view_status.tmpl
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Precompiling views"
  VERBATIM
  )

ENDIF (WIN32)


set(CPP_SOURCE
	applutil.cpp
	applutil.h
	baseclient.cpp
	baseclient.h
	cppcms_util.h
	error_codes.h
	httpclient.cpp
	httpclient.h
	localclient.cpp
	localclient.h
	main.cpp
	main.h
	platform.h
	providerclient.cpp
	providerclient.h
	proxy_global.cpp
	proxy_global.h
	remoteclient.cpp
	remoteclient.h

	../release.cpp
)

set(CPP_SOURCE
	${CPP_SOURCE}
	views.cpp
	../README.md
	../doc/uniproxy.json.sample
	../webserver/content.h
	../webserver/master.tmpl
	../webserver/view_config.tmpl
	../webserver/view_status.tmpl
)


IF (WIN32)

	set(CPP_SOURCE
		${CPP_SOURCE}
		${LANG}/openssl-1.1.0e/include/openssl/applink.c
		../src/win_util.cpp
		../src/win_util.h
		)

	add_executable( ${PROJECT_NAME} ${CPP_SOURCE})

	target_link_libraries (${PROJECT_NAME} gatehouse cppcms booster libcrypto libssl)

ELSE()

	add_executable( ${PROJECT_NAME} ${CPP_SOURCE})

	target_link_libraries(${PROJECT_NAME} gatehouse boost_filesystem.a boost_system.a boost_chrono.a boost_regex.a boost_date_time.a boost_iostreams.a cppcms.a booster.a pthread pcre.a icuuc.a icui18n.a icudata.a icuuc.a gcrypt.a gpg-error.a dl z.a ssl.a crypto.a rt.a systemd c)

	INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/)

ENDIF()

ADD_DEPENDENCIES(${PROJECT_NAME} gatehouse)


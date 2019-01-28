#
# cmake configuration file
#
#
project(uniproxy)

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
  COMMAND cppcms_tmpl_cc ../../webserver/master.tmpl ../../webserver/view_config.tmpl ../../webserver/view_status.tmpl -o views.cpp
  DEPENDS ../webserver/master.tmpl ../webserver/view_config.tmpl ../webserver/view_status.tmpl
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Precompiling views"
  VERBATIM
  )

ENDIF (WIN32)


set(CPP_SOURCE
	applutil.cpp
	baseclient.cpp
	httpclient.cpp
	localclient.cpp
	main.cpp
	providerclient.cpp
	proxy_global.cpp
	remoteclient.cpp
	#win_util.cpp
	applutil.h
	baseclient.h
	cppcms_util.h
	error_codes.h
	httpclient.h
	localclient.h
	main.h
	platform.h
	providerclient.h
	proxy_global.h
	remoteclient.h
	#win_util.h
	../release.cpp
)

set(CPP_SOURCE
	${CPP_SOURCE}
	#README.md
	#../doc/uniproxy.json.sample
	views.cpp
	../webserver/content.h
	../webserver/master.tmpl
	../webserver/view_config.tmpl
	../webserver/view_status.tmpl
)


IF (WIN32)
ELSE()
add_executable( ${PROJECT_NAME} ${CPP_SOURCE})

target_link_libraries(${PROJECT_NAME} gatehouse boost_filesystem.a boost_system.a boost_chrono.a boost_regex.a boost_date_time.a boost_iostreams.a cppcms.a booster.a pthread pcre.a icuuc.a icui18n.a icudata.a icuuc.a gcrypt.a gpg-error.a dl z.a ssl.a crypto.a rt.a systemd c)

#TARGET_LINK_LIBRARIES(${PROJECT_NAME} gatehouse)
ENDIF()

#ADD_DEPENDENCIES(${PROJECT_NAME} tracklib utillib dblib) ## btracklib)


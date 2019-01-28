#
#
project (gatehouse)

SET(CPP_SOURCES
	pghp2.cpp
	pghpinternalbase.cpp
	pghplogonreply.cpp
	pghpnmeamsg.cpp
	pghpproxyfilter.cpp
	pghputils.cpp
	pghpbase.cpp
	pghplogoffrequest.cpp
	pghplogonrequest.cpp
	pghpplugin.cpp
	pghpstartdatarequest.cpp
	pghpxmlutil.cpp

	pghp2.h
	pghpgeneral.h
	pghplogoffrequest.h
	pghplogonrequest.h
	pghpplugin.h
	pghpstartdatarequest.h
	pghpxmlutil.h
	pghpbase.h
	pghpinternalbase.h
	pghplogonreply.h
	pghpnmeamsg.h
	pghpproxyfilter.h
	pghputils.h
)

ADD_LIBRARY(${PROJECT_NAME} STATIC ${CPP_SOURCES})

#TARGET_LINK_LIBRARIES(${PROJECT_NAME})


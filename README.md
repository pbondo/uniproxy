uniproxy
========

The Universal Proxy is an application with the purpose of providing maritime authorities (and anyone else) 
with a simple application that will provide a secure and reliable transparent TCP/P socket connection between two entities over the Internet.

The project is partly sponsored by the Mona Lisa project funded by the European Union.

The documentation ie. the requirement specification is available but will not be put on github.



Log:
========

2018-11-26 PBondo
-----------------
Fixed an issue where using asio sync SSL handshake would not timeout. Replaced with an async SSL handshake.

Fixed an issue where starting uniproxy with the main ports already taken (typically from a quick restart) would return exit code 0.
Changed to return 1.

Added some additional information on some of the log messages so it will more often include remote host:port information.



2018-06-07 PBondo
-----------------
Release 1.3.2

Added additional log files (10)


6/4-2017 PBondo
---------------
Release 1.3.0

Add host test to test a local connection before sending the data to the client.

03/11-2016 PBondo
-----------------
Release 1.2.3

Misc minor fixes for first Docker release

19/4-2016 PBondo
----------------
Release 1.2.2

Fixed a possible hang


9/3-2016 PBondo
---------------
Release 1.2.1

Fixed the issue with blocking ssl handshake. The timeout was not initiated until after the handshake completed.



29/9-2015 PBondo
----------------
Moved Windows compile to VC14.

Version 1.0.2


5/5-2015 PBondo
---------------
It looks like the proper way to stop a synchronous SSL connection (that is established) from another thread is to call only 
this->m_remote_socket.lowest_layer().cancel(ec);
Then from the thread that made the connection the remaining shutdown and close should be called.

Aiming for Version 1.0.0



14/4-2015 PBondo
----------------
The log file is overwritten on restart. Possibly we should add a check to ensure we delete the oldest log file.
Seen a case with zero data throughput without a reconnect.

Changing a client (remote port) in a uploaded file did apparently not cause a reload configuration. A subsequent reload configuration did work.
However changing the hostname of the remote did work. Changing both did not obviously work if not connected.


8/4-2015 PBondo
---------------
Wellbehaving extra local connection to a client is working ok, but it is the slowest client that determine the speed.
Disconnecting is also ok. However pulling the plug timesout after 5 minutes (probably default read timeout) and it does not have any effect.


23/3-2015 PBondo
----------------
Issue with round robin. If a lot of connections to one uniproxy / or one host application are dropped becauseof a restart, then all move to the next (same) UP.
Being replaced with a random selection, to spread out the load.
Initial idea of shuffling the list does not work, since it is used for detecting if the configuration has changed. And there is currently no gate on the list, so it is readonly.

This one should also have the release number in the GateHouse connection string and displayed on the status web page.

Subsequently release 0.3.7.

17/3-2015 PBondo
----------------
Updated with TLSv12 as replacement for SSL.


24/4-2014 PBondo
----------------
Released new version with OpenSSL upgraded for heartbleed potential.


9/4-2014 PBondo
---------------
Fix various log issues. Main one being that a large log would overload the client.


12/12-2013 PBondo
-----------------
Fix TCP keep alive bug on Windows.

Switched to Visual Studio 2013

1/10-2013 PBondo
----------------
Has been running successfully deployed for more than a year now.

Added support for the provider proxy configuration.

Changed build type for Windows to 64 bit.

Installer included for Windows

Switched to Visual Studio 2012.


6/8-2012 PBondo
---------------
Preparing for initial push of the source for uniproxy.


Building instructions:
======================

Linux (Ubuntu)
--------------
Dependencies:

Python (www.python.org)
cmake (www.cmake.org)
openssl (www.openssl.org)
boost (www.boost.org)
# sudo apt-get install checkinstall cmake git g++ python libssl-dev libgcrypt20-dev libicu-dev libpcre++-dev
cppcms (cppcms.com)
# Must be installed and compiled manually.

mkdir build
cd build
cmake ..
make

The codelite project files should be correct.


Windows (Windows 7)
-------------------
Dependencies:

Python (www.python.org)
cmake (www.cmake.org)
openssl (www.openssl.org)
boost (www.boost.org)
cppcms (cppcms.com)

mkdir build64
cd build64
cmake -G "Visual Studio 12 2013 Win64 ..
msbuild /p:Configuration=Release uniproxy.sln


Also boost process
http://www.highscore.de/boost/process.zip


Contributers:
-------------
Poul Bondo (GateHouse) Founder and current Maintainer


uniproxy
========

The Universal Proxy is an application with the purpose of providing maritime authorities (and anyone else) 
with a simple application that will provide a secure and reliable transparent TCP/P socket connection between two entities over the Internet.

The project is partly sponsored by the Mona Lisa project funded by the European Union.

The documentation ie. the requirement specification is available but will not be put on github.



Log:
========


8/4-2015 PBondo
---------------
Wellbehaving extra local connection to a client is working ok, but it is the slowest client that determine the speed.
Disconnecting is also ok. However pulling the plug timesout after 5 minutes (probably TCP K/A) and it does not have any effect.



23/3-2015 PBondo
----------------
Issue with round robin. If a lot of connections to one uniproxy / or one host application are dropped becauseof a restart, then all move to the next (same) UP.
Being replaced with a random selection, to spread out the load.
Initial idea of shuffling the list does not work, since it is used for detecting if the configuration has changed. And there is currently no gate on the list, so it is readonly.


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
# sudo apt-get install cmake libssl-dev libboost-all-dev
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


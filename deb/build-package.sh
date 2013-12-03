#
# Build debian/ubuntu package using checkinstall.
#
#!/bin/bash

version=0.3.0
release=1

ln -s ../deb/postinstall-pak
ln -s ../deb/preinstall-pak
ln -s ../deb/preremove-pak
ln -s ../deb/postremove-pak
ln -s ../deb/description-pak
touch install_manifest.txt

make

#run this with sudo. It will remove some of the annoying side effects afterwards.
sudo checkinstall --install=no --pkgversion=${version} --pkgrelease=${release} --pkglicense=GPL --pkgname=uniproxy --maintainer="Poul Bondo \<pba@gatehouse.dk\>" --provides=uniproxy --requires="openssl" --nodoc --exclude=/home  --backup=no
#sudo rm /etc/init/uniproxy.conf

rm postinstall-pak  postremove-pak  preinstall-pak  preremove-pak description-pak

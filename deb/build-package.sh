#!/bin/bash

#run this with sudo. It will remove some of the annoying side effects afterwards.

ln -s ../deb/postinstall-pak
ln -s ../deb/preinstall-pak
ln -s ../deb/preremove-pak
ln -s ../deb/postremove-pak
ln -s ../deb/description-pak
touch install_manifest.txt

sudo checkinstall --install=no --pkgversion=0.0.1 --pkglicense=GPL --pkgname=uniproxy --maintainer="Poul Bondo \<pba@gatehouse.dk\>" --provides=uniproxy --requires="openssl" --nodoc --exclude=/home  --backup=no
sudo rm /etc/init/uniproxy.conf

rm postinstall-pak  postremove-pak  preinstall-pak  preremove-pak description-pak


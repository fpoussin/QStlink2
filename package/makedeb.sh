#!/bin/bash

SVNVER=`svn info ../ | grep Revision | awk '{ print $2 }'`
echo $SVNVER

BUILD_FOLDER=qstlink2-0.$SVNVER

rm -rf $BUILD_FOLDER
mkdir $BUILD_FOLDER

shopt -s extglob
cp -r ../!(package) $BUILD_FOLDER/
cp -r ../.svn $BUILD_FOLDER/

cd $BUILD_FOLDER
echo "" | dh_make -n --single -e mobyfab@gmail.com -c gpl3

cp -v ../debian/* debian/

debuild -j4 -b -uc -us

cd ..
rm -rf $BUILD_FOLDER *.build *.changes

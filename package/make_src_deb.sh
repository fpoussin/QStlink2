#!/bin/bash

SVNVER=`svn info ../ | grep Revision | awk '{ print $2 }'`
echo $SVNVER

BUILD_FOLDER=qstlink2-0.$SVNVER
ORIG_FILE=$BUILD_FOLDER.orig.tar.gz

rm -f $ORIG_FILE
rm -rf $BUILD_FOLDER *.build *.changes *.ppa* *.dsc
mkdir $BUILD_FOLDER

shopt -s extglob
cp -r ../!(package) $BUILD_FOLDER/
cp -r ../.svn $BUILD_FOLDER/

#tar czf $ORIG_FILE $BUILD_FOLDER

#rm -rf $BUILD_FOLDER

cd $BUILD_FOLDER
echo "" | dh_make -n --single -e fabien.poussin@gmail.com -c gpl3

cp -v ../debian/* debian/
rm -vf debian/*.ex debian/*.EX debian/ex.*

sed -i -e's/unstable/precise/' debian/changelog

debuild -j4 -S -sa

cd ..
#rm -rf $BUILD_FOLDER *.build *.changes
#rm -rf $BUILD_FOLDER

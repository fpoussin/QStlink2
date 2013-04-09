#!/bin/bash

if [ -z "$1" ]
  then
    echo "Please specify an Ubuntu release (ie precise)"
    exit 1
fi

SVNVER=`svn info ../ | grep Revision | awk '{ print $2 }'`
echo $SVNVER

BUILD_FOLDER=qstlink2-0.$SVNVER~$1
ORIG_FILE=$BUILD_FOLDER.orig.tar.gz

rm -f $ORIG_FILE
rm -rf $BUILD_FOLDER
mkdir $BUILD_FOLDER

shopt -s extglob
cp -r ../!(package) $BUILD_FOLDER/
cp -r ../.svn $BUILD_FOLDER/

cd $BUILD_FOLDER
echo "" | dh_make -n --single -e fabien.poussin@gmail.com -c gpl3

cp -v ../debian/* debian/
rm -vf debian/*.ex debian/*.EX debian/ex.*

/usr/bin/python ../svn2debcl.py $1

debuild -j4 -S -sa

cd ..
rm -rf $BUILD_FOLDER

#! /usr/bin/python

import sys,os, re, argparse, iso8601, shutil
from subprocess import call, check_output, Popen, PIPE
from xml.etree import ElementTree as ET
import random, string

releases = ["vivid", "trusty", "utopic"]
build_for = []

parser = argparse.ArgumentParser()
parser.add_argument('-r', '--release', help='The Ubuntu release')
parser.add_argument('-a', '--all', help='Build for all releases', action="store_true")
parser.add_argument('-s', '--source', help='Build signed source package', action="store_true")
parser.add_argument('-lb', '--bin', help='Build local binary package', action="store_true")
parser.add_argument('-b', '--sbuild', help='Build binary package with sbuild', action="store_true")
parser.add_argument('-p', '--ppa', help='Send source package to PPA', action="store_true")
parser.add_argument('-R', '--revision', help='Revision number', default=0, type=int)

def makeChangelog(release, dest, rev):

  #~ f = open(dest+"/debian/changelog", "r")
  #~ tmp = f.read()
  #~ f.close()
  
  return

def run_cmd(cmd, do_print=True, **kwargs):
    p = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE, **kwargs)
    if do_print:
        lines_iterator = iter(p.stdout.readline, b"")
        for line in lines_iterator:
            print(line.strip())  # yield line
    else:
        p.wait()
  
def copySrc(release, dest, rev):
  
  check_output(["rm -rf "+dest], shell=True)
  print check_output(["mkdir -v "+dest], shell=True)

  #~ print check_output(["shopt -s extglob; cp -r ../!(package) "+dest+"/"], shell=True)
  cmd = "find .. -maxdepth 1 \! \( -name package -o -name *.pro.user* -o -name .. \) -exec cp -r '{}' "+dest+"/ ';'"
  check_output([cmd], shell=True)
  
  print check_output(["cd "+dest+"; echo "" | dh_make -n --single -e fabien.poussin@gmail.com -c gpl3"], shell=True)

  print check_output(["cp -v debian/* "+dest+"/debian/"], shell=True)
  check_output(["rm -f "+dest+"/debian/*.ex "+dest+"/debian/*.EX "+dest+"/debian/ex.*"], shell=True)
  
  f = open(dest+"/debian/changelog", "r")
  tmp = f.read().replace("unstable; urgency=low", release+"; urgency=medium")
  f.close()
  f = open(dest+"/debian/changelog", "w")
  f.write(str(tmp))
  f.close()
  
  f = open(dest+"/version", "w")
  f.write(str(rev))
  f.close()
  
def makeSrc(dest):
  print "Building Source package"
  run_cmd(["cd "+dest+"; debuild -j4 -S -sa"])
  
def makeBin(dest):
  print "Building binary package"
  run_cmd(["cd "+dest+"; debuild -j4 -b -uc -us"])
  
def makeSBuild(dest):
  print "Building binary package (sbuild)"
  run_cmd(["sbuild -vd {0} -c {0}-amd64-shm -j4 {1}".format(args.release, dest)])

def sendSrc(ver):
  run_cmd(["dput ppa:fpoussin/ppa "+ver+"_source.changes"])

if __name__ == "__main__":

  args = parser.parse_args()
  
  if (not args.release and not args.all) or (not args.source and not args.bin and not args.sbuild):
    print " "
    parser.print_help()
    print " "
    print "You will need debuild, cdbs and dh_make to generate packages."
    print " "
    exit(1)

  if not args.all and args.release not in releases:
    print "Invalid Ubuntu release, please chose among:", ", ".join(releases)
    exit(1)
  
  if args.all:
    build_for = releases[:]
  else:
    build_for.append(args.release)
    
  ver = check_output(["grep \"VERSION =\" ../QStlink2.pro | awk '{ print $3 }' "], shell=True).replace('\n','')
  if not ver:
    print "Could not fetch last version"
    exit(1)

  print "Last version:" , ver
  
  for release in build_for:
  
    folder_name = 'qstlink2-'+str(ver)+"~"+str(args.revision)+'~'+release
    print "Package version:" , folder_name
    
    copySrc(release, folder_name, ver)
    makeChangelog(release, folder_name, ver)
    
    if args.source:
      makeSrc(folder_name)
      if args.ppa:
        sendSrc(folder_name.replace('-','_'))
    if args.sbuild:
      if not args.source:  # Need to make sources first
	  makeSrc(folder_name)
      makeSBuild(folder_name)
    if args.bin:
      makeBin(folder_name)
      
    print check_output(["rm -rf "+folder_name], shell=True)
    check_output(["schroot -e --all-sessions"], shell=True)
  

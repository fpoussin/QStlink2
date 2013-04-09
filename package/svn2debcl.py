#! /usr/bin/python

import sys,os, re, xml, argparse, datetime, time, iso8601
from subprocess import call, check_output
from xml.etree import ElementTree as ET

parser = argparse.ArgumentParser()
parser.add_argument("release")
args = parser.parse_args()

if args.release not in ["oneiric", "precise", "quantal", "raring"]:
	print "invalid Ubuntu release"
	sys.exit(1)

print "Building changelog file from svn logs"
tmp_svn = check_output(["svn log --limit 10 --xml"], shell=True)

xml_obj = ET.fromstring(tmp_svn)

result = ""
for entry in xml_obj.iter('logentry'):
	
	try:
		revmsg = entry.find('msg').text
		revdate = iso8601.parse_date(entry.find('date').text)
		revauthor = entry.find('author').text
		row = "qstlink2 (0.%s~%s) %s; urgency=low" % (entry.attrib["revision"], args.release, args.release)
		row += "\r\n\r\n  * " + revmsg.replace("\n", "\n    ") + "\r\n"
		#row += " -- Fabien Poussin <%s>  %s \r\n\r\n" %(revauthor, revdate.strftime("%a, %d %b %Y %H:%M:%S %z"))
		row += " -- Fabien Poussin <fabien.poussin@gmail.com>  %s \r\n\r\n" %(revdate.strftime("%a, %d %b %Y %H:%M:%S %z")) ## Need to force address to sign with certificate
		result +=row
	except Exception as  e:
		print str(e)

out = open("debian/changelog", "w")
out.write(result)
out.close()
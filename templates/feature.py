#create new feature class.
#run command from this directory, like so:
#'python ./feature.py TransitionCurve'
#this will copy the template to the feature directory and do string substition.

import sys
import os
import shutil
import datetime
from os import path

if (len(sys.argv) != 2):
  print >> sys.stderr, 'ERROR: Wrong number of arguments. Need classname'
  sys.exit()

if not path.isfile('./featureTemplate.h'):
  print >> sys.stderr, 'ERROR: No featureTemplate.h file'
  sys.exit()
  
if not path.isfile("./featureTemplate.cpp"):
  print >> sys.stderr, 'ERROR: No featureTemplate.cpp file'
  sys.exit()

if not path.isfile('./commandTemplate.h'):
  print >> sys.stderr, 'ERROR: No commandTemplate.h file'
  sys.exit()
  
if not path.isfile("./commandTemplate.cpp"):
  print >> sys.stderr, 'ERROR: No commandTemplate.cpp file'
  sys.exit()
  
if not path.isfile("./serialTemplate.xsd"):
  print >> sys.stderr, 'ERROR: No serialTemplate.xsd file'
  sys.exit()

className = sys.argv[1]
classNameLowerCase = className.lower()
classNameUpperCase = className.upper()

now = datetime.datetime.now()

def copyAndReplace(input, output):
  with open(input, 'r') as file :
    filedata = file.read()

  filedata = filedata.replace('%YEAR%', now.strftime("%Y"))
  filedata = filedata.replace('%CLASSNAME%', className)
  filedata = filedata.replace('%CLASSNAMELOWERCASE%', classNameLowerCase)
  filedata = filedata.replace('%CLASSNAMEUPPERCASE%', classNameUpperCase)
  
  with open(output, 'w') as file:
    file.write(filedata)

copyAndReplace('./featureTemplate.h', '../feature/ftr' + classNameLowerCase + '.h')
copyAndReplace('./featureTemplate.cpp', '../feature/ftr' + classNameLowerCase + '.cpp')

copyAndReplace('./commandTemplate.h', '../command/cmd' + classNameLowerCase + '.h')
copyAndReplace('./commandTemplate.cpp', '../command/cmd' + classNameLowerCase + '.cpp')

copyAndReplace('./serialTemplate.xsd', '../project/serial/feature' + classNameLowerCase + '.xsd')

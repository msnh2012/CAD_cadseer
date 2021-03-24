#create new feature class.
#run command from this directory, like so:
#'python ./feature.py FeatureName'
#'python ./feature.py FeatureName --feature --command --serial --dialog'
#this will copy the template to the feature directory and do string substition.

import sys
import os
import shutil
import datetime
from os import path

def commandMessage():
  print ('Examples:')
  print ('  python ./feature.py FeatureName')
  print ('  python ./feature.py FeatureName --feature')
  print ('  python ./feature.py FeatureName --command')
  print ('  python ./feature.py FeatureName --feature --command --serial --commandView')

if (len(sys.argv) < 2):
  sys.stderr.write('ERROR: Wrong number of arguments. Need at least classname\n')
  commandMessage()
  sys.exit()

#this ensures user puts classname first
if sys.argv[1].startswith('-'):
  sys.stderr.write('ERROR: invalid classname\n')
  commandMessage()
  sys.exit()

if not path.isfile('./featureTemplate.h'):
  sys.stderr.write('ERROR: No featureTemplate.h file\n')
  sys.exit()
  
if not path.isfile("./featureTemplate.cpp"):
  sys.stderr.write('ERROR: No featureTemplate.cpp file\n')
  sys.exit()

if not path.isfile('./commandTemplate.h'):
  sys.stderr.write('ERROR: No commandTemplate.h file\n')
  sys.exit()
  
if not path.isfile("./commandTemplate.cpp"):
  sys.stderr.write('ERROR: No commandTemplate.cpp file\n')
  sys.exit()
  
if not path.isfile("./commandViewTemplate.h"):
  sys.stderr.write('ERROR: No commandViewTemplate.h file\n')
  sys.exit()
  
if not path.isfile("./commandViewTemplate.cpp"):
  sys.stderr.write('ERROR: No commandViewTemplate.cpp file\n')
  sys.exit()
  
if not path.isfile("./serialTemplate.xsd"):
  sys.stderr.write('ERROR: No serialTemplate.xsd file\n')
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

class Entry:
  def __init__(self, inHeader, inSource, outHeader, outSource, argument, runState):
    self.inHeader = inHeader
    self.inSource = inSource
    self.outHeader = outHeader
    self.outSource = outSource
    self.argument = argument
    self.runState = runState
  def go(self):
    if self.inHeader and self.outHeader and self.runState == 'true':
      copyAndReplace(self.inHeader, self.outHeader)
    if self.inSource and self.outSource and self.runState == 'true':
      copyAndReplace(self.inSource, self.outSource)

entries = []
entries.append(Entry('./featureTemplate.h',
                     './featureTemplate.cpp',
                     '../feature/ftr' + classNameLowerCase + '.h',
                     '../feature/ftr' + classNameLowerCase + '.cpp',
                     '--feature',
                     'true'
                     ))
entries.append(Entry('./commandTemplate.h',
                     './commandTemplate.cpp',
                     '../command/cmd' + classNameLowerCase + '.h',
                     '../command/cmd' + classNameLowerCase + '.cpp',
                     '--command',
                     'true'
                     ))
entries.append(Entry('./serialTemplate.xsd',
                     '',
                     '../project/serial/schema/prjsrl_FIX_' + classNameLowerCase + '.xsd',
                     '',
                     '--serial',
                     'true'
                     ))
entries.append(Entry('./commandViewTemplate.h',
                     './commandViewTemplate.cpp',
                     '../commandview/cmv' + classNameLowerCase + '.h',
                     '../commandview/cmv' + classNameLowerCase + '.cpp',
                     '--commandView',
                     'true'
                     ))

#if we supply any argument beyond class name, turn off all files and turn on as needed
if (len(sys.argv) > 2):
  for e in entries:
    e.runState = 'false'
  index = 2
  while index < len(sys.argv):
    for e in entries:
      if sys.argv[index] == e.argument:
        e.runState = 'true'
        break
    index += 1

for e in entries:
  e.go()

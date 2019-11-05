/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <iostream>
#include <cassert>

#include <boost/filesystem.hpp>

#include "tools/idtools.h"
#include "application/appapplication.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "message/msgnode.h"
#include "message/msgsift.h"
#include "lod/lodmessage.h"
#include "project/prjmessage.h"
#include "feature/ftrmessage.h"
#include "feature/ftrstates.h"
#include "feature/ftrbase.h"
#include "lod/lodmanager.h"

using namespace lod;

namespace bfs = boost::filesystem;

Manager::Manager(const std::string &parentArg)
{
  bfs::path argPath = bfs::path(parentArg);
  bfs::path canonicalPath = bfs::canonical(argPath);
  assert(bfs::exists(canonicalPath));
  bfs::path folderPath = canonicalPath.parent_path();
  assert(bfs::exists(folderPath));
  lodPath = folderPath / "lodgenerator";
  assert(bfs::exists(lodPath));
  
  
  node = std::make_unique<msg::Node>();
  node->connect(msg::hub());
  sift = std::make_unique<msg::Sift>();
  sift->name = "lod::Manager";
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
  setupDispatcher();
  
  process = new QProcess(this);
  process->setProcessChannelMode(QProcess::ForwardedErrorChannel);
  process->start(QString::fromStdString(lodPath.string()));
  connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readyReadStdOutSlot()));
  connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(childFinishedSlot(int, QProcess::ExitStatus)));
  connect(process, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(childErrorSlot(QProcess::ProcessError)));
  
  logging = prf::manager().rootPtr->visual().mesh().logLOD().get();
  if (logging)
  {
    boost::filesystem::path logFilePath(app::instance()->getApplicationDirectory());
    assert(bfs::exists(logFilePath));
    logFilePath /= "LODLog.txt";
    logStream.open(logFilePath.string(), std::ios_base::trunc | std::ios_base::out);
  }
}

Manager::~Manager()
{
  //QProcess destructor should kill process.
}

void Manager::send()
{
  if (childWorking || messages.empty())
    return;
  
  cMessage = messages.back();
  messages.pop_back();
  cmValid = true;
  childWorking = true;
  
  std::ostringstream stream;
  stream
  << "BEGIN" << std::endl
  << cMessage.filePathOCCT.string() << std::endl
  << cMessage.filePathOSG.string() << std::endl
  << cMessage.filePathIds.string() << std::endl
  << std::to_string(cMessage.linear) << std::endl
  << std::to_string(cMessage.angular) << std::endl
  << "END" << std::endl;
  
  std::string temp = stream.str();
  process->write(temp.c_str());
  
  if (logging)
    logStream << "LOD Manager: sending to child process: " << cMessage.filePathOSG.string() << std::endl;
}

void Manager::readyReadStdOutSlot()
{
  if (logging)
    logStream << "LOD Manager: ready to read child process: " << std::endl;
  
  QByteArray lineBuffer;
  while (process->canReadLine())
  {
    lineBuffer = process->readLine();
    if (!lineBuffer.isEmpty())
    {
      if (lineBuffer.startsWith("FAIL"))
      {
        std::cout << lineBuffer.constData() << std::endl;
        cmValid = false;
        break;
      }
      else if(lineBuffer.startsWith("SUCCESS"))
        break;
      else
      {
        std::cout << "WARNING: unrecognized response from child process in Manager::readyReadStdOutSlot: "
        << lineBuffer.toStdString() << std::endl;
        lineBuffer.clear();
        continue;
      }
    }
  }
  if (lineBuffer.isEmpty()) //what to do here?
    return;
  
  if (cmValid)
  {
    msg::Message mOut(msg::Mask(msg::Response | msg::Construct | msg::LOD), cMessage);
    app::instance()->queuedMessage(mOut); //ensures sync, not really necessary now.
  }
  cmValid = false;
  childWorking = false;
  send();
}

void Manager::childFinishedSlot(int exitCode, QProcess::ExitStatus exitStatus)
{
  std::cout
  << "lod generator process finished. Exit code is: " << exitCode
  << " exit status is: " << ((exitStatus == QProcess::NormalExit) ? ("Normal Exit") : ("Crash Exit"))
  << std::endl;
}

void Manager::childErrorSlot(QProcess::ProcessError error)
{
  std::cout
  << "lod generator process error. Code is: " << static_cast<int>(error)
  << std::endl;
}

void Manager::setupDispatcher()
{
  sift->insert
  (
    {
      std::make_pair
      (
        msg::Request | msg::Construct | msg::LOD
        , std::bind(&Manager::constructLODRequestDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Pre | msg::Remove | msg::Feature
        , std::bind(&Manager::featureRemovedDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Feature | msg::Status
        , std::bind(&Manager::featureStateChangedDispatched, this, std::placeholders::_1)
      )
    }
  );
}

void Manager::constructLODRequestDispatched(const msg::Message &mIn)
{
  messages.push_back(mIn.getLOD());
  send();
}

void Manager::featureRemovedDispatched(const msg::Message &mIn)
{
  prj::Message pm = mIn.getPRJ();
  cleanMessages(pm.feature->getId());
}

void Manager::featureStateChangedDispatched(const msg::Message &mIn)
{
  ftr::Message fm = mIn.getFTR();
  
  auto test = [&](std::size_t offset) -> bool
  {
    return ((fm.stateOffset == offset) && (fm.state.test(offset)));
  };
  
  if (test(ftr::StateOffset::ModelDirty) || test(ftr::StateOffset::VisualDirty))
    cleanMessages(fm.featureId);
}

void Manager::cleanMessages(const boost::uuids::uuid &idIn)
{
  if (cmValid && (cMessage.featureId == idIn))
  {
    if (logging)
      logStream << "LOD Manager: setting current message to invalid for id: " << gu::idToString(idIn) << std::endl;
    cmValid = false;
  }
  
  for (auto it = messages.begin(); it != messages.end();)
  {
    if (it->featureId == idIn)
    {
      it = messages.erase(it);
      if (logging)
        logStream << "LOD Manager: cleaning message for id: " << gu::idToString(idIn) << std::endl;
    }
    else
      it++;
  }
}

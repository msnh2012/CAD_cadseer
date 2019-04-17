/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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
#include <fstream>

#include <QFile>

#include <boost/filesystem.hpp>

#include "application/appapplication.h"

#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"

//xml, xsd validation website.
//http://www.utilities-online.info/xsdvalidation/#.Vf2u9ZOVvts

//generate parsing files.
//this assumes the core xml file has been generated from the project/serial/readme
//xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --namespace-map =prf --extern-xml-schema ../xmlbase.xsd preferencesXML.xsd

//ok this is screwy. the parser, by default, validates the xml so it
//needs the xsd at runtime. We have the xsd and a default xml in
//the qt controlled resources. files will be copied into application
//directory if needed.

using namespace boost::filesystem;

using namespace prf;

prf::Manager& prf::manager()
{
  static Manager localManager;
  return localManager;
}

Manager::Manager()
{
  ok = false;
  
  path appDirectory = app::instance()->getApplicationDirectory();
  xmlPath = appDirectory / "preferences.xml";
  xsdPath = appDirectory / "preferences.xsd";
  
  //just overwrite the xsd for now so it is always current.
  if (!createDefaultXsd())
    return;
  
  if (!exists(xmlPath))
  {
    if (!createDefaultXml())
      return;
  }
  
  if (!readConfig())
  {
    //config is bad. backup and copy default.
    path backupPath = xmlPath;
    backupPath.replace_extension(".xml0");
    int index = 1;
    while (exists(backupPath))
    {
      backupPath.replace_extension(std::string(".xml") + std::to_string(index));
      index++;
    }
    copy_file(xmlPath, backupPath);
    if (!createDefaultXml())
      return;
    
    //try to read the newly created default
    if (!readConfig())
      return;
  }
    
  assert(rootPtr);
  ensureDefaults();
  saveConfig(); //ensure may set values so lets right them out.
  ok = true;
}

bool Manager::createDefaultXml()
{
  //xml
  QString xmlResourceName(":/preferences/preferences.xml");
  QFile resourceFileXML(xmlResourceName);
  if (!resourceFileXML.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    std::cerr << "couldn't open resource file: " << xmlResourceName.toStdString().c_str() << std::endl;
    return false;
  }
  QByteArray bufferXML = resourceFileXML.readAll();
  resourceFileXML.close();
  QFile newFileXML(QString::fromStdString(xmlPath.string()));
  if (!newFileXML.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
  {
    std::cerr << "couldn't open new file: " << xmlPath.string() << std::endl;
    return false;
  }
  newFileXML.write(bufferXML);
  newFileXML.close();
  
  return true;
}

bool Manager::createDefaultXsd()
{
  //xsd. just over write it.
  QString xsdResourceName(":/preferences/preferencesXML.xsd");
  QFile resourceFileXSD(xsdResourceName);
  if (!resourceFileXSD.open(QIODevice::ReadOnly | QIODevice::Text))
  {
    std::cerr << "couldn't open resource file: " << xsdResourceName.toStdString().c_str() << std::endl;
    return false;
  }
  QByteArray bufferXSD = resourceFileXSD.readAll();
  resourceFileXSD.close();
  
  QFile newFileXSD(QString::fromStdString(xsdPath.string()));
  if (!newFileXSD.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
  {
    std::cerr << "couldn't open new file" << xsdPath.string() << std::endl;
    return false;
  }
  newFileXSD.write(bufferXSD);
  newFileXSD.close();
  
  return true;
}

void Manager::saveConfig()
{
  //a very simple test has passed.
  xml_schema::NamespaceInfomap map;
  std::ofstream stream(xmlPath.string());
  root(stream, *rootPtr, map);
}

bool Manager::readConfig()
{
  try
  {
    xml_schema::Properties props;
    props.no_namespace_schema_location (xsdPath.string());
//     props.schema_location ("http://www.w3.org/XML/1998/namespace", "xml.xsd");
    
    auto tempPtr(root(xmlPath.string(), 0, props));
    rootPtr = std::move(tempPtr);
    
    return true;
  }
  catch (const xsd::cxx::xml::invalid_utf16_string&)
  {
    std::cerr << "invalid UTF-16 text in DOM model" << std::endl;
  }
  catch (const xsd::cxx::xml::invalid_utf8_string&)
  {
    std::cerr << "invalid UTF-8 text in object model" << std::endl;
  }
  catch (const xml_schema::Exception& e)
  {
    std::cerr << e << std::endl;
  }
  
  return false;
}

void Manager::setSpaceballButton(int number, const std::string &maskString)
{
  auto &buttons = rootPtr->hotKeys().spaceballButtons().array();
  
  auto it = buttons.begin();
  for (; it != buttons.end(); ++it)
  {
    if (it->number() == number)
    {
      it->mask() = maskString;
      break;
    }
  }
  if (it == buttons.end())
    buttons.push_back(SpaceballButton(number, maskString));
}

std::string Manager::getSpaceballButton(int number) const
{
  const auto &buttons = rootPtr->hotKeys().spaceballButtons().array();
  
  auto it = buttons.begin();
  for (; it != buttons.end(); ++it)
  {
    if (it->number() == number)
      return it->mask();
  }
  
  return std::string();
}

void Manager::setHotKey(int number, const std::string &maskString)
{
  auto &keys = rootPtr->hotKeys().hotKeyEntries().array();
  
  auto it = keys.begin();
  for (; it != keys.end(); ++it)
  {
    if(it->number() == number)
    {
      it->mask() = maskString;
      break;
    }
  }
  if (it == keys.end())
    keys.push_back(HotKeyEntry(number, maskString));
}

std::string Manager::getHotKey(int number) const
{
  const auto &keys = rootPtr->hotKeys().hotKeyEntries().array();
  
  for (const auto &k : keys)
  {
    if (k.number() == number)
      return k.mask();
  }
  
  return std::string();
}

void Manager::ensureDefaults()
{
  //project defaults.
  path basePath = rootPtr->project().basePath();
  if (!exists(basePath))
    rootPtr->project().basePath() = path(getenv("HOME")).string();
  
  path lastPath = rootPtr->project().basePath();
  if (rootPtr->project().lastDirectory().present())
  {
    path configLastPath = rootPtr->project().lastDirectory().get();
    if (exists(configLastPath))
      lastPath = configLastPath;
  }
  rootPtr->project().lastDirectory() = lastPath.string();
  
  if (rootPtr->project().gitName().empty())
    rootPtr->project().gitName() = qgetenv("USER").constData();
  if (rootPtr->project().gitName().empty())
    rootPtr->project().gitName() = qgetenv("USERNAME").constData();
  if (rootPtr->project().gitName().empty())
    rootPtr->project().gitName() = "Holden McGroyn";
  if (rootPtr->project().gitEmail().empty())
    rootPtr->project().gitEmail() = rootPtr->project().gitName() + "@somewhere.com";
  
  auto &features = rootPtr->features();
  
  if (!features.blend().present())
  {
    features.blend() = prf::Blend(prf::Blend::radius_default_value());
  }
  
  if (!features.box().present())
  {
    features.box() = prf::Box
    (
      prf::Box::length_default_value(),
      prf::Box::width_default_value(),
      prf::Box::height_default_value()
    );
  }
  
  if (!features.oblong().present())
  {
    features.oblong() = prf::Oblong
    (
      prf::Oblong::length_default_value(),
      prf::Oblong::width_default_value(),
      prf::Oblong::height_default_value()
    );
  }
  
  if (!features.chamfer().present())
  {
    features.chamfer() = prf::Chamfer(prf::Chamfer::distance_default_value());
  }
  
  if (!features.cone().present())
  {
    features.cone() = prf::Cone
    (
      prf::Cone::radius1_default_value(),
      prf::Cone::radius2_default_value(),
      prf::Cone::height_default_value()
    );
  }
  
  if (!features.cylinder().present())
  {
    features.cylinder() = prf::Cylinder
    (
      prf::Cylinder::radius_default_value(),
      prf::Cylinder::height_default_value()
    );
  }
  
  if (!features.datumPlane().present())
  {
    features.datumPlane() = prf::DatumPlane
    (
      prf::DatumPlane::offset_default_value()
    );
  }
  
  if (!features.draft().present())
  {
    features.draft() = prf::Draft
    (
      prf::Draft::angle_default_value()
    );
  }
  
  if (!features.hollow().present())
  {
    features.hollow() = prf::Hollow
    (
      prf::Hollow::offset_default_value()
    );
  }
  
  if (!features.sphere().present())
  {
    features.sphere() = prf::Sphere
    (
      prf::Sphere::radius_default_value()
    );
  }
  
  if (!features.dieset().present())
  {
    features.dieset() = prf::Dieset
    (
      prf::Dieset::lengthPadding_default_value(),
      prf::Dieset::widthPadding_default_value()
    );
  }
  
  if (!features.nest().present())
  {
    features.nest() = prf::Nest
    (
      prf::Nest::gap_default_value()
    );
  }
  
  if (!features.quote().present())
  {
    features.quote() = prf::Quote
    (
      prf::Quote::templateSheet_default_value()
    );
  }
  
  if (!features.squash().present())
  {
    features.squash() = prf::Squash
    (
      prf::Squash::granularity_default_value()
    );
  }
  
  if (!features.strip().present())
  {
    features.strip() = prf::Strip
    (
      prf::Strip::gap_default_value()
    );
  }
  
  if (!features.thread().present())
  {
    features.thread() = prf::Thread
    (
      prf::Thread::diameter_default_value(),
      prf::Thread::pitch_default_value(),
      prf::Thread::length_default_value(),
      prf::Thread::angle_default_value(),
      prf::Thread::internal_default_value(),
      prf::Thread::fake_default_value(),
      prf::Thread::leftHanded_default_value()
    );
  }
  
  if (!features.torus().present())
  {
    features.torus() = prf::Torus
    (
      prf::Torus::radius1_default_value(),
      prf::Torus::radius2_default_value()
    );
  }
  
  if (!rootPtr->visual().display().renderStyle().present())
    rootPtr->visual().display().renderStyle() = prf::Display::renderStyle_default_value();
  
  if (!rootPtr->visual().display().samples().present())
    rootPtr->visual().display().samples() = prf::Display::samples_default_value();
  
  if (!rootPtr->visual().mesh().lod().present())
  {
    prf::LOD level
    (
      0.0,
      prf::LODEntry(1.0, 1.0),
      0.1,
      prf::LODEntry(0.1, 0.5),
      2.0,
      prf::LODEntry(0.01, 0.25),
      std::numeric_limits<float>::max()
    );
    rootPtr->visual().mesh().lod() = level;
  }
  
  if (!rootPtr->visual().mesh().logLOD().present())
    rootPtr->visual().mesh().logLOD() = prf::Mesh::logLOD_default_value();
  
  if (!rootPtr->visual().spaceballSensitivity().present())
  {
    prf::SpaceballSensitivity sbs
    (
      prf::SpaceballSensitivity::overall_default_value()
      , prf::SpaceballSensitivity::translations_default_value()
      , prf::SpaceballSensitivity::rotations_default_value()
    );
    rootPtr->visual().spaceballSensitivity() = sbs;
  }
  else
  {
    auto &sbs = rootPtr->visual().spaceballSensitivity().get();
    
    if (sbs.overall() < 0.1 || sbs.overall() > 10.0)
      sbs.overall() = prf::SpaceballSensitivity::overall_default_value();
    if (sbs.translations() < 0.1 || sbs.translations() > 10.0)
      sbs.translations() = prf::SpaceballSensitivity::translations_default_value();
    if (sbs.rotations() < 0.1 || sbs.rotations() > 10.0)
      sbs.rotations() = prf::SpaceballSensitivity::rotations_default_value();
  }
}

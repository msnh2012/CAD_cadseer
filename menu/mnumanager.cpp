/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2019 Thomas S. Anderson blobfish.at.gmx.com
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
#include <set>

#include <boost/filesystem.hpp>
#include <boost/optional/optional.hpp>

#include <QObject>
#include <QFile>

#include "application/appapplication.h"
#include "menu/mnuserial.h"
#include "menu/mnumanager.h"

/* GENERATE parser from schema
 * cd cadseer/source/menu
 * xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix MNU_SRL --extern-xml-schema ../xmlbase.xsd ./mnuserial.xsd
 */

namespace mnu
{
  struct Mapper
  {
    struct Entry
    {
      Entry() = default;
      Entry(std::size_t idIn, const msg::Mask &maskIn) : id(idIn), mask(maskIn) {}
      std::size_t id;
      msg::Mask mask;
    };
    std::vector<Entry> entries;
    
    void insert(std::size_t idIn, const msg::Mask &maskIn)
    {
      entries.push_back(Entry(idIn, maskIn));
    }
    
    boost::optional<msg::Mask> map(std::size_t idIn) const
    {
      for (const auto &e : entries)
      {
        if (e.id == idIn)
          return e.mask;
      }
      return boost::none;
    }
    
    boost::optional<std::size_t> map(const msg::Mask &maskIn) const
    {
      for (const auto &e : entries)
      {
        if (e.mask == maskIn)
          return e.id;
      }
      return boost::none;
    }
    
    bool verify() const
    {
      //ensure that all ids and masks are unique
      std::set<std::size_t> testIds;
      std::set<std::size_t> testMaskHashes;
      std::hash<msg::Mask> hashMaskFunction;
      for (const auto &e : entries)
      {
        auto idResult = testIds.insert(e.id);
        if (!idResult.second)
          return false;
        
        auto maskResult = testMaskHashes.insert(hashMaskFunction(e.mask));
        if (!maskResult.second)
          return false;
      }
      return true;
    }
  };
  
  struct Manager::Stow
  {
    srl::Cue cueAll; //The complete default menu.
    srl::Cue cueRead; //The menu, possibly customized, read from disk.
    Mapper mapper;
    boost::filesystem::path xsdPath;
    
    bool ensureSchema();
    void buildAll();
    void writeAll();
    void setAllMenu();
    void setAllToolbars();
    void setAllCommands();
  };
}

using namespace mnu;

Manager& mnu::manager()
{
  static Manager localManager;
  return localManager;
}

Manager::Manager()
:stow(std::make_unique<Manager::Stow>())
{
  stow->buildAll();
  stow->cueRead = stow->cueAll; //always default to the full menu.
  if (stow->ensureSchema())
    stow->writeAll();
}

void Manager::loadMenu(const std::string &fileName)
{
  auto sm = [&](const std::string &m, double t)
  {
    app::instance()->queuedMessage(msg::buildStatusMessage(m, t));
  };
  
  if (!boost::filesystem::exists(fileName))
  {
    sm("Menu file doesn't exists. Default menu will be used", 5.0);
    return;
  }
  
  if (!boost::filesystem::exists(stow->xsdPath))
  {
    sm("Menu schema file doesn't exists. Default menu will be used", 5.0);
    return;
  }
  
  try
  {
    xml_schema::Properties props;
    props.schema_location("http://www.cadseer.com/mnu/srl", stow->xsdPath.string());
    auto temp(srl::cue(fileName, 0, props));
    stow->cueRead = *temp;
  }
  catch (const xml_schema::Properties::argument&)
  {
    std::string m("invalid property argument (empty namespace or location)");
    m += ". Default menu will be used";
    sm(m, 5.0);
    std::cerr << m << std::endl;
  }
  catch (const xml_schema::Exception& e)
  {
    std::ostringstream stream;
    stream << e.what() << ". " << e << ". Default menu will be used";
    sm(stream.str(), 5.0);
    std::cerr << stream.str() << std::endl;
  }
  catch (const xsd::cxx::xml::invalid_utf16_string&)
  {
    std::string m("invalid UTF-16 text in DOM model. Default menu will be used");
    sm(m, 5.0);
    std::cerr << m << std::endl;
  }
  catch (const xsd::cxx::xml::invalid_utf8_string&)
  {
    std::string m("invalid UTF-8 text in object model. Default menu will be used");
    sm(m, 5.0);
  }
}

/*! @brief convert a command id into a command mask
 * 
 * @parameter idIn is id to locate
 * @return is a command mask associated to command id or 0(msg::Mask::none == true;) for no match. 
 */
msg::Mask Manager::getMask(std::size_t idIn) const
{
  auto result = stow->mapper.map(idIn);
  if (!result)
    return msg::Mask();
  return result.get();
}

/*! @brief get command from id
 * 
 * @parameter idIn is id to locate
 * @return is a command associated to command id or empty command for no match.
 * empty command has empty string for both text and icon and 0 for id.
 */
const srl::Command& Manager::getCommand(std::size_t idIn) const
{
  const static srl::Command dummy(0);
  for (const auto &c : stow->cueRead.commands())
  {
    if (c.id() == idIn)
      return c;
  }
  return dummy;
}

const srl::Cue& Manager::getCueRead() const
{
  return stow->cueRead;
}

bool Manager::Stow::ensureSchema()
{
  auto sm = [&](const std::string &m, double t)
  {
    app::instance()->queuedMessage(msg::buildStatusMessage(m, t));
  };
  
  xsdPath = app::instance()->getApplicationDirectory();
  xsdPath /= "menuserial.xsd";
  if (!boost::filesystem::exists(xsdPath))
  {
    //create file from qt resource.
    QString xsdResourceName(":/menu/mnuserial.xsd");
    QFile resourceFileXSD(xsdResourceName);
    if (!resourceFileXSD.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      sm("Couldn't open resource file. Default menu will be used", 5.0);
      return false;
    }
    QByteArray bufferXSD = resourceFileXSD.readAll();
    resourceFileXSD.close();
    
    QFile newFileXSD(QString::fromStdString(xsdPath.string()));
    if (!newFileXSD.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
      sm("Couldn't open new menuserial.xsd file. Default menu will be used", 5.0);
      return false;
    }
    newFileXSD.write(bufferXSD);
    newFileXSD.close();
  }
  
  return true;
}

void Manager::Stow::buildAll()
{
  setAllCommands();
  cueAll.toolbarSettings() = srl::ToolbarSettings(true, false, 32);
  setAllToolbars();
  cueAll.menuSettings() = srl::MenuSettings(srl::Gesture(0.2, 32, 120, 0.05, 1.5));
  setAllMenu();
  
  assert(mapper.verify());
}

void Manager::Stow::writeAll()
{
  boost::filesystem::path appDirectory = app::instance()->getApplicationDirectory();
  appDirectory /= "systemMenu.xml";
  xml_schema::NamespaceInfomap map;
  std::ofstream stream(appDirectory.string());
  srl::cue(stream, cueAll, map);
}

void Manager::Stow::setAllMenu()
{
  srl::MenuNode start(":/resources/images/start.svg", QObject::tr("Start Menu").toStdString());
  {
    srl::MenuNode viewBase(":/resources/images/viewBase.svg", QObject::tr("View Menu").toStdString());
    {
      {
        srl::MenuNode viewStandard(":/resources/images/viewStandard.svg", QObject::tr("Standard Views Menu").toStdString());
        viewStandard.commandIds().push_back(1);
        viewStandard.commandIds().push_back(2);
        viewStandard.commandIds().push_back(3);
        viewStandard.commandIds().push_back(4);
        viewBase.subMenus().push_back(viewStandard);
      }
      {
        srl::MenuNode viewStandardCurrent(":/resources/images/viewStandardCurrent.svg", QObject::tr("Current Views Menu").toStdString());
        viewStandardCurrent.commandIds().push_back(5);
        viewStandardCurrent.commandIds().push_back(6);
        viewStandardCurrent.commandIds().push_back(7);
        viewBase.subMenus().push_back(viewStandardCurrent);
      }
      viewBase.commandIds().push_back(8);
      viewBase.commandIds().push_back(10);
      viewBase.commandIds().push_back(11);
      viewBase.commandIds().push_back(9);
      viewBase.commandIds().push_back(12);
      viewBase.commandIds().push_back(13);
      start.subMenus().push_back(viewBase);
    }
    
    srl::MenuNode constructionBase(":/resources/images/constructionBase.svg", QObject::tr("Construction Menu").toStdString());
    {
      {
        srl::MenuNode constructionPrimitives(":/resources/images/constructionPrimitives.svg", QObject::tr("Primitives Menu").toStdString());
        constructionPrimitives.commandIds().push_back(14);
        constructionPrimitives.commandIds().push_back(15);
        constructionPrimitives.commandIds().push_back(16);
        constructionPrimitives.commandIds().push_back(17);
        constructionPrimitives.commandIds().push_back(18);
        constructionPrimitives.commandIds().push_back(19);
        constructionPrimitives.commandIds().push_back(20);
        constructionBase.subMenus().push_back(constructionPrimitives);
      }
      {
        srl::MenuNode constructionFinishing(":/resources/images/constructionFinishing.svg", QObject::tr("Finishing Menu").toStdString());
        constructionFinishing.commandIds().push_back(21);
        constructionFinishing.commandIds().push_back(22);
        constructionFinishing.commandIds().push_back(23);
        constructionFinishing.commandIds().push_back(24);
        constructionFinishing.commandIds().push_back(25);
        constructionBase.subMenus().push_back(constructionFinishing);
      }
      {
        srl::MenuNode constructionUtility(":/resources/images/constructionOffset.svg", QObject::tr("Utility Menu").toStdString());
        constructionUtility.commandIds().push_back(28);
        constructionUtility.commandIds().push_back(27);
        constructionUtility.commandIds().push_back(26);
        constructionUtility.commandIds().push_back(34);
        constructionUtility.commandIds().push_back(35);
        constructionUtility.commandIds().push_back(36);
        constructionUtility.commandIds().push_back(40);
        constructionBase.subMenus().push_back(constructionUtility);
      }
      {
        srl::MenuNode constructionDatum(":/resources/images/constructionDatum.svg", QObject::tr("Datum Menu").toStdString());
        constructionDatum.commandIds().push_back(33);
        constructionDatum.commandIds().push_back(32);
        constructionDatum.commandIds().push_back(93);
        constructionBase.subMenus().push_back(constructionDatum);
      }
      {
        srl::MenuNode constructionBoolean(":/resources/images/constructionBoolean.svg", QObject::tr("Boolean Menu").toStdString());
        constructionBoolean.commandIds().push_back(29);
        constructionBoolean.commandIds().push_back(30);
        constructionBoolean.commandIds().push_back(31);
        constructionBase.subMenus().push_back(constructionBoolean);
      }
      {
        srl::MenuNode constructionInstance(":/resources/images/constructionInstance.svg", QObject::tr("Instance Menu").toStdString());
        constructionInstance.commandIds().push_back(37);
        constructionInstance.commandIds().push_back(38);
        constructionInstance.commandIds().push_back(39);
        constructionBase.subMenus().push_back(constructionInstance);
      }
      {
        srl::MenuNode constructionCurves(":/resources/images/constructionCurves.svg", QObject::tr("Curves Menu").toStdString());
        constructionCurves.commandIds().push_back(41);
        constructionCurves.commandIds().push_back(42);
        constructionCurves.commandIds().push_back(96);
        constructionBase.subMenus().push_back(constructionCurves);
      }
      {
        srl::MenuNode constructionSurface(":/resources/images/constructionSurface.svg", QObject::tr("Surface Menu").toStdString());
        constructionSurface.commandIds().push_back(43);
        constructionSurface.commandIds().push_back(44);
        constructionSurface.commandIds().push_back(90);
        constructionSurface.commandIds().push_back(91);
        constructionSurface.commandIds().push_back(92);
        constructionBase.subMenus().push_back(constructionSurface);
      }
      {
        srl::MenuNode constructionMesh(":/resources/images/constructionMesh.svg", QObject::tr("Mesh Menu").toStdString());
        constructionMesh.commandIds().push_back(45);
        constructionMesh.commandIds().push_back(94);
        constructionMesh.commandIds().push_back(95);
        constructionBase.subMenus().push_back(constructionMesh);
      }
      {
        srl::MenuNode constructionDie(":/resources/images/constructionDie.svg", QObject::tr("Stamping Die Menu").toStdString());
        constructionDie.commandIds().push_back(46);
        constructionDie.commandIds().push_back(47);
        constructionDie.commandIds().push_back(48);
        constructionDie.commandIds().push_back(49);
        constructionDie.commandIds().push_back(50);
        constructionBase.subMenus().push_back(constructionDie);
      }
      {
        srl::MenuNode systemBase(":/resources/images/systemBase.svg", QObject::tr("Coordinate System Menu").toStdString());
        systemBase.commandIds().push_back(72);
        systemBase.commandIds().push_back(73);
        systemBase.commandIds().push_back(74);
        systemBase.commandIds().push_back(75);
        constructionBase.subMenus().push_back(systemBase);
      }
      
      start.subMenus().push_back(constructionBase);
    }
    
    srl::MenuNode editBase(":/resources/images/editBase.svg", QObject::tr("Edit Menu").toStdString());
    {
      {
        srl::MenuNode editSystemBase(":/resources/images/systemBase.svg", QObject::tr("Edit Coordinate System Menu").toStdString());
        editSystemBase.commandIds().push_back(59);
        editSystemBase.commandIds().push_back(60);
        editSystemBase.commandIds().push_back(61);
        editSystemBase.commandIds().push_back(62);
        editBase.subMenus().push_back(editSystemBase);
      }

      editBase.commandIds().push_back(63);
      editBase.commandIds().push_back(51);
      editBase.commandIds().push_back(52);
      editBase.commandIds().push_back(53);
      editBase.commandIds().push_back(54);
      editBase.commandIds().push_back(55);
      editBase.commandIds().push_back(56);
      editBase.commandIds().push_back(57);
      editBase.commandIds().push_back(58);
      start.subMenus().push_back(editBase);
    }
    
    srl::MenuNode fileBase(":/resources/images/fileBase.svg", QObject::tr("File Menu").toStdString());
    {
      fileBase.commandIds().push_back(67);
      fileBase.commandIds().push_back(69);
      fileBase.commandIds().push_back(64);
      fileBase.commandIds().push_back(65);
      fileBase.commandIds().push_back(66);
      start.subMenus().push_back(fileBase);
    }
    
    srl::MenuNode inspectBase(":/resources/images/inspectBase.svg", QObject::tr("Inspect Menu").toStdString());
    {
      srl::MenuNode inspectMeasureBase(":/resources/images/inspectMeasureBase.svg", QObject::tr("Measure Menu").toStdString());
      {
        inspectMeasureBase.commandIds().push_back(79);
        inspectMeasureBase.commandIds().push_back(80);
        inspectBase.subMenus().push_back(inspectMeasureBase);
      }
      
      srl::MenuNode debugBase(":/resources/images/debugBase.svg", QObject::tr("Debug Menu").toStdString());
      {
        debugBase.commandIds().push_back(81);
        debugBase.commandIds().push_back(82);
        debugBase.commandIds().push_back(83);
        debugBase.commandIds().push_back(85);
        debugBase.commandIds().push_back(86);
        debugBase.commandIds().push_back(87);
        debugBase.commandIds().push_back(88);
        debugBase.commandIds().push_back(89);
        inspectBase.subMenus().push_back(debugBase);
      }
      
      inspectBase.commandIds().push_back(76);
      inspectBase.commandIds().push_back(77);
      inspectBase.commandIds().push_back(78);
      start.subMenus().push_back(inspectBase);
    }
  }
  
  cueAll.gestureNode() = start;
}

void Manager::Stow::setAllToolbars()
{
  {
    srl::Toolbar toolbar(":/resources/images/viewBase.svg", QObject::tr("View").toStdString());
    {
      //fit
      srl::ToolbarEntry entry;
      entry.commandIds().push_back(8);
      toolbar.entries().push_back(entry);
    }
    {
      //standard views
      srl::ToolbarEntry entry;
      entry.visual() = srl::Visual();
      entry.visual().get().icon() = ":/resources/images/viewStandard.svg";
      entry.visual().get().iconText() = QObject::tr("Standard").toStdString();
      entry.visual().get().statusText() = QObject::tr("Standard Views").toStdString();
      entry.visual().get().whatThisText() = QObject::tr("Standard Views Menu").toStdString();
      entry.visual().get().toolTipText() = QObject::tr("Standard Views").toStdString();
      entry.commandIds().push_back(1);
      entry.commandIds().push_back(2);
      entry.commandIds().push_back(3);
      entry.commandIds().push_back(4);
      toolbar.entries().push_back(entry);
    }
    {
      //standard current views
      srl::ToolbarEntry entry;
      entry.visual() = srl::Visual();
      entry.visual().get().icon() = ":/resources/images/viewStandardCurrent.svg";
      entry.visual().get().iconText() = QObject::tr("Current").toStdString();
      entry.visual().get().statusText() = QObject::tr("Current Standard Views").toStdString();
      entry.visual().get().whatThisText() = QObject::tr("Standard Views Menu Relative To Current System").toStdString();
      entry.visual().get().toolTipText() = QObject::tr("Current Standard Views").toStdString();
      entry.commandIds().push_back(5);
      entry.commandIds().push_back(6);
      entry.commandIds().push_back(7);
      toolbar.entries().push_back(entry);
    }
    {
      //render style
      srl::ToolbarEntry entry;
      entry.visual() = srl::Visual();
      entry.visual().get().icon() = ":/resources/images/renderStyleToggle.svg";
      entry.visual().get().iconText() = QObject::tr("Render Style").toStdString();
      entry.visual().get().statusText() = QObject::tr("Render Style").toStdString();
      entry.visual().get().whatThisText() = QObject::tr("Render Style Menu").toStdString();
      entry.visual().get().toolTipText() = QObject::tr("Render Views").toStdString();
      entry.commandIds().push_back(9);
      entry.commandIds().push_back(10);
      entry.commandIds().push_back(11);
      toolbar.entries().push_back(entry);
    }
    {
      //toggle hidden lines
      srl::ToolbarEntry entry;
      entry.commandIds().push_back(12);
      toolbar.entries().push_back(entry);
    }
    {
      //view isolate
      srl::ToolbarEntry entry;
      entry.commandIds().push_back(13);
      toolbar.entries().push_back(entry);
    }
    
    cueAll.toolbars().push_back(toolbar);
  }
  {
    srl::Toolbar toolbar(":/resources/images/constructionBase.svg", QObject::tr("Construction").toStdString());
    {
      //primitives
      srl::ToolbarEntry entry;
      entry.visual() = srl::Visual();
      entry.visual().get().icon() = ":/resources/images/constructionPrimitives.svg";
      entry.visual().get().iconText() = QObject::tr("Primitives").toStdString();
      entry.visual().get().statusText() = QObject::tr("Construct Primitives").toStdString();
      entry.visual().get().whatThisText() = QObject::tr("Construct Primitives Menu").toStdString();
      entry.visual().get().toolTipText() = QObject::tr("Construct Primitives").toStdString();
      entry.commandIds().push_back(14);
      entry.commandIds().push_back(17);
      entry.commandIds().push_back(15);
      entry.commandIds().push_back(16);
      entry.commandIds().push_back(18);
      entry.commandIds().push_back(19);
      entry.commandIds().push_back(20);
      toolbar.entries().push_back(entry);
    }
    {
      //finishing
      srl::ToolbarEntry entry;
      entry.visual() = srl::Visual();
      entry.visual().get().icon() = ":/resources/images/constructionFinishing.svg";
      entry.visual().get().iconText() = QObject::tr("Finishing").toStdString();
      entry.visual().get().statusText() = QObject::tr("Finishing").toStdString();
      entry.visual().get().whatThisText() = QObject::tr("Finishing Menu").toStdString();
      entry.visual().get().toolTipText() = QObject::tr("Finishing").toStdString();
      entry.commandIds().push_back(21);
      entry.commandIds().push_back(22);
      entry.commandIds().push_back(23);
      entry.commandIds().push_back(24);
      entry.commandIds().push_back(25);
      toolbar.entries().push_back(entry);
    }
    {
      //utility
      srl::ToolbarEntry entry;
      entry.visual() = srl::Visual();
      //TODO create utility icon.
      entry.visual().get().icon() = ":/resources/images/constructionOffset.svg";
      entry.visual().get().iconText() = QObject::tr("Utility").toStdString();
      entry.visual().get().statusText() = QObject::tr("Utility").toStdString();
      entry.visual().get().whatThisText() = QObject::tr("Utility Menu").toStdString();
      entry.visual().get().toolTipText() = QObject::tr("Utility").toStdString();
      entry.commandIds().push_back(28);
      entry.commandIds().push_back(27);
      entry.commandIds().push_back(26);
      entry.commandIds().push_back(34);
      entry.commandIds().push_back(35);
      entry.commandIds().push_back(36);
      entry.commandIds().push_back(40);
      toolbar.entries().push_back(entry);
    }
    {
      //datum
      srl::ToolbarEntry entry;
      entry.visual() = srl::Visual();
      entry.visual().get().icon() = ":/resources/images/constructionDatum.svg";
      entry.visual().get().iconText() = QObject::tr("Datum").toStdString();
      entry.visual().get().statusText() = QObject::tr("Datum").toStdString();
      entry.visual().get().whatThisText() = QObject::tr("Datum Menu").toStdString();
      entry.visual().get().toolTipText() = QObject::tr("Datum").toStdString();
      entry.commandIds().push_back(33); //axis
      entry.commandIds().push_back(32); //plane
      entry.commandIds().push_back(93); //system
      toolbar.entries().push_back(entry);
    }
    {
      //boolean
      srl::ToolbarEntry entry;
      entry.visual() = srl::Visual();
      entry.visual().get().icon() = ":/resources/images/constructionBoolean.svg";
      entry.visual().get().iconText() = QObject::tr("Boolean").toStdString();
      entry.visual().get().statusText() = QObject::tr("Boolean").toStdString();
      entry.visual().get().whatThisText() = QObject::tr("Boolean Menu").toStdString();
      entry.visual().get().toolTipText() = QObject::tr("Boolean").toStdString();
      entry.commandIds().push_back(30);
      entry.commandIds().push_back(29);
      entry.commandIds().push_back(31);
      toolbar.entries().push_back(entry);
    }
    {
      //instance
      srl::ToolbarEntry entry;
      entry.visual() = srl::Visual();
      entry.visual().get().icon() = ":/resources/images/constructionInstance.svg";
      entry.visual().get().iconText() = QObject::tr("Instance").toStdString();
      entry.visual().get().statusText() = QObject::tr("Instance").toStdString();
      entry.visual().get().whatThisText() = QObject::tr("Instance Menu").toStdString();
      entry.visual().get().toolTipText() = QObject::tr("Instance").toStdString();
      entry.commandIds().push_back(37);
      entry.commandIds().push_back(38);
      entry.commandIds().push_back(39);
      toolbar.entries().push_back(entry);
    }
    {
      //curves
      srl::ToolbarEntry entry;
      entry.visual() = srl::Visual();
      entry.visual().get().icon() = ":/resources/images/constructionCurves.svg";
      entry.visual().get().iconText() = QObject::tr("Curves").toStdString();
      entry.visual().get().statusText() = QObject::tr("Curves").toStdString();
      entry.visual().get().whatThisText() = QObject::tr("Curves Menu").toStdString();
      entry.visual().get().toolTipText() = QObject::tr("Curves").toStdString();
      entry.commandIds().push_back(41);
      entry.commandIds().push_back(42);
      entry.commandIds().push_back(96);
      toolbar.entries().push_back(entry);
    }
    {
      //surfaces
      srl::ToolbarEntry entry;
      entry.visual() = srl::Visual();
      entry.visual().get().icon() = ":/resources/images/constructionSurface.svg";
      entry.visual().get().iconText() = QObject::tr("Surface").toStdString();
      entry.visual().get().statusText() = QObject::tr("Surface").toStdString();
      entry.visual().get().whatThisText() = QObject::tr("Surface Menu").toStdString();
      entry.visual().get().toolTipText() = QObject::tr("Surface").toStdString();
      entry.commandIds().push_back(43);
      entry.commandIds().push_back(44);
      entry.commandIds().push_back(90);
      entry.commandIds().push_back(91);
      entry.commandIds().push_back(92);
      toolbar.entries().push_back(entry);
    }
    {
      //mesh
      srl::ToolbarEntry entry;
      entry.visual() = srl::Visual();
      entry.visual().get().icon() = ":/resources/images/constructionMesh.svg";
      entry.visual().get().iconText() = QObject::tr("Mesh").toStdString();
      entry.visual().get().statusText() = QObject::tr("Mesh").toStdString();
      entry.visual().get().whatThisText() = QObject::tr("Mesh Menu").toStdString();
      entry.visual().get().toolTipText() = QObject::tr("Mesh").toStdString();
      entry.commandIds().push_back(45);
      entry.commandIds().push_back(94);
      entry.commandIds().push_back(95);
      toolbar.entries().push_back(entry);
    }
    {
      //die
      srl::ToolbarEntry entry;
      entry.visual() = srl::Visual();
      entry.visual().get().icon() = ":/resources/images/constructionDie.svg";
      entry.visual().get().iconText() = QObject::tr("Die").toStdString();
      entry.visual().get().statusText() = QObject::tr("Die").toStdString();
      entry.visual().get().whatThisText() = QObject::tr("Stamping Die Menu").toStdString();
      entry.visual().get().toolTipText() = QObject::tr("Die").toStdString();
      entry.commandIds().push_back(46);
      entry.commandIds().push_back(47);
      entry.commandIds().push_back(48);
      entry.commandIds().push_back(49);
      entry.commandIds().push_back(50);
      toolbar.entries().push_back(entry);
    }
    {
      //current coordinate system
      srl::ToolbarEntry entry;
      entry.visual() = srl::Visual();
      entry.visual().get().icon() = ":/resources/images/systemBase.svg";
      entry.visual().get().iconText() = QObject::tr("System").toStdString();
      entry.visual().get().statusText() = QObject::tr("System").toStdString();
      entry.visual().get().whatThisText() = QObject::tr("Coordinate System Menu").toStdString();
      entry.visual().get().toolTipText() = QObject::tr("System").toStdString();
      entry.commandIds().push_back(72);
      entry.commandIds().push_back(73);
      entry.commandIds().push_back(74);
      entry.commandIds().push_back(75);
      toolbar.entries().push_back(entry);
    }
    cueAll.toolbars().push_back(toolbar);
  }
  {
    srl::Toolbar toolbar(":/resources/images/editBase.svg", QObject::tr("Edit").toStdString());
    {
      auto addCommand = [&](std::size_t id)
      {
        srl::ToolbarEntry entry;
        entry.commandIds().push_back(id);
        toolbar.entries().push_back(entry);
      };
      
      addCommand(52); //edit feature
      addCommand(54); //edit color
      addCommand(55); //edit rename
      {
        //coordinate system
        srl::ToolbarEntry entry;
        entry.visual() = srl::Visual();
        entry.visual().get().icon() = ":/resources/images/systemBase.svg";
        entry.visual().get().iconText() = QObject::tr("System").toStdString();
        entry.visual().get().statusText() = QObject::tr("System").toStdString();
        entry.visual().get().whatThisText() = QObject::tr("Coordinate System Menu").toStdString();
        entry.visual().get().toolTipText() = QObject::tr("System").toStdString();
        entry.commandIds().push_back(59);
        entry.commandIds().push_back(60);
        entry.commandIds().push_back(61);
        entry.commandIds().push_back(62);
        toolbar.entries().push_back(entry);
      }
      addCommand(58); //dissolve feature
      addCommand(53); //remove feature
      addCommand(56); //project update
      addCommand(57); //project force update
      addCommand(63); //preferences
      addCommand(51); //cancel command
    }
    cueAll.toolbars().push_back(toolbar);
  }
  {
    srl::Toolbar toolbar(":/resources/images/fileBase.svg", QObject::tr("File").toStdString());
    {
      auto addCommand = [&](std::size_t id)
      {
        srl::ToolbarEntry entry;
        entry.commandIds().push_back(id);
        toolbar.entries().push_back(entry);
      };
      
      addCommand(64); //open
      addCommand(65); //save
      addCommand(66); //revisions
      {
        //import
        srl::ToolbarEntry entry;
        entry.visual() = srl::Visual();
        entry.visual().get().icon() = ":/resources/images/fileImport.svg";
        entry.visual().get().iconText() = QObject::tr("Import").toStdString();
        entry.visual().get().statusText() = QObject::tr("Import File").toStdString();
        entry.visual().get().whatThisText() = QObject::tr("Import File").toStdString();
        entry.visual().get().toolTipText() = QObject::tr("Import File").toStdString();
        entry.commandIds().push_back(67);
        toolbar.entries().push_back(entry);
      }
      {
        //export
        srl::ToolbarEntry entry;
        entry.visual() = srl::Visual();
        entry.visual().get().icon() = ":/resources/images/fileExport.svg";
        entry.visual().get().iconText() = QObject::tr("Export").toStdString();
        entry.visual().get().statusText() = QObject::tr("Export File").toStdString();
        entry.visual().get().whatThisText() = QObject::tr("Export File").toStdString();
        entry.visual().get().toolTipText() = QObject::tr("Export File").toStdString();
        entry.commandIds().push_back(69);
        toolbar.entries().push_back(entry);
      }

    }
    cueAll.toolbars().push_back(toolbar);
  }
  {
    srl::Toolbar toolbar(":/resources/images/inspectBase.svg", QObject::tr("Inspect").toStdString());
    {
      auto addCommand = [&](std::size_t id)
      {
        srl::ToolbarEntry entry;
        entry.commandIds().push_back(id);
        toolbar.entries().push_back(entry);
      };
      
      addCommand(76);
      addCommand(77);
      addCommand(78);
      addCommand(80);
      addCommand(79);
      {
        //debug
        srl::ToolbarEntry entry;
        entry.visual() = srl::Visual();
        entry.visual().get().icon() = ":/resources/images/debugBase.svg";
        entry.visual().get().iconText() = QObject::tr("Debug").toStdString();
        entry.visual().get().statusText() = QObject::tr("Debug").toStdString();
        entry.visual().get().whatThisText() = QObject::tr("Debug Menu").toStdString();
        entry.visual().get().toolTipText() = QObject::tr("Debug").toStdString();
        entry.commandIds().push_back(81);
        entry.commandIds().push_back(82);
        entry.commandIds().push_back(83);
        entry.commandIds().push_back(85);
        entry.commandIds().push_back(86);
        entry.commandIds().push_back(87);
        entry.commandIds().push_back(88);
        entry.commandIds().push_back(89);
        toolbar.entries().push_back(entry);
      }
    }
    cueAll.toolbars().push_back(toolbar);
  }
}

void Manager::Stow::setAllCommands()
{
  auto sc = [&]
  (
    std::size_t id
    , const std::string &icon
    , const std::string &iconText
    , const std::string &statusText
    , const std::string &whatsThisText
    , const std::string &toolTipText
    , msg::Mask mask
  )
  {
    srl::Command c(id);
    srl::Visual v;
    v.icon() = icon;
    v.iconText() = iconText;
    v.statusText() = statusText;
    v.whatThisText() = whatsThisText;
    v.toolTipText() = toolTipText;
    c.visual() = v;
    cueAll.commands().push_back(c);
    mapper.insert(id, mask);
  };
  
  //never change the id, which is the 3rd argument. It will screw up users customized menu.
  //command with id of '0' is intended to be an error, so don't use '0'
  sc
  (
    1
    , ":/resources/images/viewTop.svg"
    , QObject::tr("Top").toStdString() //icon text
    , QObject::tr("Sets The View To The Top").toStdString() //status text
    , QObject::tr("Sets The View To Look Straight Down (z negative direction)").toStdString() //whats this text
    , QObject::tr("Sets The View To The Top").toStdString() // toolTipText
    , msg::Request | msg::View | msg::Top //mask
  );
  sc
  (
    2
    , ":/resources/images/viewFront.svg"
    , QObject::tr("Front").toStdString() //icon text
    , QObject::tr("Sets The View To The Front").toStdString() //status text
    , QObject::tr("Sets The View To Look Forward (y positive direction)").toStdString() //whats this text
    , QObject::tr("Sets The View To The Front").toStdString() // toolTipText
    , msg::Request | msg::View | msg::Front //mask
  );
  sc
  (
    3
    ,":/resources/images/viewRight.svg"
    , QObject::tr("Right").toStdString() //icon text
    , QObject::tr("Sets The View To The Right").toStdString() //status text
    , QObject::tr("Sets The View To Look Right (x negative direction)").toStdString() //whats this text
    , QObject::tr("Sets The View To The Right").toStdString() // toolTipText
    , msg::Request | msg::View | msg::Right
  );
  sc
  (
    4,
    ":/resources/images/viewIso.svg"
    , QObject::tr("Iso").toStdString() //icon text
    , QObject::tr("Sets The View To Isometric").toStdString() //status text
    , QObject::tr("Sets The View To Look Isometric (x negative, y positive, z negative direction)").toStdString() //whats this text
    , QObject::tr("Sets The View To The Isometric").toStdString() // toolTipText
    , msg::Request | msg::View | msg::Iso
  );
  sc
  (
    5
    , ":/resources/images/viewTopCurrent.svg"
    , QObject::tr("Top").toStdString() //icon text
    , QObject::tr("Sets The View To The Current Top").toStdString() //status text
    , QObject::tr("Sets The View To Look At The Current Coordinate System z negative").toStdString() //whats this text
    , QObject::tr("Sets The View To The Current Top").toStdString() // toolTipText
    , msg::Request | msg::View | msg::Top | msg::Current
  );
  sc
  (
    6
    , ":/resources/images/viewFrontCurrent.svg"
    , QObject::tr("Front").toStdString() //icon text
    , QObject::tr("Sets The View To The Current Front").toStdString() //status text
    , QObject::tr("Sets The View To Look At The Current Coordinate System y positive").toStdString() //whats this text
    , QObject::tr("Sets The View To The Current Front").toStdString() // toolTipText
    , msg::Request | msg::View | msg::Front | msg::Current
  );
  sc
  (
    7
    , ":/resources/images/viewRightCurrent.svg"
    , QObject::tr("Right").toStdString() //icon text
    , QObject::tr("Sets The View To The Current Right").toStdString() //status text
    , QObject::tr("Sets The View To Look At The Current Coordinate System x negative").toStdString() //whats this text
    , QObject::tr("Sets The View To The Current Right").toStdString() // toolTipText
    , msg::Request | msg::View | msg::Right | msg::Current
  );
  sc
  (
    8
    , ":/resources/images/viewFit.svg"
    , QObject::tr("Fit").toStdString() //icon text
    , QObject::tr("Fit The View").toStdString() //status text
    , QObject::tr("Fit The View To Visible Geometry").toStdString() //whats this text
    , QObject::tr("Fit The View").toStdString() // toolTipText
    , msg::Request | msg::View | msg::Fit
  );
  sc
  (
    9
    , ":/resources/images/renderStyleToggle.svg"
    , QObject::tr("Style").toStdString() //icon text
    , QObject::tr("Toggle Render Style").toStdString() //status text
    , QObject::tr("Toggle Through Render Styles").toStdString() //whats this text
    , QObject::tr("Toggle Render Style").toStdString() // toolTipText
    , msg::Request | msg::View | msg::RenderStyle | msg::Toggle
  );
  sc
  (
    10
    , ":/resources/images/viewFill.svg"
    , QObject::tr("Fill").toStdString() //icon text
    , QObject::tr("Render Style Fill").toStdString() //status text
    , QObject::tr("Sets The View To The Fill Render Style").toStdString() //whats this text
    , QObject::tr("Render Style Fill").toStdString() // toolTipText
    , msg::Request | msg::View | msg::Fill
  );
  sc
  (
    11
    , ":/resources/images/viewTriangulation.svg"
    , QObject::tr("Triangulation").toStdString() //icon text
    , QObject::tr("Render Style Triangulation").toStdString() //status text
    , QObject::tr("Sets The View To The Triangulation Render Style").toStdString() //whats this text
    , QObject::tr("Render Style Triangulation").toStdString() // toolTipText
    , msg::Request | msg::View | msg::Triangulation
  );
  sc
  (
    12
    , ":/resources/images/viewHiddenLines.svg"
    , QObject::tr("Hidden Lines").toStdString() //icon text
    , QObject::tr("Toggle Hidden Lines").toStdString() //status text
    , QObject::tr("Toggles The Display Of Hidden Lines").toStdString() //whats this text
    , QObject::tr("Toggle Hidden Lines").toStdString() // toolTipText
    , msg::Request | msg::View | msg::Toggle | msg::HiddenLine
  );
  sc
  (
    13
    , ":/resources/images/viewIsolate.svg"
    , QObject::tr("Isolate").toStdString() //icon text
    , QObject::tr("View Isolate Of Selection").toStdString() //status text
    , QObject::tr("Isolates The Current Selected Objects In The View").toStdString() //whats this text
    , QObject::tr("View Isolate Of Selection").toStdString() // toolTipText
    , msg::Request | msg::View | msg::ThreeD | msg::Overlay | msg::Isolate
  );
  sc
  (
    14
    , ":/resources/images/constructionBox.svg"
    , QObject::tr("Box").toStdString() //icon text
    , QObject::tr("Create Box").toStdString() //status text
    , QObject::tr("Create A Box").toStdString() //whats this text
    , QObject::tr("Create Box").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Box
  );
  sc
  (
    15
    , ":/resources/images/constructionSphere.svg"
    , QObject::tr("Sphere").toStdString() //icon text
    , QObject::tr("Create Sphere").toStdString() //status text
    , QObject::tr("Create A Sphere").toStdString() //whats this text
    , QObject::tr("Create Sphere").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Sphere
  );
  sc
  (
    16
    , ":/resources/images/constructionCone.svg"
    , QObject::tr("Cone").toStdString() //icon text
    , QObject::tr("Create Cone").toStdString() //status text
    , QObject::tr("Create A Cone").toStdString() //whats this text
    , QObject::tr("Create Cone").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Cone
  );
  sc
  (
    17
    , ":/resources/images/constructionCylinder.svg"
    , QObject::tr("Cylinder").toStdString() //icon text
    , QObject::tr("Create Cylinder").toStdString() //status text
    , QObject::tr("Create A Cylinder").toStdString() //whats this text
    , QObject::tr("Create Cylinder").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Cylinder
  );
  sc
  (
    18
    , ":/resources/images/constructionOblong.svg"
    , QObject::tr("Oblong").toStdString() //icon text
    , QObject::tr("Create Oblong").toStdString() //status text
    , QObject::tr("Create An Oblong").toStdString() //whats this text
    , QObject::tr("Create Oblong").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Oblong
  );
  sc
  (
    19
    , ":/resources/images/constructionTorus.svg"
    , QObject::tr("Torus").toStdString() //icon text
    , QObject::tr("Create Torus").toStdString() //status text
    , QObject::tr("Create A Torus").toStdString() //whats this text
    , QObject::tr("Create Torus").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Torus
  );
  sc
  (
    20
    , ":/resources/images/constructionThread.svg"
    , QObject::tr("Thread").toStdString() //icon text
    , QObject::tr("Create Thread").toStdString() //status text
    , QObject::tr("Create A Thread").toStdString() //whats this text
    , QObject::tr("Create Thread").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Thread
  );
  sc
  (
    21
    , ":/resources/images/constructionBlend.svg"
    , QObject::tr("Blend").toStdString() //icon text
    , QObject::tr("Create Blend").toStdString() //status text
    , QObject::tr("Blend Edges").toStdString() //whats this text
    , QObject::tr("Create Blend").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Blend
  );
  sc
  (
    22
    , ":/resources/images/constructionChamfer.svg"
    , QObject::tr("Chamfer").toStdString() //icon text
    , QObject::tr("Create Chamfer").toStdString() //status text
    , QObject::tr("Chamfer Edges").toStdString() //whats this text
    , QObject::tr("Create Chamfer").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Chamfer
  );
  sc
  (
    23
    , ":/resources/images/constructionDraft.svg"
    , QObject::tr("Draft").toStdString() //icon text
    , QObject::tr("Create Draft").toStdString() //status text
    , QObject::tr("Draft Faces From A Neutral Plane").toStdString() //whats this text
    , QObject::tr("Create Draft").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Draft
  );
  sc
  (
    24
    , ":/resources/images/constructionRefine.svg"
    , QObject::tr("Refine").toStdString() //icon text
    , QObject::tr("Create Refine").toStdString() //status text
    , QObject::tr("Removes Redundant Faces And Edges").toStdString() //whats this text
    , QObject::tr("Create Refine").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Refine
  );
  sc
  (
    25
    , ":/resources/images/constructionRemoveFaces.svg"
    , QObject::tr("Remove Faces").toStdString() //icon text
    , QObject::tr("Create Remove Faces").toStdString() //status text
    , QObject::tr("Remove Faces From A Model And Heal").toStdString() //whats this text
    , QObject::tr("Create Remove Faces").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::RemoveFaces
  );
  sc
  (
    26
    , ":/resources/images/constructionHollow.svg"
    , QObject::tr("Hollow").toStdString() //icon text
    , QObject::tr("Create Hollow").toStdString() //status text
    , QObject::tr("Hollow Out Faces Of A Model").toStdString() //whats this text
    , QObject::tr("Create Hollow").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Hollow
  );
  sc
  (
    27
    , ":/resources/images/constructionExtract.svg"
    , QObject::tr("Extract").toStdString() //icon text
    , QObject::tr("Create Extract").toStdString() //status text
    , QObject::tr("Create Copies Of Model Geometry").toStdString() //whats this text
    , QObject::tr("Create Extract").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Extract
  );
  sc
  (
    28
    , ":/resources/images/constructionOffset.svg"
    , QObject::tr("Offset").toStdString() //icon text
    , QObject::tr("Create Offset").toStdString() //status text
    , QObject::tr("Offset Faces Of A Model").toStdString() //whats this text
    , QObject::tr("Create Offset").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Offset
  );
  sc
  (
    29
    , ":/resources/images/constructionUnion.svg"
    , QObject::tr("Union").toStdString() //icon text
    , QObject::tr("Create Union").toStdString() //status text
    , QObject::tr("Merge Solid Bodies Together").toStdString() //whats this text
    , QObject::tr("Create Union").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Union
  );
  sc
  (
    30
    , ":/resources/images/constructionSubtract.svg"
    , QObject::tr("Subtract").toStdString() //icon text
    , QObject::tr("Create Subtract").toStdString() //status text
    , QObject::tr("Remove One Solid Body From Another").toStdString() //whats this text
    , QObject::tr("Create Subtract").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Subtract
  );
  sc
  (
    31
    , ":/resources/images/constructionIntersect.svg"
    , QObject::tr("Intersection").toStdString() //icon text
    , QObject::tr("Create Intersect").toStdString() //status text
    , QObject::tr("Find Common Space Between Two Solids").toStdString() //whats this text
    , QObject::tr("Create Intersect").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Intersect
  );
  sc
  (
    32
    , ":/resources/images/constructionDatumPlane.svg"
    , QObject::tr("Datum Plane").toStdString() //icon text
    , QObject::tr("Create Datum Plane").toStdString() //status text
    , QObject::tr("Create A Datum Plane").toStdString() //whats this text
    , QObject::tr("Create Datum Plane").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::DatumPlane
  );
  sc
  (
    33
    , ":/resources/images/constructionDatumAxis.svg"
    , QObject::tr("Datum Axis").toStdString() //icon text
    , QObject::tr("Create Datum Axis").toStdString() //status text
    , QObject::tr("Create A Datum Axis").toStdString() //whats this text
    , QObject::tr("Create Datum Axis").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::DatumAxis
  );
  sc
  (
    34
    , ":/resources/images/constructionSketch.svg"
    , QObject::tr("Sketch").toStdString() //icon text
    , QObject::tr("Create Sketch").toStdString() //status text
    , QObject::tr("Create A Sketch").toStdString() //whats this text
    , QObject::tr("Create Sketch").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Sketch
  );
  sc
  (
    35
    , ":/resources/images/constructionExtrude.svg"
    , QObject::tr("Extrude").toStdString() //icon text
    , QObject::tr("Create Extrude").toStdString() //status text
    , QObject::tr("Create An Extrusion").toStdString() //whats this text
    , QObject::tr("Create Extrude").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Extrude
  );
  sc
  (
    36
    , ":/resources/images/constructionRevolve.svg"
    , QObject::tr("Revolve").toStdString() //icon text
    , QObject::tr("Create Revolve").toStdString() //status text
    , QObject::tr("Create A Revolution").toStdString() //whats this text
    , QObject::tr("Create Revolve").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Revolve
  );
  sc
  (
    37
    , ":/resources/images/constructionInstanceLinear.svg"
    , QObject::tr("Linear").toStdString() //icon text
    , QObject::tr("Create Linear Instance").toStdString() //status text
    , QObject::tr("Create A Linear Instance").toStdString() //whats this text
    , QObject::tr("Create Linear Instance").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::InstanceLinear
  );
  sc
  (
    38
    , ":/resources/images/constructionInstancePolar.svg"
    , QObject::tr("Polar").toStdString() //icon text
    , QObject::tr("Create Polar Instance").toStdString() //status text
    , QObject::tr("Create A Polar Instance").toStdString() //whats this text
    , QObject::tr("Create Polar Instance").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::InstancePolar
  );
  sc
  (
    39
    , ":/resources/images/constructionInstanceMirror.svg"
    , QObject::tr("Mirror").toStdString() //icon text
    , QObject::tr("Create Mirror Instance").toStdString() //status text
    , QObject::tr("Create A Mirror Instance").toStdString() //whats this text
    , QObject::tr("Create Mirror Instance").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::InstanceMirror
  );
  sc
  (
    40
    , ":/resources/images/constructionImagePlane.svg"
    , QObject::tr("Image Plane").toStdString() //icon text
    , QObject::tr("Create Image Plane").toStdString() //status text
    , QObject::tr("Create An Image Plane").toStdString() //whats this text
    , QObject::tr("Create Image Plane").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::ImagePlane
  );
  sc
  (
    41
    , ":/resources/images/sketchLine.svg"
    , QObject::tr("Line").toStdString() //icon text
    , QObject::tr("Create Line").toStdString() //status text
    , QObject::tr("Create A Line").toStdString() //whats this text
    , QObject::tr("Create Line").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Line
  );
  sc
  (
    42
    , ":/resources/images/sketchBezeir.svg"
    , QObject::tr("Transition").toStdString() //icon text
    , QObject::tr("Create Transition Curve").toStdString() //status text
    , QObject::tr("Create A Transition Curve").toStdString() //whats this text
    , QObject::tr("Create Transition Curve").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::TransitionCurve
  );
  sc
  (
    43
    , ":/resources/images/constructionRuled.svg"
    , QObject::tr("Ruled").toStdString() //icon text
    , QObject::tr("Create Ruled Surface").toStdString() //status text
    , QObject::tr("Create A Ruled Surface").toStdString() //whats this text
    , QObject::tr("Create Ruled Surface").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Ruled
  );
  sc
  (
    44
    , ":/resources/images/constructionSweep.svg"
    , QObject::tr("Sweep").toStdString() //icon text
    , QObject::tr("Create Sweep Surface").toStdString() //status text
    , QObject::tr("Create A Sweep Surface").toStdString() //whats this text
    , QObject::tr("Create Sweep Surface").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Sweep
  );
  sc
  (
    45
    , ":/resources/images/constructionSurfaceMesh.svg"
    , QObject::tr("Surface").toStdString() //icon text
    , QObject::tr("Create Surface Mesh").toStdString() //status text
    , QObject::tr("Create A Surface Mesh").toStdString() //whats this text
    , QObject::tr("Create Surface Mesh").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::SurfaceMesh
  );
  sc
  (
    46
    , ":/resources/images/constructionSquash.svg"
    , QObject::tr("Squash").toStdString() //icon text
    , QObject::tr("Create Squash").toStdString() //status text
    , QObject::tr("Create A Flat Surface From A Shell").toStdString() //whats this text
    , QObject::tr("Create Squash").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Squash
  );
  sc
  (
    47
    , ":/resources/images/constructionStrip.svg"
    , QObject::tr("Strip").toStdString() //icon text
    , QObject::tr("Create Strip").toStdString() //status text
    , QObject::tr("Create Stamping Die Strip").toStdString() //whats this text
    , QObject::tr("Create Strip").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Strip
  );
  sc
  (
    48
    , ":/resources/images/constructionNest.svg"
    , QObject::tr("Nest").toStdString() //icon text
    , QObject::tr("Create Nest").toStdString() //status text
    , QObject::tr("Create Stamping Die Nest").toStdString() //whats this text
    , QObject::tr("Create Nest").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Nest
  );
  sc
  (
    49
    , ":/resources/images/constructionDieSet.svg"
    , QObject::tr("Die Set").toStdString() //icon text
    , QObject::tr("Create Die Set").toStdString() //status text
    , QObject::tr("Create Stamping Die Set").toStdString() //whats this text
    , QObject::tr("Create Die Set").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::DieSet
  );
  sc
  (
    50
    , ":/resources/images/constructionQuote.svg"
    , QObject::tr("Quote").toStdString() //icon text
    , QObject::tr("Create Quote").toStdString() //status text
    , QObject::tr("Create Stamping Die Quote").toStdString() //whats this text
    , QObject::tr("Create Quote").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Quote
  );
  sc
  (
    51
    , ":/resources/images/editCommandCancel.svg"
    , QObject::tr("Cancel").toStdString() //icon text
    , QObject::tr("Cancel Current Command").toStdString() //status text
    , QObject::tr("Cancel Current Command").toStdString() //whats this text
    , QObject::tr("Cancel Current Command").toStdString() // toolTipText
    , msg::Request | msg::Command | msg::Done
  );
  sc
  (
    52
    , ":/resources/images/editFeature.svg"
    , QObject::tr("Feature").toStdString() //icon text
    , QObject::tr("Edit Feature").toStdString() //status text
    , QObject::tr("Edit Feature").toStdString() //whats this text
    , QObject::tr("Edit Feature").toStdString() // toolTipText
    , msg::Request | msg::Edit | msg::Feature
  );
  sc
  (
    53
    , ":/resources/images/editRemove.svg"
    , QObject::tr("Remove").toStdString() //icon text
    , QObject::tr("Remove Feature").toStdString() //status text
    , QObject::tr("Remove Feature").toStdString() //whats this text
    , QObject::tr("Remove Feature").toStdString() // toolTipText
    , msg::Request | msg::Remove
  );
  sc
  (
    54
    , ":/resources/images/editColor.svg"
    , QObject::tr("Color").toStdString() //icon text
    , QObject::tr("Edit Color").toStdString() //status text
    , QObject::tr("Edit Object Color").toStdString() //whats this text
    , QObject::tr("Edit Color").toStdString() // toolTipText
    , msg::Request | msg::Edit | msg::Feature | msg::Color
  );
  sc
  (
    55
    , ":/resources/images/editRename.svg"
    , QObject::tr("Name").toStdString() //icon text
    , QObject::tr("Edit Name").toStdString() //status text
    , QObject::tr("Edit Feature Name").toStdString() //whats this text
    , QObject::tr("Edit Name").toStdString() // toolTipText
    , msg::Request | msg::Edit | msg::Feature | msg::Name
  );
  sc
  (
    56
    , ":/resources/images/editUpdate.svg"
    , QObject::tr("Update").toStdString() //icon text
    , QObject::tr("Update Project").toStdString() //status text
    , QObject::tr("Update The Currently Opened Project").toStdString() //whats this text
    , QObject::tr("Update Project").toStdString() // toolTipText
    , msg::Request | msg::Project | msg::Update
  );
  sc
  (
    57
    , ":/resources/images/editForceUpdate.svg"
    , QObject::tr("Force").toStdString() //icon text
    , QObject::tr("Force Update Project").toStdString() //status text
    , QObject::tr("Marks All Features Dirty And Updates The Currently Opened Project").toStdString() //whats this text
    , QObject::tr("Force Update Project").toStdString() // toolTipText
    , msg::Request | msg::Force | msg::Update
  );
  sc
  (
    58
    , ":/resources/images/editFeatureDissolve.svg"
    , QObject::tr("Dissolve").toStdString() //icon text
    , QObject::tr("Dissolve Feature").toStdString() //status text
    , QObject::tr("Remove Feature History And Makes An Inert Feature").toStdString() //whats this text
    , QObject::tr("Dissolve Feature").toStdString() // toolTipText
    , msg::Request | msg::Feature | msg::Dissolve
  );
  sc
  (
    59
    , ":/resources/images/editDraggerToFeature.svg"
    , QObject::tr("Drag To Feat").toStdString() //icon text
    , QObject::tr("Dragger To Feature").toStdString() //status text
    , QObject::tr("Put Dragger System To Feature System").toStdString() //whats this text
    , QObject::tr("Dragger To Feature").toStdString() // toolTipText
    , msg::Request | msg::DraggerToFeature
  );
  sc
  (
    60
    , ":/resources/images/editFeatureToDragger.svg"
    , QObject::tr("Feat To Drag").toStdString() //icon text
    , QObject::tr("Feature To Dragger").toStdString() //status text
    , QObject::tr("Put Feature System To Dragger System").toStdString() //whats this text
    , QObject::tr("Feature To Dragger").toStdString() // toolTipText
    , msg::Request | msg::FeatureToDragger
  );
  sc
  (
    61
    , ":/resources/images/featureToSystem.svg"
    , QObject::tr("Feat To Sys").toStdString() //icon text
    , QObject::tr("Feature To Current System").toStdString() //status text
    , QObject::tr("Put Feature System To Current System").toStdString() //whats this text
    , QObject::tr("Feature To Current System").toStdString() // toolTipText
    , msg::Request | msg::FeatureToSystem
  );
  sc
  (
    62
    , ":/resources/images/featureReposition.svg"
    , QObject::tr("Drag To Sys").toStdString() //icon text
    , QObject::tr("Dragger To System").toStdString() //status text
    , QObject::tr("Put Dragger To Current System").toStdString() //whats this text
    , QObject::tr("Dragger To System").toStdString() // toolTipText
    , msg::Request | msg::FeatureReposition
  );
  sc
  (
    63
    , ":/resources/images/preferences.svg"
    , QObject::tr("Preferences").toStdString() //icon text
    , QObject::tr("Edit Preferences").toStdString() //status text
    , QObject::tr("Opens Preferences Dialog").toStdString() //whats this text
    , QObject::tr("Edit Preferences").toStdString() // toolTipText
    , msg::Request | msg::Preferences
  );
  sc
  (
    64
    , ":/resources/images/fileOpen.svg"
    , QObject::tr("Open").toStdString() //icon text
    , QObject::tr("Open Project").toStdString() //status text
    , QObject::tr("Opens Project Dialog To Open Or Create A Project").toStdString() //whats this text
    , QObject::tr("Open Project").toStdString() // toolTipText
    , msg::Request | msg::Project | msg::Dialog
  );
  sc
  (
    65
    , ":/resources/images/fileSave.svg"
    , QObject::tr("Save").toStdString() //icon text
    , QObject::tr("Save Project").toStdString() //status text
    , QObject::tr("Saves The Currently Loaded Project").toStdString() //whats this text
    , QObject::tr("Save Project").toStdString() // toolTipText
    , msg::Request | msg::Save | msg::Project
  );
  sc
  (
    66
    , ":/resources/images/fileRevision.svg"
    , QObject::tr("Revisions").toStdString() //icon text
    , QObject::tr("Project Revisions").toStdString() //status text
    , QObject::tr("Open Project Revisions Dialog").toStdString() //whats this text
    , QObject::tr("Project Revisions").toStdString() // toolTipText
    , msg::Request | msg::Project | msg::Revision | msg::Dialog
  );
  sc
  (
    67
    , ":/resources/images/fileImport.svg"
    , QObject::tr("Import").toStdString() //icon text
    , QObject::tr("Import File").toStdString() //status text
    , QObject::tr("Import File").toStdString() //whats this text
    , QObject::tr("Import File").toStdString() // toolTipText
    , msg::Request | msg::Import
  );
  sc
  (
    69
    , ":/resources/images/fileExport.svg"
    , QObject::tr("Export").toStdString() //icon text
    , QObject::tr("Export File").toStdString() //status text
    , QObject::tr("Exports File").toStdString() //whats this text
    , QObject::tr("Export File").toStdString() // toolTipText
    , msg::Request | msg::Export
  );
  sc
  (
    72
    , ":/resources/images/systemReset.svg"
    , QObject::tr("Reset").toStdString() //icon text
    , QObject::tr("Reset Current System").toStdString() //status text
    , QObject::tr("Resets The Current System To Absolute").toStdString() //whats this text
    , QObject::tr("Reset Current System").toStdString() // toolTipText
    , msg::Request | msg::SystemReset
  );
  sc
  (
    73
    , ":/resources/images/systemToggle.svg"
    , QObject::tr("Visibility").toStdString() //icon text
    , QObject::tr("System Visibility").toStdString() //status text
    , QObject::tr("Toggles The System Visibility").toStdString() //whats this text
    , QObject::tr("System Visibility").toStdString() // toolTipText
    , msg::Request | msg::SystemToggle
  );
  sc
  (
    74
    , ":/resources/images/systemToFeature.svg"
    , QObject::tr("Sys To Feat").toStdString() //icon text
    , QObject::tr("System To Feature").toStdString() //status text
    , QObject::tr("Put The Current System To A Feature").toStdString() //whats this text
    , QObject::tr("System To Feature").toStdString() // toolTipText
    , msg::Request | msg::SystemToFeature
  );
  sc
  (
    75
    , ":/resources/images/systemToSelection.svg"
    , QObject::tr("Sys To Select").toStdString() //icon text
    , QObject::tr("System To Selection").toStdString() //status text
    , QObject::tr("Put The Current System To Selection").toStdString() //whats this text
    , QObject::tr("System To Selection").toStdString() // toolTipText
    , msg::Request | msg::SystemToSelection
  );
  sc
  (
    76
    , ":/resources/images/inspectAbout.svg"
    , QObject::tr("About").toStdString() //icon text
    , QObject::tr("About CadSeer").toStdString() //status text
    , QObject::tr("Opens The About Cadseer Dialog").toStdString() //whats this text
    , QObject::tr("About CadSeer").toStdString() // toolTipText
    , msg::Request | msg::About | msg::Dialog
  );
  sc
  (
    77
    , ":/resources/images/inspectInfo.svg"
    , QObject::tr("Info").toStdString() //icon text
    , QObject::tr("Information").toStdString() //status text
    , QObject::tr("Information About Project Or Current Selection").toStdString() //whats this text
    , QObject::tr("Information").toStdString() // toolTipText
    , msg::Request | msg::Info
  );
  sc
  (
    78
    , ":/resources/images/inspectCheckGeometry.svg"
    , QObject::tr("Check Geometry").toStdString() //icon text
    , QObject::tr("Check Geometry").toStdString() //status text
    , QObject::tr("Information And Validation About Feature Geometry").toStdString() //whats this text
    , QObject::tr("Check Geometry").toStdString() // toolTipText
    , msg::Request | msg::CheckGeometry
  );
  sc
  (
    79
    , ":/resources/images/inspectMeasureClear.svg"
    , QObject::tr("Clear").toStdString() //icon text
    , QObject::tr("Clear Measure").toStdString() //status text
    , QObject::tr("Clears All The Measures From Current View").toStdString() //whats this text
    , QObject::tr("Clear Measure").toStdString() // toolTipText
    , msg::Request | msg::Clear | msg::Overlay
  );
  sc
  (
    80
    , ":/resources/images/inspectLinearMeasure.svg"
    , QObject::tr("Linear").toStdString() //icon text
    , QObject::tr("Linear Measure").toStdString() //status text
    , QObject::tr("Creates Linear Measure").toStdString() //whats this text
    , QObject::tr("Linear Measure").toStdString() // toolTipText
    , msg::Request | msg::LinearMeasure
  );
  sc
  (
    81
    , ":/resources/images/debugCheckShapeIds.svg"
    , QObject::tr("Shaped Ids").toStdString() //icon text
    , QObject::tr("Check Shaped Ids").toStdString() //status text
    , QObject::tr("Developer Tool").toStdString() //whats this text
    , QObject::tr("Check Shaped Ids").toStdString() // toolTipText
    , msg::Request | msg::CheckShapeIds
  );
  sc
  (
    82
    , ":/resources/images/debugDump.svg"
    , QObject::tr("Dump").toStdString() //icon text
    , QObject::tr("Feature Dump").toStdString() //status text
    , QObject::tr("Developer Tool To Dump Feature Internals").toStdString() //whats this text
    , QObject::tr("Feature Dump").toStdString() // toolTipText
    , msg::Request | msg::Feature | msg::Dump
  );
  sc
  (
    83
    , ":/resources/images/debugShapeTrackUp.svg"
    , QObject::tr("Track Shape").toStdString() //icon text
    , QObject::tr("Track Shape").toStdString() //status text
    , QObject::tr("Developer Tool To Track Shapes Up And Down The History Graph").toStdString() //whats this text
    , QObject::tr("Track Shape").toStdString() // toolTipText
    , msg::Request | msg::Shape | msg::Track | msg::Dump
  );
  sc
  (
    85
    , ":/resources/images/debugShapeGraph.svg"
    , QObject::tr("Shape dot").toStdString() //icon text
    , QObject::tr("Shape dot").toStdString() //status text
    , QObject::tr("Developer Tool To Generate Shape Topology").toStdString() //whats this text
    , QObject::tr("Shape dot").toStdString() // toolTipText
    , msg::Request | msg::Shape | msg::Graph | msg::Dump
  );
  sc
  (
    86
    , ":/resources/images/debugDumpProjectGraph.svg"
    , QObject::tr("Project dot").toStdString() //icon text
    , QObject::tr("Project dot").toStdString() //status text
    , QObject::tr("Developer Tool To Generate Graphviz Project Feature Graph").toStdString() //whats this text
    , QObject::tr("Project dot").toStdString() // toolTipText
    , msg::Request | msg::Project | msg::Graph | msg::Dump
  );
  sc
  (
    87
    , ":/resources/images/debugDumpDAGViewGraph.svg"
    , QObject::tr("DAGView dot").toStdString() //icon text
    , QObject::tr("DAGView dot").toStdString() //status text
    , QObject::tr("Developer Tool To Generate Graphviz DAGView Feature Graph").toStdString() //whats this text
    , QObject::tr("DAGView dot").toStdString() // toolTipText
    , msg::Request | msg::DAG | msg::Graph | msg::Dump
  );
  sc
  (
    88
    , ":/resources/images/debugInquiry.svg"
    , QObject::tr("Test").toStdString() //icon text
    , QObject::tr("Test").toStdString() //status text
    , QObject::tr("Developer Tool To Test Functionality").toStdString() //whats this text
    , QObject::tr("Test").toStdString() // toolTipText
    , msg::Request | msg::Test
  );
  sc
  (
    89
    , ":/resources/images/dagViewPending.svg"
    , QObject::tr("Dirty features").toStdString() //icon text
    , QObject::tr("Dirty features").toStdString() //status text
    , QObject::tr("Developer Tool To Dirty Features").toStdString() //whats this text
    , QObject::tr("Dirty features").toStdString() // toolTipText
    , msg::Request | msg::Feature | msg::Model | msg::Dirty
  );
  sc
  (
    90
    , ":/resources/images/constructionThicken.svg"
    , QObject::tr("Thicken").toStdString() //icon text
    , QObject::tr("Thicken Surface").toStdString() //status text
    , QObject::tr("Thicken Surface To Make A Solid").toStdString() //whats this text
    , QObject::tr("Thicken Surface").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Thicken
  );
  sc
  (
    91
    , ":/resources/images/constructionSew.svg"
    , QObject::tr("Sew").toStdString() //icon text
    , QObject::tr("Sew Surfaces").toStdString() //status text
    , QObject::tr("Sew Surfaces And Shells").toStdString() //whats this text
    , QObject::tr("Sew Surfaces").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Sew
  );
  sc
  (
    92
    , ":/resources/images/constructionTrim.svg"
    , QObject::tr("Trim").toStdString() //icon text
    , QObject::tr("Trim Solid").toStdString() //status text
    , QObject::tr("Trim Solids With Shells Or Face").toStdString() //whats this text
    , QObject::tr("Trim Solid").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::Trim
  );
  sc
  (
    93
    , ":/resources/images/constructionDatumSystem.svg"
    , QObject::tr("Datum System").toStdString() //icon text
    , QObject::tr("Datum System").toStdString() //status text
    , QObject::tr("Create A Datum Coordinate System").toStdString() //whats this text
    , QObject::tr("Datum System").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::DatumSystem
  );
  sc
  (
    94
    , ":/resources/images/constructionSurfaceReMesh.svg"
    , QObject::tr("Surface ReMesh").toStdString() //icon text
    , QObject::tr("Surface ReMesh").toStdString() //status text
    , QObject::tr("ReMesh A Surface Mesh").toStdString() //whats this text
    , QObject::tr("Surface ReMesh").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::SurfaceReMesh
  );
  sc
  (
    95
    , ":/resources/images/constructionSurfaceMeshFill.svg"
    , QObject::tr("Surface Mesh Fill").toStdString() //icon text
    , QObject::tr("Surface Mesh Fill").toStdString() //status text
    , QObject::tr("Remove All Interal Holes In A Surface Mesh").toStdString() //whats this text
    , QObject::tr("Surface Mesh Fill").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::SurfaceMeshFill
  );
  sc
  (
    96
    , ":/resources/images/constructionMapPCurve.svg"
    , QObject::tr("Map P Curve").toStdString() //icon text
    , QObject::tr("Map P Curve").toStdString() //status text
    , QObject::tr("Construct 3d Curve From A Parametric 2d Curve.").toStdString() //whats this text
    , QObject::tr("Map P Curve").toStdString() // toolTipText
    , msg::Request | msg::Construct | msg::MapPCurve
  );
}

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
#include <sstream>
#include <assert.h>

#include <QFileDialog>
#include <QTextStream>
#include <QUrl>
#include <QDesktopServices>
#include <QMessageBox>

#include <boost/uuid/uuid.hpp>
#include <boost/timer/timer.hpp>
#include <boost/filesystem.hpp>
#include <boost/current_function.hpp>

#include <BRepTools.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Iterator.hxx>
#include <TopoDS.hxx>
#include <APIHeaderSection_MakeHeader.hxx>
#include <BOPAlgo_Builder.hxx>

#include <osgDB/WriteFile>

#include "tools/idtools.h"
#include "tools/occtools.h"
#include "message/msgsift.h"
#include "message/msgnode.h"
#include "project/prjproject.h"
#include "project/prjmessage.h"
#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "application/appmessage.h"
#include "viewer/vwrwidget.h"
#include "dialogs/dlgpreferences.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "annex/annseershape.h"
#include "feature/ftrtypes.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrbox.h"
#include "feature/ftroblong.h"
#include "feature/ftrtorus.h"
#include "feature/ftrcylinder.h"
#include "feature/ftrsphere.h"
#include "feature/ftrcone.h"
#include "feature/ftrunion.h"
#include "feature/ftrsubtract.h"
#include "feature/ftrintersect.h"
#include "feature/ftrchamfer.h"
#include "feature/ftrdraft.h"
#include "feature/ftrhollow.h"
#include "feature/ftrinert.h"
#include "tools/featuretools.h"
#include "library/lbrlineardimension.h"
#include "selection/slcmessage.h"
#include "selection/slcvisitors.h"
#include "application/appfactory.h"

using namespace app;
using boost::uuids::uuid;

Factory::Factory()
{
  node = std::make_unique<msg::Node>();
  node->connect(msg::hub());
  sift = std::make_unique<msg::Sift>();
  sift->name = "app::Factory";
  node->setHandler(std::bind(&msg::Sift::receive, sift.get(), std::placeholders::_1));
    
  setupDispatcher();
}

void Factory::setupDispatcher()
{
  sift->insert
  (
    {
      std::make_pair
      (
        msg::Response | msg::Post | msg::New | msg::Project
        , std::bind(&Factory::newProjectDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Post | msg::Open | msg::Project
      , std::bind(&Factory::openProjectDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Pre | msg::Close | msg::Project
      , std::bind(&Factory::closeProjectDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Post | msg::Selection | msg::Add
      , std::bind(&Factory::selectionAdditionDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Response | msg::Pre | msg::Selection | msg::Remove
      , std::bind(&Factory::selectionSubtractionDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::DebugDump
      , std::bind(&Factory::debugDumpDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::DebugShapeTrackUp
      , std::bind(&Factory::debugShapeTrackUpDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::DebugShapeTrackDown
      , std::bind(&Factory::debugShapeTrackDownDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::DebugShapeGraph
      , std::bind(&Factory::debugShapeGraphDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Info
      , std::bind(&Factory::viewInfoDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::Feature | msg::Model | msg::Dirty
      , std::bind(&Factory::featureModelDirtyDispatched, this, std::placeholders::_1)
      )
      , std::make_pair
      (
        msg::Request | msg::DebugInquiry
      , std::bind(&Factory::bopalgoTestDispatched, this, std::placeholders::_1)
      )
    }
  );
}

void Factory::newProjectDispatched(const msg::Message& /*messageIn*/)
{
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  project = application->getProject();
}

void Factory::openProjectDispatched(const msg::Message&)
{
  app::Application *application = dynamic_cast<app::Application *>(qApp);
  assert(application);
  project = application->getProject();
}

void Factory::closeProjectDispatched(const msg::Message&)
{
  project = nullptr;
}

void Factory::selectionAdditionDispatched(const msg::Message &messageIn)
{
  slc::Message sMessage = messageIn.getSLC();
  slc::Container aContainer;
  aContainer.selectionType = sMessage.type;
  aContainer.featureId = sMessage.featureId;
  aContainer.featureType = sMessage.featureType;
  aContainer.shapeId = sMessage.shapeId;
  aContainer.pointLocation = sMessage.pointLocation;
  containers.push_back(aContainer);
}

void Factory::selectionSubtractionDispatched(const msg::Message &messageIn)
{
  slc::Message sMessage = messageIn.getSLC();
  slc::Container aContainer;
  aContainer.selectionType = sMessage.type;
  aContainer.featureId = sMessage.featureId;
  aContainer.featureType = sMessage.featureType;
  aContainer.shapeId = sMessage.shapeId;
  aContainer.pointLocation = sMessage.pointLocation;
  
  slc::Containers::iterator it = std::find(containers.begin(), containers.end(), aContainer);
  assert(it != containers.end());
  containers.erase(it);
}

void Factory::debugDumpDispatched(const msg::Message&)
{
  assert(project);
  if (containers.empty())
    return;
  
  std::cout << std::endl << std::endl << "begin debug dump:" << std::endl;
  
  for (const auto &container : containers)
  {
    if (container.selectionType != slc::Type::Object)
      continue;
    ftr::Base *feature = project->findFeature(container.featureId);
    assert(feature);
    if (!feature->hasAnnex(ann::Type::SeerShape))
      continue;
    const ann::SeerShape &seerShape = feature->getAnnex<ann::SeerShape>();
    std::cout << std::endl;
    std::cout << "feature name: " << feature->getName().toStdString() << "    feature id: " << gu::idToString(feature->getId()) << std::endl;
    std::cout << "shape id container:" << std::endl; seerShape.dumpShapeIdContainer(std::cout); std::cout << std::endl;
    std::cout << "shape evolve container:" << std::endl; seerShape.dumpEvolveContainer(std::cout); std::cout << std::endl;
    std::cout << "feature tag container:" << std::endl; seerShape.dumpFeatureTagContainer(std::cout); std::cout << std::endl;
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

void Factory::debugShapeTrackUpDispatched(const msg::Message&)
{
  assert(project);
  if (containers.empty())
    return;
  
  for (const auto &container : containers)
  {
    if (container.shapeId.is_nil())
      continue;
    project->shapeTrackUp(container.shapeId);
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

void Factory::debugShapeTrackDownDispatched(const msg::Message&)
{
  assert(project);
  if (containers.empty())
    return;
  
  for (const auto &container : containers)
  {
    if (container.shapeId.is_nil())
      continue;
    project->shapeTrackDown(container.shapeId);
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

void Factory::debugShapeGraphDispatched(const msg::Message&)
{
    assert(project);
    if (containers.empty())
        return;
    
    boost::filesystem::path path = app::instance()->getApplicationDirectory();
    for (const auto &container : containers)
    {
        if (container.selectionType != slc::Type::Object)
            continue;
        ftr::Base *feature = project->findFeature(container.featureId);
        if (!feature->hasAnnex(ann::Type::SeerShape))
            continue;
        path /= gu::idToString(feature->getId()) + ".dot";
        const ann::SeerShape &shape = feature->getAnnex<ann::SeerShape>();
        shape.dumpGraph(path.string());
        
        QDesktopServices::openUrl(QUrl(QString::fromStdString(path.string())));
    }
    
    node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

void Factory::viewInfoDispatched(const msg::Message &)
{
  QString infoMessage;
  QTextStream stream(&infoMessage);
  stream.setRealNumberNotation(QTextStream::FixedNotation);
  forcepoint(stream) << qSetRealNumberPrecision(12);
  stream << endl;
  
  vwr::Widget *viewer = app::instance()->getMainWindow()->getViewer();
  osg::Matrixd ics = osg::Matrixd::inverse(viewer->getCurrentSystem()); //inverted current system.
  
  auto streamPoint = [&](const osg::Vec3d &p)
  {
    stream
      << "Absolute Point location: "
      << "["
      << p.x() << ", "
      << p.y() << ", "
      << p.z() << "]"
      << endl;
      
    osg::Vec3d tp = p * ics;
    stream
      << "Current System Point location: "
      << "["
      << tp.x() << ", "
      << tp.y() << ", "
      << tp.z() << "]"
      << endl;
  };
  
  
  
    //maybe if selection is empty we will dump out application information.
    //or better yet project information or even better yet both. careful
    //we might want to turn the window on and off keeping information as a 
    //reference and we don't to fill the window with shit. we will be alright, 
    //we will have a different facility for show and hiding the info window
    //other than this command.
    assert(project);
    if (containers.empty())
    {
      //nothing selected so get app and project information.
      //TODO get git hash for application version.
      project->getInfo(stream);
      app::instance()->getMainWindow()->getViewer()->getInfo(stream);
    }
    else
    {
      for (const auto &container : containers)
      {
          ftr::Base *feature = project->findFeature(container.featureId);
          stream <<  endl;
          if
          (
              (container.selectionType == slc::Type::Object) ||
              (container.selectionType == slc::Type::Feature)
          )
          {
              feature->getInfo(stream);
          }
          else if
          (
              (container.selectionType == slc::Type::Solid) ||
              (container.selectionType == slc::Type::Shell) ||
              (container.selectionType == slc::Type::Face) ||
              (container.selectionType == slc::Type::Wire) ||
              (container.selectionType == slc::Type::Edge)
          )
          {
              feature->getShapeInfo(stream, container.shapeId);
          }
          else if (container.selectionType == slc::Type::StartPoint)
          {
              const ann::SeerShape &s = feature->getAnnex<ann::SeerShape>();
              feature->getShapeInfo(stream, s.useGetStartVertex(container.shapeId));
              streamPoint(container.pointLocation);
          }
          else if (container.selectionType == slc::Type::EndPoint)
          {
            const ann::SeerShape &s = feature->getAnnex<ann::SeerShape>();
            feature->getShapeInfo(stream, s.useGetEndVertex(container.shapeId));
            streamPoint(container.pointLocation);
          }
          else //all other points.
          {
            streamPoint(container.pointLocation);
          }
      }
    }
    
    app::Message appMessage;
    appMessage.infoMessage = infoMessage;
    msg::Message viewInfoMessage(msg::Request | msg::Info | msg::Text, appMessage);
    msg::hub().send(viewInfoMessage);
}

void Factory::featureModelDirtyDispatched(const msg::Message&)
{
  assert(project);
  for (const auto &container : containers)
    project->findFeature(container.featureId)->setModelDirty();
}

//! a testing function to analyze run time cost of messages.
void Factory::messageStressTestDispatched(const msg::Message&)
{
  //not updating for switch to new message signals
  /*
  std::cout << std::endl;
  //test mask. shouldn't match any observer.
  msg::Mask test = msg::Request | msg::Response | msg::Pre | msg::Post;
  std::vector<std::unique_ptr<msg::Observer>> observers;
  
  {
    std::cout << std::endl << "approx. 1000 observers" << std::endl;
    for (std::size_t index = 0; index < 1000; ++index)
      observers.push_back(std::unique_ptr<msg::Observer>(new msg::Observer()));
    boost::timer::auto_cpu_timer t;
    node->send(msg::Message(test));
  }
  
  {
    std::cout << std::endl << "approx. 10000 observers" << std::endl;
    for (std::size_t index = 0; index < 9000; ++index)
      observers.push_back(std::unique_ptr<msg::Observer>(new msg::Observer()));
    boost::timer::auto_cpu_timer t;
    node->send(msg::Message(test));
  }
  
  {
    std::cout << std::endl << "approx. 100000 observers" << std::endl;
    for (std::size_t index = 0; index < 90000; ++index)
      observers.push_back(std::unique_ptr<msg::Observer>(new msg::Observer()));
    boost::timer::auto_cpu_timer t;
    node->send(msg::Message(test));
  }
  
  {
    std::cout << std::endl << "approx. 1000000 observers" << std::endl;
    for (std::size_t index = 0; index < 900000; ++index)
      observers.push_back(std::unique_ptr<msg::Observer>(new msg::Observer()));
    boost::timer::auto_cpu_timer t;
    node->send(msg::Message(test));
  }
  */
  
  /*
   * output:
   
   approx. 1000 observers
   0 .000766s wall, 0.000000s user + 0.000000s system = 0.000000s CPU (n/a%)
   
   approx. 10000 observers
   0.008040s wall, 0.010000s user + 0.000000s system = 0.010000s CPU (124.4%)
   
   approx. 100000 observers
   0.072139s wall, 0.070000s user + 0.000000s system = 0.070000s CPU (97.0%)
   
   approx. 1000000 observers
   0.758043s wall, 0.760000s user + 0.000000s system = 0.760000s CPU (100.3%)
   
   This looks linear. Keep in mind these test observers have no function dispatching.
   This is OK, because the main reason behind this test is for individual feature
   observers, which I don't expect to have any function dispatching.
   */
}

void Factory::osgToDotTestDispatched(const msg::Message&)
{
  assert(project);
  if (containers.empty())
    return;
  
  ftr::Base *feature = project->findFeature(containers.front().featureId);
  
  boost::filesystem::path base = app::instance()->getApplicationDirectory();
  osg::ref_ptr<osgDB::Options> options(new osgDB::Options());
  options->setOptionString("rankdir = TB;");
  
  osg::Switch *overlay = feature->getOverlaySwitch();
  std::vector<lbr::LinearDimension*> dimensions;
  slc::TypedAccrueVisitor<lbr::LinearDimension> visitor(dimensions);
  overlay->accept(visitor);
  
  for (unsigned int i = 0; i < dimensions.size(); ++i)
  {
    boost::filesystem::path path = app::instance()->getApplicationDirectory();
    path /= std::string("osg_") + std::to_string(i) + ".dot";
    osgDB::writeNodeFile(*(dimensions.at(i)), path.string(), options);
  }
  
//   QDesktopServices::openUrl(QUrl(fileName));
}

void Factory::bopalgoTestDispatched(const msg::Message&)
{
  auto formatNumber = [](int number) -> std::string
  {
    std::ostringstream out;
    if (number < 100)
      out << "0";
    if (number < 10)
      out << "0";
    out << number;
    return out.str();
  };
  
  assert(project);
  if (containers.size() < 2)
    return;
  
  std::vector<std::reference_wrapper<const ann::SeerShape>> sShapes;
  occt::ShapeVector shapes;
  for (const auto &cs : containers)
  {
    if (cs.selectionType != slc::Type::Object)
      continue;
    const ftr::Base* fb = project->findFeature(cs.featureId);
    if (!fb->hasAnnex(ann::Type::SeerShape))
      continue;
    sShapes.push_back(fb->getAnnex<ann::SeerShape>());
    shapes.push_back(sShapes.back().get().getRootOCCTShape());
  }
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  if (sShapes.size() < 2 || shapes.size() < 2)
    return;
  
  //when splitting edges we will have to compare the number of output edges to
  //the input edges too see if the operation did anything.
    
  //images is a map from input to output.
  //origins is a map from output to input.
  
  try
  {
    BOPAlgo_Builder bopBuilder;
    for (const auto &s : shapes)
      bopBuilder.AddArgument(s);

    bopBuilder.Perform();
    if (bopBuilder.HasErrors())
    {
      std::ostringstream error;
      error << "error with bopBuilder. error code: "
      << std::endl;
      bopBuilder.DumpErrors(error);
      throw std::runtime_error(error.str());
    }
    
    const TopTools_DataMapOfShapeListOfShape &images = bopBuilder.Images();
    TopTools_DataMapOfShapeListOfShape::Iterator imageIt(images);
    for (int index = 0; imageIt.More(); imageIt.Next(), index++)
    {
      const TopoDS_Shape &key = imageIt.Key();
      std::ostringstream keyBase;
      keyBase << "Output_Images_key_" << formatNumber(index);
      std::ostringstream keyName;
      keyName << keyBase.str() << "_source_shapetype:" << occt::getShapeTypeString(key);
      project->addOCCShape(key, keyName.str());
      
      int count = 0;
      for (const auto &sil : imageIt.Value()) //shape in list
      {
        count++;
        assert(key.ShapeType() == sil.ShapeType());
        
        std::ostringstream mappedName;
        mappedName << keyBase.str() << "_result_shape_" << formatNumber(count);
        
        project->addOCCShape(sil, mappedName.str());
      }
    }
    
    //splits was removed in occt v7.3
    //it appears splits only contain faces that have been split. It ignores edges.
//     const TopTools_DataMapOfShapeListOfShape &splits = bopBuilder.Splits();
//     TopTools_DataMapOfShapeListOfShape::Iterator splitIt(splits);
//     for (int index = 0; splitIt.More(); splitIt.Next(), index++)
//     {
//       const TopoDS_Shape &key = splitIt.Key();
//       std::ostringstream keyBase;
//       keyBase << "Output_Splits_key_" << ((index < 10) ? "0" : "") << index;
//       std::ostringstream keyName;
//       keyName << keyBase.str() << "_source_shapetype:" << shapeText(key.ShapeType());
//       addFeature(keyName.str(), key);
//       
//       int count = 0;
//       for (const auto &sil : splitIt.Value()) //shape in list
//       {
//         count++;
//         assert(key.ShapeType() == sil.ShapeType());
//         
//         std::ostringstream mappedName;
//         mappedName << keyBase.str() << "_result_shape_" << ((count < 10) ? "0" : "") << count;
//         
//         addFeature(mappedName.str(), sil);
//       }
//     }
    
    //origins. the keys are the new shapes and the mapped are the originals.
    //allows to trace resultant geometry to source geometry. ignores at least compounds and wires.
    const TopTools_DataMapOfShapeListOfShape &origins = bopBuilder.Origins();
    TopTools_DataMapOfShapeListOfShape::Iterator originIt(origins);
    for (int index = 0; originIt.More(); originIt.Next(), index++)
    {
      const TopoDS_Shape &key = originIt.Key();
      std::ostringstream keyBase;
      keyBase << "Output_Origins_key_" << ((index < 10) ? "0" : "") << index;
      std::ostringstream keyName;
      keyName << keyBase.str() << "_source_shapetype:" << occt::getShapeTypeString(key);
      project->addOCCShape(key, keyName.str());
      int mappedCount = 0;
      for (const auto &sil : origins(key)) //shape in list
      {
        assert(key.ShapeType() == sil.ShapeType());
        
        std::ostringstream mappedName;
        mappedName << keyBase.str() << "_result_shape_" << formatNumber(mappedCount);
        
        project->addOCCShape(sil, mappedName.str());
        mappedCount++;
      }
    }
    
    //shapesSD. This is empty for the 2 face intersect.
    const TopTools_DataMapOfShapeShape &shapesSD = bopBuilder.ShapesSD();
    TopTools_DataMapOfShapeShape::Iterator shapesSDIt(shapesSD);
    for (int index = 0; shapesSDIt.More(); shapesSDIt.Next(), index++)
    {
      const TopoDS_Shape &key = shapesSDIt.Key();
      const TopoDS_Shape &mapped = shapesSD(key);
      assert(key.ShapeType() == mapped.ShapeType());
      
      std::ostringstream keyBase;
      keyBase << "Output_ShapesSD_key_" << formatNumber(index);
      std::ostringstream keyName;
      keyName << keyBase.str() << "_source_shapetype:" << occt::getShapeTypeString(key);
      std::ostringstream mappedName;
      mappedName << keyBase.str() << "_result_shape";
      
      project->addOCCShape(key, keyName.str());
      project->addOCCShape(mapped, mappedName.str());
    }
    
    //following uses source shapes as key. so we will cache them here.
    std::vector<occt::ShapeVector> allSubShapes;
    for (const auto &s : shapes)
      allSubShapes.push_back(occt::mapShapes(s));
    

    
    int count = 0;
    //Generated for inputs.
    for (const auto &subShapes : allSubShapes)
    {
      int index = 0;
      for (const auto &subShape : subShapes)
      {
        occt::ShapeVector mappedVector = occt::ShapeVectorCast(bopBuilder.Generated(subShape));
        if (mappedVector.empty())
          continue;
          
        std::ostringstream keyBase;
        keyBase << "Output_Generated_base" << formatNumber(count) << "_Shape" << formatNumber(index);
        std::ostringstream keyName;
        keyName << keyBase.str() << "_source_shapetype:" << occt::getShapeTypeString(subShape);
        
        std::ostringstream mappedName;
        mappedName << keyBase.str() << "_result_count_" << mappedVector.size();
        
        project->addOCCShape(subShape, keyName.str());
        project->addOCCShape(static_cast<TopoDS_Compound>(occt::ShapeVectorCast(mappedVector)), mappedName.str());
        index++;
      }
      count++;
    }
    
    //Modified for input 1. ignores at least compounds, shells and wires.
    count = 0;
    for (const auto &subShapes : allSubShapes)
    {
      int index = 0;
      for (const auto &subShape : subShapes)
      {
        occt::ShapeVector mappedVector = occt::ShapeVectorCast(bopBuilder.Modified(subShape));
        if (mappedVector.empty())
          continue;
        std::ostringstream keyBase;
        keyBase << "Output_Modified_base" << formatNumber(count) << "_Shape" << formatNumber(index);
        std::ostringstream keyName;
        keyName << keyBase.str() << "_source_shapetype:" << occt::getShapeTypeString(subShape);
        project->addOCCShape(subShape, keyName.str());
        
        int mappedCount = 0;
        for (const auto &sil : mappedVector) //shape in list
        {
          std::ostringstream mappedName;
          mappedName << keyBase.str() << "_result_shape_" << formatNumber(mappedCount);
          
          project->addOCCShape(sil, mappedName.str());
          mappedCount++;
        }
        index++;
      }
      count++;
    }
    
    //IsDeleted for inputs.
    count = 0;
    for (const auto &subShapes : allSubShapes)
    {
      int index = 0;
      for (const auto &subShape : subShapes)
      {
        if (!bopBuilder.IsDeleted(subShape))
          continue;
        
        std::ostringstream keyBase;
        keyBase << "Input_Deleted_base" << formatNumber(count) << "_Shape" << formatNumber(index);
        keyBase << "_shapetype:" << occt::getShapeTypeString(subShape);
        
        project->addOCCShape(subShape, keyBase.str());
        index++;
      }
      count++;
    }
  }
  catch (const Standard_Failure &e)
  {
    std::cout << "OCC Error: " << e.GetMessageString() << std::endl;
  }
  catch (const std::exception &error)
  {
    std::cout << "My Error: " << error.what() << std::endl;
  }
  
  node->send(msg::Mask(msg::Request | msg::Project | msg::Update));
}

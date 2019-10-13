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

#include <osg/Switch>
#include <osgDB/WriteFile>

#include <BOPAlgo_Builder.hxx>

#include "tools/occtools.h"
#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "library/lbrlineardimension.h"
#include "annex/annseershape.h"
#include "feature/ftrbase.h"
#include "command/cmdtest.h"

using namespace cmd;

Test::Test() : Base() {}

Test::~Test() = default;

std::string Test::getStatusMessage()
{
  return QObject::tr("Select For Test").toStdString();
}

void Test::activate()
{
  isActive = true;
  
  go();
  sendDone();
}

void Test::deactivate()
{
  isActive = false;
}

void Test::go()
{
  bopalgoTestDispatched();
  node->sendBlocked(msg::buildStatusMessage("Test executed", 2.0));
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
}

//! a testing function to analyze run time cost of messages.
//! note this was before Node and Sift refactor, so code is obsolete.
void Test::messageStressTestDispatched()
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

void Test::osgToDotTestDispatched()
{
  assert(project);
  const slc::Containers &containers = eventHandler->getSelections();
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

void Test::bopalgoTestDispatched()
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
  const slc::Containers &containers = eventHandler->getSelections();
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
}

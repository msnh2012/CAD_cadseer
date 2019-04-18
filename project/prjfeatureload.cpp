/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2016  Thomas S. Anderson blobfish.at.gmx.com
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

#include <TopoDS_Iterator.hxx>

#include <osg/Matrixd>

#include "annex/annseershape.h"
#include "feature/ftrbox.h"
#include "feature/ftrcylinder.h"
#include "feature/ftrcone.h"
#include "feature/ftrsphere.h"
#include "feature/ftrunion.h"
#include "feature/ftrsubtract.h"
#include "feature/ftrintersect.h"
#include "feature/ftrblend.h"
#include "feature/ftrchamfer.h"
#include "feature/ftrdraft.h"
#include "feature/ftrinert.h"
#include "feature/ftrdatumplane.h"
#include "feature/ftrhollow.h"
#include "feature/ftroblong.h"
#include "feature/ftrextract.h"
#include "feature/ftrsquash.h"
#include "feature/ftrnest.h"
#include "feature/ftrdieset.h"
#include "feature/ftrstrip.h"
#include "feature/ftrquote.h"
#include "feature/ftrrefine.h"
#include "feature/ftrinstancelinear.h"
#include "feature/ftrinstancemirror.h"
#include "feature/ftrinstancepolar.h"
#include "feature/ftroffset.h"
#include "feature/ftrthicken.h"
#include "feature/ftrsew.h"
#include "feature/ftrtrim.h"
#include "feature/ftrremovefaces.h"
#include "feature/ftrtorus.h"
#include "feature/ftrthread.h"
#include "feature/ftrdatumaxis.h"
#include "feature/ftrextrude.h"
#include "feature/ftrrevolve.h"
#include "feature/ftrsketch.h"
#include "feature/ftrline.h"
#include "feature/ftrsurfacemesh.h"
#include "feature/ftrtransitioncurve.h"
#include "feature/ftrruled.h"
#include "feature/ftrimageplane.h"
#include "project/serial/xsdcxxoutput/featurebox.h"
#include "project/serial/xsdcxxoutput/featurecylinder.h"
#include "project/serial/xsdcxxoutput/featuresphere.h"
#include "project/serial/xsdcxxoutput/featurecone.h"
#include "project/serial/xsdcxxoutput/featureunion.h"
#include "project/serial/xsdcxxoutput/featureintersect.h"
#include "project/serial/xsdcxxoutput/featuresubtract.h"
#include "project/serial/xsdcxxoutput/featureinert.h"
#include "project/serial/xsdcxxoutput/featureblend.h"
#include "project/serial/xsdcxxoutput/featurechamfer.h"
#include "project/serial/xsdcxxoutput/featuredraft.h"
#include "project/serial/xsdcxxoutput/featuredatumplane.h"
#include "project/serial/xsdcxxoutput/featurehollow.h"
#include "project/serial/xsdcxxoutput/featureoblong.h"
#include "project/serial/xsdcxxoutput/featureextract.h"
#include "project/serial/xsdcxxoutput/featuresquash.h"
#include "project/serial/xsdcxxoutput/featurenest.h"
#include "project/serial/xsdcxxoutput/featuredieset.h"
#include "project/serial/xsdcxxoutput/featurestrip.h"
#include "project/serial/xsdcxxoutput/featurequote.h"
#include "project/serial/xsdcxxoutput/featurerefine.h"
#include "project/serial/xsdcxxoutput/featureinstancelinear.h"
#include "project/serial/xsdcxxoutput/featureinstancemirror.h"
#include "project/serial/xsdcxxoutput/featureinstancepolar.h"
#include "project/serial/xsdcxxoutput/featureoffset.h"
#include "project/serial/xsdcxxoutput/featurethicken.h"
#include "project/serial/xsdcxxoutput/featuresew.h"
#include "project/serial/xsdcxxoutput/featuretrim.h"
#include "project/serial/xsdcxxoutput/featureremovefaces.h"
#include "project/serial/xsdcxxoutput/featuretorus.h"
#include "project/serial/xsdcxxoutput/featurethread.h"
#include "project/serial/xsdcxxoutput/featuredatumaxis.h"
#include "project/serial/xsdcxxoutput/featureextrude.h"
#include "project/serial/xsdcxxoutput/featurerevolve.h"
#include "project/serial/xsdcxxoutput/featuresketch.h"
#include "project/serial/xsdcxxoutput/featureline.h"
#include "project/serial/xsdcxxoutput/featuresurfacemesh.h"
#include "project/serial/xsdcxxoutput/featuretransitioncurve.h"
#include "project/serial/xsdcxxoutput/featureruled.h"
#include "project/serial/xsdcxxoutput/featureimageplane.h"

#include "project/prjfeatureload.h"

using boost::filesystem::path;

using namespace prj;

/*! @brief Construct the FeatureLoad object
 * 
 * @parameter directoryIn is the directory where project feature files live.
 * @parameter masterShapeIn is the occt compound shape that contains all feature shapes.
 * @parameter validate is a boolean value to control xml validation. false by default.
 * @note I have not tested validation = true. I think in order for the validation check to work,
 * we will have to specify a path to a xsd file. That is why we have the 'preferencesXML.xsd'
 * file in resources.qrc.
 */
FeatureLoad::FeatureLoad(const path& directoryIn, const TopoDS_Shape &masterShapeIn, bool validate)
: directory(directoryIn)
{
  if (!validate)
    flags |= ::xml_schema::Flags::dont_validate;
  
  for (TopoDS_Iterator it(masterShapeIn); it.More(); it.Next())
    shapeVector.push_back(it.Value());
  
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Box), std::bind(&FeatureLoad::loadBox, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Cylinder), std::bind(&FeatureLoad::loadCylinder, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Sphere), std::bind(&FeatureLoad::loadSphere, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Cone), std::bind(&FeatureLoad::loadCone, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Union), std::bind(&FeatureLoad::loadUnion, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Intersect), std::bind(&FeatureLoad::loadIntersect, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Subtract), std::bind(&FeatureLoad::loadSubtract, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Inert), std::bind(&FeatureLoad::loadInert, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Blend), std::bind(&FeatureLoad::loadBlend, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Chamfer), std::bind(&FeatureLoad::loadChamfer, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Draft), std::bind(&FeatureLoad::loadDraft, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::DatumPlane), std::bind(&FeatureLoad::loadDatumPlane, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Hollow), std::bind(&FeatureLoad::loadHollow, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Oblong), std::bind(&FeatureLoad::loadOblong, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Extract), std::bind(&FeatureLoad::loadExtract, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Squash), std::bind(&FeatureLoad::loadSquash, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Nest), std::bind(&FeatureLoad::loadNest, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::DieSet), std::bind(&FeatureLoad::loadDieSet, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Strip), std::bind(&FeatureLoad::loadStrip, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Quote), std::bind(&FeatureLoad::loadQuote, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Refine), std::bind(&FeatureLoad::loadRefine, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::InstanceLinear), std::bind(&FeatureLoad::loadInstanceLinear, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::InstanceMirror), std::bind(&FeatureLoad::loadInstanceMirror, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::InstancePolar), std::bind(&FeatureLoad::loadInstancePolar, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Offset), std::bind(&FeatureLoad::loadOffset, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Thicken), std::bind(&FeatureLoad::loadThicken, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Sew), std::bind(&FeatureLoad::loadSew, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Trim), std::bind(&FeatureLoad::loadTrim, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::RemoveFaces), std::bind(&FeatureLoad::loadRemoveFaces, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Torus), std::bind(&FeatureLoad::loadTorus, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Thread), std::bind(&FeatureLoad::loadThread, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::DatumAxis), std::bind(&FeatureLoad::loadDatumAxis, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Extrude), std::bind(&FeatureLoad::loadExtrude, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Revolve), std::bind(&FeatureLoad::loadRevolve, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Sketch), std::bind(&FeatureLoad::loadSketch, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Line), std::bind(&FeatureLoad::loadLine, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::SurfaceMesh), std::bind(&FeatureLoad::loadSurfaceMesh, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::TransitionCurve), std::bind(&FeatureLoad::loadTransitionCurve, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Ruled), std::bind(&FeatureLoad::loadRuled, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::ImagePlane), std::bind(&FeatureLoad::loadImagePlane, this, std::placeholders::_1, std::placeholders::_2)));
}

std::shared_ptr< ftr::Base > FeatureLoad::load(const std::string& idIn, const std::string& typeIn, std::size_t shapeOffsetIn)
{
  auto it = functionMap.find(typeIn);
  assert(it != functionMap.end());
  
  boost::filesystem::path filePath = directory / (idIn + ".fetr");
  try
  {
    return it->second(filePath.string(), shapeOffsetIn);
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
  
  return std::shared_ptr< ftr::Base >();
}

std::shared_ptr< ftr::Base > FeatureLoad::loadBox(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto box = srl::box(fileNameIn, flags);
  assert(box);
  
  auto freshBox = std::make_shared<ftr::Box>();
  freshBox->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshBox->serialRead(*box);
  
  return freshBox;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadCylinder(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sCylinder = srl::cylinder(fileNameIn, flags);
  assert(sCylinder);
  
  auto freshCylinder = std::make_shared<ftr::Cylinder>();
  freshCylinder->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshCylinder->serialRead(*sCylinder);
  
  return freshCylinder;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadSphere(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sSphere = srl::sphere(fileNameIn, flags);
  assert(sSphere);
  
  auto freshSphere = std::make_shared<ftr::Sphere>();
  freshSphere->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshSphere->serialRead(*sSphere);
  
  return freshSphere;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadCone(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sCone = srl::cone(fileNameIn, flags);
  assert(sCone);
  
  auto freshCone = std::make_shared<ftr::Cone>();
  freshCone->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshCone->serialRead(*sCone);
  
  return freshCone;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadUnion(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sUnion = srl::fUnion(fileNameIn, flags);
  assert(sUnion);
  
  auto freshUnion = std::make_shared<ftr::Union>();
  freshUnion->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshUnion->serialRead(*sUnion);
  
  return freshUnion;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadIntersect(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sIntersect = srl::intersect(fileNameIn, flags);
  assert(sIntersect);
  
  auto freshIntersect = std::make_shared<ftr::Intersect>();
  freshIntersect->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshIntersect->serialRead(*sIntersect);
  
  return freshIntersect;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadSubtract(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sSubtract = srl::subtract(fileNameIn, flags);
  assert(sSubtract);
  
  auto freshSubtract = std::make_shared<ftr::Subtract>();
  freshSubtract->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshSubtract->serialRead(*sSubtract);
  
  return freshSubtract;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadInert(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sInert = srl::inert(fileNameIn, flags);
  assert(sInert);
  
  auto freshInert = std::make_shared<ftr::Inert>(shapeVector.at(shapeOffsetIn));
  freshInert->serialRead(*sInert);
  
  return freshInert;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadBlend(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sBlend = srl::blend(fileNameIn, flags);
  assert(sBlend);
  
  auto freshBlend = std::make_shared<ftr::Blend>();
  freshBlend->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshBlend->serialRead(*sBlend);
  
  return freshBlend;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadChamfer(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sChamfer = srl::chamfer(fileNameIn, flags);
  assert(sChamfer);
  
  auto freshChamfer = std::make_shared<ftr::Chamfer>();
  freshChamfer->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshChamfer->serialRead(*sChamfer);
  
  return freshChamfer;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadDraft(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sDraft = srl::draft(fileNameIn, flags);
  assert(sDraft);
  
  auto freshDraft = std::make_shared<ftr::Draft>();
  freshDraft->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshDraft->serialRead(*sDraft);
  
  return freshDraft;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadDatumPlane(const std::string &fileNameIn, std::size_t)
{
  auto sDatumPlane = srl::datumPlane(fileNameIn, flags);
  assert(sDatumPlane);
  
  auto freshDatumPlane = std::make_shared<ftr::DatumPlane>();
  freshDatumPlane->serialRead(*sDatumPlane);
  
  return freshDatumPlane;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadHollow(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sHollow = srl::hollow(fileNameIn, flags);
  assert(sHollow);
  
  auto freshHollow = std::make_shared<ftr::Hollow>();
  freshHollow->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshHollow->serialRead(*sHollow);
  
  return freshHollow;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadOblong(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sOblong = srl::oblong(fileNameIn, flags);
  assert(sOblong);
  
  auto freshOblong = std::make_shared<ftr::Oblong>();
  freshOblong->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshOblong->serialRead(*sOblong);
  
  return freshOblong;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadExtract(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sExtract = srl::extract(fileNameIn, flags);
  assert(sExtract);
  
  auto freshExtract = std::make_shared<ftr::Extract>();
  freshExtract->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshExtract->serialRead(*sExtract);
  
  return freshExtract;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadSquash(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sSquash = srl::squash(fileNameIn, flags);
  assert(sSquash);
  
  auto freshSquash = std::make_shared<ftr::Squash>();
  freshSquash->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshSquash->serialRead(*sSquash);
  
  return freshSquash;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadNest(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sNest = srl::nest(fileNameIn, flags);
  assert(sNest);
  
  auto freshNest = std::make_shared<ftr::Nest>();
  freshNest->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshNest->serialRead(*sNest);
  
  return freshNest;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadDieSet(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sds = srl::dieset(fileNameIn, flags);
  assert(sds);
  
  auto freshDieSet = std::make_shared<ftr::DieSet>();
  freshDieSet->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshDieSet->serialRead(*sds);
  
  return freshDieSet;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadStrip(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto ss = srl::strip(fileNameIn, flags);
  assert(ss);
  
  auto freshStrip = std::make_shared<ftr::Strip>();
  freshStrip->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshStrip->serialRead(*ss);
  
  return freshStrip;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadQuote(const std::string &fileNameIn, std::size_t)
{
  auto sq = srl::quote(fileNameIn, flags);
  assert(sq);
  
  auto freshQuote = std::make_shared<ftr::Quote>();
  freshQuote->serialRead(*sq);
  
  return freshQuote;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadRefine(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::refine(fileNameIn, flags);
  assert(sr);
  
  auto freshRefine = std::make_shared<ftr::Refine>();
  freshRefine->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshRefine->serialRead(*sr);
  
  return freshRefine;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadInstanceLinear(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::instanceLinear(fileNameIn, flags);
  assert(sr);
  
  auto freshInstanceLinear = std::make_shared<ftr::InstanceLinear>();
  freshInstanceLinear->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshInstanceLinear->serialRead(*sr);
  
  return freshInstanceLinear;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadInstanceMirror(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::instanceMirror(fileNameIn, flags);
  assert(sr);
  
  auto freshInstanceMirror = std::make_shared<ftr::InstanceMirror>();
  freshInstanceMirror->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshInstanceMirror->serialRead(*sr);
  
  return freshInstanceMirror;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadInstancePolar(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::instancePolar(fileNameIn, flags);
  assert(sr);
  
  auto freshInstancePolar = std::make_shared<ftr::InstancePolar>();
  freshInstancePolar->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  freshInstancePolar->serialRead(*sr);
  
  return freshInstancePolar;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadOffset(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::offset(fileNameIn, flags);
  assert(sr);
  
  auto offset = std::make_shared<ftr::Offset>();
  offset->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  offset->serialRead(*sr);
  
  return offset;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadThicken(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::thicken(fileNameIn, flags);
  assert(sr);
  
  auto thicken = std::make_shared<ftr::Thicken>();
  thicken->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  thicken->serialRead(*sr);
  
  return thicken;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadSew(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::sew(fileNameIn, flags);
  assert(sr);
  
  auto sew = std::make_shared<ftr::Sew>();
  sew->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  sew->serialRead(*sr);
  
  return sew;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadTrim(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::trim(fileNameIn, flags);
  assert(sr);
  
  auto trim = std::make_shared<ftr::Trim>();
  trim->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  trim->serialRead(*sr);
  
  return trim;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadRemoveFaces(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::removeFaces(fileNameIn, flags);
  assert(sr);
  
  auto rfs = std::make_shared<ftr::RemoveFaces>();
  rfs->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  rfs->serialRead(*sr);
  
  return rfs;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadTorus(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto st = srl::torus(fileNameIn, flags);
  assert(st);
  
  auto tf = std::make_shared<ftr::Torus>();
  tf->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  tf->serialRead(*st);
  
  return tf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadThread(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto st = srl::thread(fileNameIn, flags);
  assert(st);
  
  auto tf = std::make_shared<ftr::Thread>();
  tf->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  tf->serialRead(*st);
  
  return tf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadDatumAxis(const std::string &fileNameIn, std::size_t)
{
  auto sda = srl::datumAxis(fileNameIn, flags);
  assert(sda);
  
  auto daf = std::make_shared<ftr::DatumAxis>();
  daf->serialRead(*sda);
  
  return daf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadExtrude(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto se = srl::extrude(fileNameIn, flags);
  assert(se);
  
  auto ef = std::make_shared<ftr::Extrude>();
  ef->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  ef->serialRead(*se);
  
  return ef;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadRevolve(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto se = srl::revolve(fileNameIn, flags);
  assert(se);
  
  auto ef = std::make_shared<ftr::Revolve>();
  ef->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  ef->serialRead(*se);
  
  return ef;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadSketch(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto ss = srl::sketch(fileNameIn, flags);
  assert(ss);
  
  auto sf = std::make_shared<ftr::Sketch>();
  sf->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  sf->serialRead(*ss);
  
  return sf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadLine(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto ss = srl::line(fileNameIn, flags);
  assert(ss);
  
  auto sf = std::make_shared<ftr::Line>();
  sf->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  sf->serialRead(*ss);
  
  return sf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadSurfaceMesh(const std::string &fileNameIn, std::size_t)
{
  auto ss = srl::surfaceMesh(fileNameIn, flags);
  assert(ss);
  
  auto sf = std::make_shared<ftr::SurfaceMesh>();
  sf->serialRead(*ss);
  
  return sf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadTransitionCurve(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto ss = srl::transitionCurve(fileNameIn, flags);
  assert(ss);
  
  auto sf = std::make_shared<ftr::TransitionCurve>();
  sf->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  sf->serialRead(*ss);
  
  return sf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadRuled(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto ss = srl::ruled(fileNameIn, flags);
  assert(ss);
  
  auto sf = std::make_shared<ftr::Ruled>();
  sf->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn));
  sf->serialRead(*ss);
  
  return sf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadImagePlane(const std::string &fileNameIn, std::size_t)
{
  auto ss = srl::imageplane(fileNameIn, flags);
  assert(ss);
  
  auto sf = std::make_shared<ftr::ImagePlane>();
  sf->serialRead(*ss);
  
  return sf;
}

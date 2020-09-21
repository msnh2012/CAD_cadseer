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
#include "feature/ftrsweep.h"
#include "feature/ftrdatumsystem.h"
#include "feature/ftrsurfaceremesh.h"
#include "feature/ftrsurfacemeshfill.h"
#include "feature/ftrmappcurve.h"
#include "feature/ftruntrim.h"
#include "feature/ftrface.h"
#include "project/serial/generated/prjsrlbxsbox.h"
#include "project/serial/generated/prjsrlblnsblend.h"
#include "project/serial/generated/prjsrlchmschamfer.h"
#include "project/serial/generated/prjsrlcnscone.h"
#include "project/serial/generated/prjsrlcylscylinder.h"
#include "project/serial/generated/prjsrldtasdatumaxis.h"
#include "project/serial/generated/prjsrldtpsdatumplane.h"
#include "project/serial/generated/prjsrldtmsdatumsystem.h"
#include "project/serial/generated/prjsrldstsdieset.h"
#include "project/serial/generated/prjsrldrfsdraft.h"
#include "project/serial/generated/prjsrlextsextract.h"
#include "project/serial/generated/prjsrlexrsextrude.h"
#include "project/serial/generated/prjsrlhllshollow.h"
#include "project/serial/generated/prjsrlimpsimageplane.h"
#include "project/serial/generated/prjsrlintsinert.h"
#include "project/serial/generated/prjsrlinlsinstancelinear.h"
#include "project/serial/generated/prjsrlinmsinstancemirror.h"
#include "project/serial/generated/prjsrlinpsinstancepolar.h"
#include "project/serial/generated/prjsrlinssintersect.h"
#include "project/serial/generated/prjsrllnsline.h"
#include "project/serial/generated/prjsrlnstsnest.h"
#include "project/serial/generated/prjsrloblsoblong.h"
#include "project/serial/generated/prjsrloffsoffset.h"
#include "project/serial/generated/prjsrlqtsquote.h"
#include "project/serial/generated/prjsrlrfnsrefine.h"
#include "project/serial/generated/prjsrlrmfsremovefaces.h"
#include "project/serial/generated/prjsrlrvlsrevolve.h"
#include "project/serial/generated/prjsrlrldsruled.h"
#include "project/serial/generated/prjsrlswssew.h"
#include "project/serial/generated/prjsrlsktssketch.h"
#include "project/serial/generated/prjsrlsprssphere.h"
#include "project/serial/generated/prjsrlsqsssquash.h"
#include "project/serial/generated/prjsrlstpsstrip.h"
#include "project/serial/generated/prjsrlsbtssubtract.h"
#include "project/serial/generated/prjsrlsfmssurfacemesh.h"
#include "project/serial/generated/prjsrlsmfssurfacemeshfill.h"
#include "project/serial/generated/prjsrlsrmssurfaceremesh.h"
#include "project/serial/generated/prjsrlswpssweep.h"
#include "project/serial/generated/prjsrlthksthicken.h"
#include "project/serial/generated/prjsrlthdsthread.h"
#include "project/serial/generated/prjsrltrsstorus.h"
#include "project/serial/generated/prjsrltscstransitioncurve.h"
#include "project/serial/generated/prjsrltrmstrim.h"
#include "project/serial/generated/prjsrlunnsunion.h"
#include "project/serial/generated/prjsrlmpcmappcurve.h"
#include "project/serial/generated/prjsrlutruntrim.h"
#include "project/serial/generated/prjsrlfceface.h"

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
, featureId(gu::createNilId())
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
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Sweep), std::bind(&FeatureLoad::loadSweep, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::DatumSystem), std::bind(&FeatureLoad::loadDatumSystem, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::SurfaceReMesh), std::bind(&FeatureLoad::loadSurfaceReMesh, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::SurfaceMeshFill), std::bind(&FeatureLoad::loadSurfaceMeshFill, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::MapPCurve), std::bind(&FeatureLoad::loadMapPCurve, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Untrim), std::bind(&FeatureLoad::loadUntrim, this, std::placeholders::_1, std::placeholders::_2)));
  functionMap.insert(std::make_pair(ftr::toString(ftr::Type::Face), std::bind(&FeatureLoad::loadFace, this, std::placeholders::_1, std::placeholders::_2)));
}

std::shared_ptr< ftr::Base > FeatureLoad::load(const std::string& idIn, const std::string& typeIn, std::size_t shapeOffsetIn)
{
  featureId = gu::stringToId(idIn);
  
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
  auto box = srl::bxs::box(fileNameIn, flags);
  assert(box);
  
  auto freshBox = std::make_shared<ftr::Box>();
  freshBox->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshBox->serialRead(*box);
  
  return freshBox;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadCylinder(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sCylinder = srl::cyls::cylinder(fileNameIn, flags);
  assert(sCylinder);
  
  auto freshCylinder = std::make_shared<ftr::Cylinder>();
  freshCylinder->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshCylinder->serialRead(*sCylinder);
  
  return freshCylinder;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadSphere(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sSphere = srl::sprs::sphere(fileNameIn, flags);
  assert(sSphere);
  
  auto freshSphere = std::make_shared<ftr::Sphere>();
  freshSphere->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshSphere->serialRead(*sSphere);
  
  return freshSphere;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadCone(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sCone = srl::cns::cone(fileNameIn, flags);
  assert(sCone);
  
  auto freshCone = std::make_shared<ftr::Cone>();
  freshCone->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshCone->serialRead(*sCone);
  
  return freshCone;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadUnion(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sUnion = srl::unns::fUnion(fileNameIn, flags);
  assert(sUnion);
  
  auto freshUnion = std::make_shared<ftr::Union>();
  freshUnion->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshUnion->serialRead(*sUnion);
  
  return freshUnion;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadIntersect(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sIntersect = srl::inss::intersect(fileNameIn, flags);
  assert(sIntersect);
  
  auto freshIntersect = std::make_shared<ftr::Intersect>();
  freshIntersect->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshIntersect->serialRead(*sIntersect);
  
  return freshIntersect;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadSubtract(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sSubtract = srl::sbts::subtract(fileNameIn, flags);
  assert(sSubtract);
  
  auto freshSubtract = std::make_shared<ftr::Subtract>();
  freshSubtract->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshSubtract->serialRead(*sSubtract);
  
  return freshSubtract;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadInert(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sInert = srl::ints::inert(fileNameIn, flags);
  assert(sInert);
  
  auto freshInert = std::make_shared<ftr::Inert>(shapeVector.at(shapeOffsetIn));
  freshInert->serialRead(*sInert);
  
  return freshInert;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadBlend(const std::string& fileNameIn, std::size_t shapeOffsetIn)
{
  auto sBlend = srl::blns::blend(fileNameIn, flags);
  assert(sBlend);
  
  auto freshBlend = std::make_shared<ftr::Blend::Feature>();
  freshBlend->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshBlend->serialRead(*sBlend);
  
  return freshBlend;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadChamfer(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sChamfer = srl::chms::chamfer(fileNameIn, flags);
  assert(sChamfer);
  
  auto freshChamfer = std::make_shared<ftr::Chamfer::Feature>();
  freshChamfer->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshChamfer->serialRead(*sChamfer);
  
  return freshChamfer;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadDraft(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sDraft = srl::drfs::draft(fileNameIn, flags);
  assert(sDraft);
  
  auto freshDraft = std::make_shared<ftr::Draft>();
  freshDraft->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshDraft->serialRead(*sDraft);
  
  return freshDraft;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadDatumPlane(const std::string &fileNameIn, std::size_t)
{
  auto sDatumPlane = srl::dtps::datumPlane(fileNameIn, flags);
  assert(sDatumPlane);
  
  auto freshDatumPlane = std::make_shared<ftr::DatumPlane>();
  freshDatumPlane->serialRead(*sDatumPlane);
  
  return freshDatumPlane;
}

std::shared_ptr< ftr::Base > FeatureLoad::loadHollow(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sHollow = srl::hlls::hollow(fileNameIn, flags);
  assert(sHollow);
  
  auto freshHollow = std::make_shared<ftr::Hollow>();
  freshHollow->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshHollow->serialRead(*sHollow);
  
  return freshHollow;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadOblong(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sOblong = srl::obls::oblong(fileNameIn, flags);
  assert(sOblong);
  
  auto freshOblong = std::make_shared<ftr::Oblong>();
  freshOblong->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshOblong->serialRead(*sOblong);
  
  return freshOblong;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadExtract(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sExtract = srl::exts::extract(fileNameIn, flags);
  assert(sExtract);
  
  auto freshExtract = std::make_shared<ftr::Extract>();
  freshExtract->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshExtract->serialRead(*sExtract);
  
  return freshExtract;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadSquash(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sSquash = srl::sqss::squash(fileNameIn, flags);
  assert(sSquash);
  
  auto freshSquash = std::make_shared<ftr::Squash>();
  freshSquash->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshSquash->serialRead(*sSquash);
  
  return freshSquash;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadNest(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sNest = srl::nsts::nest(fileNameIn, flags);
  assert(sNest);
  
  auto freshNest = std::make_shared<ftr::Nest>();
  freshNest->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshNest->serialRead(*sNest);
  
  return freshNest;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadDieSet(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sds = srl::dsts::dieset(fileNameIn, flags);
  assert(sds);
  
  auto freshDieSet = std::make_shared<ftr::DieSet>();
  freshDieSet->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshDieSet->serialRead(*sds);
  
  return freshDieSet;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadStrip(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto ss = srl::stps::strip(fileNameIn, flags);
  assert(ss);
  
  auto freshStrip = std::make_shared<ftr::Strip>();
  freshStrip->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshStrip->serialRead(*ss);
  
  return freshStrip;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadQuote(const std::string &fileNameIn, std::size_t)
{
  auto sq = srl::qts::quote(fileNameIn, flags);
  assert(sq);
  
  auto freshQuote = std::make_shared<ftr::Quote>();
  freshQuote->serialRead(*sq);
  
  return freshQuote;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadRefine(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::rfns::refine(fileNameIn, flags);
  assert(sr);
  
  auto freshRefine = std::make_shared<ftr::Refine>();
  freshRefine->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshRefine->serialRead(*sr);
  
  return freshRefine;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadInstanceLinear(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::inls::instanceLinear(fileNameIn, flags);
  assert(sr);
  
  auto freshInstanceLinear = std::make_shared<ftr::InstanceLinear>();
  freshInstanceLinear->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshInstanceLinear->serialRead(*sr);
  
  return freshInstanceLinear;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadInstanceMirror(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::inms::instanceMirror(fileNameIn, flags);
  assert(sr);
  
  auto freshInstanceMirror = std::make_shared<ftr::InstanceMirror>();
  freshInstanceMirror->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshInstanceMirror->serialRead(*sr);
  
  return freshInstanceMirror;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadInstancePolar(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::inps::instancePolar(fileNameIn, flags);
  assert(sr);
  
  auto freshInstancePolar = std::make_shared<ftr::InstancePolar>();
  freshInstancePolar->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  freshInstancePolar->serialRead(*sr);
  
  return freshInstancePolar;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadOffset(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::offs::offset(fileNameIn, flags);
  assert(sr);
  
  auto offset = std::make_shared<ftr::Offset>();
  offset->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  offset->serialRead(*sr);
  
  return offset;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadThicken(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::thks::thicken(fileNameIn, flags);
  assert(sr);
  
  auto thicken = std::make_shared<ftr::Thicken>();
  thicken->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  thicken->serialRead(*sr);
  
  return thicken;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadSew(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::sws::sew(fileNameIn, flags);
  assert(sr);
  
  auto sew = std::make_shared<ftr::Sew>();
  sew->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  sew->serialRead(*sr);
  
  return sew;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadTrim(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::trms::trim(fileNameIn, flags);
  assert(sr);
  
  auto trim = std::make_shared<ftr::Trim>();
  trim->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  trim->serialRead(*sr);
  
  return trim;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadRemoveFaces(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto sr = srl::rmfs::removeFaces(fileNameIn, flags);
  assert(sr);
  
  auto rfs = std::make_shared<ftr::RemoveFaces>();
  rfs->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  rfs->serialRead(*sr);
  
  return rfs;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadTorus(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto st = srl::trss::torus(fileNameIn, flags);
  assert(st);
  
  auto tf = std::make_shared<ftr::Torus>();
  tf->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  tf->serialRead(*st);
  
  return tf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadThread(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto st = srl::thds::thread(fileNameIn, flags);
  assert(st);
  
  auto tf = std::make_shared<ftr::Thread>();
  tf->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  tf->serialRead(*st);
  
  return tf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadDatumAxis(const std::string &fileNameIn, std::size_t)
{
  auto sda = srl::dtas::datumAxis(fileNameIn, flags);
  assert(sda);
  
  auto daf = std::make_shared<ftr::DatumAxis>();
  daf->serialRead(*sda);
  
  return daf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadExtrude(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto se = srl::exrs::extrude(fileNameIn, flags);
  assert(se);
  
  auto ef = std::make_shared<ftr::Extrude>();
  ef->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  ef->serialRead(*se);
  
  return ef;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadRevolve(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto se = srl::rvls::revolve(fileNameIn, flags);
  assert(se);
  
  auto ef = std::make_shared<ftr::Revolve>();
  ef->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  ef->serialRead(*se);
  
  return ef;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadSketch(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto ss = srl::skts::sketch(fileNameIn, flags);
  assert(ss);
  
  auto sf = std::make_shared<ftr::Sketch>();
  sf->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  sf->serialRead(*ss);
  
  return sf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadLine(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto ss = srl::lns::line(fileNameIn, flags);
  assert(ss);
  
  auto sf = std::make_shared<ftr::Line>();
  sf->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  sf->serialRead(*ss);
  
  return sf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadSurfaceMesh(const std::string &fileNameIn, std::size_t)
{
  auto ss = srl::sfms::surfaceMesh(fileNameIn, flags);
  assert(ss);
  
  auto sf = std::make_shared<ftr::SurfaceMesh>();
  sf->serialRead(*ss);
  
  return sf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadTransitionCurve(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto ss = srl::tscs::transitionCurve(fileNameIn, flags);
  assert(ss);
  
  auto sf = std::make_shared<ftr::TransitionCurve>();
  sf->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  sf->serialRead(*ss);
  
  return sf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadRuled(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto ss = srl::rlds::ruled(fileNameIn, flags);
  assert(ss);
  
  auto sf = std::make_shared<ftr::Ruled>();
  sf->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  sf->serialRead(*ss);
  
  return sf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadImagePlane(const std::string &fileNameIn, std::size_t)
{
  auto ss = srl::imps::imageplane(fileNameIn, flags);
  assert(ss);
  
  auto sf = std::make_shared<ftr::ImagePlane>();
  sf->serialRead(*ss);
  
  return sf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadSweep(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto ss = srl::swps::sweep(fileNameIn, flags);
  assert(ss);
  
  auto sf = std::make_shared<ftr::Sweep>();
  sf->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  sf->serialRead(*ss);
  
  return sf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadDatumSystem(const std::string &fileNameIn, std::size_t)
{
  auto sds = srl::dtms::datumsystem(fileNameIn, flags);
  assert(sds);
  
  auto daf = std::make_shared<ftr::DatumSystem::Feature>();
  daf->serialRead(*sds);
  
  return daf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadSurfaceReMesh(const std::string &fileNameIn, std::size_t)
{
  auto ssrm = srl::srms::surfaceremesh(fileNameIn, flags);
  assert(ssrm);
  
  auto daf = std::make_shared<ftr::SurfaceReMesh>();
  daf->serialRead(*ssrm);
  
  return daf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadSurfaceMeshFill(const std::string &fileNameIn, std::size_t)
{
  auto ssrm = srl::smfs::surfacemeshfill(fileNameIn, flags);
  assert(ssrm);
  
  auto daf = std::make_shared<ftr::SurfaceMeshFill>();
  daf->serialRead(*ssrm);
  
  return daf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadMapPCurve(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto ss = srl::mpc::mappcurve(fileNameIn, flags);
  assert(ss);
  
  auto sf = std::make_shared<ftr::MapPCurve>();
  sf->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  sf->serialRead(*ss);
  
  return sf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadUntrim(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto ss = srl::utr::untrim(fileNameIn, flags);
  assert(ss);
  
  auto sf = std::make_shared<ftr::Untrim>();
  sf->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  sf->serialRead(*ss);
  
  return sf;
}

std::shared_ptr<ftr::Base> FeatureLoad::loadFace(const std::string &fileNameIn, std::size_t shapeOffsetIn)
{
  auto ss = srl::fce::face(fileNameIn, flags);
  assert(ss);
  
  auto sf = std::make_shared<ftr::Face>();
  sf->getAnnex<ann::SeerShape>().setOCCTShape(shapeVector.at(shapeOffsetIn), featureId);
  sf->serialRead(*ss);
  
  return sf;
}

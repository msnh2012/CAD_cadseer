/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#include <boost/filesystem/path.hpp>

#include <Precision.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <BRepTools_History.hxx>

#include <osg/Switch>

#include "tools/idtools.h"
#include "globalutilities.h"
#include "project/serial/generated/prjsrlrfnsrefine.h"
#include "annex/annseershape.h"
#include "feature/ftrupdatepayload.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrrefine.h"

using namespace ftr;

using boost::uuids::uuid;

QIcon Refine::icon;

Refine::Refine() : Base(), sShape(new ann::SeerShape())
{
  if (icon.isNull())
    icon = QIcon(":/resources/images/constructionRefine.svg");
  
  name = QObject::tr("Refine");
  mainSwitch->setUserValue<int>(gu::featureTypeAttributeTitle, static_cast<int>(getType()));
  
  annexes.insert(std::make_pair(ann::Type::SeerShape, sShape.get()));
}

Refine::~Refine(){}

void Refine::updateModel(const UpdatePayload &payloadIn)
{
  setFailure();
  lastUpdateLog.clear();
  sShape->reset();
  try
  {
    std::vector<const Base*> tfs = payloadIn.getFeatures(ftr::InputType::target);
    if (tfs.size() != 1)
      throw std::runtime_error("wrong number of 'target' inputs");
    if(!tfs.front()->hasAnnex(ann::Type::SeerShape))
      throw std::runtime_error("no seer shape for target");
    const ann::SeerShape &tss = tfs.front()->getAnnex<ann::SeerShape>(); //target seer shape.
    if (tss.isNull())
      throw std::runtime_error("target seer shape is null");
    const TopoDS_Shape &shape = tss.getRootOCCTShape();
    
    //setup new failure state.
    sShape->setOCCTShape(tss.getRootOCCTShape(), getId());
    sShape->shapeMatch(tss);
    sShape->uniqueTypeMatch(tss);
    sShape->outerWireMatch(tss);
    sShape->derivedMatch();
    sShape->ensureNoNils(); //just in case
    sShape->ensureNoDuplicates(); //just in case
    
    if (isSkipped())
    {
      setSuccess();
      throw std::runtime_error("feature is skipped");
    }
    
    ShapeUpgrade_UnifySameDomain usd(shape);
    usd.Build();
    sShape->setOCCTShape(usd.Shape(), getId());
    sShape->shapeMatch(tss);
    sShape->uniqueTypeMatch(tss);
    
    historyMatch(*(usd.History()), tss);
    
    sShape->outerWireMatch(tss);
    sShape->derivedMatch();
    sShape->dumpNils("refine feature");
    sShape->dumpDuplicates("refine feature");
    sShape->ensureNoNils();
    sShape->ensureNoDuplicates();
    
    setSuccess();
  }
  catch (const Standard_Failure &e)
  {
    std::ostringstream s; s << "OCC Error in refine update: " << e.GetMessageString() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (const std::exception &e)
  {
    std::ostringstream s; s << "Standard error in refine update: " << e.what() << std::endl;
    lastUpdateLog += s.str();
  }
  catch (...)
  {
    std::ostringstream s; s << "Unknown error in refine update. " << std::endl;
    lastUpdateLog += s.str();
  }
  setModelClean();
  if (!lastUpdateLog.empty())
    std::cout << std::endl << lastUpdateLog;
}

void Refine::historyMatch(const BRepTools_History &hIn , const ann::SeerShape &tIn)
{
  std::vector<uuid> tIds = tIn.getAllShapeIds();
  
  for (const auto &ctId : tIds) //cuurent target id
  {
    const TopoDS_Shape &cs = tIn.findShape(ctId);
    
    //not getting anything from generated
    //modified appears to be a one to one mapping of faces and edges
    
    const TopTools_ListOfShape &modified = hIn.Modified(cs);
    for (const auto &ms : modified)
    {
      assert(sShape->hasShape(ms));
      if (sShape->findId(ms).is_nil())
      {
        std::map<uuid, uuid>::iterator mapItFace;
        bool dummy;
        std::tie(mapItFace, dummy) = shapeMap.insert(std::make_pair(ctId, gu::createRandomId()));
        sShape->updateId(ms, mapItFace->second);
      }
      sShape->insertEvolve(ctId, sShape->findId(ms));
    }
    if (hIn.IsRemoved(cs)) //appears to remove only edges and vertices.
      sShape->insertEvolve(ctId, gu::createNilId());
  }
}

void Refine::serialWrite(const boost::filesystem::path &dIn)
{
  prj::srl::rfns::Refine ro
  (
    Base::serialOut(),
    sShape->serialOut()
  );
  
  for (const auto &p : shapeMap)
    ro.shapeMap().push_back(prj::srl::spt::EvolveRecord(gu::idToString(p.first), gu::idToString(p.second)));
  
  xml_schema::NamespaceInfomap infoMap;
  std::ofstream stream(buildFilePathName(dIn).string());
  prj::srl::rfns::refine(stream, ro, infoMap);
}

void Refine::serialRead(const prj::srl::rfns::Refine &rfIn)
{
  Base::serialIn(rfIn.base());
  sShape->serialIn(rfIn.seerShape());
  
  shapeMap.clear();
  for (const auto &r : rfIn.shapeMap())
    shapeMap.insert(std::make_pair(gu::stringToId(r.idIn()), gu::stringToId(r.idOut())));
}

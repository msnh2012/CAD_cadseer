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

#include <boost/optional.hpp>

#include "tools/featuretools.h"
#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "message/msgnode.h"
#include "selection/slceventhandler.h"
#include "library/lbrplabel.h"
#include "annex/annseershape.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrsweep.h"
#include "commandview/cmvmessage.h"
#include "commandview/cmvsweep.h"
#include "command/cmdsweep.h"

using boost::uuids::uuid;

using namespace cmd;

Sweep::Sweep()
: Base("cmd::Sweep")
, leafManager()
{
  feature = new ftr::Sweep();
  project->addFeature(std::unique_ptr<ftr::Sweep>(feature));
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
  isEdit = false;
  isFirstRun = true;
}

Sweep::Sweep(ftr::Base *fIn)
: Base("cmd::Sweep")
, leafManager(fIn)
{
  feature = dynamic_cast<ftr::Sweep*>(fIn);
  assert(feature);
  viewBase = std::make_unique<cmv::Sweep>(this);
  node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
  isEdit = true;
  isFirstRun = false;
}

Sweep::~Sweep() = default;

std::string Sweep::getStatusMessage()
{
  return QObject::tr("Select geometry for Sweep feature").toStdString();
}

void Sweep::activate()
{
  isActive = true;
  leafManager.rewind();
  if (isFirstRun.get())
  {
    isFirstRun = false;
    go();
  }
  if (viewBase)
  {
    feature->setEditing();
    cmv::Message vm(viewBase.get(), viewBase->getPaneWidth());
    msg::Message out(msg::Mask(msg::Request | msg::Command | msg::View | msg::Show), vm);
    node->sendBlocked(out);
  }
  else
    sendDone();
}

void Sweep::deactivate()
{
  if (viewBase)
  {
    feature->setNotEditing();
    msg::Message out(msg::Mask(msg::Request | msg::Command | msg::View | msg::Hide));
    node->sendBlocked(out);
  }
  leafManager.fastForward();
  if (!isEdit.get())
  {
    node->sendBlocked(msg::buildShowThreeD(feature->getId()));
    node->sendBlocked(msg::buildShowOverlay(feature->getId()));
  }
  isActive = false;
}

boost::optional<std::tuple<const ftr::Base*, const ann::SeerShape*>> getTuple(const slc::Message &mIn)
{
  const ftr::Base *feature = nullptr;
  const ann::SeerShape *seerShape = nullptr;
  
  auto *p = app::instance()->getProject();
  
  if (!p->hasFeature(mIn.featureId))
    return boost::none;
  feature = p->findFeature(mIn.featureId);
  
  if (!feature->hasAnnex(ann::Type::SeerShape))
    return boost::none;
  seerShape = &feature->getAnnex<ann::SeerShape>();
  if (seerShape->isNull())
    return boost::none;
  
  return std::make_tuple(feature, seerShape);
}

void Sweep::connectCommon(const slc::Message &spine, const Profiles &profiles)
{
  project->clearAllInputs(feature->getId());
  
  auto st = getTuple(spine);
  if (st)
  {
    const ftr::Base *spineFeature = nullptr;
    const ann::SeerShape *spineSeerShape = nullptr;
    std::tie(spineFeature, spineSeerShape) = st.get();
    
    ftr::Pick sp = tls::convertToPick(spine, *spineSeerShape, project->getShapeHistory());
    sp.tag = ftr::Sweep::spineTag;
    feature->setSpine(sp);
    project->connectInsert(spineFeature->getId(), feature->getId(), ftr::InputType{sp.tag});
  }
  ftr::SweepProfiles sps;
  for (const auto &p : profiles)
  {
    const auto &m = std::get<0>(p);
    auto pt = getTuple(m);
    if (!pt)
      continue;
    const ftr::Base *pf = nullptr;
    const ann::SeerShape *pss = nullptr;
    std::tie(pf, pss) = pt.get();

    ftr::Pick pp = tls::convertToPick(m, *pss, project->getShapeHistory());
    pp.tag = ftr::Sweep::profileTag + std::to_string(sps.size());
    sps.emplace_back(pp, std::get<1>(p), std::get<2>(p));
    project->connectInsert(pf->getId(), feature->getId(), ftr::InputType{pp.tag});
  }
  feature->setProfiles(sps);
}

// Corrected Frenet, Frenet, Discrete, Fixed
void Sweep::setCommon(const slc::Message &spine, const Profiles &profiles, int trihedron)
{
  assert(trihedron >=0 && trihedron <= 3);
  feature->setTrihedron(trihedron);
  connectCommon(spine, profiles);
}

void Sweep::setBinormal(const slc::Message &spine, const Profiles &profiles, const slc::Messages &bms)
{
  //trihedron is set by 'setBinormal'
  connectCommon(spine, profiles);
  
  ftr::Picks bps;
  for (const auto &m : bms)
  {
    //not using getTuple because we might have a datum axis with no seer shape.
    const ftr::Base *aFeature = project->findFeature(m.featureId);
    assert(aFeature);
    
    std::string tag = std::string(ftr::Sweep::binormalTag) + std::to_string(bps.size());
    ftr::Pick ap = tls::convertToPick(m, *project->findFeature(m.featureId), project->getShapeHistory());
    ap.tag = tag;
    project->connectInsert(aFeature->getId(), feature->getId(), ftr::InputType{ap.tag});
    bps.push_back(ap);
  }
  feature->setBinormal(ftr::SweepBinormal(bps));
}

void Sweep::setSupport(const slc::Message &spine, const Profiles &profiles, const slc::Message &support)
{
  //trihedron is set by 'setSupport'
  connectCommon(spine, profiles);
  
  auto st = getTuple(support);
  if (!st)
    return;
  ftr::Pick sp = tls::convertToPick(support, *std::get<1>(st.get()), project->getShapeHistory());
  sp.tag = ftr::Sweep::supportTag;
  project->connectInsert(support.featureId, feature->getId(), ftr::InputType{sp.tag});
  feature->setSupport(sp);
}

void Sweep::setAuxiliary(const slc::Message &spine, const Profiles &profiles, const Auxiliary &auxiliary)
{
  //trihedron is set by 'setAuxiliary'
  connectCommon(spine, profiles);
  
  auto st = getTuple(std::get<0>(auxiliary));
  if (!st)
    return;
  
  ftr::Pick ap = tls::convertToPick(std::get<0>(auxiliary), *std::get<1>(st.get()), project->getShapeHistory());
  ap.tag = ftr::Sweep::auxiliaryTag;
  project->connectInsert(std::get<0>(auxiliary).featureId, feature->getId(), ftr::InputType{ap.tag});
  feature->setAuxiliary(ftr::SweepAuxiliary(ap, std::get<1>(auxiliary), std::get<2>(auxiliary)));
}


void Sweep::localUpdate()
{
  assert(isActive);
  feature->updateModel(project->getPayload(feature->getId()));
  feature->updateVisual();
  feature->setModelDirty();
  node->sendBlocked(msg::Request | msg::DAG | msg::View | msg::Update);
}

bool Sweep::isValidSelection(const slc::Message &mIn)
{
  const ftr::Base *lf = project->findFeature(mIn.featureId);
  if (!lf->hasAnnex(ann::Type::SeerShape) || lf->getAnnex<ann::SeerShape>().isNull())
    return false;
  
  auto t = mIn.type;
  if
  (
    (t != slc::Type::Object)
    && (t != slc::Type::Wire)
    && (t != slc::Type::Edge)
  )
    return false;
  return true;
}

void Sweep::go()
{
  const slc::Containers &cs = eventHandler->getSelections();
  boost::optional<slc::Message> spine;
  Profiles profiles;
  
  for (const auto &c : cs)
  {
    auto m = slc::EventHandler::containerToMessage(c);
    if (!isValidSelection(m))
      continue;
    if (!spine)
    {
      spine = m;
      continue;
    }
    profiles.push_back({m, false, false});
  }
  if (spine && !profiles.empty())
  {
    setCommon(spine.get(), profiles, 0);
    node->sendBlocked(msg::buildStatusMessage("Sweep Added", 2.0));
    node->sendBlocked(msg::Message(msg::Request | msg::Selection | msg::Clear));
    return;
  }
  
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  node->sendBlocked(msg::buildShowThreeD(feature->getId()));
  node->sendBlocked(msg::buildShowOverlay(feature->getId()));
  viewBase = std::make_unique<cmv::Sweep>(this);
}

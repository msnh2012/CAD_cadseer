/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2020 Thomas S. Anderson blobfish.at.gmx.com
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

#include <cassert>

#include <boost/uuid/uuid_generators.hpp>

#include "globalutilities.h"
#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgmessage.h"
#include "feature/ftrbase.h"
#include "command/cmdleafmanager.h"

using boost::uuids::uuid;

struct cmd::LeafManager::Stow
{
  Stow() = delete;
  Stow(const uuid& fIdIn) : featureId(fIdIn){}
  uuid featureId;
  struct LeafState
  {
    uuid leafId;
    bool isVisible3d = false;
    bool isVisibleOverlay = false;
  };
  std::vector<LeafState> leafStates;
};

using namespace cmd;

/*! @brief constuct LeafManager
 * @details sets feature id to nil.
 * Works for new feature construction
 * where no rewind or fast forward is needed.
 */
LeafManager::LeafManager()
: LeafManager(boost::uuids::nil_uuid())
{}

/*! @brief Construct with feature id
 * @param fIdIn is the new feature id
 * @details Simply assigns the feature id.
 */
LeafManager::LeafManager(const uuid &fIdIn)
: stow(std::make_unique<Stow>(fIdIn))
{
  assert(!fIdIn.is_nil());
  assert(app::instance()->getProject()->hasFeature(fIdIn));
}

/*! @brief Construct with feature id
 * @param fIn feature for editing
 * @details convenience, just calls other constructor with feature id.
 */
LeafManager::LeafManager(const ftr::Base *fIn)
: LeafManager(fIn->getId())
{}

LeafManager::~LeafManager() = default;

/*! @brief puts project graph into a conducive state for editing the feature
 * @details this will store a snapshot of the current state of the project graph.
 * use fastForward to return to this prior state.
 */
void LeafManager::rewind()
{
  if (stow->featureId.is_nil())
    return;
  
  prj::Project *p = app::instance()->getProject();
  assert(p);
  assert(p->hasFeature(stow->featureId));
  
  //store current state
  //what happens to children belonging to the same path and we set the earlier one leaf last.
  //then the later one is inactive?????? see 'editing with rewind' in document.svg
  //TODO fix this bug.
  stow->leafStates.clear();
  ftr::UpdatePayload payload = p->getPayload(stow->featureId);
  auto features = payload.getFeatures("");
  for (const auto *f : features)
  {
    auto lc = p->getLeafChildren(f->getId());
    for (const uuid &id : lc)
    {
      assert(p->hasFeature(id));
      const ftr::Base *f = p->findFeature(id);
      stow->leafStates.push_back({id, f->isVisible3D(), f->isVisibleOverlay()});
      app::instance()->messageSlot(msg::buildHideOverlay(f->getId()));
    }
  }
  //TODO remove duplicates. But keep same order! Don't use gu::uniquefy.
  
  //set current state.
  for (const auto *f : features)
    p->setCurrentLeaf(f->getId());
  //editing feature is hidden by call setCurrentLeaf on inputs, so turn it back on for user.
  app::instance()->messageSlot(msg::buildShowThreeD(stow->featureId));
}

/*! @brief puts project graph into original state.
 * @details this will reflect the project graph state when rewind was last called.
 * Not going to assert on feature existence, project might have changed. Just test
 * and move on.
 */
void LeafManager::fastForward()
{
  if (stow->featureId.is_nil())
    return;
  
  prj::Project *p = app::instance()->getProject();
  assert(p);
  
  for (const auto &lf : stow->leafStates)
  {
    if (p->hasFeature(lf.leafId))
    {
      p->setCurrentLeaf(lf.leafId);
      if (lf.isVisible3d)
        app::instance()->messageSlot(msg::buildShowThreeD(lf.leafId));
      else
        app::instance()->messageSlot(msg::buildHideThreeD(lf.leafId));
      if (lf.isVisibleOverlay)
        app::instance()->messageSlot(msg::buildShowOverlay(lf.leafId));
      else
        app::instance()->messageSlot(msg::buildHideOverlay(lf.leafId));
    }
  }
}

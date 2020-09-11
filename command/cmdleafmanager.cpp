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

#include <boost/uuid/nil_generator.hpp>

#include "globalutilities.h"
#include "application/appapplication.h"
#include "project/prjproject.h"
#include "message/msgmessage.h"
#include "selection/slcmessage.h"
#include "feature/ftrbase.h"
#include "command/cmdleafmanager.h"

using boost::uuids::uuid;

struct cmd::LeafManager::Stow
{
  Stow() = delete;
  Stow(const uuid& fIdIn)
  {
    featureState.id = fIdIn;
  }
  struct LeafState
  {
    uuid id;
    bool isVisible3d = false;
    bool isVisibleOverlay = false;
    bool isSelectable = false;
  };
  LeafState featureState;
  std::vector<LeafState> leafStates;
};

using namespace cmd;

/*! @brief constuct LeafManager
 * @details sets feature id to nil.
 * Works for new feature construction
 * where no rewind or fast forward is needed.
 */
LeafManager::LeafManager()
: stow(std::make_unique<Stow>(boost::uuids::nil_uuid()))
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
  if (stow->featureState.id.is_nil())
    return;
  
  prj::Project *p = app::instance()->getProject();
  assert(p);
  assert(p->hasFeature(stow->featureState.id));
  
  //store current state
  auto setState = [&](const uuid &fId, Stow::LeafState &state)
  {
    assert(p->hasFeature(fId));
    const ftr::Base *f = p->findFeature(fId);
    state = {fId, f->isVisible3D(), f->isVisibleOverlay(), f->isSelectable()};
  };
  
  setState(stow->featureState.id, stow->featureState);
  
  stow->leafStates.clear();
  auto leafIds = p->getRelatedLeafs(stow->featureState.id);
  for (const auto &fId : leafIds)
  {
    stow->leafStates.emplace_back();
    setState(fId, stow->leafStates.back());
    app::instance()->messageSlot(msg::buildHideOverlay(stow->leafStates.back().id));
  }
  
  //set current state.
  for (const auto &id : p->getRewindInputs(stow->featureState.id))
    p->setCurrentLeaf(id);
  
  //rewind inputs are cleansed to remove redundant calls, so it can't be used it for setting visibility.
  for (const auto *ple : p->getPayload(stow->featureState.id).getFeatures(""))
  {
    app::instance()->messageSlot(msg::buildShowThreeD(ple->getId()));
    slc::Message sm;
    sm.featureId = ple->getId();
    app::instance()->messageSlot(msg::Message(msg::Request | msg::Selection | msg::Feature | msg::Thaw, sm));
  }
  
  //editing feature is hidden by call setCurrentLeaf on inputs, so turn it back on for user. Make unselectable.
  app::instance()->messageSlot(msg::buildShowThreeD(stow->featureState.id));
  slc::Message sm;
  sm.featureId = stow->featureState.id;
  app::instance()->messageSlot(msg::Message(msg::Request | msg::Selection | msg::Feature | msg::Freeze, sm));
}

/*! @brief puts project graph into original state.
 * @details this will reflect the project graph state when rewind was last called.
 * Not going to assert on feature existence, project might have changed. Just test
 * and move on.
 */
void LeafManager::fastForward()
{
  if (stow->featureState.id.is_nil())
    return;
  
  prj::Project *p = app::instance()->getProject();
  assert(p);
  
  auto restoreState = [&](const Stow::LeafState &stateIn)
  {
    if (!p->hasFeature(stateIn.id))
      return;
    if (stateIn.isVisible3d)
      app::instance()->queuedMessage(msg::buildShowThreeD(stateIn.id));
    else
      app::instance()->queuedMessage(msg::buildHideThreeD(stateIn.id));
    if (stateIn.isVisibleOverlay)
      app::instance()->queuedMessage(msg::buildShowOverlay(stateIn.id));
    else
      app::instance()->queuedMessage(msg::buildHideOverlay(stateIn.id));
    slc::Message sm;
    sm.featureId = stateIn.id;
    if (stateIn.isSelectable)
      app::instance()->queuedMessage(msg::Message(msg::Request | msg::Selection | msg::Feature | msg::Thaw, sm));
    else
      app::instance()->queuedMessage(msg::Message(msg::Request | msg::Selection | msg::Feature | msg::Freeze, sm));
  };
  
  restoreState(stow->featureState);
  
  for (const auto &lf : stow->leafStates)
  {
    if (p->hasFeature(lf.id))
    {
      p->setCurrentLeaf(lf.id);
      restoreState(lf);
    }
  }
}

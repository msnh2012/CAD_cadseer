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

#include "project/prjproject.h"
#include "selection/slceventhandler.h"
#include "feature/ftrbase.h"
#include "command/cmddirty.h"

using namespace cmd;

Dirty::Dirty() : Base() {}

Dirty::~Dirty() = default;

std::string Dirty::getStatusMessage()
{
  return QObject::tr("Select feature to Dirty").toStdString();
}

void Dirty::activate()
{
  isActive = true;
  go();
  sendDone();
}

void Dirty::deactivate()
{
  isActive = false;
}

void Dirty::go()
{
  shouldUpdate = false;
  assert(project);
  const slc::Containers &cs = eventHandler->getSelections();
  for (const auto &c : cs)
    project->findFeature(c.featureId)->setModelDirty();
}

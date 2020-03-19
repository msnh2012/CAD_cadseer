/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2018  Thomas S. Anderson blobfish.at.gmx.com
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

#include <QSettings>
#include <QGridLayout>
#include <QButtonGroup>
#include <QLabel>
#include <QDialogButtonBox>
#include <QTimer>

#include "globalutilities.h"
#include "tools/idtools.h"
#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "message/msgmessage.h"
#include "project/prjproject.h"
#include "selection/slcmessage.h"
#include "annex/annseershape.h"
#include "tools/featuretools.h"
#include "feature/ftrinputtype.h"
#include "feature/ftrintersect.h"
#include "feature/ftrsubtract.h"
#include "feature/ftrunion.h"
#include "dialogs/dlgwidgetgeometry.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgboolean.h"

using boost::uuids::uuid;

using namespace dlg;

Boolean::Boolean(ftr::Intersect *i, QWidget *parent) : QDialog(parent)
{
  intersect = i;
  booleanId = intersect->getId();
  init();
}

Boolean::Boolean(ftr::Subtract *s, QWidget *parent) : QDialog(parent)
{
  subtract = s;
  booleanId = subtract->getId();
  init();
}

Boolean::Boolean(ftr::Union *u, QWidget *parent) : QDialog(parent)
{
  onion = u;
  booleanId = onion->getId();
  init();
}

Boolean::~Boolean()
{
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("dlg::Boolean");

  settings.endGroup();
}

void Boolean::init()
{
  buildGui();
  
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("dlg::Boolean");

  settings.endGroup();
  
  WidgetGeometry *filter = new WidgetGeometry(this, "dlg::Boolean");
  this->installEventFilter(filter);
  
  QTimer::singleShot(0, targetButton, &QPushButton::click);
}

void Boolean::setEditDialog()
{
  isEditDialog =  true;
  
  prj::Project *project = app::instance()->getProject();
  assert(project);
  
  std::vector<ftr::Pick> targetPicks;
  std::vector<ftr::Pick> toolPicks;
  if (intersect)
  {
    targetPicks = intersect->getTargetPicks();
    toolPicks = intersect->getToolPicks();
  }
  else if (subtract)
  {
    targetPicks = subtract->getTargetPicks();
    toolPicks = subtract->getToolPicks();
  }
  else if (onion)
  {
    targetPicks = onion->getTargetPicks();
    toolPicks = onion->getToolPicks();
  }
  
  ftr::UpdatePayload payload = project->getPayload(booleanId);
  tls::Resolver tResolver(payload);
  if (!targetPicks.empty())
    tResolver.resolve(targetPicks.front());
  
  std::vector<tls::Resolver> toolResolvers;
  for (const auto &tp : toolPicks)
  {
    toolResolvers.emplace_back(payload);
    toolResolvers.back().resolve(tp);
  }

  slc::Messages targetMessages = tResolver.convertToMessages();
  slc::Messages toolMessages;
  for (const auto &tr : toolResolvers)
  {
    slc::Messages temp = tr.convertToMessages();
    std::copy(temp.begin(), temp.end(), std::back_inserter(toolMessages));
  }
  
  targetButton->setMessages(targetMessages);
  toolButton->setMessages(toolMessages);
  
  if ((!targetMessages.empty()) && (targetMessages.front().type == slc::Type::Solid))
      targetButton->mask = slc::None | slc::ObjectsEnabled | slc::SolidsEnabled | slc::SolidsSelectable;
  if ((!toolMessages.empty()) && (toolMessages.front().type == slc::Type::Solid))
      toolButton->mask = slc::None | slc::ObjectsEnabled | slc::SolidsEnabled | slc::SolidsSelectable;
  
  leafChildren.clear();

  auto lc = project->getLeafChildren(tResolver.getFeature()->getId());
  std::copy(lc.begin(), lc.end(), std::back_inserter(leafChildren));
  
  for (const auto &tr : toolResolvers)
  {
    auto lc = project->getLeafChildren(tr.getFeature()->getId());
    std::copy(lc.begin(), lc.end(), std::back_inserter(leafChildren));
  }
  
  project->setCurrentLeaf(tResolver.getFeature()->getId());
  for (const auto &tr : toolResolvers)
    project->setCurrentLeaf(tr.getFeature()->getId());
}

void Boolean::reject()
{
  isAccepted = false;
  finishDialog();
  QDialog::reject();
}

void Boolean::accept()
{
  isAccepted = true;
  finishDialog();
  QDialog::accept();
}

void Boolean::finishDialog()
{
  auto finishCommand = [&]()
  {
    msg::Message mOut(msg::Mask(msg::Request | msg::Command | msg::Done));
    app::instance()->queuedMessage(mOut);
  };
  
  prj::Project *p = static_cast<app::Application *>(qApp)->getProject();
  auto getSeerShape = [&](const uuid &idIn) -> const ann::SeerShape&
  {
    ftr::Base *f = p->findFeature(idIn);
    assert(f);
    assert(f->hasAnnex(ann::Type::SeerShape));
    return f->getAnnex<ann::SeerShape>();
  };
  
  if (isAccepted)
  {
    const slc::Messages &tms = targetButton->getMessages(); //target messages
    if (tms.empty())
    {
      finishCommand();
      return;
    }
    tls::Connector connector; //stores feature ids and tags for later connection.
    uuid tId = tms.front().featureId;
    ftr::Pick tp = tls::convertToPick(tms.front(), getSeerShape(tId), p->getShapeHistory());
    tp.tag = ftr::InputType::target;
    connector.add(tId, tp.tag);
    ftr::Picks tps(1, tp); //target picks
    
    const slc::Messages &tlms = toolButton->getMessages(); //tool messages.
    if (tlms.empty())
    {
      finishCommand();
      return;
    }
    ftr::Picks tlps; //tool picks
    int toolIndex = 0;
    for (const auto &m : tlms)
    {
      ftr::Pick pk = tls::convertToPick(tms.front(), getSeerShape(m.featureId), p->getShapeHistory());
      pk.tag = std::string(ftr::InputType::tool) + std::to_string(toolIndex);
      tlps.push_back(pk);
      connector.add(m.featureId, pk.tag);
      toolIndex++;
    }
    
    const osg::Vec4 &targetColor = p->findFeature(tId)->getColor();
    if (intersect)
    {
      booleanId = intersect->getId();
      intersect->setTargetPicks(tps);
      intersect->setToolPicks(tlps);
      intersect->setColor(targetColor);
      intersect->setModelDirty();
    }
    else if (subtract)
    {
      booleanId = subtract->getId();
      subtract->setTargetPicks(tps);
      subtract->setToolPicks(tlps);
      subtract->setColor(targetColor);
      subtract->setModelDirty();
    }
    else if (onion)
    {
      booleanId = onion->getId();
      onion->setTargetPicks(tps);
      onion->setToolPicks(tlps);
      onion->setColor(targetColor);
      onion->setModelDirty();
    }
    
    if (isEditDialog)
    {
      p->clearAllInputs(booleanId);
    }
    
    for (const auto &cp : connector.pairs)
    {
      p->connectInsert(cp.first, booleanId, {cp.second});
      app::instance()->messageSlot(msg::buildHideThreeD(cp.first));
      app::instance()->messageSlot(msg::buildHideOverlay(cp.first));
    }
  }
  else //rejected dialog
  {
    if (!isEditDialog)
    {
      if (intersect)
        p->removeFeature(intersect->getId());
      else if (subtract)
        p->removeFeature(subtract->getId());
      else if (onion)
        p->removeFeature(onion->getId());
    }
  }
  
  if (isEditDialog)
  {
    for (const auto &id : leafChildren)
      p->setCurrentLeaf(id);
  }
  
  finishCommand();
}

void Boolean::buildGui()
{
  QGridLayout *gl = new QGridLayout();
  bGroup = new QButtonGroup(this);
  
  QPixmap pmap = QPixmap(":resources/images/cursor.svg").scaled(32, 32, Qt::KeepAspectRatio);
  
  QLabel *targetLabel = new QLabel(tr("Target:"), this);
  targetButton = new SelectionButton(pmap, QString(), this);
  targetButton->isSingleSelection = true;
  targetButton->mask = slc::None | slc::ObjectsEnabled | slc::ObjectsSelectable | slc::SolidsEnabled;
  targetButton->statusPrompt = tr("Select Target");
  gl->addWidget(targetLabel, 0, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(targetButton, 0, 1, Qt::AlignVCenter | Qt::AlignCenter);
  connect(targetButton, &SelectionButton::advance, this, &Boolean::advanceSlot);
  bGroup->addButton(targetButton);
  
  QLabel *toolLabel = new QLabel(tr("Tools:"), this);
  toolButton = new SelectionButton(pmap, QString(), this);
  toolButton->isSingleSelection = true;
  toolButton->mask = slc::None | slc::ObjectsEnabled | slc::ObjectsSelectable | slc::SolidsEnabled;
  toolButton->statusPrompt = tr("Select Tools");
  gl->addWidget(toolLabel, 1, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(toolButton, 1, 1, Qt::AlignVCenter | Qt::AlignCenter);
  bGroup->addButton(toolButton);
  
  QDialogButtonBox *buttons = new QDialogButtonBox
    (QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  buttonLayout->addStretch();
  buttonLayout->addWidget(buttons);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
  
  QHBoxLayout *hl = new QHBoxLayout();
  hl->addLayout(gl);
  hl->addStretch();
  
  QVBoxLayout *vl = new QVBoxLayout();
  vl->addLayout(hl);
  vl->addSpacing(10);
  vl->addLayout(buttonLayout);
  vl->addStretch();
  
  this->setLayout(vl);
}

void Boolean::advanceSlot()
{
  QAbstractButton *cb = bGroup->checkedButton();
  if (!cb)
    return;
  if (cb == targetButton)
    toolButton->setChecked(true);
}

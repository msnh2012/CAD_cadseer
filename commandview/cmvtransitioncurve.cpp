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

#include <QSettings>
#include <QVBoxLayout>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionwidget.h"
#include "commandview/cmvparameterwidgets.h"
#include "parameter/prmparameter.h"
#include "tools/featuretools.h"
#include "feature/ftrtransitioncurve.h"
#include "command/cmdtransitioncurve.h"
#include "commandview/cmvtransitioncurve.h"

using boost::uuids::uuid;

using namespace cmv;

struct TransitionCurve::Stow
{
  cmd::TransitionCurve *command;
  cmv::TransitionCurve *view;
  dlg::SelectionWidget *selectionWidget = nullptr;
  cmv::ParameterWidget *parameterWidget = nullptr;
  
  Stow(cmd::TransitionCurve *cIn, cmv::TransitionCurve *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::TransitionCurve");
    //load settings
    settings.endGroup();
    
    loadFeatureData();
    glue();
    selectionWidget->activate(0);
  }
  
  void buildGui()
  {
    QSizePolicy adjust = view->sizePolicy();
    adjust.setVerticalPolicy(QSizePolicy::Maximum);
    view->setSizePolicy(adjust);
    
    QVBoxLayout *mainLayout = new QVBoxLayout();
    view->setLayout(mainLayout);
    
    std::vector<dlg::SelectionWidgetCue> cues;
    dlg::SelectionWidgetCue cue;
    cue.name = tr("Item 1");
    cue.singleSelection = true;
    cue.mask = slc::PointsEnabled | slc::PointsSelectable | slc::AllPointsEnabled | slc::EndPointsEnabled;
    cue.statusPrompt = tr("Select Point On Curve");
    cue.showAccrueColumn = false;
    cues.push_back(cue);
    cue.name = tr("Item 2");
    cues.push_back(cue);
    
    selectionWidget = new dlg::SelectionWidget(view, cues);
    mainLayout->addWidget(selectionWidget);
    
    parameterWidget = new cmv::ParameterWidget(view, command->feature->getParameters());
    mainLayout->addWidget(parameterWidget);
    mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
  }
  
  void loadFeatureData()
  {
    ftr::UpdatePayload up = view->project->getPayload(command->feature->getId());
    tls::Resolver resolver(up);
    
    auto picksToMessages = [&](const ftr::Picks &pIn) -> slc::Messages
    {
      slc::Messages out;
      for (const auto &p : pIn)
      {
        if (resolver.resolve(p))
        {
          auto msgs = resolver.convertToMessages();
          out.insert(out.end(), msgs.begin(), msgs.end());
        }
      }
      return out;
    };
    
    const auto &ps = picksToMessages(command->feature->getPicks());
    if (ps.size() > 0)
      selectionWidget->initializeButton(0, ps.at(0));
    if (ps.size() > 1)
      selectionWidget->initializeButton(1, ps.at(1));
  }
  
  void glue()
  {
    QObject::connect(selectionWidget->getButton(0), &dlg::SelectionButton::dirty, view, &TransitionCurve::selectionChanged);
    QObject::connect(selectionWidget->getButton(1), &dlg::SelectionButton::dirty, view, &TransitionCurve::selectionChanged);
    QObject::connect(parameterWidget, &ParameterBase::prmValueChanged, view, &TransitionCurve::parameterChanged);
  }
  
  void goSelections()
  {
    auto msgs = selectionWidget->getMessages(0);
    const auto &temp = selectionWidget->getMessages(1);
    msgs.insert(msgs.end(), temp.begin(), temp.end());
    command->setSelections(msgs);
    goUpdate();
  }
  
  void goUpdate()
  {
    command->localUpdate();
  }
};

TransitionCurve::TransitionCurve(cmd::TransitionCurve *cIn)
: Base("cmv::TransitionCurve")
, stow(new Stow(cIn, this))
{}

TransitionCurve::~TransitionCurve() = default;

void TransitionCurve::selectionChanged()
{
  stow->goSelections();
}

void TransitionCurve::parameterChanged()
{
  stow->goUpdate();
}

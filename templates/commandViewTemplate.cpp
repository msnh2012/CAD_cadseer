/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) %YEAR% Thomas S. Anderson blobfish.at.gmx.com
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
// #include <QDialogButtonBox>
// #include <QVBoxLayout>
// #include <QHBoxLayout>
// #include <QLineEdit>

#include "application/appapplication.h"
#include "project/prjproject.h"
// #include "annex/annseershape.h"
// #include "preferences/preferencesXML.h"
// #include "preferences/prfmanager.h"
// #include "message/msgmessage.h"
// #include "message/msgnode.h"
// #include "dialogs/dlgselectionbutton.h"
// #include "dialogs/dlgselectionlist.h"
// #include "dialogs/dlgselectionwidget.h"
// #include "dialogs/dlgparameterwidget.h"
// #include "parameter/prmparameter.h"
// #include "expressions/exprmanager.h"
// #include "expressions/exprstringtranslator.h"
// #include "expressions/exprvalue.h"
// #include "library/lbrplabel.h"
// #include "tools/featuretools.h"
// #include "tools/idtools.h"
// #include "feature/ftrinputtype.h"
#include "feature/ftr%CLASSNAMELOWERCASE%.h"
#include "commandview/cmv%CLASSNAMELOWERCASE%.h"

using boost::uuids::uuid;

using namespace cmv;

struct %CLASSNAME%::Stow
{
  ftr::%CLASSNAME% *feature;
//   SelectionWidget *selectionWidget = nullptr;
//   ParameterWidget *parameterWidget = nullptr;
//   std::shared_ptr<prm::Parameter> parameter;
  
  Stow(ftr::%CLASSNAME% *fIn)
  : feature(fIn)
//   , parameter(std::make_shared<prm::Parameter>(*feature->getParameter()))
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::%CLASSNAME%");
    //load settings
    settings.endGroup();
    
    loadFeatureData();
//     stow->selectionWidget->activate(0);
  }
  
  void buildGui()
  {}
  
  void loadFeatureData()
  {}
};

%CLASSNAME%::%CLASSNAME%(ftr::%CLASSNAME% *fIn)
: Base()
, stow(new Stow(fIn))
{}

%CLASSNAME%::~%CLASSNAME%() = default;

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

// #include <cassert>
// #include <boost/optional/optional.hpp>
#include <boost/filesystem.hpp>

#include <QSettings>
#include <QTabWidget>
#include <QComboBox>
// #include <QPushButton>
#include <QButtonGroup>
#include <QLabel>
#include <QLineEdit>
// #include <QStackedWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QFileDialog>

#include "application/appapplication.h"
#include "application/appmainwindow.h"
#include "project/prjproject.h"
#include "viewer/vwrwidget.h"
// #include "annex/annseershape.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
// #include "message/msgmessage.h"
// #include "message/msgnode.h"
#include "dialogs/dlgselectionbutton.h"
// #include "dialogs/dlgselectionlist.h"
#include "dialogs/dlgselectionwidget.h"
// #include "commandview/cmvparameterwidgets.h"
#include "parameter/prmparameter.h"
// #include "expressions/exprmanager.h"
// #include "expressions/exprstringtranslator.h"
// #include "expressions/exprvalue.h"
// #include "library/lbrplabel.h"
// #include "tools/featuretools.h"
#include "tools/idtools.h"
// #include "feature/ftrinputtype.h"
#include "feature/ftrquote.h"
#include "command/cmdquote.h"
#include "commandview/cmvquote.h"

using boost::uuids::uuid;

using namespace cmv;

struct Quote::Stow
{
  cmd::Quote *command;
  cmv::Quote *view;
  dlg::SelectionWidget *selectionWidget = nullptr;
//   cmv::ParameterWidget *parameterWidget = nullptr;
//   std::vector<prm::Observer> observers;
    
  QTabWidget *tabWidget;
  dlg::SelectionButton *stripButton;
  dlg::SelectionButton *diesetButton;
  QLabel *stripIdLabel;
  QLabel *diesetIdLabel;
  QLineEdit *tFileEdit; //!< template sheet in.
  QLineEdit *oFileEdit; //!< quote sheet out
  QButtonGroup *bGroup;
  QLineEdit *pNameEdit;
  QLineEdit *pNumberEdit;
  QLineEdit *pRevisionEdit;
  QLineEdit *pmTypeEdit;
  QLineEdit *pmThicknessEdit;
  QLineEdit *sQuoteNumberEdit;
  QLineEdit *sCustomerNameEdit;
  QComboBox *sPartSetupCombo;
  QComboBox *sProcessTypeCombo;
  QLineEdit *sAnnualVolumeEdit;
  QLabel *pLabel;
  QPushButton *browseForTemplate;
  QPushButton *browseForOutput;
  QPushButton *takePicture;
  
  Stow(cmd::Quote *cIn, cmv::Quote *vIn)
  : command(cIn)
  , view(vIn)
  {
    buildGui();
    
    QSettings &settings = app::instance()->getUserSettings();
    settings.beginGroup("cmv::Quote");
    //load settings
    settings.endGroup();
    
    loadFeatureData();
    glue();
    selectionWidget->activate(0);
  }
  
  void buildGui()
  {
    tabWidget = new QTabWidget(view);
    tabWidget->addTab(buildInputPage(), tr("Input"));
    tabWidget->addTab(buildPartPage(), tr("Part"));
    tabWidget->addTab(buildQuotePage(), tr("Quote"));
    tabWidget->addTab(buildPicturePage(), tr("Picture"));
    
    QVBoxLayout *vl = new QVBoxLayout();
    vl->addWidget(tabWidget);
    view->setLayout(vl);
  }
  
  QWidget* buildInputPage()
  {
    QWidget *out = new QWidget(tabWidget);
    
    std::vector<dlg::SelectionWidgetCue> cues;
    dlg::SelectionWidgetCue cue;
    cue.name = tr("Strip");
    cue.singleSelection = true;
    cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable;
    cue.statusPrompt = tr("Select Strip Object");
    cue.showAccrueColumn = false;
    cues.push_back(cue);
    cue.name = tr("Die Set");
    cue.singleSelection = false;
    cue.mask = slc::ObjectsEnabled | slc::ObjectsSelectable;
    cue.statusPrompt = tr("Select Die Set Object Object Or Solid");
    cues.push_back(cue);
    selectionWidget = new dlg::SelectionWidget(view, cues);
    
    QGridLayout *gl = new QGridLayout();
    QLabel *bftLabel = new QLabel(tr("Template Sheet:"), out);
    browseForTemplate = new QPushButton(tr("Browse"), out);
    tFileEdit = new QLineEdit(out);
    gl->addWidget(bftLabel, 0, 0, Qt::AlignVCenter | Qt::AlignRight);
    gl->addWidget(browseForTemplate, 0, 1, Qt::AlignVCenter | Qt::AlignCenter);
    gl->addWidget(tFileEdit, 0, 2);
    
    //bfo = browse for output.
    QLabel *bfoLabel = new QLabel(tr("Output Sheet:"), out);
    browseForOutput = new QPushButton(tr("Browse"), out);
    oFileEdit = new QLineEdit(out);
    gl->addWidget(bfoLabel, 1, 0, Qt::AlignVCenter | Qt::AlignRight);
    gl->addWidget(browseForOutput, 1, 1, Qt::AlignVCenter | Qt::AlignCenter);
    gl->addWidget(oFileEdit, 1, 2);
    
    QVBoxLayout *vl = new QVBoxLayout();
    vl->addWidget(selectionWidget);
    vl->addLayout(gl);
    vl->addStretch();
    
    out->setLayout(vl);
    
    return out;
  }

  QWidget* buildPartPage()
  {
    QWidget *out = new QWidget(tabWidget);
    
    QGridLayout *gl = new QGridLayout();
    
    QLabel *pNameLabel = new QLabel(tr("Part Name:"), out);
    pNameEdit = new QLineEdit(out);
    gl->addWidget(pNameLabel, 0, 0, Qt::AlignVCenter | Qt::AlignRight);
    gl->addWidget(pNameEdit, 0, 1, Qt::AlignVCenter | Qt::AlignLeft);
    
    QLabel *pNumberLabel = new QLabel(tr("Part Number:"), out);
    pNumberEdit = new QLineEdit(out);
    gl->addWidget(pNumberLabel, 1, 0, Qt::AlignVCenter | Qt::AlignRight);
    gl->addWidget(pNumberEdit, 1, 1, Qt::AlignVCenter | Qt::AlignLeft);
    
    QLabel *pRevisionLabel = new QLabel(tr("Part Revision:"), out);
    pRevisionEdit = new QLineEdit(out);
    gl->addWidget(pRevisionLabel, 2, 0, Qt::AlignVCenter | Qt::AlignRight);
    gl->addWidget(pRevisionEdit, 2, 1, Qt::AlignVCenter | Qt::AlignLeft);
    
    QLabel *pmTypeLabel = new QLabel(tr("Material Type:"), out);
    pmTypeEdit = new QLineEdit(out);
    gl->addWidget(pmTypeLabel, 3, 0, Qt::AlignVCenter | Qt::AlignRight);
    gl->addWidget(pmTypeEdit, 3, 1, Qt::AlignVCenter | Qt::AlignLeft);
    
    QLabel *pmThicknessLabel = new QLabel(tr("Material Thickness:"), out);
    pmThicknessEdit = new QLineEdit(out);
    gl->addWidget(pmThicknessLabel, 4, 0, Qt::AlignVCenter | Qt::AlignRight);
    gl->addWidget(pmThicknessEdit, 4, 1, Qt::AlignVCenter | Qt::AlignLeft);
    
    QHBoxLayout *hl = new QHBoxLayout();
    hl->addLayout(gl);
    hl->addStretch();
    
    QVBoxLayout *vl = new QVBoxLayout();
    vl->addLayout(hl);
    vl->addStretch();
    
    out->setLayout(vl);
    
    return out;
  }

  QWidget* buildQuotePage()
  {
    QWidget *out = new QWidget(tabWidget);
    
    QGridLayout *gl = new QGridLayout();
    
    QLabel *sQuoteNumberLabel = new QLabel(tr("Quote Number:"), out);
    sQuoteNumberEdit = new QLineEdit(out);
    gl->addWidget(sQuoteNumberLabel, 0, 0, Qt::AlignVCenter | Qt::AlignRight);
    gl->addWidget(sQuoteNumberEdit, 0, 1, Qt::AlignVCenter | Qt::AlignLeft);
    
    QLabel *sCustomerNameLabel = new QLabel(tr("Customer Name:"), out);
    sCustomerNameEdit = new QLineEdit(out);
    gl->addWidget(sCustomerNameLabel, 1, 0, Qt::AlignVCenter | Qt::AlignRight);
    gl->addWidget(sCustomerNameEdit, 1, 1, Qt::AlignVCenter | Qt::AlignLeft);
    
    QLabel *sPartSetupLabel = new QLabel(tr("Part Setup:"), out);
    sPartSetupCombo = new QComboBox(out);
    sPartSetupCombo->setEditable(true);
    sPartSetupCombo->setInsertPolicy(QComboBox::NoInsert);
    sPartSetupCombo->addItem(tr("One Out"));
    sPartSetupCombo->addItem(tr("Two Out"));
    sPartSetupCombo->addItem(tr("Sym Opposite"));
    gl->addWidget(sPartSetupLabel, 2, 0, Qt::AlignVCenter | Qt::AlignRight);
    gl->addWidget(sPartSetupCombo, 2, 1, Qt::AlignVCenter | Qt::AlignLeft);
    
    QLabel *sProcessTypeLabel = new QLabel(tr("Process Type:"), out);
    sProcessTypeCombo = new QComboBox(out);
    sProcessTypeCombo->setEditable(true);
    sProcessTypeCombo->setInsertPolicy(QComboBox::NoInsert);
    sProcessTypeCombo->addItem(tr("Prog"));
    sProcessTypeCombo->addItem(tr("Prog Partial"));
    sProcessTypeCombo->addItem(tr("Mech Transfer"));
    sProcessTypeCombo->addItem(tr("Hand Transfer"));
    gl->addWidget(sProcessTypeLabel, 3, 0, Qt::AlignVCenter | Qt::AlignRight);
    gl->addWidget(sProcessTypeCombo, 3, 1, Qt::AlignVCenter | Qt::AlignLeft);
    
    QLabel *sAnnualVolumeLabel = new QLabel(tr("Annual Volume:"), out);
    sAnnualVolumeEdit = new QLineEdit(out);
    gl->addWidget(sAnnualVolumeLabel, 4, 0, Qt::AlignVCenter | Qt::AlignRight);
    gl->addWidget(sAnnualVolumeEdit, 4, 1, Qt::AlignVCenter | Qt::AlignLeft);
    
    QHBoxLayout *hl = new QHBoxLayout();
    hl->addLayout(gl);
    hl->addStretch();
    
    QVBoxLayout *vl = new QVBoxLayout();
    vl->addLayout(hl);
    vl->addStretch();
    
    out->setLayout(vl);
    
    return out;
  }

  QWidget* buildPicturePage()
  {
    QWidget *out = new QWidget(tabWidget);
    
    QVBoxLayout *vl = new QVBoxLayout();
    out->setLayout(vl);
    
    QHBoxLayout *bl = new QHBoxLayout();
    vl->addLayout(bl);
    takePicture = new QPushButton(tr("Take Picture"), out);
    bl->addWidget(takePicture);
    bl->addStretch();
    
    pLabel = new QLabel(out);
    pLabel->setScaledContents(true);
    vl->addWidget(pLabel);
    
    return out;
  }
  
  void loadLabelPixmapSlot()
  {
    pLabel->setText(QString::fromStdString(command->feature->pFile.string() + " not found"));
    QPixmap pMap(command->feature->pFile.c_str());
    if (!pMap.isNull())
    {
      QPixmap scaled = pMap.scaledToHeight(pMap.height() / 2.0);
      pLabel->setPixmap(scaled);
    }
  }
  
  void loadFeatureData()
  {
    pNameEdit->setText(command->feature->quoteData.partName);
    pNumberEdit->setText(command->feature->quoteData.partNumber);
    pRevisionEdit->setText(command->feature->quoteData.partRevision);
    pmTypeEdit->setText(command->feature->quoteData.materialType);
    pmThicknessEdit->setText(QString::number(command->feature->quoteData.materialThickness));
    sQuoteNumberEdit->setText(QString::number(command->feature->quoteData.quoteNumber));
    sCustomerNameEdit->setText(command->feature->quoteData.customerName);
    sPartSetupCombo->setCurrentText(command->feature->quoteData.partSetup);
    sProcessTypeCombo->setCurrentText(command->feature->quoteData.processType);
    sAnnualVolumeEdit->setText(QString::number(command->feature->quoteData.annualVolume));
    
    loadLabelPixmapSlot();
    
    tFileEdit->setText(QString::fromStdString(static_cast<boost::filesystem::path>(*command->feature->tFile).string()));
    oFileEdit->setText(QString::fromStdString(static_cast<boost::filesystem::path>(*command->feature->oFile).string()));
    
    auto payload = view->project->getPayload(command->feature->getId());
    auto buildMessage = [](const std::vector<const ftr::Base*> &parents) -> slc::Message
    {
      slc::Message mOut;
      mOut.type = slc::Type::Object;
      mOut.featureType = parents.front()->getType();
      mOut.featureId = parents.front()->getId();
      return mOut;
    };
    
    {
      auto strip = payload.getFeatures(ftr::Quote::strip);
      if (!strip.empty())
      {
        slc::Message mOut = buildMessage(strip);
        selectionWidget->initializeButton(0, mOut);
      }
    }
    {
      auto dieset = payload.getFeatures(ftr::Quote::dieSet);
      if (!dieset.empty())
      {
        slc::Message mOut = buildMessage(dieset);
        selectionWidget->initializeButton(1, mOut);
      }
    }
  }
  
  void glue()
  {
    connect(browseForTemplate, &QPushButton::clicked, view, &Quote::browseForTemplateSlot);
    connect(browseForOutput, &QPushButton::clicked, view, &Quote::browseForOutputSlot);
    connect(takePicture, &QPushButton::clicked, view, &Quote::takePictureSlot);
    connect(selectionWidget->getButton(0), &dlg::SelectionButton::dirty, view, &Quote::selectionsChanged);
    connect(selectionWidget->getButton(1), &dlg::SelectionButton::dirty, view, &Quote::selectionsChanged);
    
    connect(pNameEdit, &QLineEdit::editingFinished, view, &Quote::valueChanged);
    connect(pNumberEdit, &QLineEdit::editingFinished, view, &Quote::valueChanged);
    connect(pRevisionEdit, &QLineEdit::editingFinished, view, &Quote::valueChanged);
    connect(pmTypeEdit, &QLineEdit::editingFinished, view, &Quote::valueChanged);
    connect(pmThicknessEdit, &QLineEdit::editingFinished, view, &Quote::valueChanged);
    
    connect(sQuoteNumberEdit, &QLineEdit::editingFinished, view, &Quote::valueChanged);
    connect(sCustomerNameEdit, &QLineEdit::editingFinished, view, &Quote::valueChanged);
    connect(sPartSetupCombo, SIGNAL(currentIndexChanged(int)), view, SLOT(comboValueChanged(int)));
    connect(sProcessTypeCombo, SIGNAL(currentIndexChanged(int)), view, SLOT(comboValueChanged(int)));
    connect(sAnnualVolumeEdit, &QLineEdit::editingFinished, view, &Quote::valueChanged);
  }
  
  void updateValues()
  {
    command->feature->quoteData.partName = pNameEdit->text();
    command->feature->quoteData.partNumber = pNumberEdit->text();
    command->feature->quoteData.partRevision = pRevisionEdit->text();
    command->feature->quoteData.materialType = pmTypeEdit->text();
    command->feature->quoteData.materialThickness = pmThicknessEdit->text().toDouble();
    command->feature->quoteData.quoteNumber = sQuoteNumberEdit->text().toInt();
    command->feature->quoteData.customerName = sCustomerNameEdit->text();
    command->feature->quoteData.partSetup = sPartSetupCombo->currentText();
    command->feature->quoteData.processType = sProcessTypeCombo->currentText();
    command->feature->quoteData.annualVolume = sAnnualVolumeEdit->text().toInt();
  }
};

Quote::Quote(cmd::Quote *cIn)
: Base("cmv::Quote")
, stow(new Stow(cIn, this))
{}

Quote::~Quote() = default;

void Quote::takePictureSlot()
{
  /* the osg screen capture handler is designed to automatically
   * add indexes to the file names. We have to work around that here.
   * it will add '_0' to the filename we give it before the dot and extension
   */
  
  namespace bfs = boost::filesystem;
  
  app::Application *app = static_cast<app::Application *>(qApp);
  bfs::path pp = (app->getProject()->getSaveDirectory());
  assert(bfs::exists(pp));
  bfs::path fp = pp /= gu::idToString(stow->command->feature->getId());
  std::string ext = "png";
  bfs::path fullPath = fp.string() + "_0." + ext;
  
  stow->command->feature->pFile = fullPath;
  
  app->getMainWindow()->getViewer()->screenCapture(fp.string(), ext);
  
  QTimer::singleShot(1000, this, SLOT(loadLabelPixmapSlot()));
}

void Quote::loadLabelPixmapSlot()
{
  stow->pLabel->setText(QString::fromStdString(stow->command->feature->pFile.string() + " not found"));
  QPixmap pMap(stow->command->feature->pFile.c_str());
  if (!pMap.isNull())
  {
    QPixmap scaled = pMap.scaledToHeight(pMap.height() / 2.0);
    stow->pLabel->setPixmap(scaled);
  }
}

void Quote::browseForTemplateSlot()
{
  namespace bfs = boost::filesystem;
  bfs::path t = stow->tFileEdit->text().toStdString();
  if (!bfs::exists(t)) //todo use a parameter from preferences.
    t = prf::manager().rootPtr->project().lastDirectory().get();
  
  QString fileName = QFileDialog::getOpenFileName
  (
    this,
    tr("Browse For Template"),
    QString::fromStdString(t.string()),
    tr("SpreadSheet (*.ods)")
  );
  
  if (fileName.isEmpty())
    return;
  
  boost::filesystem::path p = fileName.toStdString();
  prf::manager().rootPtr->project().lastDirectory() = p.remove_filename().string();
  prf::manager().saveConfig();
  
  stow->tFileEdit->setText(fileName);
}

void Quote::browseForOutputSlot()
{
  namespace bfs = boost::filesystem;
  bfs::path t = stow->oFileEdit->text().toStdString();
  if (!bfs::exists(t.parent_path()))
  {
    t = prf::manager().rootPtr->project().lastDirectory().get();
    t /= stow->command->feature->getName().toStdString() + ".ods";
  }
  
  QString fileName = QFileDialog::getSaveFileName
  (
    this,
    tr("Browse For Output"),
    QString::fromStdString(t.string()),
    tr("SpreadSheet (*.ods)")
  );
  
  if (fileName.isEmpty())
    return;
  
  boost::filesystem::path p = fileName.toStdString();
  prf::manager().rootPtr->project().lastDirectory() = p.remove_filename().string();
  prf::manager().saveConfig();
  
  stow->oFileEdit->setText(fileName);
}

void Quote::selectionsChanged()
{
  stow->command->setSelections(stow->selectionWidget->getButton(0)->getMessages(), stow->selectionWidget->getButton(1)->getMessages());
  stow->command->localUpdate();
}

void Quote::templateChanged(const QString&)
{
  stow->command->feature->tFile->setValue(boost::filesystem::path(stow->tFileEdit->text().toStdString()));
  //call update here? If the user is actually typing, it might be slow.
}

void Quote::outputChanged(const QString&)
{
  stow->command->feature->oFile->setValue(boost::filesystem::path(stow->oFileEdit->text().toStdString()));
  //call update here?
}

void Quote::valueChanged()
{
  stow->updateValues();
  stow->command->localUpdate();
}

void Quote::comboValueChanged(int)
{
  stow->updateValues();
  stow->command->localUpdate();
}

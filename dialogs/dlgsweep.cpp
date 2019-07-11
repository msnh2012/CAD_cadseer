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

#include <cassert>

#include <QSettings>
#include <QGridLayout>
#include <QButtonGroup>
#include <QLabel>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QListWidget>
#include <QTimer>
#include <QAction>
#include <QCheckBox>
#include <QStackedWidget>

#include "globalutilities.h"
#include "tools/idtools.h"
#include "tools/featuretools.h"
#include "application/appapplication.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "project/prjproject.h"
#include "selection/slcmessage.h"
#include "annex/annseershape.h"
#include "annex/annlawfunction.h"
#include "tools/featuretools.h"
#include "feature/ftrinputtype.h"
#include "library/lbrplabel.h"
#include "parameter/prmparameter.h"
#include "feature/ftrsweep.h"
#include "law/lwfcue.h"
#include "dialogs/dlglawfunctionwidget.h"
#include "dialogs/dlgwidgetgeometry.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgsweep.h"

using boost::uuids::uuid;

class ListItem : public QListWidgetItem
{
public:
  ListItem(const QString &text) : QListWidgetItem(text){}
  bool contact = false;
  bool correction = false;
};

namespace dlg
{
  class SelectionList : public QListWidget
  {
  public:
    SelectionList(QWidget *parent, SelectionButton *button)
    : QListWidget(parent)
    , sButton(button)
    {
      this->setSelectionMode(QAbstractItemView::NoSelection);
      connect(sButton, &SelectionButton::dirty, this, &SelectionList::populateList);
    }
    
    static QString makeShortString(const slc::Message &m)
    {
      QString out = QString::fromStdString(gu::idToShortString(m.featureId));
      if (!m.shapeId.is_nil())
        out += ":" + QString::fromStdString(gu::idToShortString(m.shapeId));
      return out;
    }
    
    static QString makeFullString(const slc::Message &m)
    {
      QString out = QString::fromStdString(gu::idToString(m.featureId));
      if (!m.shapeId.is_nil())
        out += ":" + QString::fromStdString(gu::idToString(m.shapeId));
      return out;
    }
    
    void populateList()
    {
      //take all widget items and pack into a map. list widget is empty
      QMap<QString, QListWidgetItem*> map;
      while(count())
      {
        map.insert(item(0)->statusTip(), item(0));
        takeItem(0);
      }
      
      for (const auto &m : sButton->getMessages())
      {
        QString idString = makeFullString(m);
        auto it = map.find(idString);
        if (it != map.end())
        {
          addItem(*it);
          map.erase(it);
        }
        else
        {
          QString name = makeShortString(m);
          ListItem *ni = new ListItem(name);
          ni->setStatusTip(idString);
          this->addItem(ni);
        }
      }
      
      //now we have to manually delete any remaining items in the map.
      for (auto *v : map.values())
        delete v;
    }
  private:
    SelectionButton *sButton;
  };
  
  class ProfilePage : public QWidget
  {
  public:
    ProfilePage(QWidget *parent, SelectionButton *buttonIn)
    : QWidget(parent)
    , button(buttonIn)
    {
      list = new SelectionList(this, button);
      contact = new QCheckBox(tr("Contact"), this);
      correction = new QCheckBox(tr("Correction"), this);
      lawButton = new QPushButton(tr("Law"), this);
      lawButton->setWhatsThis(tr("Homothetic Law For One Profile"));
      lawCheck = new QCheckBox(tr("Use Law"), this);
      lawCheck->setWhatsThis(tr("To Enable Or Disable The Law"));
      contact->setDisabled(true);
      correction->setDisabled(true);
      lawButton->setDisabled(true); //enabled in selection handler
      lawCheck->setDisabled(true);
      
      list->setSelectionMode(QAbstractItemView::SingleSelection);
      connect(list, &QListWidget::itemSelectionChanged, this, &ProfilePage::rowChanged);
      connect(contact, &QCheckBox::toggled, this, &ProfilePage::contactSlot);
      connect(correction, &QCheckBox::toggled, this, &ProfilePage::correctionSlot);
      connect(lawButton, &QPushButton::clicked, this, &ProfilePage::lawButtonClicked);
      connect(lawCheck, &QPushButton::toggled, this, &ProfilePage::lawCheckToggled);
      //insert law button connection
      
      QVBoxLayout *lawLayout = new QVBoxLayout();
      lawLayout->addWidget(lawButton);
      lawLayout->addWidget(lawCheck);
      lawLayout->addStretch();
      
      QGridLayout *gl = new QGridLayout();
      gl->addWidget(contact, 0, 0, Qt::AlignVCenter | Qt::AlignLeft);
      gl->addWidget(correction, 1, 0, Qt::AlignVCenter | Qt::AlignLeft);
      
      QHBoxLayout *checkLayout = new QHBoxLayout();
      checkLayout->addLayout(lawLayout);
      checkLayout->addStretch();
      checkLayout->addLayout(gl);
      
      QVBoxLayout *mainLayout = new QVBoxLayout();
      mainLayout->setContentsMargins(0, 0, 0, 0);
      mainLayout->addWidget(list);
      mainLayout->addLayout(checkLayout);
      mainLayout->addStretch();
      this->setLayout(mainLayout);
      
    }
    
    void rowChanged()
    {
      assert(this->isVisible());
      if (!list->selectionModel()->hasSelection())
      {
        contact->setDisabled(true);
        correction->setDisabled(true);
        lawButton->setDisabled(true);
        lawCheck->setDisabled(true);
        return;
      }
      int index = list->selectionModel()->selectedRows().front().row();
      
      if (index < 0 || index >= list->count())
        return;
      button->highlightIndex(index);
      
      contact->setEnabled(true);
      correction->setEnabled(true);
      
      //don't announce these check box changes
      QSignalBlocker contactBlocker(contact);
      QSignalBlocker correctionBlocker(correction);
      
      ListItem *item = dynamic_cast<ListItem*>(list->item(index));
      assert(item);
      if (item->contact)
        contact->setChecked(true);
      else
        contact->setChecked(false);
      if (item->correction)
        correction->setChecked(true);
      else
        correction->setChecked(false);
    }
    
    void contactSlot(bool state)
    {
      int index = list->currentRow();
      if (index < 0 || index >= list->count())
        return;
      
      ListItem *item = dynamic_cast<ListItem*>(list->item(index));
      assert(item);
      if (state)
        item->contact = true;
      else
        item->contact = false;
    }

    void correctionSlot(bool state)
    {
      int index = list->currentRow();
      if (index < 0 || index >= list->count())
        return;
      
      ListItem *item = dynamic_cast<ListItem*>(list->item(index));
      assert(item);
      if (state)
        item->correction = true;
      else
        item->correction = false;
    }
    
    void lawCheckToggled(bool state)
    {
      if (!state)
        lawButton->setChecked(false);
      lawButton->setEnabled(state);
      lawDirty = true;
    }
    
    void lawButtonClicked(bool)
    {
      LawFunctionDialog dlg;
      dlg.setCue(cue);
      auto result = dlg.exec();
      if (result == QDialog::Accepted)
      {
        cue = dlg.getCue();
        if (dlg.hasStructureChanged())
          lawDirty = true;
      }
    }
    
    void setLawCue(const lwf::Cue &cueIn)
    {
      cue = cueIn;
    }
    
    SelectionButton *button;
    SelectionList *list;
    QCheckBox *contact;
    QCheckBox *correction;
    QPushButton *lawButton;
    QCheckBox *lawCheck;
    lwf::Cue cue;
    bool lawDirty = false;
  };
  
  class AuxiliaryPage : public QWidget
  {
  public:
    AuxiliaryPage(QWidget *parent, SelectionButton *buttonIn)
    : QWidget(parent)
    , button(buttonIn)
    {
      list = new SelectionList(this, button);
      curvilinearEquivalence = new QCheckBox(tr("Equal"), this);
      curvilinearEquivalence->setWhatsThis(tr("Curvelinear Equivalence"));
      curvilinearEquivalence->setChecked(true);
      contactType = new QComboBox(this);
      contactType->setWhatsThis(tr("Contact Type"));
      QStringList tStrings =
      {
        QObject::tr("No Contact")
        , QObject::tr("Contact")
        , QObject::tr("Contact On Border")
      };
      contactType->addItems(tStrings);
      
      QVBoxLayout *mainLayout = new QVBoxLayout();
      mainLayout->setContentsMargins(0, 0, 0, 0);
      mainLayout->addWidget(list);
      
      QHBoxLayout *equalLayout = new QHBoxLayout();
      equalLayout->addStretch();
      equalLayout->addWidget(curvilinearEquivalence);
      QHBoxLayout *contactLayout = new QHBoxLayout();
      contactLayout->addStretch();
      contactLayout->addWidget(contactType);
      mainLayout->addLayout(equalLayout);
      mainLayout->addLayout(contactLayout);
      mainLayout->addStretch();
      this->setLayout(mainLayout);
    }
    
    SelectionButton *button;
    SelectionList *list;
    QCheckBox *curvilinearEquivalence;
    QComboBox *contactType;
  };
  
  struct Sweep::Stow
  {
    //qt pointers will be deleted by qt parents.
    QComboBox *triBox;
    QCheckBox *forceC1;
    QCheckBox *solid;
    QComboBox *transitionBox;
    QButtonGroup *bGroup;
    SelectionButton *spineButton;
    SelectionButton *profilesButton;
    SelectionButton *auxiliaryButton;
    SelectionButton *supportButton;
    SelectionButton *binormalButton;
    QLabel *auxiliaryLabel;
    QLabel *supportLabel;
    QLabel *binormalLabel;
//     QLabel *helpLabel;
    QStackedWidget *stackedLists;
    SelectionList *spineList;
    ProfilePage *profilesList;
    AuxiliaryPage *auxiliaryList;
    SelectionList *supportList;
    SelectionList *binormalList;
  };
}

using namespace dlg;

Sweep::Sweep(ftr::Sweep *sIn, QWidget *pIn, bool isEditIn)
: QDialog(pIn)
, node(new msg::Node())
, sweep(sIn)
, stow(new Stow())
, isEditDialog(isEditIn)
{
  node->connect(msg::hub());
  
  setWindowTitle(tr("Sweep"));
  init();
}

Sweep::~Sweep()
{
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("dlg::Sweep");

  settings.endGroup();
}

void Sweep::init()
{
  buildGui();
  
  QSettings &settings = app::instance()->getUserSettings();
  settings.beginGroup("dlg::Sweep");

  settings.endGroup();
  
  WidgetGeometry *filter = new WidgetGeometry(this, "dlg::Sweep");
  this->installEventFilter(filter);
  
  loadFeatureData();
  QTimer::singleShot(0, stow->spineButton, SLOT(setFocus()));
  QTimer::singleShot(0, stow->spineButton, &QPushButton::click);
}

void Sweep::loadFeatureData()
{
  const ann::LawFunction &lfa = sweep->getAnnex<ann::LawFunction>(ann::Type::LawFunction);
  stow->profilesList->setLawCue(lfa.getCue());
  
  if (!isEditDialog)
  {
    QTimer::singleShot(0, [this]() {this->stow->triBox->setCurrentIndex(0);});
    return;
  }
  node->send(msg::Message(msg::Request | msg::Selection | msg::Clear));
  
  ftr::SweepData data = sweep->getSweepData();
  int triValue = data.trihedron;
  QTimer::singleShot(0, [this, triValue]() {this->stow->triBox->setCurrentIndex(triValue);});
  ftr::UpdatePayload payload = app::instance()->getProject()->getPayload(sweep->getId());
  
  if (data.forceC1)
    stow->forceC1->setChecked(true);
  if (data.solid)
    stow->solid->setChecked(true);
  stow->transitionBox->setCurrentIndex(data.transition);
  stow->profilesList->lawCheck->setChecked(data.useLaw);
  
  //spine
  {
    auto resolved = tls::resolvePick(data.spine, payload);
    if (resolved.feature)
    {
      auto spineMessage = tls::convertToMessage(resolved.pick, resolved.feature);
      stow->spineButton->setMessages(spineMessage);
      //no extra data so button should handle filling in widget data
    }
  }
  
  //profiles
  {
    for (const auto &p : data.profiles)
    {
      auto resolved = tls::resolvePick(p.pick, payload);
      if (resolved.feature)
      {
        auto profileMessages = tls::convertToMessage(resolved.pick, resolved.feature);
        //we have extra data so we build items and add to widget.
        //we have to call addMessages after so selection list uses items we added.
        for (const auto &m : profileMessages)
        {
          ListItem *ni = new ListItem(SelectionList::makeShortString(m));
          ni->setStatusTip(SelectionList::makeFullString(m));
          ni->contact = static_cast<bool>(*p.contact);
          ni->correction = static_cast<bool>(*p.correction);
          stow->profilesList->list->addItem(ni);
        }
        stow->profilesButton->addMessages(profileMessages);
      }
    }
  }
  
  switch (data.trihedron)
  {
    case 0:
    case 1:
    case 2:
    case 3:
      break;
    case 4:
    {
      //binormal
      for (const auto &p : data.binormal.picks)
      {
        auto resolved = tls::resolvePick(p, payload);
        if (resolved.feature)
        {
          auto binormalMessage = tls::convertToMessage(resolved.pick, resolved.feature);
          stow->binormalButton->addMessages(binormalMessage);
          //no extra data so button should handle filling in widget data
        }
      }
      break;
    }
    case 5:
    {
      //support
      auto resolved = tls::resolvePick(data.support, payload);
      if (resolved.feature)
      {
        auto supportMessage = tls::convertToMessage(resolved.pick, resolved.feature);
        stow->supportButton->addMessages(supportMessage);
        //no extra data so button should handle filling in widget data
      }
      break;
    }
    case 6:
    {
      //auxiliary
      auto resolved = tls::resolvePick(data.auxiliary.pick, payload);
      if (resolved.feature)
      {
        auto auxMessage = tls::convertToMessage(resolved.pick, resolved.feature);
        stow->auxiliaryButton->addMessages(auxMessage);
        //there is only 1 auxiliary pick so we just store the data in the checkboxes
        //not in the list widget items.
        stow->auxiliaryList->curvilinearEquivalence->setChecked(static_cast<bool>(*data.auxiliary.curvilinearEquivalence));
        stow->auxiliaryList->contactType->setCurrentIndex(static_cast<int>(*data.auxiliary.contactType));
      }
      break;
    }
  }
}

void Sweep::reject()
{
  isAccepted = false;
  finishDialog();
  QDialog::reject();
}

void Sweep::accept()
{
  isAccepted = true;
  finishDialog();
  QDialog::accept();
}

void Sweep::finishDialog()
{
  prj::Project *p = app::instance()->getProject();
  
  auto finishCommand = [&]()
  {
    msg::Message mOut(msg::Mask(msg::Request | msg::Command | msg::Done));
    app::instance()->queuedMessage(mOut);
  };
  auto goUpdate = [&]()
  {
    if (prf::manager().rootPtr->dragger().triggerUpdateOnFinish())
      app::instance()->queuedMessage(msg::Message(msg::Request | msg::Project | msg::Update));
  };
  auto getTuple = [&](const slc::Message &mIn) -> std::tuple<bool, const ftr::Base*, const ann::SeerShape*>
  {
    const ftr::Base *feature = nullptr;
    const ann::SeerShape *seerShape = nullptr;
    
    if (!p->hasFeature(mIn.featureId))
      return std::make_tuple(false, feature, seerShape);
    feature = p->findFeature(mIn.featureId);
    
    if (!feature->hasAnnex(ann::Type::SeerShape))
      return std::make_tuple(false, feature, seerShape);
    seerShape = &feature->getAnnex<ann::SeerShape>();
    if (seerShape->isNull())
      return std::make_tuple(false, feature, seerShape);
    
    return std::make_tuple(true, feature, seerShape);
  };
  
  if (isAccepted)
  {
    finishCommand(); //queue this first in case of exception.
    
    //needed for edit and shouldn't hurt new feature
    p->clearAllInputs(sweep->getId());
    
    const auto &sms = stow->spineButton->getMessages();
    const auto &pms = stow->profilesButton->getMessages(); 

    if (sms.size() == 1 && (!pms.empty()))
    {
      const ftr::Base *spineFeature = nullptr;
      const ann::SeerShape *spineSeerShape = nullptr;
      bool valid;
      std::tie(valid, spineFeature, spineSeerShape) = getTuple(sms.front());
      if (valid)
      {
        ftr::SweepData data;
        
        ftr::Pick cp = tls::convertToPick(sms.front(), *spineSeerShape, p->getShapeHistory());
        cp.tag = ftr::Sweep::spineTag;
        data.spine = cp;
        data.trihedron = stow->triBox->currentIndex();
        data.transition = stow->transitionBox->currentIndex();
        data.solid = stow->solid->isChecked();
        data.useLaw = false;
        sweep->severLaw();
        if (stow->profilesList->list->count() == 1 && stow->profilesList->lawCheck->isChecked())
        {
          data.useLaw = true;
          ann::LawFunction &alf = sweep->getAnnex<ann::LawFunction>(ann::Type::LawFunction);
          alf.setCue(stow->profilesList->cue);
          sweep->attachLaw();
          if (stow->profilesList->lawDirty)
            sweep->regenerateLawViz();
        }
        p->connectInsert(spineFeature->getId(), sweep->getId(), ftr::InputType{cp.tag});
        
        int profileCount = -1;
        for (const auto &pm : pms)
        {
          //occt sweep actually will place the profiles in order, so it would seem
          //we don't need to put an index on the tags. However we need the index on
          //the tags so we can associate the internal picks to the feature picks. FYI
          profileCount++;
          const ftr::Base *pFeature = nullptr;
          const ann::SeerShape *pSeerShape = nullptr;
          std::tie(valid, pFeature, pSeerShape) = getTuple(pm);
          if (valid)
          {
            ftr::Pick pp = tls::convertToPick(pm, *pSeerShape, p->getShapeHistory());
            pp.tag = ftr::Sweep::profileTag + std::to_string(data.profiles.size());
            ListItem *item = dynamic_cast<ListItem*>(stow->profilesList->list->item(profileCount));
            assert(item);
            data.profiles.push_back(ftr::SweepProfile(pp, item->contact, item->correction));
            p->connectInsert(pFeature->getId(), sweep->getId(), ftr::InputType{pp.tag});
          }
        }
        const auto &bms = stow->binormalButton->getMessages();
        if (stow->triBox->currentIndex() == 4 && (!bms.empty()))
        {
          const ftr::Base *aFeature = nullptr;
          const ann::SeerShape *aSeerShape = nullptr;
          int index = -1;
          for (const auto &m : bms)
          {
            index++;
            std::string tag = std::string(ftr::Sweep::binormalTag) + std::to_string(index);
            std::tie(valid, aFeature, aSeerShape) = getTuple(m);
            if (valid)
            {
              ftr::Pick ap = tls::convertToPick(m, *aSeerShape, p->getShapeHistory());
              ap.tag = tag;
              p->connectInsert(aFeature->getId(), sweep->getId(), ftr::InputType{ap.tag});
              data.binormal.picks.push_back(ap);
            }
            else if (aFeature)
            {
              //we have a feature with no seershape.
              if (aFeature->getType() == ftr::Type::DatumAxis)
              {
                ftr::Pick pick;
                pick.tag = tag;
                p->connectInsert(aFeature->getId(), sweep->getId(), ftr::InputType{pick.tag});
                data.binormal.picks.push_back(pick);
              }
            }
          }
        }
        const auto &ams = stow->auxiliaryButton->getMessages();
        if (stow->triBox->currentIndex() == 6 && (!ams.empty()))
        {
          const ftr::Base *aFeature = nullptr;
          const ann::SeerShape *aSeerShape = nullptr;
          std::tie(valid, aFeature, aSeerShape) = getTuple(ams.front());
          if (valid)
          {
            ftr::Pick ap = tls::convertToPick(ams.front(), *aSeerShape, p->getShapeHistory());
            ap.tag = ftr::Sweep::auxiliaryTag;
            //assign values for auxiliary options.
            data.auxiliary = ftr::SweepAuxiliary
            (
              ap, stow->auxiliaryList->curvilinearEquivalence->isChecked()
              , stow->auxiliaryList->contactType->currentIndex()
            );
            p->connectInsert(aFeature->getId(), sweep->getId(), ftr::InputType{ap.tag});
          }
        }
        const auto &sms = stow->supportButton->getMessages();
        if (stow->triBox->currentIndex() == 5 && (!sms.empty()))
        {
          const ftr::Base *sFeature = nullptr;
          const ann::SeerShape *sSeerShape = nullptr;
          std::tie(valid, sFeature, sSeerShape) = getTuple(sms.front());
          if (valid)
          {
            ftr::Pick sp = tls::convertToPick(sms.front(), *sSeerShape, p->getShapeHistory());
            sp.tag = ftr::Sweep::supportTag;
            data.support = sp;
            p->connectInsert(sFeature->getId(), sweep->getId(), ftr::InputType{sp.tag});
          }
        }
        if (stow->forceC1->isChecked())
          data.forceC1 = true;
        else
          data.forceC1 = false;
        sweep->setSweepData(data);
      }
      goUpdate();
    }
    else
    {
      if (!isEditDialog)
        p->removeFeature(sweep->getId());
      node->sendBlocked(msg::buildStatusMessage("Missing spine and/or profiles", 2.0));
    }
  }
  else //rejected dialog
  {
    finishCommand();
    
    if (!isEditDialog)
      p->removeFeature(sweep->getId());
  }
}

void Sweep::buildGui()
{
  //build trihedron combo box
  stow->triBox = new QComboBox(this); //defaults to no edit.
  stow->triBox->addItem(tr("Corrected Frenet"));
  stow->triBox->addItem(tr("Frenet"));
  stow->triBox->addItem(tr("Discrete"));
  stow->triBox->addItem(tr("Fixed"));
  stow->triBox->addItem(tr("Constant Binormal"));
  stow->triBox->addItem(tr("Support"));
  stow->triBox->addItem(tr("Auxiliary"));
  connect(stow->triBox, SIGNAL(currentIndexChanged(int)), this, SLOT(comboSlot(int)));
  
  //build transition combo box
  stow->transitionBox = new QComboBox(this);
  stow->transitionBox->addItem(tr("Modified"));
  stow->transitionBox->addItem(tr("Right"));
  stow->transitionBox->addItem(tr("Round"));
  stow->transitionBox->setCurrentIndex(0);
  
  //build help label and layout
//   stow->helpLabel = new QLabel(tr("Help"), this);
//   stow->helpLabel->setOpenExternalLinks(true);
  QHBoxLayout *comboLayout = new QHBoxLayout();
  comboLayout->addWidget(stow->triBox);
  comboLayout->addStretch();
  comboLayout->addWidget(stow->transitionBox);
  
  //build selection buttons.
  QGridLayout *gl = new QGridLayout();
  stow->bGroup = new QButtonGroup(this);
  connect(stow->bGroup, SIGNAL(buttonToggled(QAbstractButton*, bool))
    , this, SLOT(buttonToggled(QAbstractButton*, bool)));
  QPixmap pmap = QPixmap(":resources/images/cursor.svg").scaled(32, 32, Qt::KeepAspectRatio);
  slc::Mask mask
    = slc::ObjectsEnabled
    | slc::ObjectsSelectable
    | slc::WiresEnabled
    | slc::EdgesEnabled;
  
  QLabel *spineLabel = new QLabel(tr("Spine:"), this);
  stow->spineButton = new SelectionButton(pmap, QString(), this);
  stow->spineButton->isSingleSelection = true;
  stow->spineButton->mask = mask;
  stow->spineButton->statusPrompt = tr("Select 1 Spine");
  gl->addWidget(spineLabel, 0, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(stow->spineButton, 0, 1, Qt::AlignVCenter | Qt::AlignCenter);
  connect(stow->spineButton, &SelectionButton::advance, this, &Sweep::advanceSlot);
  stow->bGroup->addButton(stow->spineButton, 0);
  
  QLabel *profilesLabel = new QLabel(tr("Profiles:"), this);
  stow->profilesButton = new SelectionButton(pmap, QString(), this);
  stow->profilesButton->isSingleSelection = false;
  stow->profilesButton->mask = mask;
  stow->profilesButton->statusPrompt = tr("Select Profiles");
  gl->addWidget(profilesLabel, 1, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(stow->profilesButton, 1, 1, Qt::AlignVCenter | Qt::AlignCenter);
  stow->bGroup->addButton(stow->profilesButton, 1);
  
  stow->auxiliaryLabel = new QLabel(tr("Auxiliary:"), this);
  stow->auxiliaryButton = new SelectionButton(pmap, QString(), this);
  stow->auxiliaryButton->isSingleSelection = true;
  stow->auxiliaryButton->mask = mask;
  stow->auxiliaryButton->statusPrompt = tr("Select 1 Auxiliary Spine");
  gl->addWidget(stow->auxiliaryLabel, 2, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(stow->auxiliaryButton, 2, 1, Qt::AlignVCenter | Qt::AlignCenter);
  stow->bGroup->addButton(stow->auxiliaryButton, 2);
  QString auxiliaryHelp(tr("Auxiliary spine. Only enabled in 'Auxiliary' mode"));
  stow->auxiliaryLabel->setWhatsThis(auxiliaryHelp);
  stow->auxiliaryButton->setWhatsThis(auxiliaryHelp);
  
  stow->supportLabel = new QLabel(tr("Support:"), this);
  stow->supportButton = new SelectionButton(pmap, QString(), this);
  stow->supportButton->isSingleSelection = true;
  stow->supportButton->mask = slc::ShellsEnabled | slc::FacesEnabled | slc::FacesSelectable;
  stow->supportButton->statusPrompt = tr("Select 1 Support Spine");
  gl->addWidget(stow->supportLabel, 3, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(stow->supportButton, 3, 1, Qt::AlignVCenter | Qt::AlignCenter);
  stow->bGroup->addButton(stow->supportButton, 3);
  QString supportHelp(tr("Support shape. Only enabled in 'Support' mode"));
  stow->supportLabel->setWhatsThis(supportHelp);
  stow->supportButton->setWhatsThis(supportHelp);
  
  stow->binormalLabel = new QLabel(tr("Binormal:"), this);
  stow->binormalButton = new SelectionButton(pmap, QString(), this);
  stow->binormalButton->isSingleSelection = false;
  stow->binormalButton->mask
    = slc::ObjectsEnabled
    | slc::ObjectsSelectable
    | slc::FacesEnabled
    | slc::EdgesEnabled
    | slc::PointsEnabled
    | slc::EndPointsEnabled
    | slc::MidPointsEnabled
    | slc::CenterPointsEnabled;
  stow->binormalButton->statusPrompt = tr("Select Binormal");
  gl->addWidget(stow->binormalLabel, 4, 0, Qt::AlignVCenter | Qt::AlignRight);
  gl->addWidget(stow->binormalButton, 4, 1, Qt::AlignVCenter | Qt::AlignCenter);
  stow->bGroup->addButton(stow->binormalButton, 4);
  QString binormalHelp(tr("Binormal shapes. Only enabled in 'Binormal' mode"));
  stow->binormalLabel->setWhatsThis(binormalHelp);
  stow->binormalButton->setWhatsThis(binormalHelp);
  
  QVBoxLayout *buttonLayout = new QVBoxLayout();
  buttonLayout->addLayout(gl);
  buttonLayout->addStretch();
  
  //disable these in default mode.
  stow->auxiliaryLabel->setDisabled(true);
  stow->auxiliaryButton->setDisabled(true);
  stow->supportLabel->setDisabled(true);
  stow->supportButton->setDisabled(true);
  stow->binormalLabel->setDisabled(true);
  stow->binormalButton->setDisabled(true);
  
  //build listWidgets
  stow->stackedLists = new QStackedWidget(this);
  stow->spineList = new SelectionList(this, stow->spineButton);
  stow->profilesList = new ProfilePage(this, stow->profilesButton);
  stow->auxiliaryList = new AuxiliaryPage(this, stow->auxiliaryButton);
  stow->supportList = new SelectionList(this, stow->supportButton);
  stow->binormalList = new SelectionList(this, stow->binormalButton);
  stow->stackedLists->addWidget(stow->spineList);
  stow->stackedLists->addWidget(stow->profilesList);
  stow->stackedLists->addWidget(stow->auxiliaryList);
  stow->stackedLists->addWidget(stow->supportList);
  stow->stackedLists->addWidget(stow->binormalList);
  stow->spineList->setWhatsThis(tr("List of spine selections"));
  stow->profilesList->setWhatsThis(tr("List of profile selections"));
  stow->auxiliaryList->setWhatsThis(tr("List of auxiliary selections"));
  stow->supportList->setWhatsThis(tr("List of support selections"));
  stow->binormalList->setWhatsThis(tr("List of binormal selections"));
  connect(stow->profilesButton, &SelectionButton::dirty, this, &Sweep::profileSelectionDirtySlot);
  
  QHBoxLayout *selectionLayout = new QHBoxLayout();
  selectionLayout->addLayout(buttonLayout);
  selectionLayout->addWidget(stow->stackedLists);
  
  //dialog box buttons
  QDialogButtonBox *buttons = new QDialogButtonBox
    (QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
  QVBoxLayout *dButtonLayout = new QVBoxLayout();
  dButtonLayout->addStretch();
  dButtonLayout->addWidget(buttons);
  connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
  
  //more options
  //build forceC1 checkbox
  stow->forceC1 = new QCheckBox(tr("Force C1"), this);
  stow->forceC1->setWhatsThis(tr("approximate a C1-continuous surface if a swept surface proved to be C0"));
  stow->solid = new QCheckBox(tr("Solid"), this);
  stow->solid->setWhatsThis(tr("Make a Solid"));
  QVBoxLayout *moLayout = new QVBoxLayout();
  moLayout->addStretch();
  moLayout->addWidget(stow->forceC1);
  moLayout->addWidget(stow->solid);
  
  QHBoxLayout *bottomLayout = new QHBoxLayout();
  bottomLayout->addLayout(moLayout);
  bottomLayout->addStretch();
  bottomLayout->addLayout(dButtonLayout);
  
  QVBoxLayout *mainLayout = new QVBoxLayout();
  mainLayout->addLayout(comboLayout);
  mainLayout->addLayout(selectionLayout);
  mainLayout->addStretch();
  mainLayout->addLayout(bottomLayout);
  this->setLayout(mainLayout);
}

void Sweep::comboSlot(int indexIn)
{
  //we might have auxiliary or support buttons currently engaged.
  //These might end up being disabled so we need to change
  //selected button to profiles.
  if (stow->auxiliaryButton->isChecked() || stow->supportButton->isChecked() || stow->binormalButton->isChecked())
    stow->profilesButton->setChecked(true);
  
  //auxiliary and support buttons are relative to their modes.
  //so assume they should be off and turn them back on in the
  //appropriate case. Turning labels on and off also for better visual
  stow->auxiliaryLabel->setDisabled(true);
  stow->auxiliaryButton->setDisabled(true);
  stow->supportLabel->setDisabled(true);
  stow->supportButton->setDisabled(true);
  stow->binormalLabel->setDisabled(true);
  stow->binormalButton->setDisabled(true);
  
  switch (indexIn)
  {
    case 0:
      stow->triBox->setWhatsThis
      (
        tr
        (
          "<html><head/><body><p>Corrected Frenet: "
          "Frenet with torsion minimization. This is the default. "
          "</span></p></body></html>"
        )
      );
//       stow->helpLabel->setText
//       (
//         tr
//         (
//           "<html><head/><body><p>"
//           "<a href=\"https://en.wikipedia.org/wiki/Frenet%E2%80%93Serret_formulas\"> "
//           "<span style=\" text-decoration: underline; color:#2980b9;\">Help</span></a></p></body></html>"
//         )
//       );
      break;
    case 4:
      stow->binormalLabel->setEnabled(true);
      stow->binormalButton->setEnabled(true);
      stow->triBox->setWhatsThis(tr("<html><head/><body><p>Constant Binormal</p></body></html>"));
      break;
    case 5:
      stow->supportLabel->setEnabled(true);
      stow->supportButton->setEnabled(true);
      stow->triBox->setWhatsThis(tr("<html><head/><body><p>Normal From Face Or Shell</p></body></html>"));
      break;
    case 6:
      stow->auxiliaryLabel->setEnabled(true);
      stow->auxiliaryButton->setEnabled(true);
      stow->triBox->setWhatsThis(tr("<html><head/><body><p>Normal From Spine To Auxilary Spine</p></body></html>"));
      break;
    default:
      stow->triBox->setWhatsThis(tr("<html><head/><body><p>Unknown</p></body></html>"));
  }
}

void Sweep::advanceSlot()
{
  QAbstractButton *cb = stow->bGroup->checkedButton();
  if (!cb)
    return;
  if (cb == stow->spineButton)
    stow->profilesButton->setChecked(true);
}

void Sweep::buttonToggled(QAbstractButton *bIn, bool bState)
{
  SelectionButton *sButton = qobject_cast<SelectionButton*>(bIn);
  if (!sButton)
    return;
  
  int bi = stow->bGroup->id(bIn);
  assert (bi >= 0 && bi < stow->stackedLists->count());
  if (bState)
    stow->stackedLists->setCurrentIndex(bi);
  
  //enable the law check button if we are on profiles and have only 1 profile
  if (bi == 1)
  {
    if (stow->profilesList->list->count() == 1)
    {
      stow->profilesList->lawCheck->setEnabled(true);
      stow->profilesList->lawButton->setEnabled(stow->profilesList->lawCheck->isChecked());
    }
    else
    {
      stow->profilesList->lawCheck->setEnabled(false);
      stow->profilesList->lawButton->setEnabled(false);
    }
  }
}

/* this needed so we can enable the 'use law' checkbox
 * when the profile count is 1.
 */
void Sweep::profileSelectionDirtySlot()
{
  if (stow->profilesButton->getMessages().size() == 1)
  {
    stow->profilesList->lawCheck->setEnabled(true);
    stow->profilesList->lawButton->setEnabled(stow->profilesList->lawCheck->isChecked());
  }
  else
  {
    stow->profilesList->lawCheck->setChecked(false);
    stow->profilesList->lawCheck->setEnabled(false);
    stow->profilesList->lawButton->setEnabled(false);
  }
}

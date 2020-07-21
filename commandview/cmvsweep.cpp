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

#include <QSettings>
#include <QGridLayout>
#include <QButtonGroup>
#include <QLabel>
#include <QComboBox>
#include <QListWidget>
#include <QTimer>
#include <QAction>
#include <QCheckBox>
#include <QStackedWidget>

#include "application/appapplication.h"
#include "project/prjproject.h"
#include "annex/annlawfunction.h"
#include "message/msgmessage.h"
#include "message/msgnode.h"
#include "dialogs/dlgselectionbutton.h"
#include "dialogs/dlgselectionlist.h"
#include "tools/featuretools.h"
#include "tools/idtools.h"
#include "law/lwfcue.h"
#include "commandview/cmvlawfunctionwidget.h"
#include "feature/ftrsweep.h"
#include "command/cmdsweep.h"
#include "commandview/cmvsweep.h"

using boost::uuids::uuid;

class ListItem : public QListWidgetItem
{
public:
  ListItem(const QString &text) : QListWidgetItem(text){}
  bool contact = false;
  bool correction = false;
};

namespace cmv
{
  class SelectionList : public QListWidget
  {
  public:
    SelectionList(QWidget *parent, dlg::SelectionButton *button)
    : QListWidget(parent)
    , sButton(button)
    {
      this->setSelectionMode(QAbstractItemView::NoSelection);
      connect(sButton, &dlg::SelectionButton::dirty, this, &SelectionList::populateList);
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
    dlg::SelectionButton *sButton;
  };
  
  class ProfilePage : public QWidget
  {
  public:
    ProfilePage(Sweep *parent, dlg::SelectionButton *buttonIn)
    : QWidget(parent)
    , view(parent)
    , button(buttonIn)
    {
      QSizePolicy adjust = sizePolicy();
      adjust.setVerticalPolicy(QSizePolicy::Expanding);
      setSizePolicy(adjust);
      
      list = new SelectionList(this, button);
      contact = new QCheckBox(tr("Contact"), this);
      correction = new QCheckBox(tr("Correction"), this);
      lawCheck = new QCheckBox(tr("Use Law"), this);
      lawCheck->setWhatsThis(tr("To Enable Or Disable The Law"));
      contact->setDisabled(true);
      correction->setDisabled(true);
      lawCheck->setDisabled(true);
      
      list->setSelectionMode(QAbstractItemView::SingleSelection);
      connect(list, &QListWidget::itemSelectionChanged, this, &ProfilePage::rowChanged);
      connect(contact, &QCheckBox::toggled, this, &ProfilePage::contactSlot);
      connect(correction, &QCheckBox::toggled, this, &ProfilePage::correctionSlot);
      connect(lawCheck, &QPushButton::toggled, parent, &Sweep::lawCheckToggled);
      
      QVBoxLayout *lawLayout = new QVBoxLayout();
      lawLayout->addWidget(contact);
      lawLayout->addWidget(correction);
      lawLayout->addWidget(lawCheck);
//       lawLayout->addWidget(lawWidget);
//       lawLayout->addStretch();
//       lawLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
      
//       QHBoxLayout *checkLayout = new QHBoxLayout();
//       checkLayout->addLayout(lawLayout);
//       checkLayout->addStretch();
      
      QVBoxLayout *mainLayout = new QVBoxLayout();
      mainLayout->setContentsMargins(0, 0, 0, 0);
      mainLayout->addWidget(list);
      mainLayout->addLayout(lawLayout);
//       mainLayout->addStretch();
//       mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
      this->setLayout(mainLayout);
    }
    
    void rowChanged()
    {
      assert(this->isVisible());
      if (!list->selectionModel()->hasSelection())
      {
        contact->setDisabled(true);
        correction->setDisabled(true);
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
    
    Sweep *view = nullptr;
    dlg::SelectionButton *button;
    SelectionList *list;
    QCheckBox *contact;
    QCheckBox *correction;
    QCheckBox *lawCheck;
  };
  
  class AuxiliaryPage : public QWidget
  {
  public:
    AuxiliaryPage(QWidget *parent, dlg::SelectionButton *buttonIn)
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
    
    dlg::SelectionButton *button;
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
    dlg::SelectionButton *spineButton;
    dlg::SelectionButton *profilesButton;
    dlg::SelectionButton *auxiliaryButton;
    dlg::SelectionButton *supportButton;
    dlg::SelectionButton *binormalButton;
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
    LawFunctionWidget *lawWidget;
    
    cmd::Sweep *command;
    cmv::Sweep *view;
    
    Stow(cmd::Sweep *cIn, cmv::Sweep *vIn)
    : command(cIn)
    , view(vIn)
    {
      buildGui();
      
      QSettings &settings = app::instance()->getUserSettings();
      settings.beginGroup("cmv::Sweep");
      //load settings
      settings.endGroup();
      
      loadFeatureData();
      glue();
      
      QTimer::singleShot(0, spineButton, SLOT(setFocus()));
      QTimer::singleShot(0, spineButton, &QPushButton::click);
    }
    
    void buildGui()
    {
      //build trihedron combo box
      triBox = new QComboBox(view); //defaults to no edit.
      triBox->addItem(tr("Corrected Frenet"));
      triBox->addItem(tr("Frenet"));
      triBox->addItem(tr("Discrete"));
      triBox->addItem(tr("Fixed"));
      triBox->addItem(tr("Constant Binormal"));
      triBox->addItem(tr("Support"));
      triBox->addItem(tr("Auxiliary"));
      
      //build transition combo box
      transitionBox = new QComboBox(view);
      transitionBox->addItem(tr("Modified"));
      transitionBox->addItem(tr("Right"));
      transitionBox->addItem(tr("Round"));
      transitionBox->setCurrentIndex(0);
      
      forceC1 = new QCheckBox(tr("Force C1"), view);
      forceC1->setWhatsThis(tr("approximate a C1-continuous surface if a swept surface proved to be C0"));
      solid = new QCheckBox(tr("Solid"), view);
      solid->setWhatsThis(tr("Make a Solid"));
      QHBoxLayout *optionsLayout = new QHBoxLayout();
      optionsLayout->addWidget(solid);
      optionsLayout->addStretch();
      optionsLayout->addWidget(forceC1);
      
      //build help label and layout
    //   stow->helpLabel = new QLabel(tr("Help"), this);
    //   stow->helpLabel->setOpenExternalLinks(true);
      QHBoxLayout *comboLayout = new QHBoxLayout();
      comboLayout->addWidget(triBox);
      comboLayout->addStretch();
      comboLayout->addWidget(transitionBox);
      
      //build selection buttons.
      QGridLayout *gl = new QGridLayout();
      bGroup = new QButtonGroup(view);
      QPixmap pmap = QPixmap(":resources/images/cursor.svg").scaled(32, 32, Qt::KeepAspectRatio);
      slc::Mask mask
        = slc::ObjectsEnabled
        | slc::ObjectsSelectable
        | slc::WiresEnabled
        | slc::EdgesEnabled;
      
      QLabel *spineLabel = new QLabel(tr("Spine:"), view);
      spineButton = new dlg::SelectionButton(pmap, QString(), view);
      spineButton->isSingleSelection = true;
      spineButton->mask = mask;
      spineButton->statusPrompt = tr("Select 1 Spine");
      gl->addWidget(spineLabel, 0, 0, Qt::AlignVCenter | Qt::AlignRight);
      gl->addWidget(spineButton, 0, 1, Qt::AlignVCenter | Qt::AlignCenter);
      bGroup->addButton(spineButton, 0);
      
      QLabel *profilesLabel = new QLabel(tr("Profiles:"), view);
      profilesButton = new dlg::SelectionButton(pmap, QString(), view);
      profilesButton->isSingleSelection = false;
      profilesButton->mask = mask;
      profilesButton->statusPrompt = tr("Select Profiles");
      gl->addWidget(profilesLabel, 1, 0, Qt::AlignVCenter | Qt::AlignRight);
      gl->addWidget(profilesButton, 1, 1, Qt::AlignVCenter | Qt::AlignCenter);
      bGroup->addButton(profilesButton, 1);
      
      auxiliaryLabel = new QLabel(tr("Auxiliary:"), view);
      auxiliaryButton = new dlg::SelectionButton(pmap, QString(), view);
      auxiliaryButton->isSingleSelection = true;
      auxiliaryButton->mask = mask;
      auxiliaryButton->statusPrompt = tr("Select 1 Auxiliary Spine");
      gl->addWidget(auxiliaryLabel, 2, 0, Qt::AlignVCenter | Qt::AlignRight);
      gl->addWidget(auxiliaryButton, 2, 1, Qt::AlignVCenter | Qt::AlignCenter);
      bGroup->addButton(auxiliaryButton, 2);
      QString auxiliaryHelp(tr("Auxiliary spine. Only enabled in 'Auxiliary' mode"));
      auxiliaryLabel->setWhatsThis(auxiliaryHelp);
      auxiliaryButton->setWhatsThis(auxiliaryHelp);
      
      supportLabel = new QLabel(tr("Support:"), view);
      supportButton = new dlg::SelectionButton(pmap, QString(), view);
      supportButton->isSingleSelection = true;
      supportButton->mask = slc::ShellsEnabled | slc::FacesEnabled | slc::FacesSelectable;
      supportButton->statusPrompt = tr("Select 1 Support Spine");
      gl->addWidget(supportLabel, 3, 0, Qt::AlignVCenter | Qt::AlignRight);
      gl->addWidget(supportButton, 3, 1, Qt::AlignVCenter | Qt::AlignCenter);
      bGroup->addButton(supportButton, 3);
      QString supportHelp(tr("Support shape. Only enabled in 'Support' mode"));
      supportLabel->setWhatsThis(supportHelp);
      supportButton->setWhatsThis(supportHelp);
      
      binormalLabel = new QLabel(tr("Binormal:"), view);
      binormalButton = new dlg::SelectionButton(pmap, QString(), view);
      binormalButton->isSingleSelection = false;
      binormalButton->mask
        = slc::ObjectsEnabled
        | slc::ObjectsSelectable
        | slc::FacesEnabled
        | slc::EdgesEnabled
        | slc::PointsEnabled
        | slc::EndPointsEnabled
        | slc::MidPointsEnabled
        | slc::CenterPointsEnabled;
      binormalButton->statusPrompt = tr("Select Binormal");
      gl->addWidget(binormalLabel, 4, 0, Qt::AlignVCenter | Qt::AlignRight);
      gl->addWidget(binormalButton, 4, 1, Qt::AlignVCenter | Qt::AlignCenter);
      bGroup->addButton(binormalButton, 4);
      QString binormalHelp(tr("Binormal shapes. Only enabled in 'Binormal' mode"));
      binormalLabel->setWhatsThis(binormalHelp);
      binormalButton->setWhatsThis(binormalHelp);
      
      QVBoxLayout *buttonLayout = new QVBoxLayout();
      buttonLayout->addLayout(gl);
      buttonLayout->addStretch();
      
      //disable these in default mode.
      auxiliaryLabel->setDisabled(true);
      auxiliaryButton->setDisabled(true);
      supportLabel->setDisabled(true);
      supportButton->setDisabled(true);
      binormalLabel->setDisabled(true);
      binormalButton->setDisabled(true);
      
      //build listWidgets
      stackedLists = new QStackedWidget(view);
      spineList = new SelectionList(view, spineButton);
      profilesList = new ProfilePage(view, profilesButton);
      auxiliaryList = new AuxiliaryPage(view, auxiliaryButton);
      supportList = new SelectionList(view, supportButton);
      binormalList = new SelectionList(view, binormalButton);
      stackedLists->addWidget(spineList);
      stackedLists->addWidget(profilesList);
      stackedLists->addWidget(auxiliaryList);
      stackedLists->addWidget(supportList);
      stackedLists->addWidget(binormalList);
      spineList->setWhatsThis(tr("List of spine selections"));
      profilesList->setWhatsThis(tr("List of profile selections"));
      auxiliaryList->setWhatsThis(tr("List of auxiliary selections"));
      supportList->setWhatsThis(tr("List of support selections"));
      binormalList->setWhatsThis(tr("List of binormal selections"));
      
      QHBoxLayout *selectionLayout = new QHBoxLayout();
      selectionLayout->addLayout(buttonLayout);
      selectionLayout->addWidget(stackedLists);
      
      lawWidget = new LawFunctionWidget(view);
      lawWidget->setWhatsThis(tr("Modifies The Law"));
      lawWidget->setVisible(false);
      
      view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      
      QVBoxLayout *mainLayout = new QVBoxLayout();
      mainLayout->addLayout(comboLayout);
      mainLayout->addLayout(optionsLayout);
      mainLayout->addLayout(selectionLayout);
      mainLayout->addItem(new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
      mainLayout->addWidget(lawWidget, 255);
      
      view->setLayout(mainLayout);
    }
    
    void loadFeatureData()
    {
      const ann::LawFunction &lfa = command->feature->getAnnex<ann::LawFunction>(ann::Type::LawFunction);
      lawWidget->setCue(lfa.getCue());
      
      int triValue = command->feature->getTrihedron();
      QTimer::singleShot(0, [this, triValue]() {triBox->setCurrentIndex(triValue);});
      ftr::UpdatePayload payload = app::instance()->getProject()->getPayload(command->feature->getId());
      
      if (command->feature->getForceC1())
        forceC1->setChecked(true);
      if (command->feature->getSolid())
        solid->setChecked(true);
      transitionBox->setCurrentIndex(command->feature->getTransition());
      QSignalBlocker lawCheckBlocker(profilesList->lawCheck);
      profilesList->lawCheck->setChecked(command->feature->getUseLaw());
      
      tls::Resolver resolver(payload);
      //spine
      {
        resolver.resolve(command->feature->getSpine());
        if (resolver.getFeature())
        {
          spineButton->setMessages(resolver.convertToMessages());
        }
      }
      
      //profiles
      {
        slc::Messages buttonMessages;
        for (const auto &p : command->feature->getProfiles())
        {
          resolver.resolve(p.pick);
          if (resolver.getFeature())
          {
            auto profileMessages = resolver.convertToMessages();
            //we have extra data so we build items and add to widget.
            //we have to call addMessages after so selection list uses items we added.
            for (const auto &m : profileMessages)
            {
              ListItem *ni = new ListItem(SelectionList::makeShortString(m));
              ni->setStatusTip(SelectionList::makeFullString(m));
              ni->contact = static_cast<bool>(*p.contact);
              ni->correction = static_cast<bool>(*p.correction);
              profilesList->list->addItem(ni);
            }
            buttonMessages.insert(buttonMessages.end(), profileMessages.begin(), profileMessages.end());
          }
        }
        profilesButton->setMessages(buttonMessages);
      }
      
      switch (triValue)
      {
        case 0:
        case 1:
        case 2:
        case 3:
          break;
        case 4:
        {
          //binormal
          for (const auto &p : command->feature->getBinormal().picks)
          {
            resolver.resolve(p);
            if (resolver.getFeature())
              binormalButton->addMessages(resolver.convertToMessages());
              //no extra data so button should handle filling in widget data
          }
          break;
        }
        case 5:
        {
          //support
          resolver.resolve(command->feature->getSupport());
          if (resolver.getFeature())
            supportButton->addMessages(resolver.convertToMessages());
            //no extra data so button should handle filling in widget data
          break;
        }
        case 6:
        {
          //auxiliary
          resolver.resolve(command->feature->getAuxiliary().pick);
          if (resolver.getFeature())
          {
            auxiliaryButton->addMessages(resolver.convertToMessages());
            //there is only 1 auxiliary pick so we just store the data in the checkboxes
            //not in the list widget items.
            auxiliaryList->curvilinearEquivalence->setChecked(static_cast<bool>(*command->feature->getAuxiliary().curvilinearEquivalence));
            auxiliaryList->contactType->setCurrentIndex(static_cast<int>(*command->feature->getAuxiliary().contactType));
          }
          break;
        }
      }
    }
    
    void glue()
    {
      //so we can connect signals/slots after loading feature data, so we don't trigger when initializing controls
      connect(triBox, SIGNAL(currentIndexChanged(int)), view, SLOT(comboTriSlot(int)));
      connect(transitionBox, SIGNAL(currentIndexChanged(int)), view, SLOT(comboTransitionSlot(int)));
      connect(bGroup, SIGNAL(buttonToggled(QAbstractButton*, bool)), view, SLOT(buttonToggled(QAbstractButton*, bool)));
      connect(spineButton, &dlg::SelectionButton::advance, view, &Sweep::advanceSlot);
      connect(spineButton, &dlg::SelectionButton::dirty, view, &Sweep::spineSelectionDirtySlot);
      connect(profilesButton, &dlg::SelectionButton::dirty, view, &Sweep::profileSelectionDirtySlot);
      connect(auxiliaryButton, &dlg::SelectionButton::dirty, view, &Sweep::auxSelectionDirtySlot);
      connect(binormalButton, &dlg::SelectionButton::dirty, view, &Sweep::binormalSelectionDirtySlot);
      connect(supportButton, &dlg::SelectionButton::dirty, view, &Sweep::supportSelectionDirtySlot);
      connect(solid, &QPushButton::toggled, view, &Sweep::solidCheckToggled);
      connect(forceC1, &QPushButton::toggled, view, &Sweep::forceC1CheckToggled);
      connect(auxiliaryList->curvilinearEquivalence, &QPushButton::toggled, view, &Sweep::auxCurvilinearToggled);
      connect(auxiliaryList->contactType, SIGNAL(currentIndexChanged(int)), view, SLOT(auxCombo(int)));
      connect(lawWidget, &LawFunctionWidget::lawIsDirty, view, &Sweep::lawDirtySlot);
      connect(lawWidget, &LawFunctionWidget::valueChanged, view, &Sweep::lawValueChangedSlot);
    }
    
    void goUpdate()
    {
      //start with the assumption of failure.
      view->project->clearAllInputs(command->feature->getId());
      
      const auto &sms = spineButton->getMessages();
      const auto &pms = profilesButton->getMessages();
      int tri = triBox->currentIndex();
      
      cmd::Sweep::Profiles profiles;
      int profileCount = -1;
      for (const auto &m : pms)
      {
        profileCount++;
        ListItem *item = dynamic_cast<ListItem*>(profilesList->list->item(profileCount));
        assert(item);
        profiles.push_back(cmd::Sweep::Profile(m, item->contact, item->correction));
      }
      
      if (sms.empty() || profiles.empty())
        return;
      
      if (tri >= 0 && tri <= 3)
      {
        command->setCommon(sms.front(), profiles, tri);
      }
      else if (tri == 4)
      {
        const auto &bnms = binormalButton->getMessages();
        if (!bnms.empty())
          command->setBinormal(sms.front(), profiles, bnms);
      }
      else if (tri == 5)
      {
        const auto &supportMsgs = supportButton->getMessages();
        if (!supportMsgs.empty())
          command->setSupport(sms.front(), profiles, supportMsgs.front());
      }
      else if (tri == 6)
      {
        const auto &ams = auxiliaryButton->getMessages();
        if (!ams.empty())
        {
          cmd::Sweep::Auxiliary aux(ams.front(), auxiliaryList->curvilinearEquivalence->isChecked(), auxiliaryList->contactType->currentIndex());
          command->setAuxiliary(sms.front(), profiles, aux);
        }
      }
      
      command->localUpdate();
    }
  };
}

using namespace cmv;

Sweep::Sweep(cmd::Sweep *cIn)
: Base("cmv::Sweep")
, stow(new Stow(cIn, this))
{}

Sweep::~Sweep() = default;

void Sweep::advanceSlot()
{
  QAbstractButton *cb = stow->bGroup->checkedButton();
  if (!cb)
    return;
  if (cb == stow->spineButton)
    stow->profilesButton->setChecked(true);
}

void Sweep::comboTriSlot(int indexIn)
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
  
  stow->goUpdate();
}

void Sweep::comboTransitionSlot(int index)
{
  stow->command->feature->setTransition(index);
  stow->goUpdate();
}

void Sweep::buttonToggled(QAbstractButton *bIn, bool bState)
{
  dlg::SelectionButton *sButton = qobject_cast<dlg::SelectionButton*>(bIn);
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
      if (stow->profilesList->lawCheck->isChecked())
        stow->lawWidget->setVisible(true);
    }
    else
    {
      stow->profilesList->lawCheck->setEnabled(false);
      stow->lawWidget->setVisible(false);
    }
  }
  else
    stow->lawWidget->setVisible(false);
}

/* this needed so we can enable the 'use law' checkbox
 * when the profile count is 1.
 */
void Sweep::profileSelectionDirtySlot()
{
  if (stow->profilesButton->getMessages().size() == 1)
  {
    stow->profilesList->lawCheck->setEnabled(true);
    if (stow->profilesList->lawCheck->isChecked())
        stow->lawWidget->setVisible(true);
  }
  else
  {
    stow->profilesList->lawCheck->setChecked(false);
    stow->profilesList->lawCheck->setEnabled(false);
    stow->lawWidget->setVisible(false);
  }
  
  stow->goUpdate();
}

void Sweep::spineSelectionDirtySlot()
{
  stow->goUpdate();
}

void Sweep::auxSelectionDirtySlot()
{
  stow->goUpdate();
}

void Sweep::binormalSelectionDirtySlot()
{
  stow->goUpdate();
}

void Sweep::supportSelectionDirtySlot()
{
  stow->goUpdate();
}

void Sweep::lawCheckToggled(bool state)
{
  stow->lawWidget->setVisible(state);
  stow->command->feature->setUseLaw(state);
  stow->goUpdate();
}

void Sweep::lawChanged()
{
  stow->command->feature->setLaw(stow->lawWidget->getCue());
  stow->goUpdate();
}

void Sweep::solidCheckToggled(bool state)
{
  stow->command->feature->setSolid(state);
  stow->goUpdate();
}

void Sweep::forceC1CheckToggled(bool state)
{
  stow->command->feature->setForceC1(state);
  stow->goUpdate();
}

void Sweep::auxCombo(int)
{
  stow->goUpdate();
}

void Sweep::auxCurvilinearToggled(bool)
{
  stow->goUpdate();
}

void Sweep::lawDirtySlot()
{
  stow->command->feature->setLaw(stow->lawWidget->getCue());
  stow->goUpdate();
}

void Sweep::lawValueChangedSlot()
{
  //this is goofy. law cue in law widget is a copy so widget doesn't change
  //parameters in the actual feature. So we have to sync by parameter ids.
  for (const prm::Parameter *wp : stow->lawWidget->getCue().getParameters())
  {
    prm::Parameter *fp = stow->command->feature->getParameter(wp->getId());
    if (!fp)
    {
      std::cout << "WARNING: law widget cue parameter doesn't exist in feature" << std::endl;
      continue;
    }
    (*fp) = (*wp);
  }
  
  stow->goUpdate();
}

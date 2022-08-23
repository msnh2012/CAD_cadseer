/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2015  Thomas S. Anderson blobfish.at.gmx.com
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


#ifndef CMD_MANAGER_H
#define CMD_MANAGER_H

#include <stack>
#include <memory>
#include <functional>
#include <map>

#include "selection/slcdefinitions.h"
#include "feature/ftrtypes.h"
#include "command/cmdbase.h"

namespace msg{struct Message; struct Node; struct Sift;}
namespace ftr{class Base;}

namespace cmd
{
  class Manager
  {
  public:
    Manager();
    
    void addCommand(BasePtr);
  private:
    std::unique_ptr<msg::Node> node;
    std::unique_ptr<msg::Sift> sift;
    slc::Mask selectionMask;
    void cancelCommandDispatched(const msg::Message &);
    void doneCommandDispatched(const msg::Message &);
    void clearCommandDispatched(const msg::Message &);
    void setupDispatcher();
    void doneSlot();
    void activateTop();
    void sendCommandMessage(const std::string &messageIn);
    
    void clearSelection();
    
    std::stack<BasePtr> stack;
    
    void featureToSystemDispatched(const msg::Message &);
    void systemToFeatureDispatched(const msg::Message &);
    void featureToDraggerDispatched(const msg::Message &);
    void draggerToFeatureDispatched(const msg::Message &);
    void checkGeometryDispatched(const msg::Message&);
    void selectionMaskDispatched(const msg::Message&);
    void editColorDispatched(const msg::Message&);
    void featureRenameDispatched(const msg::Message&);
    void constructBlendDispatched(const msg::Message&);
    void constructExtractDispatched(const msg::Message&);
    void editFeatureDispatched(const msg::Message&);
    void featureRepositionDispatched(const msg::Message&);
    void featureDissolveDispatched(const msg::Message&);
    void systemToSelectionDispatched(const msg::Message&);
    void constructSquashDispatched(const msg::Message&);
    void constructStripDispatched(const msg::Message&);
    void constructNestDispatched(const msg::Message&);
    void constructDieSetDispatched(const msg::Message&);
    void constructQuoteDispatched(const msg::Message&);
    void viewIsolateDispatched(const msg::Message&);
    void measureLinearDispatched(const msg::Message&);
    void constructRefineDispatched(const msg::Message&);
    void constructInstanceLinearDispatched(const msg::Message&);
    void constructInstanceMirrorDispatched(const msg::Message&);
    void constructInstancePolarDispatched(const msg::Message&);
    void constructBooleanDispatched(const msg::Message&);
    void constructOffsetDispatched(const msg::Message&);
    void constructThickenDispatched(const msg::Message&);
    void constructSewDispatched(const msg::Message&);
    void constructTrimDispatched(const msg::Message&);
    void constructRemoveFacesDispatched(const msg::Message&);
    void constructThreadDispatched(const msg::Message&);
    void constructDatumAxisDispatched(const msg::Message&);
    void constructDatumPlaneDispatched(const msg::Message&);
    void constructSketchDispatched(const msg::Message&);
    void constructExtrudeDispatched(const msg::Message&);
    void constructRevolveDispatched(const msg::Message&);
    void constructSurfaceMeshDispatched(const msg::Message&);
    void constructLineDispatched(const msg::Message&);
    void constructTransitionCurveDispatched(const msg::Message&);
    void constructRuledDispatched(const msg::Message&);
    void constructImagePlaneDispatched(const msg::Message&);
    void constructSweepDispatched(const msg::Message&);
    void constructDraftDispatched(const msg::Message&);
    void constructChamferDispatched(const msg::Message&);
    void constructBoxDispatched(const msg::Message&);
    void constructOblongDispatched(const msg::Message&);
    void constructTorusDispatched(const msg::Message&);
    void constructCylinderDispatched(const msg::Message&);
    void constructSphereDispatched(const msg::Message&);
    void constructConeDispatched(const msg::Message&);
    void constructHollowDispatched(const msg::Message&);
    void constructDatumSystemDispatched(const msg::Message&);
    void constructSurfaceReMeshDispatched(const msg::Message&);
    void constructSurfaceMeshFillDispatched(const msg::Message&);
    void constructMapPCurveDispatched(const msg::Message&);
    void constructUntrimDispatched(const msg::Message&);
    void constructFaceDispatched(const msg::Message&);
    void constructFillDispatched(const msg::Message&);
    void constructPrismDispatched(const msg::Message&);
    void constructUnderCutDispatched(const msg::Message&);
    void constructMutateDispatched(const msg::Message&);
    void constructLawSpineDispatched(const msg::Message&);
    void revisionDispatched(const msg::Message&);
    void importDispatched(const msg::Message&);
    void exportDispatched(const msg::Message&);
    void preferencesDispatched(const msg::Message&);
    void removeDispatched(const msg::Message&);
    void infoDispatched(const msg::Message&);
    void dirtyDispatched(const msg::Message&);
    void featureDumpDispatched(const msg::Message&);
    void shapeTrackDumpDispatched(const msg::Message&);
    void shapeGraphDumpDispatched(const msg::Message&);
    void testDispatched(const msg::Message&);
    
    //editing functions
    typedef std::function<BasePtr (ftr::Base *)> EditFunction;
    typedef std::map<ftr::Type, EditFunction> EditFunctionMap;
    EditFunctionMap editFunctionMap;
    void setupEditFunctionMap();
  };
  
  Manager& manager();
}

#endif // CMD_MANAGER_H

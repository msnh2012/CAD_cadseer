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

#ifndef MSG_MESSAGE_H
#define MSG_MESSAGE_H

#include <bitset>
#include <functional>
#include <memory>

#include "selection/slcdefinitions.h"

namespace boost{namespace uuids{struct uuid;}}

namespace prj {struct Message;}
namespace app {struct Message;}
namespace slc {struct Message;}
namespace vwr {struct Message;}
namespace ftr {struct Message;}
namespace lod {struct Message;}
namespace cmv {struct Message;}

namespace msg
{
    //! Mask is for a key in a function dispatch message handler. We might need a validation routine.
    // no pre and post on requests only on response.
    typedef std::bitset<192> Mask;
    static const Mask Request(Mask().set(                        0));//!< message classification
    static const Mask Response(Mask().set(                       1));//!< message classification
    static const Mask Pre(Mask().set(                            2));//!< message response timing. think "about to". Data still valid
    static const Mask Post(Mask().set(                           3));//!< message response timing. think "done". Data invalid.
    static const Mask Preselection(Mask().set(                   4));//!< selection classification
    static const Mask Selection(Mask().set(                      5));//!< selection classification
    static const Mask Add(Mask().set(                            6));//!< modifier
    static const Mask Remove(Mask().set(                         7));//!< modifier
    static const Mask Clear(Mask().set(                          8));//!< modifier
    static const Mask DAG(Mask().set(                            9));//!< modifier
    static const Mask Current(Mask().set(                       10));//!< modifier referencing current coordinate system.
    static const Mask SetCurrentLeaf(Mask().set(                11));//!< project action
    static const Mask Feature(Mask().set(                       12));//!< project action & command
    static const Mask Update(Mask().set(                        13));//!< project action. request only for all: model, visual etc..
    static const Mask Force(Mask().set(                         14));//!< project action. request only. marks dirty to force updates
    static const Mask Model(Mask().set(                         15));//!< project action
    static const Mask Visual(Mask().set(                        16));//!< project action
    static const Mask Connection(Mask().set(                    17));//!< project action
    static const Mask Reorder(Mask().set(                       18));//!< project action.
    static const Mask Skipped(Mask().set(                       19));//!< project action.
    static const Mask Project(Mask().set(                       20));//!< application action
    static const Mask Dialog(Mask().set(                        21));//!< application action
    static const Mask Open(Mask().set(                          22));//!< application action
    static const Mask Close(Mask().set(                         23));//!< application action
    static const Mask Save(Mask().set(                          24));//!< application action
    static const Mask New(Mask().set(                           25));//!< application action
    static const Mask Git(Mask().set(                           26));//!< git project integration
    static const Mask Freeze(Mask().set(                        27));//!< git modifier
    static const Mask Thaw(Mask().set(                          28));//!< git modifier
//     static const Mask Cancel(Mask().set(                        29));//!< UNUSED
    static const Mask Done(Mask().set(                          30));//!< command manager
    static const Mask Command(Mask().set(                       31));//!< command manager
    static const Mask Active(Mask().set(                        32));//!< command manager
    static const Mask Inactive(Mask().set(                      33));//!< command manager
    static const Mask Status(Mask().set(                        34));//!< display status
    static const Mask Text(Mask().set(                          35));//!< text
    static const Mask Overlay(Mask().set(                       36));//!< visual
    static const Mask Construct(Mask().set(                     37));//!< build.
    static const Mask Edit(Mask().set(                          38));//!< modify.
    static const Mask SetMask(Mask().set(                       39));//!< selection mask.
    static const Mask Info(Mask().set(                          40));//!< opens info window.
    static const Mask ThreeD(Mask().set(                        41));//!< visual
    static const Mask Show(Mask().set(                          42));//!< visual
    static const Mask Hide(Mask().set(                          43));//!< visual
    static const Mask Toggle(Mask().set(                        44));//!< visual
    static const Mask View(Mask().set(                          45));//!< visual
    static const Mask Top(Mask().set(                           46));//!< command
    static const Mask Front(Mask().set(                         47));//!< command
    static const Mask Right(Mask().set(                         48));//!< command
    static const Mask Iso(Mask().set(                           49));//!< command
    static const Mask HiddenLine(Mask().set(                    50));//!< command
    static const Mask Isolate(Mask().set(                       51));//!< command
    static const Mask Fit(Mask().set(                           52));//!< command
    static const Mask Fill(Mask().set(                          53));//!< command for view and feature
    static const Mask Triangulation(Mask().set(                 54));//!< command
    static const Mask RenderStyle(Mask().set(                   55));//!< command
    static const Mask Box(Mask().set(                           56));//!< command
    static const Mask Sphere(Mask().set(                        57));//!< command
    static const Mask Cone(Mask().set(                          58));//!< command
    static const Mask Cylinder(Mask().set(                      59));//!< command
    static const Mask Blend(Mask().set(                         60));//!< command
    static const Mask Chamfer(Mask().set(                       61));//!< command
    static const Mask Draft(Mask().set(                         62));//!< command
    static const Mask Union(Mask().set(                         63));//!< command
    static const Mask Subtract(Mask().set(                      64));//!< command
    static const Mask Intersect(Mask().set(                     65));//!< command
    static const Mask Import(Mask().set(                        66));//!< command
    static const Mask Export(Mask().set(                        67));//!< command
    static const Mask OCC(Mask().set(                           68));//!< command
    static const Mask Step(Mask().set(                          69));//!< command
    static const Mask OSG(Mask().set(                           70));//!< command
    static const Mask Preferences(Mask().set(                   71));//!< command
    static const Mask SystemReset(Mask().set(                   72));//!< command
    static const Mask SystemToggle(Mask().set(                  73));//!< command
    static const Mask FeatureToSystem(Mask().set(               74));//!< command
    static const Mask SystemToFeature(Mask().set(               75));//!< command
    static const Mask DraggerToFeature(Mask().set(              76));//!< command
    static const Mask FeatureToDragger(Mask().set(              77));//!< command
    static const Mask DatumPlane(Mask().set(                    78));//!< command
    static const Mask CheckShapeIds(Mask().set(                 79));//!< command
    static const Mask LinearMeasure(Mask().set(                 80));//!< command
    static const Mask CheckGeometry(Mask().set(                 81));//!< command
    static const Mask Color(Mask().set(                         82));//!< command
    static const Mask Name(Mask().set(                          83));//!< command
    static const Mask Hollow(Mask().set(                        84));//!< command
    static const Mask Oblong(Mask().set(                        85));//!< command
    static const Mask Extract(Mask().set(                       86));//!< command
    static const Mask FeatureReposition(Mask().set(             87));//!< command
    static const Mask Squash(Mask().set(                        88));//!< command
    static const Mask Strip(Mask().set(                         89));//!< command
    static const Mask Nest(Mask().set(                          90));//!< command
    static const Mask DieSet(Mask().set(                        91));//!< command
    static const Mask Quote(Mask().set(                         92));//!< command
    static const Mask Dirty(Mask().set(                         93));//!< command
    static const Mask Refine(Mask().set(                        94));//!< command
    static const Mask InstanceLinear(Mask().set(                95));//!< command
    static const Mask InstanceMirror(Mask().set(                96));//!< command
    static const Mask InstancePolar(Mask().set(                 97));//!< command
    static const Mask SystemToSelection(Mask().set(             98));//!< command
    static const Mask Offset(Mask().set(                        99));//!< command
    static const Mask Thicken(Mask().set(                      100));//!< command
    static const Mask Sew(Mask().set(                          101));//!< command
    static const Mask Trim(Mask().set(                         102));//!< command
    static const Mask Revision(Mask().set(                     103));//!< command
    static const Mask LOD(Mask().set(                          104));//!< command
    static const Mask RemoveFaces(Mask().set(                  105));//!< command
    static const Mask Torus(Mask().set(                        106));//!< command
    static const Mask Thread(Mask().set(                       107));//!< command
    static const Mask Dissolve(Mask().set(                     108));//!< command
    static const Mask About(Mask().set(                        109));//!< command
    static const Mask DatumAxis(Mask().set(                    110));//!< command
    static const Mask Sketch(Mask().set(                       111));//!< command
    static const Mask Extrude(Mask().set(                      112));//!< command
    static const Mask Revolve(Mask().set(                      113));//!< command
    static const Mask SurfaceMesh(Mask().set(                  114));//!< command
    static const Mask Line(Mask().set(                         115));//!< command
    static const Mask TransitionCurve(Mask().set(              116));//!< command
    static const Mask Ruled(Mask().set(                        117));//!< command
    static const Mask ImagePlane(Mask().set(                   118));//!< command
    static const Mask Sweep(Mask().set(                        119));//!< command
    static const Mask Dump(Mask().set(                         120));//!< command
    static const Mask Graph(Mask().set(                        121));//!< command
    static const Mask Shape(Mask().set(                        122));//!< command
    static const Mask Track(Mask().set(                        123));//!< command
    static const Mask Test(Mask().set(                         124));//!< command
    static const Mask DatumSystem(Mask().set(                  125));//!< command
    static const Mask SurfaceReMesh(Mask().set(                126));//!< command
    static const Mask SurfaceMeshFill(Mask().set(              127));//!< command
    static const Mask MapPCurve(Mask().set(                    128));//!< command
    static const Mask Untrim(Mask().set(                       129));//!< command
    static const Mask Wireframe(Mask().set(                    130));//!< render style
    static const Mask Face(Mask().set(                         131));//!< command
    static const Mask Prism(Mask().set(                        132));//!< command
    static const Mask Toolbar(Mask().set(                      133));//!< application main window

    struct Stow; // forward declare see message/variant.h
    struct Message
    {
      Message();
      Message(const Mask&);
      Message(const Mask&, const Stow&);
      Message(const Mask&, const prj::Message&);
      Message(const Mask&, const app::Message&);
      Message(const Mask&, const slc::Message&);
      Message(const Mask&, const vwr::Message&);
      Message(const Mask&, const ftr::Message&);
      Message(const Mask&, const lod::Message&);
      Message(const Mask&, const cmv::Message&);
      Mask mask;
      std::shared_ptr<Stow> stow;
      bool isPRJ() const;
      bool isAPP() const;
      bool isSLC() const;
      bool isVWR() const;
      bool isFTR() const;
      bool isLOD() const;
      bool isCMV() const;
      const prj::Message& getPRJ() const;
      const app::Message& getAPP() const;
      const slc::Message& getSLC() const;
      const vwr::Message& getVWR() const;
      const ftr::Message& getFTR() const;
      const lod::Message& getLOD() const;
      const cmv::Message& getCMV() const;
    };

    typedef std::function<void (const Message&)> Handler;
    
    //@{
    //! Some convenient functions for common messages
    msg::Message buildGitMessage(const std::string &);
    msg::Message buildStatusMessage(const std::string &);
    msg::Message buildStatusMessage(const std::string&, float);
    msg::Message buildSelectionMask(slc::Mask);
    msg::Message buildShowThreeD(const boost::uuids::uuid&);
    msg::Message buildHideThreeD(const boost::uuids::uuid&);
    msg::Message buildShowOverlay(const boost::uuids::uuid&);
    msg::Message buildHideOverlay(const boost::uuids::uuid&);
    //@}
}



#endif // MSG_MESSAGE_H

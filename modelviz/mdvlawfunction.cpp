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

#include <osg/Switch>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osg/Point>
#include <osg/LineWidth>
#include <osg/Depth>
#include <osg/PrimitiveSet>
#include <osg/BlendFunc>
#include <osgUtil/UpdateVisitor>

#include <Law_Composite.hxx>

#include "law/lwfcue.h"
#include "modelviz/mdvnodemaskdefs.h"
#include "library/lbrplabel.h"
#include "library/lbrchildnamevisitor.h"
#include "library/lbrcsysdragger.h"
#include "preferences/preferencesXML.h"
#include "preferences/prfmanager.h"
#include "modelviz/mdvlawfunction.h"

using namespace mdv;

//we cant use const cue, because plabel doesn't take const parameter
osg::Switch* mdv::generate(lwf::Cue &cIn)
{
  osg::ref_ptr<osg::Switch> out = new osg::Switch();
  osg::ref_ptr<LawCallback> callback = new LawCallback(cIn);
  out->setUpdateCallback(callback);
  
  osg::MatrixTransform *transform = new osg::MatrixTransform();
  transform->setName("transform");
  out->addChild(transform);
  
  osg::PositionAttitudeTransform *scale = new osg::PositionAttitudeTransform();
  scale->setName("scale");
  transform->addChild(scale);
  
  lbr::CSysDragger *dragger = new lbr::CSysDragger();
  dragger->setName("dragger");
  dragger->setScreenScale(75.0);
  dragger->setRotationIncrement(prf::manager().rootPtr->dragger().angularIncrement());
  dragger->setTranslationIncrement(prf::manager().rootPtr->dragger().linearIncrement());
  dragger->setHandleEvents(false);
  dragger->setupDefaultGeometry();
  dragger->setLink();
  dragger->hide(lbr::CSysDragger::SwitchIndexes::LinkIcon);
  dragger->addDraggerCallback(new lbr::CSysCallBack(dragger));
  dragger->linkToMatrix(transform);
  out->addChild(dragger);
  
  //build and add labels and points.
  osg::Geometry *pointGeometry = new osg::Geometry();
  pointGeometry->setNodeMask(mdv::noIntersect);
  pointGeometry->setName("points");
  pointGeometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
//   pointGeometry->getOrCreateStateSet()->setAttribute(data->planeDepth.get());
  pointGeometry->setDataVariance(osg::Object::DYNAMIC);
  pointGeometry->setUseDisplayList(false);
  pointGeometry->getOrCreateStateSet()->setAttribute(new osg::Point(5.0));
  osg::Vec3Array *pointVertices = new osg::Vec3Array();
  pointGeometry->setVertexArray(pointVertices);
  for (prm::Parameter *p : cIn.getParameters())
  {
    pointVertices->push_back(static_cast<osg::Vec3d>(*p));
    lbr::PLabel *tempLabel = new lbr::PLabel(p);
    scale->addChild(tempLabel);
  }
  osg::Vec4Array *pointColor = new osg::Vec4Array();
  pointColor->push_back(osg::Vec4(0.0, 0.0, 1.0, 1.0));
  pointGeometry->setColorArray(pointColor);
  pointGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);
  pointGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, pointVertices->size()));
  scale->addChild(pointGeometry);
    
  //build and add planequad
  osg::Geometry *planeQuad = new osg::Geometry();
  planeQuad->setNodeMask(mdv::noIntersect);
  planeQuad->setName("planeQuad");
  planeQuad->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  planeQuad->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
//   planeQuad->getOrCreateStateSet()->setAttribute(data->planeDepth.get());
  planeQuad->setDataVariance(osg::Object::DYNAMIC);
  planeQuad->setUseDisplayList(false);
  osg::Depth *planeDepth = new osg::Depth();
  planeDepth->setRange(0.004, 1.004);
  planeQuad->getOrCreateStateSet()->setAttributeAndModes(planeDepth);
  osg::BlendFunc* bf = new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA); 
  planeQuad->getOrCreateStateSet()->setAttributeAndModes(bf);
  
  osg::Vec4Array *ca = new osg::Vec4Array();
  ca->push_back(osg::Vec4(1.0, 1.0, 1.0, .5));
  planeQuad->setColorArray(ca);
  planeQuad->setColorBinding(osg::Geometry::BIND_OVERALL);
  
  osg::Vec3Array *normals = new osg::Vec3Array();
  normals->push_back(osg::Vec3d(0.0, 0.0, 1.0));
  planeQuad->setNormalArray(normals);
  planeQuad->setNormalBinding(osg::Geometry::BIND_OVERALL);
  
  osg::Vec3Array *verts = new osg::Vec3Array();
  verts->resizeArray(4);
  (*verts)[0] = osg::Vec3(1.0, 1.0, 0.0);
  (*verts)[1] = osg::Vec3(0.0, 1.0, 0.0);
  (*verts)[2] = osg::Vec3(0.0, 0.0, 0.0);
  (*verts)[3] = osg::Vec3(1.0, 0.0, 0.0);
  planeQuad->setVertexArray(verts);
  
  planeQuad->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0 , verts->size()));
  scale->addChild(planeQuad);
  
  //build lines
  osg::Geometry *lineGeometry = new osg::Geometry();
  lineGeometry->setNodeMask(mdv::noIntersect);
  lineGeometry->setName("lines");
  lineGeometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
  lineGeometry->getOrCreateStateSet()->setAttribute(new osg::LineWidth(4.0f));
//   lineGeometry->getOrCreateStateSet()->setAttribute(data->planeDepth.get());
  lineGeometry->setDataVariance(osg::Object::DYNAMIC);
  lineGeometry->setUseDisplayList(false);
  osg::Vec3Array *lineVertices = new osg::Vec3Array();
  lineVertices->push_back(osg::Vec3d(0.0, 1.0, 0.0));
  lineVertices->push_back(osg::Vec3d(1.0, 1.0, 0.0));
  lineGeometry->setVertexArray(lineVertices);
  osg::Vec4Array *lineColor = new osg::Vec4Array();
  lineColor->push_back(osg::Vec4(1.0, 0.0, 0.0, 1.0));
  lineGeometry->setColorArray(lineColor);
  lineGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);
  lineGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0 , lineVertices->size()));
  scale->addChild(lineGeometry);
  
  out->setAllChildrenOn();
  return out.release();
}

LawCallback::LawCallback(lwf::Cue &cIn)
: cue(cIn)
{
}

LawCallback::~LawCallback() = default;

void LawCallback::setScale(double sIn)
{
  scale = sIn;
  dirty = true;
}

osg::Matrixd LawCallback::getMatrix(osg::Node *node)
{
  lbr::ChildNameVisitor v("dragger");
  node->accept(v);
  lbr::CSysDragger *d = v.castResult<lbr::CSysDragger>();
  assert(d);
  return d->getMatrix();
}

void LawCallback::setMatrix(const osg::Matrixd &mIn, osg::Node *node)
{
  lbr::ChildNameVisitor dv("dragger");
  node->accept(dv);
  lbr::CSysDragger *d = dv.castResult<lbr::CSysDragger>();
  assert(d);
  d->setMatrix(mIn);
  
  lbr::ChildNameVisitor tv("transform");
  node->accept(tv);
  osg::MatrixTransform *t = tv.castResult<osg::MatrixTransform>();
  assert(t);
  t->setMatrix(mIn);
}

void LawCallback::operator()(osg::Node *node, osg::NodeVisitor *visitor)
{
  osgUtil::UpdateVisitor *uVisitor = visitor->asUpdateVisitor();
  if (uVisitor)
  {
    if (dirty)
    {
      update(node);
      dirty = false;
    }
  }
  traverse(node, visitor);
}

/*! @brief Update the law curve representation
 * 
 * @details This won't update for a structural law change.
 * If the law changes type or number of parameters then
 * generate a new node
 */
void LawCallback::update(osg::Node *node)
{
  lbr::ChildNameVisitor sv("scale");
  node->accept(sv);
  osg::PositionAttitudeTransform *at = sv.castResult<osg::PositionAttitudeTransform>();
  assert(at);
  at->setScale(osg::Vec3d(scale, scale, scale));
  
  //update points and get x and y range.
  lbr::ChildrenNameVisitor lv("lbr::PLabel");
  node->accept(lv);
  std::vector<lbr::PLabel*> labels = lv.castResult<lbr::PLabel>();
  
  lbr::ChildNameVisitor pv("points");
  node->accept(pv);
  osg::Geometry *pg = pv.castResult<osg::Geometry>();
  osg::Vec3Array *pverts = dynamic_cast<osg::Vec3Array*>(pg->getVertexArray());
  assert(pverts);
  assert(pverts->size() == labels.size());
  if (pverts->size() != labels.size())
    return;
  
  double xmin = 0.0;
  double xmax = 1.0;
  double ymin = 0.0;
  double ymax = 1.0;
  for (std::size_t i = 0; i < labels.size(); ++i)
  {
    osg::Vec3d loc = static_cast<osg::Vec3d>(*(labels.at(i)->getParameter()));
    loc.z() = 0.0;
    labels.at(i)->setMatrix(osg::Matrixd::translate(loc));
    pverts->at(i) = loc;
    xmin = std::min(xmin, loc.x());
    xmax = std::max(xmax, loc.x());
    ymin = std::min(ymin, loc.y());
    ymax = std::max(ymax, loc.y());
  }
  
  //update plane boundary.
  lbr::ChildNameVisitor planeVisitor("planeQuad");
  node->accept(planeVisitor);
  osg::Geometry *plane = planeVisitor.castResult<osg::Geometry>();
  assert(plane);
  osg::Vec3Array *planePoints = dynamic_cast<osg::Vec3Array*>(plane->getVertexArray());
  assert(planePoints->size() == 4);
  (*planePoints)[0] = osg::Vec3(xmax, ymax, 0.0);
  (*planePoints)[1] = osg::Vec3(xmin, ymax, 0.0);
  (*planePoints)[2] = osg::Vec3(xmin, ymin, 0.0);
  (*planePoints)[3] = osg::Vec3(xmax, ymin, 0.0);
  
  //update lines.
  lbr::ChildNameVisitor linev("lines");
  node->accept(linev);
  osg::Geometry *lg = linev.castResult<osg::Geometry>();
  assert(lg);
  if (!lg)
    return;
  osg::Vec3Array *lverts = dynamic_cast<osg::Vec3Array*>(lg->getVertexArray());
  assert(lverts);
  if (!lverts)
    return;
  lverts->clear();
  osg::DrawArrays *lda = dynamic_cast<osg::DrawArrays*>(lg->getPrimitiveSet(0));
  assert(lda);
  if (!lda)
    return;
  
  auto convertFlatten = [](const prm::Parameter &pIn) -> osg::Vec3d
  {
    osg::Vec3d out = static_cast<osg::Vec3d>(pIn);
    out.z() = 0.0;
    return out;
  };
  
  if (cue.type == lwf::Type::constant || cue.type == lwf::Type::linear)
  {
    assert(cue.boundaries.size() == 2);
    lverts->push_back(convertFlatten(cue.boundaries.front()));
    lverts->push_back(convertFlatten(cue.boundaries.back()));
  }
  else if (cue.type == lwf::Type::interpolate)
  {
    auto law = cue.buildLawFunction();
    auto points = lwf::discreteLaw(law, 20); //magic number, not getting into lod.
    for (const auto &p : points)
      lverts->push_back(p);
  }
  else if (cue.type == lwf::Type::composite)
  {
    opencascade::handle<Law_Composite> compLaw = dynamic_cast<Law_Composite*>(cue.buildLawFunction().get());
    assert(!compLaw.IsNull());
    Law_Laws &laws = compLaw->ChangeLaws();
    auto itLaws = laws.begin();
    if ((cue.datas.size() + 1) == cue.boundaries.size())
    {
      for (std::size_t index = 0; index < cue.datas.size(); ++index, ++itLaws)
      {
        const lwf::Data &data = cue.datas.at(index);
        if (data.subType == lwf::Type::constant || data.subType == lwf::Type::linear)
        {
          lverts->push_back(convertFlatten(cue.boundaries.at(index)));
          lverts->push_back(convertFlatten(cue.boundaries.at(index + 1)));
        }
        else if (data.subType == lwf::Type::interpolate)
        {
          auto points = lwf::discreteLaw(*itLaws, 20);
          for (const auto &p : points)
            lverts->push_back(p);
        }
      }
      //ok we will have duplicate points at boundaries between sub-laws. skipping, doesn't appear to be a problem
//       auto last = std::unique(lverts->begin(), lverts->end());
//       lverts->erase(last, lverts->end());
    }
    else
      std::cout << "ERROR: Inconsistent composite" << std::endl;
  }
  lda->setCount(lverts->size());
}

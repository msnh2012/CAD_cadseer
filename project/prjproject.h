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

#ifndef PRJ_PROJECT_H
#define PRJ_PROJECT_H

#include <map>
#include <memory>
#include <vector>

#include <osg/Vec4>

#include "feature/ftrupdatepayload.h"

namespace boost
{
  namespace uuids{struct uuid;}
  namespace filesystem{class path;}
}

class QTextStream;
class TopoDS_Shape;

namespace msg{struct Message;}
namespace expr{class Manager;}
namespace prm{class Parameter;}
namespace ftr{class ShapeHistory; class InputType;}

namespace prj
{
  class GitManager;
  class Stow;
  
class Project
{
public:
    Project();
    ~Project();
    void readOCC(const std::string &fileName);
    void addOCCShape(const TopoDS_Shape &shapeIn, std::string);
    prm::Parameter* findParameter(const boost::uuids::uuid &idIn) const;
    void updateModel();
    void updateVisual();
    void writeGraphViz(const std::string &fileName);
    void setAllVisualDirty();
    void setColor(const boost::uuids::uuid&, const osg::Vec4&);
    std::vector<boost::uuids::uuid> getAllFeatureIds() const;
    
    ftr::Base* addFeature(std::unique_ptr<ftr::Base> feature);
    bool hasFeature(const boost::uuids::uuid &idIn) const;
    ftr::Base* findFeature(const boost::uuids::uuid &idIn) const;
    void removeFeature(const boost::uuids::uuid &idIn);
    void setCurrentLeaf(const boost::uuids::uuid &idIn);
    void connect(const boost::uuids::uuid &parentIn, const boost::uuids::uuid &childIn, const ftr::InputType &type);
    
    /*! @brief Connects and possibly inserts
    * 
    * @param parentIn id of parent/source feature.
    * @param childIn id of child/target feature.
    * @param type input type.
    */
    void connectInsert(const boost::uuids::uuid &parentIn, const boost::uuids::uuid &childIn, const ftr::InputType &type);
     //!< removes all tags from edges with id as target. removes edge if only tag is parameter.
    void removeParentTag(const boost::uuids::uuid &, const std::string&);
    void clearAllInputs(const boost::uuids::uuid&);
    
    void setFeatureActive(const boost::uuids::uuid&);
    void setFeatureInactive(const boost::uuids::uuid&);
    bool isFeatureActive(const boost::uuids::uuid&);
    bool isFeatureInactive(const boost::uuids::uuid&);
    void setFeatureLeaf(const boost::uuids::uuid&);
    void setFeatureNonLeaf(const boost::uuids::uuid&);
    bool isFeatureLeaf(const boost::uuids::uuid&);
    bool isFeatureNonLeaf(const boost::uuids::uuid&);
    
    void setSaveDirectory(const boost::filesystem::path &directoryIn);
    const boost::filesystem::path& getSaveDirectory() const;
    void save();
    void open(); //!< call setSaveDirectory prior.
    void initializeNew(); //!< call setSaveDirectory prior.
    
    const ftr::ShapeHistory& getShapeHistory() const;
    void shapeTrackDump(const boost::uuids::uuid &shapeId, const boost::filesystem::path &directory) const;
    ftr::UpdatePayload::UpdateMap getParentMap(const boost::uuids::uuid&) const;
    ftr::UpdatePayload getPayload(const boost::uuids::uuid&) const;
    std::vector<boost::uuids::uuid> getLeafChildren(const boost::uuids::uuid&) const; //obsolete
    std::vector<boost::uuids::uuid> getRelatedLeafs(const boost::uuids::uuid&) const;
    std::vector<boost::uuids::uuid> getRewindInputs(const boost::uuids::uuid&) const;
    void hideAlterParents(const boost::uuids::uuid&) const;
    
    expr::Manager& getManager();
    GitManager& getGitManager();
    
    QTextStream& getInfo(QTextStream&) const;
    
    void expressionLink(const boost::uuids::uuid&, int);
    void expressionUnlink(const boost::uuids::uuid&);
    
private:
    void serialWrite();
    std::unique_ptr<Stow> stow; //think pimpl
};
}

#endif // PRJ_PROJECT_H

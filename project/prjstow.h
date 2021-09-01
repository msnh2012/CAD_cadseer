/*
 * CadSeer. Parametric Solid Modeling.
 * Copyright (C) 2017  Thomas S. Anderson blobfish.at.gmx.com
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

#ifndef PRJ_STOW_H
#define PRJ_STOW_H

#include <memory>
#include <vector>

#include <boost/filesystem/path.hpp>

#include "message/msgnode.h"
#include "message/msgsift.h"
#include "expressions/exprmanager.h"
#include "project/prjgitmanager.h"
#include "project/prjgraph.h"
#include "feature/ftrshapehistory.h"

namespace prm{class Parameter;}
namespace ftr{class Base;}

namespace prj
{
  class Project; //forward declaration
  class Stow
  {
  public:
    Stow() = delete;
    Stow(Project&);
    ~Stow();
    
    Vertex addFeature(std::unique_ptr<ftr::Base> feature);
    void removeFeature(Vertex);
    Edge connect(const Vertex &parentIn, const Vertex &childIn, const ftr::InputType &type);
    void sendConnectMessage(const Vertex&, const Vertex&, const ftr::InputType&) const;
    void clearVertex(const Vertex&);
    void disconnect(const Edge&);
    void sendDisconnectMessage(const Vertex&, const Vertex&, const ftr::InputType&) const;
    void removeEdges(Edges);
    
    bool hasFeature(const uuid &idIn) const;
    ftr::Base* findFeature(const uuid&) const;
    Vertex findVertex(const uuid&) const;
    std::vector<uuid> getAllFeatureIds() const;
    prm::Parameter* findParameter(const uuid &idIn) const;
    
    void writeGraphViz(const std::string &fileName);
    void updateLeafStatus();
    void buildShapeHistory();
    
    void setFeatureActive(Vertex);
    void setFeatureInactive(Vertex);
    bool isFeatureActive(Vertex);
    bool isFeatureInactive(Vertex);
    void setFeatureLeaf(Vertex);
    void setFeatureNonLeaf(Vertex);
    bool isFeatureLeaf(Vertex);
    bool isFeatureNonLeaf(Vertex);
    
    void setupDispatcher();
    void setCurrentLeafDispatched(const msg::Message &);
    void removeFeatureDispatched(const msg::Message &);
    void updateDispatched(const msg::Message &);
    void forceUpdateDispatched(const msg::Message &);
    void updateModelDispatched(const msg::Message &);
    void updateVisualDispatched(const msg::Message &);
    void saveProjectRequestDispatched(const msg::Message &);
    void checkShapeIdsDispatched(const msg::Message &);
    void featureStateChangedDispatched(const msg::Message &);
    void dumpProjectGraphDispatched(const msg::Message &);
    void shownThreeDDispatched(const msg::Message&);
    void reorderFeatureDispatched(const msg::Message&);
    void toggleSkippedDispatched(const msg::Message&);
    void dissolveFeatureDispatched(const msg::Message&);
    
    Project &project;
    Graph graph;
    msg::Node node;
    msg::Sift sift;
    expr::Manager expressionManager;
    GitManager gitManager;
    ftr::ShapeHistory shapeHistory;
    boost::filesystem::path saveDirectory;
    bool isLoading = false;
  private:
    void sendStateMessage(const Vertex&, std::size_t);
  };
}

#endif // PRJ_STOW_H

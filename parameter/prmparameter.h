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

#ifndef PRM_PARAMETER_H
#define PRM_PARAMETER_H

#include <memory>
#include <functional>

#include <QObject>
#include <QString>
#include <QStringList>

#include <boost/uuid/uuid.hpp>

namespace boost{namespace filesystem{class path;}}
namespace osg{class Vec3d; class Quat; class Matrixd;}
namespace prj{namespace srl{namespace spt{class Parameter;}}}

namespace prm
{
  namespace Names
  {
    static const QString Radius = QObject::tr("Radius"); //!< cylinder, sphere
    static const QString Height = QObject::tr("Height"); //!< cylinder, box, cone
    static const QString Length = QObject::tr("Length"); //!< box
    static const QString Width = QObject::tr("Width"); //!< box
    static const QString Radius1 = QObject::tr("Radius1"); //!< cone
    static const QString Radius2 = QObject::tr("Radius2"); //!< cone
    static const QString Position = QObject::tr("Position"); //!< blend
    static const QString Distance = QObject::tr("Distance"); //!< chamfer, sketch
    static const QString Angle = QObject::tr("Angle"); //!< draft
    static const QString Offset = QObject::tr("Offset"); //!< datum plane
    static const QString CSys = QObject::tr("CSys"); //!< feature with a coordinate system.
    static const QString Diameter = QObject::tr("Diameter"); //!< sketch
    static const QString Direction = QObject::tr("Direction"); //!< extrude.
    static const QString Scale = QObject::tr("Scale"); //!< image plane.
    static const QString Path = QObject::tr("Path"); //!< image plane.
    static const QString Size = QObject::tr("Size"); //!< datums.
    static const QString AutoSize = QObject::tr("AutoSize"); //!< datums.
  }
  
  /*! Descriptor for path parameters.*/
  enum class PathType
  {
    None, //!< shouldn't be used.
    Directory, //!< future?
    Read, //!< think file open.
    Write, //!< think file save as.
  };
  
  struct Boundary
  {
    enum class End
    {
      None,
      Open, //!< doesn't contain value.
      Closed //!< does contain value.
    };
    Boundary(double, End);
    double value;
    End end = End::None;
  };
  
  struct Interval
  {
    Interval(const Boundary&, const Boundary&);
    Boundary lower;
    Boundary upper;
    bool test(double) const;
  };
  
  struct Constraint
  {
    std::vector<Interval> intervals;
    bool test(double) const;
    
    static Constraint buildAll();
    static Constraint buildNonZeroPositive();
    static Constraint buildZeroPositive();
    static Constraint buildNonZeroNegative();
    static Constraint buildZeroNegative();
    static Constraint buildNonZero();
    static Constraint buildUnit(); //!< parametric range of 0 to 1
    static Constraint buildNonZeroAngle(); //!< -360 to 360 excludes zero
    static Constraint buildNonZeroPositiveAngle(); //!< greater than 0 to 360
    static void unitTest();
  };
  
  typedef std::function<void ()> Handler;
  
  /*! @struct Observer 
  * @brief observes parameter changes and triggers handlers
  * 
  * pimpl idiom for compile performance. Lives in objects
  * interested in parameter changes.
  */
  struct Observer
  {
    Observer();
    Observer(Handler);
    Observer(Handler, Handler);
    ~Observer();
    Observer(const Observer&) = delete; //no copy
    Observer& operator=(const Observer&) = delete; //no copy
    Observer(Observer &&);
    Observer& operator=(Observer &&);
    
    void block();
    void unblock();
    
    struct Stow; //!< forward declare pimpl
    std::unique_ptr<Stow> stow; //!< private data
    
    Handler valueHandler;
    Handler constantHandler;
  };
  
  /*! @struct ObserverBlocker
  * @brief blocks the observer upon construction
  * and unblocks upon destruction
  * 
  * @note not std::vector friendly. need to wrap
  * in unique_ptr to add to vector. Yes even emplace_back.
  */
  struct ObserverBlocker
  {
    ObserverBlocker(Observer &oIn):o(oIn){o.block();}
    ~ObserverBlocker(){o.unblock();}
    Observer &o;
  };
  
  /*! @struct Subject 
  * @brief Sources/Signals of parameter changes.
  * 
  * pimpl idiom for compile performance. Lives in parameter
  * and is reponsible for triggering change signals of owning parameter.
  * 
  * There are two modes of use.
  * 1st, persistent observations like features:
  * Because lifetimes of parameters are <= features, we can directly
  * connect feature functions to the signals. This frees us from the burden
  * of features having connection/wrapping objects for each parameter.
  * For this mode see @see Persistent.
  * 
  * 2nd, transient observations, like parameter dialog:
  * Connection objects live in an observer object that
  * will remove the connection upon observer destruction.
  * For this mode see @see Transient.
  */
  struct Subject
  {
  public:
    Subject();
    ~Subject();
    Subject(const Subject&) = delete; //no copy
    Subject& operator=(const Subject&) = delete; //no copy
    Subject(Subject &&) noexcept;
    Subject& operator=(Subject &&) noexcept;
    
    //@{
    /*! @anchor Persistent
      * @name Persistent
      * 1 way, persistent communication.
      * assumes Handler target life >= Subject life
      */
    void addValueHandler(Handler);
    void addConstantHandler(Handler);
    //@}
    
    //@{
    /*! @anchor Transient
      * @name Transient 
      * 1 way, transient communication.
      * no constraints on target life vs Subject life
      */
    void connect(Observer&);
    //@}
    
    void sendValueChanged() const;
    void sendConstantChanged() const;
    
  private:
    struct Stow; //!< forward declare for private data
    std::unique_ptr<Stow> stow; //!< private data
  };
  
  struct Stow; //forward see variant.h
  
  /*! @brief Parameters are values linked to features
    * 
    * We are using a boost variant to model various types that parameters
    * can take. This presents some challenges as variants can change types
    * implicitly. We want parameters of different types but we want that
    * type to stay constant after being constructed. So no default constructor
    * and parameters' type established at construction by value passed in. setValue
    * functions will assert types are equal and no exception. Release build undefined
    * behaviour. value retrieval using explicit conversion operator using static_cast.
    * a little ugly, but should be safer. value retrieval will also assert with no
    * exception. Enums are a hack onto the int type. int will store the index into
    * the enum string array.
    * 
    */
  class Parameter
  {
  public:
    Parameter() = delete;
    Parameter(const prj::srl::spt::Parameter&);
    Parameter(const QString &nameIn, double valueIn);
    Parameter(const QString &nameIn, int valueIn);
    Parameter(const QString &nameIn, bool valueIn);
    Parameter(const QString &nameIn, const boost::filesystem::path &valueIn, PathType);
    Parameter(const QString &nameIn, const osg::Vec3d &valueIn);
    Parameter(const QString &nameIn, const osg::Matrixd &valueIn);
    Parameter(const Parameter&); //<! warning same Id
    Parameter(const Parameter&, const boost::uuids::uuid&);
    ~Parameter();
    Parameter& operator=(const Parameter&);
    
    QString getName() const {return name;}
    void setName(const QString &nameIn){name = nameIn;}
    bool isConstant() const {return constant;} //!< true = not linked to forumla.
    void setConstant(bool constantIn);
    const boost::uuids::uuid& getId() const {return id;}
    const std::type_info& getValueType() const; //!< compare return by: "getValueType() == typeid(std::string)"
    std::string getValueTypeString() const;
    const Stow& getStow() const; //!< to get access to actual variant for visitation.
    PathType getPathType() const {return pathType;}
    bool isEnumeration() const; //!< has to be int type
    void setEnumeration(const QStringList&); //!< has to be int type
    const QStringList& getEnumeration() const; //!< has to be int type
    const QString& getEnumerationString() const; //!< has to be int type
    
    //@{
    //! original functions from when only doubles supported.
    bool setValue(double valueIn);
    bool setValueQuiet(double valueIn); //!< don't trigger changed signal. use sparingly!
    explicit operator double() const;
    bool isValidValue(const double &valueIn) const;
    void setConstraint(const Constraint &); //!< only for doubles ..maybe ints?
    //@}
    
    //@{
    //! int support functions.
    bool setValue(int);
    bool setValueQuiet(int);
    explicit operator int() const;
    bool isValidValue(const int &valueIn) const; //!< just casting to double and using double constraints.
    //maybe setConstraint
    //@}
    
    //@{
    //! bool support functions.
    bool setValue(bool); //<! true = value was changed.
    bool setValueQuiet(bool);
    explicit operator bool() const;
    //@}
    
    //@{
    //! std::string support functions.
    explicit operator std::string() const;
    //@}
    
    //@{
    //! QString support functions. We don't store QString, only for conversion out.
    explicit operator QString() const;
    //@}
    
    //@{
    //! boost::filesystem::path support functions.
    bool setValue(const boost::filesystem::path&); //<! true = value was changed.
    bool setValueQuiet(const boost::filesystem::path&);
    explicit operator boost::filesystem::path() const;
    //@}
    
    //@{
    //! osg::Vec3d support functions.
    bool setValue(const osg::Vec3d&); //<! true = value was changed.
    bool setValueQuiet(const osg::Vec3d&);
    explicit operator osg::Vec3d() const;
    //@}
    
    //@{
    //! osg::Quat support functions.
    explicit operator osg::Quat() const;
    //@}
    
    //@{
    //! osg::Matrixd support functions.
    bool setValue(const osg::Matrixd&); //<! true = value was changed.
    bool setValueQuiet(const osg::Matrixd&);
    explicit operator osg::Matrixd() const;
    //@}
    
    //! observations of parameter changes.
    //@{
    void connect(Observer&);
    void connectValue(Handler);
    void connectConstant(Handler);
    //@}
    
    prj::srl::spt::Parameter serialOut() const; //serial rename
    void serialIn(const prj::srl::spt::Parameter&); //serial rename
    
  private:
    bool constant = true;
    QString name;
    std::unique_ptr<Stow> stow;
    boost::uuids::uuid id;
    Constraint constraint;
    PathType pathType;
    Subject subject;
    QStringList enumeration;
  };
  
  typedef std::vector<Parameter*> Parameters;
}

#endif // PRM_PARAMETER_H

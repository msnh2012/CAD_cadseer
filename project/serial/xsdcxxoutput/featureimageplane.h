// Copyright (c) 2005-2014 Code Synthesis Tools CC
//
// This program was generated by CodeSynthesis XSD, an XML Schema to
// C++ data binding compiler.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
//
// In addition, as a special exception, Code Synthesis Tools CC gives
// permission to link this program with the Xerces-C++ library (or with
// modified versions of Xerces-C++ that use the same license as Xerces-C++),
// and distribute linked combinations including the two. You must obey
// the GNU General Public License version 2 in all respects for all of
// the code used other than Xerces-C++. If you modify this copy of the
// program, you may extend this exception to your version of the program,
// but you are not obligated to do so. If you do not wish to do so, delete
// this exception statement from your version.
//
// Furthermore, Code Synthesis Tools CC makes a special exception for
// the Free/Libre and Open Source Software (FLOSS) which is described
// in the accompanying FLOSSE file.
//

/**
 * @file
 * @brief Generated from featureimageplane.xsd.
 */

#ifndef PRJ_SRL_FEATUREIMAGEPLANE_H
#define PRJ_SRL_FEATUREIMAGEPLANE_H

#ifndef XSD_CXX11
#define XSD_CXX11
#endif

#ifndef XSD_USE_CHAR
#define XSD_USE_CHAR
#endif

#ifndef XSD_CXX_TREE_USE_CHAR
#define XSD_CXX_TREE_USE_CHAR
#endif

// Begin prologue.
//
//
// End prologue.

#include <xsd/cxx/config.hxx>

#if (XSD_INT_VERSION != 4000000L)
#error XSD runtime version mismatch
#endif

#include <xsd/cxx/pre.hxx>

#include "../../../xmlbase.h"

// Forward declarations.
//
namespace prj
{
  namespace srl
  {
    class FeatureImagePlane;
  }
}


#include <memory>    // ::std::unique_ptr
#include <limits>    // std::numeric_limits
#include <algorithm> // std::binary_search
#include <utility>   // std::move

#include <xsd/cxx/xml/char-utf8.hxx>

#include <xsd/cxx/tree/exceptions.hxx>
#include <xsd/cxx/tree/elements.hxx>
#include <xsd/cxx/tree/containers.hxx>
#include <xsd/cxx/tree/list.hxx>

#include <xsd/cxx/xml/dom/parsing-header.hxx>

#include "featurebase.h"

namespace prj
{
  /**
   * @brief C++ namespace for the %http://www.cadseer.com/prj/srl
   * schema namespace.
   */
  namespace srl
  {
    /**
     * @brief Class corresponding to the %FeatureImagePlane schema type.
     *
     * @nosubgrouping
     */
    class FeatureImagePlane: public ::xml_schema::Type
    {
      public:
      /**
       * @name featureBase
       *
       * @brief Accessor and modifier functions for the %featureBase
       * required element.
       */
      //@{

      /**
       * @brief Element type.
       */
      typedef ::prj::srl::FeatureBase FeatureBaseType;

      /**
       * @brief Element traits type.
       */
      typedef ::xsd::cxx::tree::traits< FeatureBaseType, char > FeatureBaseTraits;

      /**
       * @brief Return a read-only (constant) reference to the element.
       *
       * @return A constant reference to the element.
       */
      const FeatureBaseType&
      featureBase () const;

      /**
       * @brief Return a read-write reference to the element.
       *
       * @return A reference to the element.
       */
      FeatureBaseType&
      featureBase ();

      /**
       * @brief Set the element value.
       *
       * @param x A new value to set.
       *
       * This function makes a copy of its argument and sets it as
       * the new value of the element.
       */
      void
      featureBase (const FeatureBaseType& x);

      /**
       * @brief Set the element value without copying.
       *
       * @param p A new value to use.
       *
       * This function will try to use the passed value directly
       * instead of making a copy.
       */
      void
      featureBase (::std::unique_ptr< FeatureBaseType > p);

      //@}

      /**
       * @name scale
       *
       * @brief Accessor and modifier functions for the %scale
       * required element.
       */
      //@{

      /**
       * @brief Element type.
       */
      typedef ::prj::srl::Parameter ScaleType;

      /**
       * @brief Element traits type.
       */
      typedef ::xsd::cxx::tree::traits< ScaleType, char > ScaleTraits;

      /**
       * @brief Return a read-only (constant) reference to the element.
       *
       * @return A constant reference to the element.
       */
      const ScaleType&
      scale () const;

      /**
       * @brief Return a read-write reference to the element.
       *
       * @return A reference to the element.
       */
      ScaleType&
      scale ();

      /**
       * @brief Set the element value.
       *
       * @param x A new value to set.
       *
       * This function makes a copy of its argument and sets it as
       * the new value of the element.
       */
      void
      scale (const ScaleType& x);

      /**
       * @brief Set the element value without copying.
       *
       * @param p A new value to use.
       *
       * This function will try to use the passed value directly
       * instead of making a copy.
       */
      void
      scale (::std::unique_ptr< ScaleType > p);

      //@}

      /**
       * @name csys
       *
       * @brief Accessor and modifier functions for the %csys
       * required element.
       */
      //@{

      /**
       * @brief Element type.
       */
      typedef ::prj::srl::Parameter CsysType;

      /**
       * @brief Element traits type.
       */
      typedef ::xsd::cxx::tree::traits< CsysType, char > CsysTraits;

      /**
       * @brief Return a read-only (constant) reference to the element.
       *
       * @return A constant reference to the element.
       */
      const CsysType&
      csys () const;

      /**
       * @brief Return a read-write reference to the element.
       *
       * @return A reference to the element.
       */
      CsysType&
      csys ();

      /**
       * @brief Set the element value.
       *
       * @param x A new value to set.
       *
       * This function makes a copy of its argument and sets it as
       * the new value of the element.
       */
      void
      csys (const CsysType& x);

      /**
       * @brief Set the element value without copying.
       *
       * @param p A new value to use.
       *
       * This function will try to use the passed value directly
       * instead of making a copy.
       */
      void
      csys (::std::unique_ptr< CsysType > p);

      //@}

      /**
       * @name csysDragger
       *
       * @brief Accessor and modifier functions for the %csysDragger
       * required element.
       */
      //@{

      /**
       * @brief Element type.
       */
      typedef ::prj::srl::CSysDragger CsysDraggerType;

      /**
       * @brief Element traits type.
       */
      typedef ::xsd::cxx::tree::traits< CsysDraggerType, char > CsysDraggerTraits;

      /**
       * @brief Return a read-only (constant) reference to the element.
       *
       * @return A constant reference to the element.
       */
      const CsysDraggerType&
      csysDragger () const;

      /**
       * @brief Return a read-write reference to the element.
       *
       * @return A reference to the element.
       */
      CsysDraggerType&
      csysDragger ();

      /**
       * @brief Set the element value.
       *
       * @param x A new value to set.
       *
       * This function makes a copy of its argument and sets it as
       * the new value of the element.
       */
      void
      csysDragger (const CsysDraggerType& x);

      /**
       * @brief Set the element value without copying.
       *
       * @param p A new value to use.
       *
       * This function will try to use the passed value directly
       * instead of making a copy.
       */
      void
      csysDragger (::std::unique_ptr< CsysDraggerType > p);

      //@}

      /**
       * @name scaleLabel
       *
       * @brief Accessor and modifier functions for the %scaleLabel
       * required element.
       */
      //@{

      /**
       * @brief Element type.
       */
      typedef ::prj::srl::PLabel ScaleLabelType;

      /**
       * @brief Element traits type.
       */
      typedef ::xsd::cxx::tree::traits< ScaleLabelType, char > ScaleLabelTraits;

      /**
       * @brief Return a read-only (constant) reference to the element.
       *
       * @return A constant reference to the element.
       */
      const ScaleLabelType&
      scaleLabel () const;

      /**
       * @brief Return a read-write reference to the element.
       *
       * @return A reference to the element.
       */
      ScaleLabelType&
      scaleLabel ();

      /**
       * @brief Set the element value.
       *
       * @param x A new value to set.
       *
       * This function makes a copy of its argument and sets it as
       * the new value of the element.
       */
      void
      scaleLabel (const ScaleLabelType& x);

      /**
       * @brief Set the element value without copying.
       *
       * @param p A new value to use.
       *
       * This function will try to use the passed value directly
       * instead of making a copy.
       */
      void
      scaleLabel (::std::unique_ptr< ScaleLabelType > p);

      //@}

      /**
       * @name cornerVec
       *
       * @brief Accessor and modifier functions for the %cornerVec
       * required element.
       */
      //@{

      /**
       * @brief Element type.
       */
      typedef ::prj::srl::Vec3d CornerVecType;

      /**
       * @brief Element traits type.
       */
      typedef ::xsd::cxx::tree::traits< CornerVecType, char > CornerVecTraits;

      /**
       * @brief Return a read-only (constant) reference to the element.
       *
       * @return A constant reference to the element.
       */
      const CornerVecType&
      cornerVec () const;

      /**
       * @brief Return a read-write reference to the element.
       *
       * @return A reference to the element.
       */
      CornerVecType&
      cornerVec ();

      /**
       * @brief Set the element value.
       *
       * @param x A new value to set.
       *
       * This function makes a copy of its argument and sets it as
       * the new value of the element.
       */
      void
      cornerVec (const CornerVecType& x);

      /**
       * @brief Set the element value without copying.
       *
       * @param p A new value to use.
       *
       * This function will try to use the passed value directly
       * instead of making a copy.
       */
      void
      cornerVec (::std::unique_ptr< CornerVecType > p);

      //@}

      /**
       * @name geometry
       *
       * @brief Accessor and modifier functions for the %geometry
       * required element.
       */
      //@{

      /**
       * @brief Element type.
       */
      typedef ::xml_schema::String GeometryType;

      /**
       * @brief Element traits type.
       */
      typedef ::xsd::cxx::tree::traits< GeometryType, char > GeometryTraits;

      /**
       * @brief Return a read-only (constant) reference to the element.
       *
       * @return A constant reference to the element.
       */
      const GeometryType&
      geometry () const;

      /**
       * @brief Return a read-write reference to the element.
       *
       * @return A reference to the element.
       */
      GeometryType&
      geometry ();

      /**
       * @brief Set the element value.
       *
       * @param x A new value to set.
       *
       * This function makes a copy of its argument and sets it as
       * the new value of the element.
       */
      void
      geometry (const GeometryType& x);

      /**
       * @brief Set the element value without copying.
       *
       * @param p A new value to use.
       *
       * This function will try to use the passed value directly
       * instead of making a copy.
       */
      void
      geometry (::std::unique_ptr< GeometryType > p);

      //@}

      /**
       * @name Constructors
       */
      //@{

      /**
       * @brief Create an instance from the ultimate base and
       * initializers for required elements and attributes.
       */
      FeatureImagePlane (const FeatureBaseType&,
                         const ScaleType&,
                         const CsysType&,
                         const CsysDraggerType&,
                         const ScaleLabelType&,
                         const CornerVecType&,
                         const GeometryType&);

      /**
       * @brief Create an instance from the ultimate base and
       * initializers for required elements and attributes
       * (::std::unique_ptr version).
       *
       * This constructor will try to use the passed values directly
       * instead of making copies.
       */
      FeatureImagePlane (::std::unique_ptr< FeatureBaseType >,
                         ::std::unique_ptr< ScaleType >,
                         ::std::unique_ptr< CsysType >,
                         ::std::unique_ptr< CsysDraggerType >,
                         ::std::unique_ptr< ScaleLabelType >,
                         ::std::unique_ptr< CornerVecType >,
                         const GeometryType&);

      /**
       * @brief Create an instance from a DOM element.
       *
       * @param e A DOM element to extract the data from.
       * @param f Flags to create the new instance with.
       * @param c A pointer to the object that will contain the new
       * instance.
       */
      FeatureImagePlane (const ::xercesc::DOMElement& e,
                         ::xml_schema::Flags f = 0,
                         ::xml_schema::Container* c = 0);

      /**
       * @brief Copy constructor.
       *
       * @param x An instance to make a copy of.
       * @param f Flags to create the copy with.
       * @param c A pointer to the object that will contain the copy.
       *
       * For polymorphic object models use the @c _clone function instead.
       */
      FeatureImagePlane (const FeatureImagePlane& x,
                         ::xml_schema::Flags f = 0,
                         ::xml_schema::Container* c = 0);

      /**
       * @brief Copy the instance polymorphically.
       *
       * @param f Flags to create the copy with.
       * @param c A pointer to the object that will contain the copy.
       * @return A pointer to the dynamically allocated copy.
       *
       * This function ensures that the dynamic type of the instance is
       * used for copying and should be used for polymorphic object
       * models instead of the copy constructor.
       */
      virtual FeatureImagePlane*
      _clone (::xml_schema::Flags f = 0,
              ::xml_schema::Container* c = 0) const;

      /**
       * @brief Copy assignment operator.
       *
       * @param x An instance to make a copy of.
       * @return A reference to itself.
       *
       * For polymorphic object models use the @c _clone function instead.
       */
      FeatureImagePlane&
      operator= (const FeatureImagePlane& x);

      //@}

      /**
       * @brief Destructor.
       */
      virtual 
      ~FeatureImagePlane ();

      // Implementation.
      //

      //@cond

      protected:
      void
      parse (::xsd::cxx::xml::dom::parser< char >&,
             ::xml_schema::Flags);

      protected:
      ::xsd::cxx::tree::one< FeatureBaseType > featureBase_;
      ::xsd::cxx::tree::one< ScaleType > scale_;
      ::xsd::cxx::tree::one< CsysType > csys_;
      ::xsd::cxx::tree::one< CsysDraggerType > csysDragger_;
      ::xsd::cxx::tree::one< ScaleLabelType > scaleLabel_;
      ::xsd::cxx::tree::one< CornerVecType > cornerVec_;
      ::xsd::cxx::tree::one< GeometryType > geometry_;

      //@endcond
    };
  }
}

#include <iosfwd>

#include <xercesc/sax/InputSource.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMErrorHandler.hpp>

namespace prj
{
  namespace srl
  {
    /**
     * @name Parsing functions for the %imageplane document root.
     */
    //@{

    /**
     * @brief Parse a URI or a local file.
     *
     * @param uri A URI or a local file name.
     * @param f Parsing flags.
     * @param p Parsing properties. 
     * @return A pointer to the root of the object model.
     *
     * This function uses exceptions to report parsing errors.
     */
    ::std::unique_ptr< ::prj::srl::FeatureImagePlane >
    imageplane (const ::std::string& uri,
                ::xml_schema::Flags f = 0,
                const ::xml_schema::Properties& p = ::xml_schema::Properties ());

    /**
     * @brief Parse a URI or a local file with an error handler.
     *
     * @param uri A URI or a local file name.
     * @param eh An error handler.
     * @param f Parsing flags.
     * @param p Parsing properties. 
     * @return A pointer to the root of the object model.
     *
     * This function reports parsing errors by calling the error handler.
     */
    ::std::unique_ptr< ::prj::srl::FeatureImagePlane >
    imageplane (const ::std::string& uri,
                ::xml_schema::ErrorHandler& eh,
                ::xml_schema::Flags f = 0,
                const ::xml_schema::Properties& p = ::xml_schema::Properties ());

    /**
     * @brief Parse a URI or a local file with a Xerces-C++ DOM error
     * handler.
     *
     * @param uri A URI or a local file name.
     * @param eh A Xerces-C++ DOM error handler.
     * @param f Parsing flags.
     * @param p Parsing properties. 
     * @return A pointer to the root of the object model.
     *
     * This function reports parsing errors by calling the error handler.
     */
    ::std::unique_ptr< ::prj::srl::FeatureImagePlane >
    imageplane (const ::std::string& uri,
                ::xercesc::DOMErrorHandler& eh,
                ::xml_schema::Flags f = 0,
                const ::xml_schema::Properties& p = ::xml_schema::Properties ());

    /**
     * @brief Parse a standard input stream.
     *
     * @param is A standrad input stream.
     * @param f Parsing flags.
     * @param p Parsing properties. 
     * @return A pointer to the root of the object model.
     *
     * This function uses exceptions to report parsing errors.
     */
    ::std::unique_ptr< ::prj::srl::FeatureImagePlane >
    imageplane (::std::istream& is,
                ::xml_schema::Flags f = 0,
                const ::xml_schema::Properties& p = ::xml_schema::Properties ());

    /**
     * @brief Parse a standard input stream with an error handler.
     *
     * @param is A standrad input stream.
     * @param eh An error handler.
     * @param f Parsing flags.
     * @param p Parsing properties. 
     * @return A pointer to the root of the object model.
     *
     * This function reports parsing errors by calling the error handler.
     */
    ::std::unique_ptr< ::prj::srl::FeatureImagePlane >
    imageplane (::std::istream& is,
                ::xml_schema::ErrorHandler& eh,
                ::xml_schema::Flags f = 0,
                const ::xml_schema::Properties& p = ::xml_schema::Properties ());

    /**
     * @brief Parse a standard input stream with a Xerces-C++ DOM error
     * handler.
     *
     * @param is A standrad input stream.
     * @param eh A Xerces-C++ DOM error handler.
     * @param f Parsing flags.
     * @param p Parsing properties. 
     * @return A pointer to the root of the object model.
     *
     * This function reports parsing errors by calling the error handler.
     */
    ::std::unique_ptr< ::prj::srl::FeatureImagePlane >
    imageplane (::std::istream& is,
                ::xercesc::DOMErrorHandler& eh,
                ::xml_schema::Flags f = 0,
                const ::xml_schema::Properties& p = ::xml_schema::Properties ());

    /**
     * @brief Parse a standard input stream with a resource id.
     *
     * @param is A standrad input stream.
     * @param id A resource id.
     * @param f Parsing flags.
     * @param p Parsing properties. 
     * @return A pointer to the root of the object model.
     *
     * The resource id is used to identify the document being parsed in
     * diagnostics as well as to resolve relative paths.
     *
     * This function uses exceptions to report parsing errors.
     */
    ::std::unique_ptr< ::prj::srl::FeatureImagePlane >
    imageplane (::std::istream& is,
                const ::std::string& id,
                ::xml_schema::Flags f = 0,
                const ::xml_schema::Properties& p = ::xml_schema::Properties ());

    /**
     * @brief Parse a standard input stream with a resource id and an
     * error handler.
     *
     * @param is A standrad input stream.
     * @param id A resource id.
     * @param eh An error handler.
     * @param f Parsing flags.
     * @param p Parsing properties. 
     * @return A pointer to the root of the object model.
     *
     * The resource id is used to identify the document being parsed in
     * diagnostics as well as to resolve relative paths.
     *
     * This function reports parsing errors by calling the error handler.
     */
    ::std::unique_ptr< ::prj::srl::FeatureImagePlane >
    imageplane (::std::istream& is,
                const ::std::string& id,
                ::xml_schema::ErrorHandler& eh,
                ::xml_schema::Flags f = 0,
                const ::xml_schema::Properties& p = ::xml_schema::Properties ());

    /**
     * @brief Parse a standard input stream with a resource id and a
     * Xerces-C++ DOM error handler.
     *
     * @param is A standrad input stream.
     * @param id A resource id.
     * @param eh A Xerces-C++ DOM error handler.
     * @param f Parsing flags.
     * @param p Parsing properties. 
     * @return A pointer to the root of the object model.
     *
     * The resource id is used to identify the document being parsed in
     * diagnostics as well as to resolve relative paths.
     *
     * This function reports parsing errors by calling the error handler.
     */
    ::std::unique_ptr< ::prj::srl::FeatureImagePlane >
    imageplane (::std::istream& is,
                const ::std::string& id,
                ::xercesc::DOMErrorHandler& eh,
                ::xml_schema::Flags f = 0,
                const ::xml_schema::Properties& p = ::xml_schema::Properties ());

    /**
     * @brief Parse a Xerces-C++ input source.
     *
     * @param is A Xerces-C++ input source.
     * @param f Parsing flags.
     * @param p Parsing properties. 
     * @return A pointer to the root of the object model.
     *
     * This function uses exceptions to report parsing errors.
     */
    ::std::unique_ptr< ::prj::srl::FeatureImagePlane >
    imageplane (::xercesc::InputSource& is,
                ::xml_schema::Flags f = 0,
                const ::xml_schema::Properties& p = ::xml_schema::Properties ());

    /**
     * @brief Parse a Xerces-C++ input source with an error handler.
     *
     * @param is A Xerces-C++ input source.
     * @param eh An error handler.
     * @param f Parsing flags.
     * @param p Parsing properties. 
     * @return A pointer to the root of the object model.
     *
     * This function reports parsing errors by calling the error handler.
     */
    ::std::unique_ptr< ::prj::srl::FeatureImagePlane >
    imageplane (::xercesc::InputSource& is,
                ::xml_schema::ErrorHandler& eh,
                ::xml_schema::Flags f = 0,
                const ::xml_schema::Properties& p = ::xml_schema::Properties ());

    /**
     * @brief Parse a Xerces-C++ input source with a Xerces-C++ DOM
     * error handler.
     *
     * @param is A Xerces-C++ input source.
     * @param eh A Xerces-C++ DOM error handler.
     * @param f Parsing flags.
     * @param p Parsing properties. 
     * @return A pointer to the root of the object model.
     *
     * This function reports parsing errors by calling the error handler.
     */
    ::std::unique_ptr< ::prj::srl::FeatureImagePlane >
    imageplane (::xercesc::InputSource& is,
                ::xercesc::DOMErrorHandler& eh,
                ::xml_schema::Flags f = 0,
                const ::xml_schema::Properties& p = ::xml_schema::Properties ());

    /**
     * @brief Parse a Xerces-C++ DOM document.
     *
     * @param d A Xerces-C++ DOM document.
     * @param f Parsing flags.
     * @param p Parsing properties. 
     * @return A pointer to the root of the object model.
     */
    ::std::unique_ptr< ::prj::srl::FeatureImagePlane >
    imageplane (const ::xercesc::DOMDocument& d,
                ::xml_schema::Flags f = 0,
                const ::xml_schema::Properties& p = ::xml_schema::Properties ());

    /**
     * @brief Parse a Xerces-C++ DOM document.
     *
     * @param d A pointer to the Xerces-C++ DOM document.
     * @param f Parsing flags.
     * @param p Parsing properties. 
     * @return A pointer to the root of the object model.
     *
     * This function is normally used together with the keep_dom and
     * own_dom parsing flags to assign ownership of the DOM document
     * to the object model.
     */
    ::std::unique_ptr< ::prj::srl::FeatureImagePlane >
    imageplane (::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d,
                ::xml_schema::Flags f = 0,
                const ::xml_schema::Properties& p = ::xml_schema::Properties ());

    //@}
  }
}

#include <iosfwd>

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMErrorHandler.hpp>
#include <xercesc/framework/XMLFormatter.hpp>

#include <xsd/cxx/xml/dom/auto-ptr.hxx>

namespace prj
{
  namespace srl
  {
    void
    operator<< (::xercesc::DOMElement&, const FeatureImagePlane&);

    /**
     * @name Serialization functions for the %imageplane document root.
     */
    //@{

    /**
     * @brief Serialize to a standard output stream.
     *
     * @param os A standrad output stream.
     * @param x An object model to serialize.
     * @param m A namespace information map.
     * @param e A character encoding to produce XML in.
     * @param f Serialization flags.
     *
     * This function uses exceptions to report serialization errors.
     */
    void
    imageplane (::std::ostream& os,
                const ::prj::srl::FeatureImagePlane& x, 
                const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                const ::std::string& e = "UTF-8",
                ::xml_schema::Flags f = 0);

    /**
     * @brief Serialize to a standard output stream with an error handler.
     *
     * @param os A standrad output stream.
     * @param x An object model to serialize.
     * @param eh An error handler.
     * @param m A namespace information map.
     * @param e A character encoding to produce XML in.
     * @param f Serialization flags.
     *
     * This function reports serialization errors by calling the error
     * handler.
     */
    void
    imageplane (::std::ostream& os,
                const ::prj::srl::FeatureImagePlane& x, 
                ::xml_schema::ErrorHandler& eh,
                const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                const ::std::string& e = "UTF-8",
                ::xml_schema::Flags f = 0);

    /**
     * @brief Serialize to a standard output stream with a Xerces-C++ DOM
     * error handler.
     *
     * @param os A standrad output stream.
     * @param x An object model to serialize.
     * @param eh A Xerces-C++ DOM error handler.
     * @param m A namespace information map.
     * @param e A character encoding to produce XML in.
     * @param f Serialization flags.
     *
     * This function reports serialization errors by calling the error
     * handler.
     */
    void
    imageplane (::std::ostream& os,
                const ::prj::srl::FeatureImagePlane& x, 
                ::xercesc::DOMErrorHandler& eh,
                const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                const ::std::string& e = "UTF-8",
                ::xml_schema::Flags f = 0);

    /**
     * @brief Serialize to a Xerces-C++ XML format target.
     *
     * @param ft A Xerces-C++ XML format target.
     * @param x An object model to serialize.
     * @param m A namespace information map.
     * @param e A character encoding to produce XML in.
     * @param f Serialization flags.
     *
     * This function uses exceptions to report serialization errors.
     */
    void
    imageplane (::xercesc::XMLFormatTarget& ft,
                const ::prj::srl::FeatureImagePlane& x, 
                const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                const ::std::string& e = "UTF-8",
                ::xml_schema::Flags f = 0);

    /**
     * @brief Serialize to a Xerces-C++ XML format target with an error
     * handler.
     *
     * @param ft A Xerces-C++ XML format target.
     * @param x An object model to serialize.
     * @param eh An error handler.
     * @param m A namespace information map.
     * @param e A character encoding to produce XML in.
     * @param f Serialization flags.
     *
     * This function reports serialization errors by calling the error
     * handler.
     */
    void
    imageplane (::xercesc::XMLFormatTarget& ft,
                const ::prj::srl::FeatureImagePlane& x, 
                ::xml_schema::ErrorHandler& eh,
                const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                const ::std::string& e = "UTF-8",
                ::xml_schema::Flags f = 0);

    /**
     * @brief Serialize to a Xerces-C++ XML format target with a
     * Xerces-C++ DOM error handler.
     *
     * @param ft A Xerces-C++ XML format target.
     * @param x An object model to serialize.
     * @param eh A Xerces-C++ DOM error handler.
     * @param m A namespace information map.
     * @param e A character encoding to produce XML in.
     * @param f Serialization flags.
     *
     * This function reports serialization errors by calling the error
     * handler.
     */
    void
    imageplane (::xercesc::XMLFormatTarget& ft,
                const ::prj::srl::FeatureImagePlane& x, 
                ::xercesc::DOMErrorHandler& eh,
                const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                const ::std::string& e = "UTF-8",
                ::xml_schema::Flags f = 0);

    /**
     * @brief Serialize to an existing Xerces-C++ DOM document.
     *
     * @param d A Xerces-C++ DOM document.
     * @param x An object model to serialize.
     * @param f Serialization flags.
     *
     * Note that it is your responsibility to create the DOM document
     * with the correct root element as well as set the necessary
     * namespace mapping attributes.
     */
    void
    imageplane (::xercesc::DOMDocument& d,
                const ::prj::srl::FeatureImagePlane& x,
                ::xml_schema::Flags f = 0);

    /**
     * @brief Serialize to a new Xerces-C++ DOM document.
     *
     * @param x An object model to serialize.
     * @param m A namespace information map.
     * @param f Serialization flags.
     * @return A pointer to the new Xerces-C++ DOM document.
     */
    ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument >
    imageplane (const ::prj::srl::FeatureImagePlane& x, 
                const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                ::xml_schema::Flags f = 0);

    //@}
  }
}

#include <xsd/cxx/post.hxx>

// Begin epilogue.
//
//
// End epilogue.

#endif // PRJ_SRL_FEATUREIMAGEPLANE_H
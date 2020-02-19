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

#ifndef PRJ_SRL_INSS_PRJSRLINSSINTERSECT_H
#define PRJ_SRL_INSS_PRJSRLINSSINTERSECT_H

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
    namespace inss
    {
      class Intersect;
    }
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

#include "prjsrlsptpick.h"

#include "prjsrlsptseershape.h"

#include "prjsrlsptintersectionmapping.h"

#include "prjsrlsptbase.h"

namespace prj
{
  namespace srl
  {
    namespace inss
    {
      class Intersect: public ::xml_schema::Type
      {
        public:
        // base
        //
        typedef ::prj::srl::spt::Base BaseType;
        typedef ::xsd::cxx::tree::traits< BaseType, char > BaseTraits;

        const BaseType&
        base () const;

        BaseType&
        base ();

        void
        base (const BaseType& x);

        void
        base (::std::unique_ptr< BaseType > p);

        // seerShape
        //
        typedef ::prj::srl::spt::SeerShape SeerShapeType;
        typedef ::xsd::cxx::tree::traits< SeerShapeType, char > SeerShapeTraits;

        const SeerShapeType&
        seerShape () const;

        SeerShapeType&
        seerShape ();

        void
        seerShape (const SeerShapeType& x);

        void
        seerShape (::std::unique_ptr< SeerShapeType > p);

        // intersectionMapper
        //
        typedef ::prj::srl::spt::IntersectionMapper IntersectionMapperType;
        typedef ::xsd::cxx::tree::traits< IntersectionMapperType, char > IntersectionMapperTraits;

        const IntersectionMapperType&
        intersectionMapper () const;

        IntersectionMapperType&
        intersectionMapper ();

        void
        intersectionMapper (const IntersectionMapperType& x);

        void
        intersectionMapper (::std::unique_ptr< IntersectionMapperType > p);

        // targetPicks
        //
        typedef ::prj::srl::spt::Pick TargetPicksType;
        typedef ::xsd::cxx::tree::sequence< TargetPicksType > TargetPicksSequence;
        typedef TargetPicksSequence::iterator TargetPicksIterator;
        typedef TargetPicksSequence::const_iterator TargetPicksConstIterator;
        typedef ::xsd::cxx::tree::traits< TargetPicksType, char > TargetPicksTraits;

        const TargetPicksSequence&
        targetPicks () const;

        TargetPicksSequence&
        targetPicks ();

        void
        targetPicks (const TargetPicksSequence& s);

        // toolPicks
        //
        typedef ::prj::srl::spt::Pick ToolPicksType;
        typedef ::xsd::cxx::tree::sequence< ToolPicksType > ToolPicksSequence;
        typedef ToolPicksSequence::iterator ToolPicksIterator;
        typedef ToolPicksSequence::const_iterator ToolPicksConstIterator;
        typedef ::xsd::cxx::tree::traits< ToolPicksType, char > ToolPicksTraits;

        const ToolPicksSequence&
        toolPicks () const;

        ToolPicksSequence&
        toolPicks ();

        void
        toolPicks (const ToolPicksSequence& s);

        // Constructors.
        //
        Intersect (const BaseType&,
                   const SeerShapeType&,
                   const IntersectionMapperType&);

        Intersect (::std::unique_ptr< BaseType >,
                   ::std::unique_ptr< SeerShapeType >,
                   ::std::unique_ptr< IntersectionMapperType >);

        Intersect (const ::xercesc::DOMElement& e,
                   ::xml_schema::Flags f = 0,
                   ::xml_schema::Container* c = 0);

        Intersect (const Intersect& x,
                   ::xml_schema::Flags f = 0,
                   ::xml_schema::Container* c = 0);

        virtual Intersect*
        _clone (::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0) const;

        Intersect&
        operator= (const Intersect& x);

        virtual 
        ~Intersect ();

        // Implementation.
        //
        protected:
        void
        parse (::xsd::cxx::xml::dom::parser< char >&,
               ::xml_schema::Flags);

        protected:
        ::xsd::cxx::tree::one< BaseType > base_;
        ::xsd::cxx::tree::one< SeerShapeType > seerShape_;
        ::xsd::cxx::tree::one< IntersectionMapperType > intersectionMapper_;
        TargetPicksSequence targetPicks_;
        ToolPicksSequence toolPicks_;
      };
    }
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
    namespace inss
    {
      // Parse a URI or a local file.
      //

      ::std::unique_ptr< ::prj::srl::inss::Intersect >
      intersect (const ::std::string& uri,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::inss::Intersect >
      intersect (const ::std::string& uri,
                 ::xml_schema::ErrorHandler& eh,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::inss::Intersect >
      intersect (const ::std::string& uri,
                 ::xercesc::DOMErrorHandler& eh,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      // Parse std::istream.
      //

      ::std::unique_ptr< ::prj::srl::inss::Intersect >
      intersect (::std::istream& is,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::inss::Intersect >
      intersect (::std::istream& is,
                 ::xml_schema::ErrorHandler& eh,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::inss::Intersect >
      intersect (::std::istream& is,
                 ::xercesc::DOMErrorHandler& eh,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::inss::Intersect >
      intersect (::std::istream& is,
                 const ::std::string& id,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::inss::Intersect >
      intersect (::std::istream& is,
                 const ::std::string& id,
                 ::xml_schema::ErrorHandler& eh,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::inss::Intersect >
      intersect (::std::istream& is,
                 const ::std::string& id,
                 ::xercesc::DOMErrorHandler& eh,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      // Parse xercesc::InputSource.
      //

      ::std::unique_ptr< ::prj::srl::inss::Intersect >
      intersect (::xercesc::InputSource& is,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::inss::Intersect >
      intersect (::xercesc::InputSource& is,
                 ::xml_schema::ErrorHandler& eh,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::inss::Intersect >
      intersect (::xercesc::InputSource& is,
                 ::xercesc::DOMErrorHandler& eh,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      // Parse xercesc::DOMDocument.
      //

      ::std::unique_ptr< ::prj::srl::inss::Intersect >
      intersect (const ::xercesc::DOMDocument& d,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::inss::Intersect >
      intersect (::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d,
                 ::xml_schema::Flags f = 0,
                 const ::xml_schema::Properties& p = ::xml_schema::Properties ());
    }
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
    namespace inss
    {
      void
      operator<< (::xercesc::DOMElement&, const Intersect&);

      // Serialize to std::ostream.
      //

      void
      intersect (::std::ostream& os,
                 const ::prj::srl::inss::Intersect& x, 
                 const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                 const ::std::string& e = "UTF-8",
                 ::xml_schema::Flags f = 0);

      void
      intersect (::std::ostream& os,
                 const ::prj::srl::inss::Intersect& x, 
                 ::xml_schema::ErrorHandler& eh,
                 const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                 const ::std::string& e = "UTF-8",
                 ::xml_schema::Flags f = 0);

      void
      intersect (::std::ostream& os,
                 const ::prj::srl::inss::Intersect& x, 
                 ::xercesc::DOMErrorHandler& eh,
                 const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                 const ::std::string& e = "UTF-8",
                 ::xml_schema::Flags f = 0);

      // Serialize to xercesc::XMLFormatTarget.
      //

      void
      intersect (::xercesc::XMLFormatTarget& ft,
                 const ::prj::srl::inss::Intersect& x, 
                 const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                 const ::std::string& e = "UTF-8",
                 ::xml_schema::Flags f = 0);

      void
      intersect (::xercesc::XMLFormatTarget& ft,
                 const ::prj::srl::inss::Intersect& x, 
                 ::xml_schema::ErrorHandler& eh,
                 const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                 const ::std::string& e = "UTF-8",
                 ::xml_schema::Flags f = 0);

      void
      intersect (::xercesc::XMLFormatTarget& ft,
                 const ::prj::srl::inss::Intersect& x, 
                 ::xercesc::DOMErrorHandler& eh,
                 const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                 const ::std::string& e = "UTF-8",
                 ::xml_schema::Flags f = 0);

      // Serialize to an existing xercesc::DOMDocument.
      //

      void
      intersect (::xercesc::DOMDocument& d,
                 const ::prj::srl::inss::Intersect& x,
                 ::xml_schema::Flags f = 0);

      // Serialize to a new xercesc::DOMDocument.
      //

      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument >
      intersect (const ::prj::srl::inss::Intersect& x, 
                 const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                 ::xml_schema::Flags f = 0);
    }
  }
}

#include <xsd/cxx/post.hxx>

// Begin epilogue.
//
//
// End epilogue.

#endif // PRJ_SRL_INSS_PRJSRLINSSINTERSECT_H

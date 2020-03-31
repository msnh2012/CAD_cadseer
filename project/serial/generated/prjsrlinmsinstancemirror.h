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

#ifndef PRJ_SRL_INMS_PRJSRLINMSINSTANCEMIRROR_H
#define PRJ_SRL_INMS_PRJSRLINMSINSTANCEMIRROR_H

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
    namespace inms
    {
      class InstanceMirror;
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

#include "prjsrlsptparameter.h"

#include "prjsrlsptoverlay.h"

#include "prjsrlsptpick.h"

#include "prjsrlsptseershape.h"

#include "prjsrlsptinstancemapping.h"

#include "prjsrlsptbase.h"

namespace prj
{
  namespace srl
  {
    namespace inms
    {
      class InstanceMirror: public ::xml_schema::Type
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

        // instanceMaps
        //
        typedef ::prj::srl::spt::InstanceMaps InstanceMapsType;
        typedef ::xsd::cxx::tree::traits< InstanceMapsType, char > InstanceMapsTraits;

        const InstanceMapsType&
        instanceMaps () const;

        InstanceMapsType&
        instanceMaps ();

        void
        instanceMaps (const InstanceMapsType& x);

        void
        instanceMaps (::std::unique_ptr< InstanceMapsType > p);

        // csysDragger
        //
        typedef ::prj::srl::spt::CSysDragger CsysDraggerType;
        typedef ::xsd::cxx::tree::traits< CsysDraggerType, char > CsysDraggerTraits;

        const CsysDraggerType&
        csysDragger () const;

        CsysDraggerType&
        csysDragger ();

        void
        csysDragger (const CsysDraggerType& x);

        void
        csysDragger (::std::unique_ptr< CsysDraggerType > p);

        // csys
        //
        typedef ::prj::srl::spt::Parameter CsysType;
        typedef ::xsd::cxx::tree::traits< CsysType, char > CsysTraits;

        const CsysType&
        csys () const;

        CsysType&
        csys ();

        void
        csys (const CsysType& x);

        void
        csys (::std::unique_ptr< CsysType > p);

        // includeSource
        //
        typedef ::prj::srl::spt::Parameter IncludeSourceType;
        typedef ::xsd::cxx::tree::traits< IncludeSourceType, char > IncludeSourceTraits;

        const IncludeSourceType&
        includeSource () const;

        IncludeSourceType&
        includeSource ();

        void
        includeSource (const IncludeSourceType& x);

        void
        includeSource (::std::unique_ptr< IncludeSourceType > p);

        // shapePick
        //
        typedef ::prj::srl::spt::Pick ShapePickType;
        typedef ::xsd::cxx::tree::traits< ShapePickType, char > ShapePickTraits;

        const ShapePickType&
        shapePick () const;

        ShapePickType&
        shapePick ();

        void
        shapePick (const ShapePickType& x);

        void
        shapePick (::std::unique_ptr< ShapePickType > p);

        // planePick
        //
        typedef ::prj::srl::spt::Pick PlanePickType;
        typedef ::xsd::cxx::tree::traits< PlanePickType, char > PlanePickTraits;

        const PlanePickType&
        planePick () const;

        PlanePickType&
        planePick ();

        void
        planePick (const PlanePickType& x);

        void
        planePick (::std::unique_ptr< PlanePickType > p);

        // includeSourceLabel
        //
        typedef ::prj::srl::spt::PLabel IncludeSourceLabelType;
        typedef ::xsd::cxx::tree::traits< IncludeSourceLabelType, char > IncludeSourceLabelTraits;

        const IncludeSourceLabelType&
        includeSourceLabel () const;

        IncludeSourceLabelType&
        includeSourceLabel ();

        void
        includeSourceLabel (const IncludeSourceLabelType& x);

        void
        includeSourceLabel (::std::unique_ptr< IncludeSourceLabelType > p);

        // draggerVisible
        //
        typedef ::xml_schema::Boolean DraggerVisibleType;
        typedef ::xsd::cxx::tree::traits< DraggerVisibleType, char > DraggerVisibleTraits;

        const DraggerVisibleType&
        draggerVisible () const;

        DraggerVisibleType&
        draggerVisible ();

        void
        draggerVisible (const DraggerVisibleType& x);

        // Constructors.
        //
        InstanceMirror (const BaseType&,
                        const SeerShapeType&,
                        const InstanceMapsType&,
                        const CsysDraggerType&,
                        const CsysType&,
                        const IncludeSourceType&,
                        const ShapePickType&,
                        const PlanePickType&,
                        const IncludeSourceLabelType&,
                        const DraggerVisibleType&);

        InstanceMirror (::std::unique_ptr< BaseType >,
                        ::std::unique_ptr< SeerShapeType >,
                        ::std::unique_ptr< InstanceMapsType >,
                        ::std::unique_ptr< CsysDraggerType >,
                        ::std::unique_ptr< CsysType >,
                        ::std::unique_ptr< IncludeSourceType >,
                        ::std::unique_ptr< ShapePickType >,
                        ::std::unique_ptr< PlanePickType >,
                        ::std::unique_ptr< IncludeSourceLabelType >,
                        const DraggerVisibleType&);

        InstanceMirror (const ::xercesc::DOMElement& e,
                        ::xml_schema::Flags f = 0,
                        ::xml_schema::Container* c = 0);

        InstanceMirror (const InstanceMirror& x,
                        ::xml_schema::Flags f = 0,
                        ::xml_schema::Container* c = 0);

        virtual InstanceMirror*
        _clone (::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0) const;

        InstanceMirror&
        operator= (const InstanceMirror& x);

        virtual 
        ~InstanceMirror ();

        // Implementation.
        //
        protected:
        void
        parse (::xsd::cxx::xml::dom::parser< char >&,
               ::xml_schema::Flags);

        protected:
        ::xsd::cxx::tree::one< BaseType > base_;
        ::xsd::cxx::tree::one< SeerShapeType > seerShape_;
        ::xsd::cxx::tree::one< InstanceMapsType > instanceMaps_;
        ::xsd::cxx::tree::one< CsysDraggerType > csysDragger_;
        ::xsd::cxx::tree::one< CsysType > csys_;
        ::xsd::cxx::tree::one< IncludeSourceType > includeSource_;
        ::xsd::cxx::tree::one< ShapePickType > shapePick_;
        ::xsd::cxx::tree::one< PlanePickType > planePick_;
        ::xsd::cxx::tree::one< IncludeSourceLabelType > includeSourceLabel_;
        ::xsd::cxx::tree::one< DraggerVisibleType > draggerVisible_;
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
    namespace inms
    {
      // Parse a URI or a local file.
      //

      ::std::unique_ptr< ::prj::srl::inms::InstanceMirror >
      instanceMirror (const ::std::string& uri,
                      ::xml_schema::Flags f = 0,
                      const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::inms::InstanceMirror >
      instanceMirror (const ::std::string& uri,
                      ::xml_schema::ErrorHandler& eh,
                      ::xml_schema::Flags f = 0,
                      const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::inms::InstanceMirror >
      instanceMirror (const ::std::string& uri,
                      ::xercesc::DOMErrorHandler& eh,
                      ::xml_schema::Flags f = 0,
                      const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      // Parse std::istream.
      //

      ::std::unique_ptr< ::prj::srl::inms::InstanceMirror >
      instanceMirror (::std::istream& is,
                      ::xml_schema::Flags f = 0,
                      const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::inms::InstanceMirror >
      instanceMirror (::std::istream& is,
                      ::xml_schema::ErrorHandler& eh,
                      ::xml_schema::Flags f = 0,
                      const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::inms::InstanceMirror >
      instanceMirror (::std::istream& is,
                      ::xercesc::DOMErrorHandler& eh,
                      ::xml_schema::Flags f = 0,
                      const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::inms::InstanceMirror >
      instanceMirror (::std::istream& is,
                      const ::std::string& id,
                      ::xml_schema::Flags f = 0,
                      const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::inms::InstanceMirror >
      instanceMirror (::std::istream& is,
                      const ::std::string& id,
                      ::xml_schema::ErrorHandler& eh,
                      ::xml_schema::Flags f = 0,
                      const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::inms::InstanceMirror >
      instanceMirror (::std::istream& is,
                      const ::std::string& id,
                      ::xercesc::DOMErrorHandler& eh,
                      ::xml_schema::Flags f = 0,
                      const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      // Parse xercesc::InputSource.
      //

      ::std::unique_ptr< ::prj::srl::inms::InstanceMirror >
      instanceMirror (::xercesc::InputSource& is,
                      ::xml_schema::Flags f = 0,
                      const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::inms::InstanceMirror >
      instanceMirror (::xercesc::InputSource& is,
                      ::xml_schema::ErrorHandler& eh,
                      ::xml_schema::Flags f = 0,
                      const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::inms::InstanceMirror >
      instanceMirror (::xercesc::InputSource& is,
                      ::xercesc::DOMErrorHandler& eh,
                      ::xml_schema::Flags f = 0,
                      const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      // Parse xercesc::DOMDocument.
      //

      ::std::unique_ptr< ::prj::srl::inms::InstanceMirror >
      instanceMirror (const ::xercesc::DOMDocument& d,
                      ::xml_schema::Flags f = 0,
                      const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::inms::InstanceMirror >
      instanceMirror (::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d,
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
    namespace inms
    {
      void
      operator<< (::xercesc::DOMElement&, const InstanceMirror&);

      // Serialize to std::ostream.
      //

      void
      instanceMirror (::std::ostream& os,
                      const ::prj::srl::inms::InstanceMirror& x, 
                      const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                      const ::std::string& e = "UTF-8",
                      ::xml_schema::Flags f = 0);

      void
      instanceMirror (::std::ostream& os,
                      const ::prj::srl::inms::InstanceMirror& x, 
                      ::xml_schema::ErrorHandler& eh,
                      const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                      const ::std::string& e = "UTF-8",
                      ::xml_schema::Flags f = 0);

      void
      instanceMirror (::std::ostream& os,
                      const ::prj::srl::inms::InstanceMirror& x, 
                      ::xercesc::DOMErrorHandler& eh,
                      const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                      const ::std::string& e = "UTF-8",
                      ::xml_schema::Flags f = 0);

      // Serialize to xercesc::XMLFormatTarget.
      //

      void
      instanceMirror (::xercesc::XMLFormatTarget& ft,
                      const ::prj::srl::inms::InstanceMirror& x, 
                      const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                      const ::std::string& e = "UTF-8",
                      ::xml_schema::Flags f = 0);

      void
      instanceMirror (::xercesc::XMLFormatTarget& ft,
                      const ::prj::srl::inms::InstanceMirror& x, 
                      ::xml_schema::ErrorHandler& eh,
                      const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                      const ::std::string& e = "UTF-8",
                      ::xml_schema::Flags f = 0);

      void
      instanceMirror (::xercesc::XMLFormatTarget& ft,
                      const ::prj::srl::inms::InstanceMirror& x, 
                      ::xercesc::DOMErrorHandler& eh,
                      const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                      const ::std::string& e = "UTF-8",
                      ::xml_schema::Flags f = 0);

      // Serialize to an existing xercesc::DOMDocument.
      //

      void
      instanceMirror (::xercesc::DOMDocument& d,
                      const ::prj::srl::inms::InstanceMirror& x,
                      ::xml_schema::Flags f = 0);

      // Serialize to a new xercesc::DOMDocument.
      //

      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument >
      instanceMirror (const ::prj::srl::inms::InstanceMirror& x, 
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

#endif // PRJ_SRL_INMS_PRJSRLINMSINSTANCEMIRROR_H
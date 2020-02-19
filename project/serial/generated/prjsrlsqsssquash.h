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

#ifndef PRJ_SRL_SQSS_PRJSRLSQSSSQUASH_H
#define PRJ_SRL_SQSS_PRJSRLSQSSSQUASH_H

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
    namespace sqss
    {
      class Squash;
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

#include "prjsrlsptseershape.h"

#include "prjsrlsptbase.h"

#include "prjsrlsptpick.h"

#include "prjsrlsptoverlay.h"

namespace prj
{
  namespace srl
  {
    namespace sqss
    {
      class Squash: public ::xml_schema::Type
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

        // picks
        //
        typedef ::prj::srl::spt::Pick PicksType;
        typedef ::xsd::cxx::tree::sequence< PicksType > PicksSequence;
        typedef PicksSequence::iterator PicksIterator;
        typedef PicksSequence::const_iterator PicksConstIterator;
        typedef ::xsd::cxx::tree::traits< PicksType, char > PicksTraits;

        const PicksSequence&
        picks () const;

        PicksSequence&
        picks ();

        void
        picks (const PicksSequence& s);

        // faceId
        //
        typedef ::xml_schema::String FaceIdType;
        typedef ::xsd::cxx::tree::traits< FaceIdType, char > FaceIdTraits;

        const FaceIdType&
        faceId () const;

        FaceIdType&
        faceId ();

        void
        faceId (const FaceIdType& x);

        void
        faceId (::std::unique_ptr< FaceIdType > p);

        static const FaceIdType&
        faceId_default_value ();

        // wireId
        //
        typedef ::xml_schema::String WireIdType;
        typedef ::xsd::cxx::tree::traits< WireIdType, char > WireIdTraits;

        const WireIdType&
        wireId () const;

        WireIdType&
        wireId ();

        void
        wireId (const WireIdType& x);

        void
        wireId (::std::unique_ptr< WireIdType > p);

        static const WireIdType&
        wireId_default_value ();

        // granularity
        //
        typedef ::prj::srl::spt::Parameter GranularityType;
        typedef ::xsd::cxx::tree::traits< GranularityType, char > GranularityTraits;

        const GranularityType&
        granularity () const;

        GranularityType&
        granularity ();

        void
        granularity (const GranularityType& x);

        void
        granularity (::std::unique_ptr< GranularityType > p);

        // label
        //
        typedef ::prj::srl::spt::PLabel LabelType;
        typedef ::xsd::cxx::tree::traits< LabelType, char > LabelTraits;

        const LabelType&
        label () const;

        LabelType&
        label ();

        void
        label (const LabelType& x);

        void
        label (::std::unique_ptr< LabelType > p);

        // Constructors.
        //
        Squash (const BaseType&,
                const SeerShapeType&,
                const FaceIdType&,
                const WireIdType&,
                const GranularityType&,
                const LabelType&);

        Squash (::std::unique_ptr< BaseType >,
                ::std::unique_ptr< SeerShapeType >,
                const FaceIdType&,
                const WireIdType&,
                ::std::unique_ptr< GranularityType >,
                ::std::unique_ptr< LabelType >);

        Squash (const ::xercesc::DOMElement& e,
                ::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0);

        Squash (const Squash& x,
                ::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0);

        virtual Squash*
        _clone (::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0) const;

        Squash&
        operator= (const Squash& x);

        virtual 
        ~Squash ();

        // Implementation.
        //
        protected:
        void
        parse (::xsd::cxx::xml::dom::parser< char >&,
               ::xml_schema::Flags);

        protected:
        ::xsd::cxx::tree::one< BaseType > base_;
        ::xsd::cxx::tree::one< SeerShapeType > seerShape_;
        PicksSequence picks_;
        ::xsd::cxx::tree::one< FaceIdType > faceId_;
        static const FaceIdType faceId_default_value_;
        ::xsd::cxx::tree::one< WireIdType > wireId_;
        static const WireIdType wireId_default_value_;
        ::xsd::cxx::tree::one< GranularityType > granularity_;
        ::xsd::cxx::tree::one< LabelType > label_;
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
    namespace sqss
    {
      // Parse a URI or a local file.
      //

      ::std::unique_ptr< ::prj::srl::sqss::Squash >
      squash (const ::std::string& uri,
              ::xml_schema::Flags f = 0,
              const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::sqss::Squash >
      squash (const ::std::string& uri,
              ::xml_schema::ErrorHandler& eh,
              ::xml_schema::Flags f = 0,
              const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::sqss::Squash >
      squash (const ::std::string& uri,
              ::xercesc::DOMErrorHandler& eh,
              ::xml_schema::Flags f = 0,
              const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      // Parse std::istream.
      //

      ::std::unique_ptr< ::prj::srl::sqss::Squash >
      squash (::std::istream& is,
              ::xml_schema::Flags f = 0,
              const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::sqss::Squash >
      squash (::std::istream& is,
              ::xml_schema::ErrorHandler& eh,
              ::xml_schema::Flags f = 0,
              const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::sqss::Squash >
      squash (::std::istream& is,
              ::xercesc::DOMErrorHandler& eh,
              ::xml_schema::Flags f = 0,
              const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::sqss::Squash >
      squash (::std::istream& is,
              const ::std::string& id,
              ::xml_schema::Flags f = 0,
              const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::sqss::Squash >
      squash (::std::istream& is,
              const ::std::string& id,
              ::xml_schema::ErrorHandler& eh,
              ::xml_schema::Flags f = 0,
              const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::sqss::Squash >
      squash (::std::istream& is,
              const ::std::string& id,
              ::xercesc::DOMErrorHandler& eh,
              ::xml_schema::Flags f = 0,
              const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      // Parse xercesc::InputSource.
      //

      ::std::unique_ptr< ::prj::srl::sqss::Squash >
      squash (::xercesc::InputSource& is,
              ::xml_schema::Flags f = 0,
              const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::sqss::Squash >
      squash (::xercesc::InputSource& is,
              ::xml_schema::ErrorHandler& eh,
              ::xml_schema::Flags f = 0,
              const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::sqss::Squash >
      squash (::xercesc::InputSource& is,
              ::xercesc::DOMErrorHandler& eh,
              ::xml_schema::Flags f = 0,
              const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      // Parse xercesc::DOMDocument.
      //

      ::std::unique_ptr< ::prj::srl::sqss::Squash >
      squash (const ::xercesc::DOMDocument& d,
              ::xml_schema::Flags f = 0,
              const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::sqss::Squash >
      squash (::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d,
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
    namespace sqss
    {
      void
      operator<< (::xercesc::DOMElement&, const Squash&);

      // Serialize to std::ostream.
      //

      void
      squash (::std::ostream& os,
              const ::prj::srl::sqss::Squash& x, 
              const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
              const ::std::string& e = "UTF-8",
              ::xml_schema::Flags f = 0);

      void
      squash (::std::ostream& os,
              const ::prj::srl::sqss::Squash& x, 
              ::xml_schema::ErrorHandler& eh,
              const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
              const ::std::string& e = "UTF-8",
              ::xml_schema::Flags f = 0);

      void
      squash (::std::ostream& os,
              const ::prj::srl::sqss::Squash& x, 
              ::xercesc::DOMErrorHandler& eh,
              const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
              const ::std::string& e = "UTF-8",
              ::xml_schema::Flags f = 0);

      // Serialize to xercesc::XMLFormatTarget.
      //

      void
      squash (::xercesc::XMLFormatTarget& ft,
              const ::prj::srl::sqss::Squash& x, 
              const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
              const ::std::string& e = "UTF-8",
              ::xml_schema::Flags f = 0);

      void
      squash (::xercesc::XMLFormatTarget& ft,
              const ::prj::srl::sqss::Squash& x, 
              ::xml_schema::ErrorHandler& eh,
              const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
              const ::std::string& e = "UTF-8",
              ::xml_schema::Flags f = 0);

      void
      squash (::xercesc::XMLFormatTarget& ft,
              const ::prj::srl::sqss::Squash& x, 
              ::xercesc::DOMErrorHandler& eh,
              const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
              const ::std::string& e = "UTF-8",
              ::xml_schema::Flags f = 0);

      // Serialize to an existing xercesc::DOMDocument.
      //

      void
      squash (::xercesc::DOMDocument& d,
              const ::prj::srl::sqss::Squash& x,
              ::xml_schema::Flags f = 0);

      // Serialize to a new xercesc::DOMDocument.
      //

      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument >
      squash (const ::prj::srl::sqss::Squash& x, 
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

#endif // PRJ_SRL_SQSS_PRJSRLSQSSSQUASH_H

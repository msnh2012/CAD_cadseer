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

#ifndef PRJ_SRL_TSCS_PRJSRLTSCSTRANSITIONCURVE_H
#define PRJ_SRL_TSCS_PRJSRLTSCSTRANSITIONCURVE_H

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
    namespace tscs
    {
      class TransitionCurve;
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

#include "prjsrlsptseershape.h"

#include "prjsrlsptparameter.h"

#include "prjsrlsptpick.h"

#include "prjsrlsptoverlay.h"

#include "prjsrlsptbase.h"

namespace prj
{
  namespace srl
  {
    namespace tscs
    {
      class TransitionCurve: public ::xml_schema::Type
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

        // pick0Direction
        //
        typedef ::prj::srl::spt::Parameter Pick0DirectionType;
        typedef ::xsd::cxx::tree::traits< Pick0DirectionType, char > Pick0DirectionTraits;

        const Pick0DirectionType&
        pick0Direction () const;

        Pick0DirectionType&
        pick0Direction ();

        void
        pick0Direction (const Pick0DirectionType& x);

        void
        pick0Direction (::std::unique_ptr< Pick0DirectionType > p);

        // pick1Direction
        //
        typedef ::prj::srl::spt::Parameter Pick1DirectionType;
        typedef ::xsd::cxx::tree::traits< Pick1DirectionType, char > Pick1DirectionTraits;

        const Pick1DirectionType&
        pick1Direction () const;

        Pick1DirectionType&
        pick1Direction ();

        void
        pick1Direction (const Pick1DirectionType& x);

        void
        pick1Direction (::std::unique_ptr< Pick1DirectionType > p);

        // pick0Magnitude
        //
        typedef ::prj::srl::spt::Parameter Pick0MagnitudeType;
        typedef ::xsd::cxx::tree::traits< Pick0MagnitudeType, char > Pick0MagnitudeTraits;

        const Pick0MagnitudeType&
        pick0Magnitude () const;

        Pick0MagnitudeType&
        pick0Magnitude ();

        void
        pick0Magnitude (const Pick0MagnitudeType& x);

        void
        pick0Magnitude (::std::unique_ptr< Pick0MagnitudeType > p);

        // pick1Magnitude
        //
        typedef ::prj::srl::spt::Parameter Pick1MagnitudeType;
        typedef ::xsd::cxx::tree::traits< Pick1MagnitudeType, char > Pick1MagnitudeTraits;

        const Pick1MagnitudeType&
        pick1Magnitude () const;

        Pick1MagnitudeType&
        pick1Magnitude ();

        void
        pick1Magnitude (const Pick1MagnitudeType& x);

        void
        pick1Magnitude (::std::unique_ptr< Pick1MagnitudeType > p);

        // directionLabel0
        //
        typedef ::prj::srl::spt::PLabel DirectionLabel0Type;
        typedef ::xsd::cxx::tree::traits< DirectionLabel0Type, char > DirectionLabel0Traits;

        const DirectionLabel0Type&
        directionLabel0 () const;

        DirectionLabel0Type&
        directionLabel0 ();

        void
        directionLabel0 (const DirectionLabel0Type& x);

        void
        directionLabel0 (::std::unique_ptr< DirectionLabel0Type > p);

        // directionLabel1
        //
        typedef ::prj::srl::spt::PLabel DirectionLabel1Type;
        typedef ::xsd::cxx::tree::traits< DirectionLabel1Type, char > DirectionLabel1Traits;

        const DirectionLabel1Type&
        directionLabel1 () const;

        DirectionLabel1Type&
        directionLabel1 ();

        void
        directionLabel1 (const DirectionLabel1Type& x);

        void
        directionLabel1 (::std::unique_ptr< DirectionLabel1Type > p);

        // magnitudeLabel0
        //
        typedef ::prj::srl::spt::PLabel MagnitudeLabel0Type;
        typedef ::xsd::cxx::tree::traits< MagnitudeLabel0Type, char > MagnitudeLabel0Traits;

        const MagnitudeLabel0Type&
        magnitudeLabel0 () const;

        MagnitudeLabel0Type&
        magnitudeLabel0 ();

        void
        magnitudeLabel0 (const MagnitudeLabel0Type& x);

        void
        magnitudeLabel0 (::std::unique_ptr< MagnitudeLabel0Type > p);

        // magnitudeLabel1
        //
        typedef ::prj::srl::spt::PLabel MagnitudeLabel1Type;
        typedef ::xsd::cxx::tree::traits< MagnitudeLabel1Type, char > MagnitudeLabel1Traits;

        const MagnitudeLabel1Type&
        magnitudeLabel1 () const;

        MagnitudeLabel1Type&
        magnitudeLabel1 ();

        void
        magnitudeLabel1 (const MagnitudeLabel1Type& x);

        void
        magnitudeLabel1 (::std::unique_ptr< MagnitudeLabel1Type > p);

        // curveId
        //
        typedef ::xml_schema::String CurveIdType;
        typedef ::xsd::cxx::tree::traits< CurveIdType, char > CurveIdTraits;

        const CurveIdType&
        curveId () const;

        CurveIdType&
        curveId ();

        void
        curveId (const CurveIdType& x);

        void
        curveId (::std::unique_ptr< CurveIdType > p);

        static const CurveIdType&
        curveId_default_value ();

        // vertex0Id
        //
        typedef ::xml_schema::String Vertex0IdType;
        typedef ::xsd::cxx::tree::traits< Vertex0IdType, char > Vertex0IdTraits;

        const Vertex0IdType&
        vertex0Id () const;

        Vertex0IdType&
        vertex0Id ();

        void
        vertex0Id (const Vertex0IdType& x);

        void
        vertex0Id (::std::unique_ptr< Vertex0IdType > p);

        static const Vertex0IdType&
        vertex0Id_default_value ();

        // vertex1Id
        //
        typedef ::xml_schema::String Vertex1IdType;
        typedef ::xsd::cxx::tree::traits< Vertex1IdType, char > Vertex1IdTraits;

        const Vertex1IdType&
        vertex1Id () const;

        Vertex1IdType&
        vertex1Id ();

        void
        vertex1Id (const Vertex1IdType& x);

        void
        vertex1Id (::std::unique_ptr< Vertex1IdType > p);

        static const Vertex1IdType&
        vertex1Id_default_value ();

        // Constructors.
        //
        TransitionCurve (const BaseType&,
                         const SeerShapeType&,
                         const Pick0DirectionType&,
                         const Pick1DirectionType&,
                         const Pick0MagnitudeType&,
                         const Pick1MagnitudeType&,
                         const DirectionLabel0Type&,
                         const DirectionLabel1Type&,
                         const MagnitudeLabel0Type&,
                         const MagnitudeLabel1Type&,
                         const CurveIdType&,
                         const Vertex0IdType&,
                         const Vertex1IdType&);

        TransitionCurve (::std::unique_ptr< BaseType >,
                         ::std::unique_ptr< SeerShapeType >,
                         ::std::unique_ptr< Pick0DirectionType >,
                         ::std::unique_ptr< Pick1DirectionType >,
                         ::std::unique_ptr< Pick0MagnitudeType >,
                         ::std::unique_ptr< Pick1MagnitudeType >,
                         ::std::unique_ptr< DirectionLabel0Type >,
                         ::std::unique_ptr< DirectionLabel1Type >,
                         ::std::unique_ptr< MagnitudeLabel0Type >,
                         ::std::unique_ptr< MagnitudeLabel1Type >,
                         const CurveIdType&,
                         const Vertex0IdType&,
                         const Vertex1IdType&);

        TransitionCurve (const ::xercesc::DOMElement& e,
                         ::xml_schema::Flags f = 0,
                         ::xml_schema::Container* c = 0);

        TransitionCurve (const TransitionCurve& x,
                         ::xml_schema::Flags f = 0,
                         ::xml_schema::Container* c = 0);

        virtual TransitionCurve*
        _clone (::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0) const;

        TransitionCurve&
        operator= (const TransitionCurve& x);

        virtual 
        ~TransitionCurve ();

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
        ::xsd::cxx::tree::one< Pick0DirectionType > pick0Direction_;
        ::xsd::cxx::tree::one< Pick1DirectionType > pick1Direction_;
        ::xsd::cxx::tree::one< Pick0MagnitudeType > pick0Magnitude_;
        ::xsd::cxx::tree::one< Pick1MagnitudeType > pick1Magnitude_;
        ::xsd::cxx::tree::one< DirectionLabel0Type > directionLabel0_;
        ::xsd::cxx::tree::one< DirectionLabel1Type > directionLabel1_;
        ::xsd::cxx::tree::one< MagnitudeLabel0Type > magnitudeLabel0_;
        ::xsd::cxx::tree::one< MagnitudeLabel1Type > magnitudeLabel1_;
        ::xsd::cxx::tree::one< CurveIdType > curveId_;
        static const CurveIdType curveId_default_value_;
        ::xsd::cxx::tree::one< Vertex0IdType > vertex0Id_;
        static const Vertex0IdType vertex0Id_default_value_;
        ::xsd::cxx::tree::one< Vertex1IdType > vertex1Id_;
        static const Vertex1IdType vertex1Id_default_value_;
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
    namespace tscs
    {
      // Parse a URI or a local file.
      //

      ::std::unique_ptr< ::prj::srl::tscs::TransitionCurve >
      transitionCurve (const ::std::string& uri,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::tscs::TransitionCurve >
      transitionCurve (const ::std::string& uri,
                       ::xml_schema::ErrorHandler& eh,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::tscs::TransitionCurve >
      transitionCurve (const ::std::string& uri,
                       ::xercesc::DOMErrorHandler& eh,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      // Parse std::istream.
      //

      ::std::unique_ptr< ::prj::srl::tscs::TransitionCurve >
      transitionCurve (::std::istream& is,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::tscs::TransitionCurve >
      transitionCurve (::std::istream& is,
                       ::xml_schema::ErrorHandler& eh,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::tscs::TransitionCurve >
      transitionCurve (::std::istream& is,
                       ::xercesc::DOMErrorHandler& eh,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::tscs::TransitionCurve >
      transitionCurve (::std::istream& is,
                       const ::std::string& id,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::tscs::TransitionCurve >
      transitionCurve (::std::istream& is,
                       const ::std::string& id,
                       ::xml_schema::ErrorHandler& eh,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::tscs::TransitionCurve >
      transitionCurve (::std::istream& is,
                       const ::std::string& id,
                       ::xercesc::DOMErrorHandler& eh,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      // Parse xercesc::InputSource.
      //

      ::std::unique_ptr< ::prj::srl::tscs::TransitionCurve >
      transitionCurve (::xercesc::InputSource& is,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::tscs::TransitionCurve >
      transitionCurve (::xercesc::InputSource& is,
                       ::xml_schema::ErrorHandler& eh,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::tscs::TransitionCurve >
      transitionCurve (::xercesc::InputSource& is,
                       ::xercesc::DOMErrorHandler& eh,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      // Parse xercesc::DOMDocument.
      //

      ::std::unique_ptr< ::prj::srl::tscs::TransitionCurve >
      transitionCurve (const ::xercesc::DOMDocument& d,
                       ::xml_schema::Flags f = 0,
                       const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::tscs::TransitionCurve >
      transitionCurve (::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d,
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
    namespace tscs
    {
      void
      operator<< (::xercesc::DOMElement&, const TransitionCurve&);

      // Serialize to std::ostream.
      //

      void
      transitionCurve (::std::ostream& os,
                       const ::prj::srl::tscs::TransitionCurve& x, 
                       const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                       const ::std::string& e = "UTF-8",
                       ::xml_schema::Flags f = 0);

      void
      transitionCurve (::std::ostream& os,
                       const ::prj::srl::tscs::TransitionCurve& x, 
                       ::xml_schema::ErrorHandler& eh,
                       const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                       const ::std::string& e = "UTF-8",
                       ::xml_schema::Flags f = 0);

      void
      transitionCurve (::std::ostream& os,
                       const ::prj::srl::tscs::TransitionCurve& x, 
                       ::xercesc::DOMErrorHandler& eh,
                       const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                       const ::std::string& e = "UTF-8",
                       ::xml_schema::Flags f = 0);

      // Serialize to xercesc::XMLFormatTarget.
      //

      void
      transitionCurve (::xercesc::XMLFormatTarget& ft,
                       const ::prj::srl::tscs::TransitionCurve& x, 
                       const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                       const ::std::string& e = "UTF-8",
                       ::xml_schema::Flags f = 0);

      void
      transitionCurve (::xercesc::XMLFormatTarget& ft,
                       const ::prj::srl::tscs::TransitionCurve& x, 
                       ::xml_schema::ErrorHandler& eh,
                       const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                       const ::std::string& e = "UTF-8",
                       ::xml_schema::Flags f = 0);

      void
      transitionCurve (::xercesc::XMLFormatTarget& ft,
                       const ::prj::srl::tscs::TransitionCurve& x, 
                       ::xercesc::DOMErrorHandler& eh,
                       const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
                       const ::std::string& e = "UTF-8",
                       ::xml_schema::Flags f = 0);

      // Serialize to an existing xercesc::DOMDocument.
      //

      void
      transitionCurve (::xercesc::DOMDocument& d,
                       const ::prj::srl::tscs::TransitionCurve& x,
                       ::xml_schema::Flags f = 0);

      // Serialize to a new xercesc::DOMDocument.
      //

      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument >
      transitionCurve (const ::prj::srl::tscs::TransitionCurve& x, 
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

#endif // PRJ_SRL_TSCS_PRJSRLTSCSTRANSITIONCURVE_H

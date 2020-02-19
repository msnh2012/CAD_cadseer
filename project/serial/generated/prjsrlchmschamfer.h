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

#ifndef PRJ_SRL_CHMS_PRJSRLCHMSCHAMFER_H
#define PRJ_SRL_CHMS_PRJSRLCHMSCHAMFER_H

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
    namespace chms
    {
      class Entry;
      class Cue;
      class Chamfer;
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

#include "prjsrlsptpick.h"

#include "prjsrlsptoverlay.h"

#include "prjsrlsptseershape.h"

#include "prjsrlsptbase.h"

namespace prj
{
  namespace srl
  {
    namespace chms
    {
      class Entry: public ::xml_schema::Type
      {
        public:
        // style
        //
        typedef ::xml_schema::Int StyleType;
        typedef ::xsd::cxx::tree::traits< StyleType, char > StyleTraits;

        const StyleType&
        style () const;

        StyleType&
        style ();

        void
        style (const StyleType& x);

        // parameter1
        //
        typedef ::prj::srl::spt::Parameter Parameter1Type;
        typedef ::xsd::cxx::tree::traits< Parameter1Type, char > Parameter1Traits;

        const Parameter1Type&
        parameter1 () const;

        Parameter1Type&
        parameter1 ();

        void
        parameter1 (const Parameter1Type& x);

        void
        parameter1 (::std::unique_ptr< Parameter1Type > p);

        // parameter2
        //
        typedef ::prj::srl::spt::Parameter Parameter2Type;
        typedef ::xsd::cxx::tree::optional< Parameter2Type > Parameter2Optional;
        typedef ::xsd::cxx::tree::traits< Parameter2Type, char > Parameter2Traits;

        const Parameter2Optional&
        parameter2 () const;

        Parameter2Optional&
        parameter2 ();

        void
        parameter2 (const Parameter2Type& x);

        void
        parameter2 (const Parameter2Optional& x);

        void
        parameter2 (::std::unique_ptr< Parameter2Type > p);

        // label1
        //
        typedef ::prj::srl::spt::PLabel Label1Type;
        typedef ::xsd::cxx::tree::traits< Label1Type, char > Label1Traits;

        const Label1Type&
        label1 () const;

        Label1Type&
        label1 ();

        void
        label1 (const Label1Type& x);

        void
        label1 (::std::unique_ptr< Label1Type > p);

        // label2
        //
        typedef ::prj::srl::spt::PLabel Label2Type;
        typedef ::xsd::cxx::tree::optional< Label2Type > Label2Optional;
        typedef ::xsd::cxx::tree::traits< Label2Type, char > Label2Traits;

        const Label2Optional&
        label2 () const;

        Label2Optional&
        label2 ();

        void
        label2 (const Label2Type& x);

        void
        label2 (const Label2Optional& x);

        void
        label2 (::std::unique_ptr< Label2Type > p);

        // edgePicks
        //
        typedef ::prj::srl::spt::Pick EdgePicksType;
        typedef ::xsd::cxx::tree::sequence< EdgePicksType > EdgePicksSequence;
        typedef EdgePicksSequence::iterator EdgePicksIterator;
        typedef EdgePicksSequence::const_iterator EdgePicksConstIterator;
        typedef ::xsd::cxx::tree::traits< EdgePicksType, char > EdgePicksTraits;

        const EdgePicksSequence&
        edgePicks () const;

        EdgePicksSequence&
        edgePicks ();

        void
        edgePicks (const EdgePicksSequence& s);

        // facePicks
        //
        typedef ::prj::srl::spt::Pick FacePicksType;
        typedef ::xsd::cxx::tree::sequence< FacePicksType > FacePicksSequence;
        typedef FacePicksSequence::iterator FacePicksIterator;
        typedef FacePicksSequence::const_iterator FacePicksConstIterator;
        typedef ::xsd::cxx::tree::traits< FacePicksType, char > FacePicksTraits;

        const FacePicksSequence&
        facePicks () const;

        FacePicksSequence&
        facePicks ();

        void
        facePicks (const FacePicksSequence& s);

        // Constructors.
        //
        Entry (const StyleType&,
               const Parameter1Type&,
               const Label1Type&);

        Entry (const StyleType&,
               ::std::unique_ptr< Parameter1Type >,
               ::std::unique_ptr< Label1Type >);

        Entry (const ::xercesc::DOMElement& e,
               ::xml_schema::Flags f = 0,
               ::xml_schema::Container* c = 0);

        Entry (const Entry& x,
               ::xml_schema::Flags f = 0,
               ::xml_schema::Container* c = 0);

        virtual Entry*
        _clone (::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0) const;

        Entry&
        operator= (const Entry& x);

        virtual 
        ~Entry ();

        // Implementation.
        //
        protected:
        void
        parse (::xsd::cxx::xml::dom::parser< char >&,
               ::xml_schema::Flags);

        protected:
        ::xsd::cxx::tree::one< StyleType > style_;
        ::xsd::cxx::tree::one< Parameter1Type > parameter1_;
        Parameter2Optional parameter2_;
        ::xsd::cxx::tree::one< Label1Type > label1_;
        Label2Optional label2_;
        EdgePicksSequence edgePicks_;
        FacePicksSequence facePicks_;
      };

      class Cue: public ::xml_schema::Type
      {
        public:
        // mode
        //
        typedef ::xml_schema::Int ModeType;
        typedef ::xsd::cxx::tree::traits< ModeType, char > ModeTraits;

        const ModeType&
        mode () const;

        ModeType&
        mode ();

        void
        mode (const ModeType& x);

        // entries
        //
        typedef ::prj::srl::chms::Entry EntriesType;
        typedef ::xsd::cxx::tree::sequence< EntriesType > EntriesSequence;
        typedef EntriesSequence::iterator EntriesIterator;
        typedef EntriesSequence::const_iterator EntriesConstIterator;
        typedef ::xsd::cxx::tree::traits< EntriesType, char > EntriesTraits;

        const EntriesSequence&
        entries () const;

        EntriesSequence&
        entries ();

        void
        entries (const EntriesSequence& s);

        // Constructors.
        //
        Cue (const ModeType&);

        Cue (const ::xercesc::DOMElement& e,
             ::xml_schema::Flags f = 0,
             ::xml_schema::Container* c = 0);

        Cue (const Cue& x,
             ::xml_schema::Flags f = 0,
             ::xml_schema::Container* c = 0);

        virtual Cue*
        _clone (::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0) const;

        Cue&
        operator= (const Cue& x);

        virtual 
        ~Cue ();

        // Implementation.
        //
        protected:
        void
        parse (::xsd::cxx::xml::dom::parser< char >&,
               ::xml_schema::Flags);

        protected:
        ::xsd::cxx::tree::one< ModeType > mode_;
        EntriesSequence entries_;
      };

      class Chamfer: public ::xml_schema::Type
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

        // shapeMap
        //
        typedef ::prj::srl::spt::EvolveRecord ShapeMapType;
        typedef ::xsd::cxx::tree::sequence< ShapeMapType > ShapeMapSequence;
        typedef ShapeMapSequence::iterator ShapeMapIterator;
        typedef ShapeMapSequence::const_iterator ShapeMapConstIterator;
        typedef ::xsd::cxx::tree::traits< ShapeMapType, char > ShapeMapTraits;

        const ShapeMapSequence&
        shapeMap () const;

        ShapeMapSequence&
        shapeMap ();

        void
        shapeMap (const ShapeMapSequence& s);

        // cue
        //
        typedef ::prj::srl::chms::Cue CueType;
        typedef ::xsd::cxx::tree::traits< CueType, char > CueTraits;

        const CueType&
        cue () const;

        CueType&
        cue ();

        void
        cue (const CueType& x);

        void
        cue (::std::unique_ptr< CueType > p);

        // Constructors.
        //
        Chamfer (const BaseType&,
                 const SeerShapeType&,
                 const CueType&);

        Chamfer (::std::unique_ptr< BaseType >,
                 ::std::unique_ptr< SeerShapeType >,
                 ::std::unique_ptr< CueType >);

        Chamfer (const ::xercesc::DOMElement& e,
                 ::xml_schema::Flags f = 0,
                 ::xml_schema::Container* c = 0);

        Chamfer (const Chamfer& x,
                 ::xml_schema::Flags f = 0,
                 ::xml_schema::Container* c = 0);

        virtual Chamfer*
        _clone (::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0) const;

        Chamfer&
        operator= (const Chamfer& x);

        virtual 
        ~Chamfer ();

        // Implementation.
        //
        protected:
        void
        parse (::xsd::cxx::xml::dom::parser< char >&,
               ::xml_schema::Flags);

        protected:
        ::xsd::cxx::tree::one< BaseType > base_;
        ::xsd::cxx::tree::one< SeerShapeType > seerShape_;
        ShapeMapSequence shapeMap_;
        ::xsd::cxx::tree::one< CueType > cue_;
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
    namespace chms
    {
      // Parse a URI or a local file.
      //

      ::std::unique_ptr< ::prj::srl::chms::Chamfer >
      chamfer (const ::std::string& uri,
               ::xml_schema::Flags f = 0,
               const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::chms::Chamfer >
      chamfer (const ::std::string& uri,
               ::xml_schema::ErrorHandler& eh,
               ::xml_schema::Flags f = 0,
               const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::chms::Chamfer >
      chamfer (const ::std::string& uri,
               ::xercesc::DOMErrorHandler& eh,
               ::xml_schema::Flags f = 0,
               const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      // Parse std::istream.
      //

      ::std::unique_ptr< ::prj::srl::chms::Chamfer >
      chamfer (::std::istream& is,
               ::xml_schema::Flags f = 0,
               const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::chms::Chamfer >
      chamfer (::std::istream& is,
               ::xml_schema::ErrorHandler& eh,
               ::xml_schema::Flags f = 0,
               const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::chms::Chamfer >
      chamfer (::std::istream& is,
               ::xercesc::DOMErrorHandler& eh,
               ::xml_schema::Flags f = 0,
               const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::chms::Chamfer >
      chamfer (::std::istream& is,
               const ::std::string& id,
               ::xml_schema::Flags f = 0,
               const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::chms::Chamfer >
      chamfer (::std::istream& is,
               const ::std::string& id,
               ::xml_schema::ErrorHandler& eh,
               ::xml_schema::Flags f = 0,
               const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::chms::Chamfer >
      chamfer (::std::istream& is,
               const ::std::string& id,
               ::xercesc::DOMErrorHandler& eh,
               ::xml_schema::Flags f = 0,
               const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      // Parse xercesc::InputSource.
      //

      ::std::unique_ptr< ::prj::srl::chms::Chamfer >
      chamfer (::xercesc::InputSource& is,
               ::xml_schema::Flags f = 0,
               const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::chms::Chamfer >
      chamfer (::xercesc::InputSource& is,
               ::xml_schema::ErrorHandler& eh,
               ::xml_schema::Flags f = 0,
               const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::chms::Chamfer >
      chamfer (::xercesc::InputSource& is,
               ::xercesc::DOMErrorHandler& eh,
               ::xml_schema::Flags f = 0,
               const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      // Parse xercesc::DOMDocument.
      //

      ::std::unique_ptr< ::prj::srl::chms::Chamfer >
      chamfer (const ::xercesc::DOMDocument& d,
               ::xml_schema::Flags f = 0,
               const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::chms::Chamfer >
      chamfer (::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d,
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
    namespace chms
    {
      void
      operator<< (::xercesc::DOMElement&, const Entry&);

      void
      operator<< (::xercesc::DOMElement&, const Cue&);

      void
      operator<< (::xercesc::DOMElement&, const Chamfer&);

      // Serialize to std::ostream.
      //

      void
      chamfer (::std::ostream& os,
               const ::prj::srl::chms::Chamfer& x, 
               const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
               const ::std::string& e = "UTF-8",
               ::xml_schema::Flags f = 0);

      void
      chamfer (::std::ostream& os,
               const ::prj::srl::chms::Chamfer& x, 
               ::xml_schema::ErrorHandler& eh,
               const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
               const ::std::string& e = "UTF-8",
               ::xml_schema::Flags f = 0);

      void
      chamfer (::std::ostream& os,
               const ::prj::srl::chms::Chamfer& x, 
               ::xercesc::DOMErrorHandler& eh,
               const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
               const ::std::string& e = "UTF-8",
               ::xml_schema::Flags f = 0);

      // Serialize to xercesc::XMLFormatTarget.
      //

      void
      chamfer (::xercesc::XMLFormatTarget& ft,
               const ::prj::srl::chms::Chamfer& x, 
               const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
               const ::std::string& e = "UTF-8",
               ::xml_schema::Flags f = 0);

      void
      chamfer (::xercesc::XMLFormatTarget& ft,
               const ::prj::srl::chms::Chamfer& x, 
               ::xml_schema::ErrorHandler& eh,
               const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
               const ::std::string& e = "UTF-8",
               ::xml_schema::Flags f = 0);

      void
      chamfer (::xercesc::XMLFormatTarget& ft,
               const ::prj::srl::chms::Chamfer& x, 
               ::xercesc::DOMErrorHandler& eh,
               const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
               const ::std::string& e = "UTF-8",
               ::xml_schema::Flags f = 0);

      // Serialize to an existing xercesc::DOMDocument.
      //

      void
      chamfer (::xercesc::DOMDocument& d,
               const ::prj::srl::chms::Chamfer& x,
               ::xml_schema::Flags f = 0);

      // Serialize to a new xercesc::DOMDocument.
      //

      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument >
      chamfer (const ::prj::srl::chms::Chamfer& x, 
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

#endif // PRJ_SRL_CHMS_PRJSRLCHMSCHAMFER_H

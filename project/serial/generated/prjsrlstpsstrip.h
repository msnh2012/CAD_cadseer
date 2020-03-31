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

#ifndef PRJ_SRL_STPS_PRJSRLSTPSSTRIP_H
#define PRJ_SRL_STPS_PRJSRLSTPSSTRIP_H

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
    namespace stps
    {
      class Station;
      class Strip;
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

#include "prjsrlsptoverlay.h"

namespace prj
{
  namespace srl
  {
    namespace stps
    {
      class Station: public ::xml_schema::Type
      {
        public:
        // text
        //
        typedef ::xml_schema::String TextType;
        typedef ::xsd::cxx::tree::traits< TextType, char > TextTraits;

        const TextType&
        text () const;

        TextType&
        text ();

        void
        text (const TextType& x);

        void
        text (::std::unique_ptr< TextType > p);

        // matrix
        //
        typedef ::prj::srl::spt::Matrixd MatrixType;
        typedef ::xsd::cxx::tree::traits< MatrixType, char > MatrixTraits;

        const MatrixType&
        matrix () const;

        MatrixType&
        matrix ();

        void
        matrix (const MatrixType& x);

        void
        matrix (::std::unique_ptr< MatrixType > p);

        // Constructors.
        //
        Station (const TextType&,
                 const MatrixType&);

        Station (const TextType&,
                 ::std::unique_ptr< MatrixType >);

        Station (const ::xercesc::DOMElement& e,
                 ::xml_schema::Flags f = 0,
                 ::xml_schema::Container* c = 0);

        Station (const Station& x,
                 ::xml_schema::Flags f = 0,
                 ::xml_schema::Container* c = 0);

        virtual Station*
        _clone (::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0) const;

        Station&
        operator= (const Station& x);

        virtual 
        ~Station ();

        // Implementation.
        //
        protected:
        void
        parse (::xsd::cxx::xml::dom::parser< char >&,
               ::xml_schema::Flags);

        protected:
        ::xsd::cxx::tree::one< TextType > text_;
        ::xsd::cxx::tree::one< MatrixType > matrix_;
      };

      class Strip: public ::xml_schema::Type
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

        // feedDirection
        //
        typedef ::prj::srl::spt::Parameter FeedDirectionType;
        typedef ::xsd::cxx::tree::traits< FeedDirectionType, char > FeedDirectionTraits;

        const FeedDirectionType&
        feedDirection () const;

        FeedDirectionType&
        feedDirection ();

        void
        feedDirection (const FeedDirectionType& x);

        void
        feedDirection (::std::unique_ptr< FeedDirectionType > p);

        // pitch
        //
        typedef ::prj::srl::spt::Parameter PitchType;
        typedef ::xsd::cxx::tree::traits< PitchType, char > PitchTraits;

        const PitchType&
        pitch () const;

        PitchType&
        pitch ();

        void
        pitch (const PitchType& x);

        void
        pitch (::std::unique_ptr< PitchType > p);

        // width
        //
        typedef ::prj::srl::spt::Parameter WidthType;
        typedef ::xsd::cxx::tree::traits< WidthType, char > WidthTraits;

        const WidthType&
        width () const;

        WidthType&
        width ();

        void
        width (const WidthType& x);

        void
        width (::std::unique_ptr< WidthType > p);

        // widthOffset
        //
        typedef ::prj::srl::spt::Parameter WidthOffsetType;
        typedef ::xsd::cxx::tree::traits< WidthOffsetType, char > WidthOffsetTraits;

        const WidthOffsetType&
        widthOffset () const;

        WidthOffsetType&
        widthOffset ();

        void
        widthOffset (const WidthOffsetType& x);

        void
        widthOffset (::std::unique_ptr< WidthOffsetType > p);

        // gap
        //
        typedef ::prj::srl::spt::Parameter GapType;
        typedef ::xsd::cxx::tree::traits< GapType, char > GapTraits;

        const GapType&
        gap () const;

        GapType&
        gap ();

        void
        gap (const GapType& x);

        void
        gap (::std::unique_ptr< GapType > p);

        // autoCalc
        //
        typedef ::prj::srl::spt::Parameter AutoCalcType;
        typedef ::xsd::cxx::tree::traits< AutoCalcType, char > AutoCalcTraits;

        const AutoCalcType&
        autoCalc () const;

        AutoCalcType&
        autoCalc ();

        void
        autoCalc (const AutoCalcType& x);

        void
        autoCalc (::std::unique_ptr< AutoCalcType > p);

        // stripHeight
        //
        typedef ::xml_schema::Double StripHeightType;
        typedef ::xsd::cxx::tree::traits< StripHeightType, char, ::xsd::cxx::tree::schema_type::double_ > StripHeightTraits;

        const StripHeightType&
        stripHeight () const;

        StripHeightType&
        stripHeight ();

        void
        stripHeight (const StripHeightType& x);

        // feedDirectionLabel
        //
        typedef ::prj::srl::spt::PLabel FeedDirectionLabelType;
        typedef ::xsd::cxx::tree::traits< FeedDirectionLabelType, char > FeedDirectionLabelTraits;

        const FeedDirectionLabelType&
        feedDirectionLabel () const;

        FeedDirectionLabelType&
        feedDirectionLabel ();

        void
        feedDirectionLabel (const FeedDirectionLabelType& x);

        void
        feedDirectionLabel (::std::unique_ptr< FeedDirectionLabelType > p);

        // pitchLabel
        //
        typedef ::prj::srl::spt::PLabel PitchLabelType;
        typedef ::xsd::cxx::tree::traits< PitchLabelType, char > PitchLabelTraits;

        const PitchLabelType&
        pitchLabel () const;

        PitchLabelType&
        pitchLabel ();

        void
        pitchLabel (const PitchLabelType& x);

        void
        pitchLabel (::std::unique_ptr< PitchLabelType > p);

        // widthLabel
        //
        typedef ::prj::srl::spt::PLabel WidthLabelType;
        typedef ::xsd::cxx::tree::traits< WidthLabelType, char > WidthLabelTraits;

        const WidthLabelType&
        widthLabel () const;

        WidthLabelType&
        widthLabel ();

        void
        widthLabel (const WidthLabelType& x);

        void
        widthLabel (::std::unique_ptr< WidthLabelType > p);

        // widthOffsetLabel
        //
        typedef ::prj::srl::spt::PLabel WidthOffsetLabelType;
        typedef ::xsd::cxx::tree::traits< WidthOffsetLabelType, char > WidthOffsetLabelTraits;

        const WidthOffsetLabelType&
        widthOffsetLabel () const;

        WidthOffsetLabelType&
        widthOffsetLabel ();

        void
        widthOffsetLabel (const WidthOffsetLabelType& x);

        void
        widthOffsetLabel (::std::unique_ptr< WidthOffsetLabelType > p);

        // gapLabel
        //
        typedef ::prj::srl::spt::PLabel GapLabelType;
        typedef ::xsd::cxx::tree::traits< GapLabelType, char > GapLabelTraits;

        const GapLabelType&
        gapLabel () const;

        GapLabelType&
        gapLabel ();

        void
        gapLabel (const GapLabelType& x);

        void
        gapLabel (::std::unique_ptr< GapLabelType > p);

        // autoCalcLabel
        //
        typedef ::prj::srl::spt::PLabel AutoCalcLabelType;
        typedef ::xsd::cxx::tree::traits< AutoCalcLabelType, char > AutoCalcLabelTraits;

        const AutoCalcLabelType&
        autoCalcLabel () const;

        AutoCalcLabelType&
        autoCalcLabel ();

        void
        autoCalcLabel (const AutoCalcLabelType& x);

        void
        autoCalcLabel (::std::unique_ptr< AutoCalcLabelType > p);

        // stations
        //
        typedef ::prj::srl::stps::Station StationsType;
        typedef ::xsd::cxx::tree::sequence< StationsType > StationsSequence;
        typedef StationsSequence::iterator StationsIterator;
        typedef StationsSequence::const_iterator StationsConstIterator;
        typedef ::xsd::cxx::tree::traits< StationsType, char > StationsTraits;

        const StationsSequence&
        stations () const;

        StationsSequence&
        stations ();

        void
        stations (const StationsSequence& s);

        // Constructors.
        //
        Strip (const BaseType&,
               const SeerShapeType&,
               const FeedDirectionType&,
               const PitchType&,
               const WidthType&,
               const WidthOffsetType&,
               const GapType&,
               const AutoCalcType&,
               const StripHeightType&,
               const FeedDirectionLabelType&,
               const PitchLabelType&,
               const WidthLabelType&,
               const WidthOffsetLabelType&,
               const GapLabelType&,
               const AutoCalcLabelType&);

        Strip (::std::unique_ptr< BaseType >,
               ::std::unique_ptr< SeerShapeType >,
               ::std::unique_ptr< FeedDirectionType >,
               ::std::unique_ptr< PitchType >,
               ::std::unique_ptr< WidthType >,
               ::std::unique_ptr< WidthOffsetType >,
               ::std::unique_ptr< GapType >,
               ::std::unique_ptr< AutoCalcType >,
               const StripHeightType&,
               ::std::unique_ptr< FeedDirectionLabelType >,
               ::std::unique_ptr< PitchLabelType >,
               ::std::unique_ptr< WidthLabelType >,
               ::std::unique_ptr< WidthOffsetLabelType >,
               ::std::unique_ptr< GapLabelType >,
               ::std::unique_ptr< AutoCalcLabelType >);

        Strip (const ::xercesc::DOMElement& e,
               ::xml_schema::Flags f = 0,
               ::xml_schema::Container* c = 0);

        Strip (const Strip& x,
               ::xml_schema::Flags f = 0,
               ::xml_schema::Container* c = 0);

        virtual Strip*
        _clone (::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0) const;

        Strip&
        operator= (const Strip& x);

        virtual 
        ~Strip ();

        // Implementation.
        //
        protected:
        void
        parse (::xsd::cxx::xml::dom::parser< char >&,
               ::xml_schema::Flags);

        protected:
        ::xsd::cxx::tree::one< BaseType > base_;
        ::xsd::cxx::tree::one< SeerShapeType > seerShape_;
        ::xsd::cxx::tree::one< FeedDirectionType > feedDirection_;
        ::xsd::cxx::tree::one< PitchType > pitch_;
        ::xsd::cxx::tree::one< WidthType > width_;
        ::xsd::cxx::tree::one< WidthOffsetType > widthOffset_;
        ::xsd::cxx::tree::one< GapType > gap_;
        ::xsd::cxx::tree::one< AutoCalcType > autoCalc_;
        ::xsd::cxx::tree::one< StripHeightType > stripHeight_;
        ::xsd::cxx::tree::one< FeedDirectionLabelType > feedDirectionLabel_;
        ::xsd::cxx::tree::one< PitchLabelType > pitchLabel_;
        ::xsd::cxx::tree::one< WidthLabelType > widthLabel_;
        ::xsd::cxx::tree::one< WidthOffsetLabelType > widthOffsetLabel_;
        ::xsd::cxx::tree::one< GapLabelType > gapLabel_;
        ::xsd::cxx::tree::one< AutoCalcLabelType > autoCalcLabel_;
        StationsSequence stations_;
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
    namespace stps
    {
      // Parse a URI or a local file.
      //

      ::std::unique_ptr< ::prj::srl::stps::Strip >
      strip (const ::std::string& uri,
             ::xml_schema::Flags f = 0,
             const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::stps::Strip >
      strip (const ::std::string& uri,
             ::xml_schema::ErrorHandler& eh,
             ::xml_schema::Flags f = 0,
             const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::stps::Strip >
      strip (const ::std::string& uri,
             ::xercesc::DOMErrorHandler& eh,
             ::xml_schema::Flags f = 0,
             const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      // Parse std::istream.
      //

      ::std::unique_ptr< ::prj::srl::stps::Strip >
      strip (::std::istream& is,
             ::xml_schema::Flags f = 0,
             const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::stps::Strip >
      strip (::std::istream& is,
             ::xml_schema::ErrorHandler& eh,
             ::xml_schema::Flags f = 0,
             const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::stps::Strip >
      strip (::std::istream& is,
             ::xercesc::DOMErrorHandler& eh,
             ::xml_schema::Flags f = 0,
             const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::stps::Strip >
      strip (::std::istream& is,
             const ::std::string& id,
             ::xml_schema::Flags f = 0,
             const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::stps::Strip >
      strip (::std::istream& is,
             const ::std::string& id,
             ::xml_schema::ErrorHandler& eh,
             ::xml_schema::Flags f = 0,
             const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::stps::Strip >
      strip (::std::istream& is,
             const ::std::string& id,
             ::xercesc::DOMErrorHandler& eh,
             ::xml_schema::Flags f = 0,
             const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      // Parse xercesc::InputSource.
      //

      ::std::unique_ptr< ::prj::srl::stps::Strip >
      strip (::xercesc::InputSource& is,
             ::xml_schema::Flags f = 0,
             const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::stps::Strip >
      strip (::xercesc::InputSource& is,
             ::xml_schema::ErrorHandler& eh,
             ::xml_schema::Flags f = 0,
             const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::stps::Strip >
      strip (::xercesc::InputSource& is,
             ::xercesc::DOMErrorHandler& eh,
             ::xml_schema::Flags f = 0,
             const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      // Parse xercesc::DOMDocument.
      //

      ::std::unique_ptr< ::prj::srl::stps::Strip >
      strip (const ::xercesc::DOMDocument& d,
             ::xml_schema::Flags f = 0,
             const ::xml_schema::Properties& p = ::xml_schema::Properties ());

      ::std::unique_ptr< ::prj::srl::stps::Strip >
      strip (::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d,
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
    namespace stps
    {
      void
      operator<< (::xercesc::DOMElement&, const Station&);

      void
      operator<< (::xercesc::DOMElement&, const Strip&);

      // Serialize to std::ostream.
      //

      void
      strip (::std::ostream& os,
             const ::prj::srl::stps::Strip& x, 
             const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
             const ::std::string& e = "UTF-8",
             ::xml_schema::Flags f = 0);

      void
      strip (::std::ostream& os,
             const ::prj::srl::stps::Strip& x, 
             ::xml_schema::ErrorHandler& eh,
             const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
             const ::std::string& e = "UTF-8",
             ::xml_schema::Flags f = 0);

      void
      strip (::std::ostream& os,
             const ::prj::srl::stps::Strip& x, 
             ::xercesc::DOMErrorHandler& eh,
             const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
             const ::std::string& e = "UTF-8",
             ::xml_schema::Flags f = 0);

      // Serialize to xercesc::XMLFormatTarget.
      //

      void
      strip (::xercesc::XMLFormatTarget& ft,
             const ::prj::srl::stps::Strip& x, 
             const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
             const ::std::string& e = "UTF-8",
             ::xml_schema::Flags f = 0);

      void
      strip (::xercesc::XMLFormatTarget& ft,
             const ::prj::srl::stps::Strip& x, 
             ::xml_schema::ErrorHandler& eh,
             const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
             const ::std::string& e = "UTF-8",
             ::xml_schema::Flags f = 0);

      void
      strip (::xercesc::XMLFormatTarget& ft,
             const ::prj::srl::stps::Strip& x, 
             ::xercesc::DOMErrorHandler& eh,
             const ::xml_schema::NamespaceInfomap& m = ::xml_schema::NamespaceInfomap (),
             const ::std::string& e = "UTF-8",
             ::xml_schema::Flags f = 0);

      // Serialize to an existing xercesc::DOMDocument.
      //

      void
      strip (::xercesc::DOMDocument& d,
             const ::prj::srl::stps::Strip& x,
             ::xml_schema::Flags f = 0);

      // Serialize to a new xercesc::DOMDocument.
      //

      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument >
      strip (const ::prj::srl::stps::Strip& x, 
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

#endif // PRJ_SRL_STPS_PRJSRLSTPSSTRIP_H
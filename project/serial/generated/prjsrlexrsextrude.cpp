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

// Begin prologue.
//
//
// End prologue.

#include <xsd/cxx/pre.hxx>

#include "prjsrlexrsextrude.h"

namespace prj
{
  namespace srl
  {
    namespace exrs
    {
      // Extrude
      // 

      const Extrude::BaseType& Extrude::
      base () const
      {
        return this->base_.get ();
      }

      Extrude::BaseType& Extrude::
      base ()
      {
        return this->base_.get ();
      }

      void Extrude::
      base (const BaseType& x)
      {
        this->base_.set (x);
      }

      void Extrude::
      base (::std::unique_ptr< BaseType > x)
      {
        this->base_.set (std::move (x));
      }

      const Extrude::SeerShapeType& Extrude::
      seerShape () const
      {
        return this->seerShape_.get ();
      }

      Extrude::SeerShapeType& Extrude::
      seerShape ()
      {
        return this->seerShape_.get ();
      }

      void Extrude::
      seerShape (const SeerShapeType& x)
      {
        this->seerShape_.set (x);
      }

      void Extrude::
      seerShape (::std::unique_ptr< SeerShapeType > x)
      {
        this->seerShape_.set (std::move (x));
      }

      const Extrude::PicksSequence& Extrude::
      picks () const
      {
        return this->picks_;
      }

      Extrude::PicksSequence& Extrude::
      picks ()
      {
        return this->picks_;
      }

      void Extrude::
      picks (const PicksSequence& s)
      {
        this->picks_ = s;
      }

      const Extrude::AxisPicksSequence& Extrude::
      axisPicks () const
      {
        return this->axisPicks_;
      }

      Extrude::AxisPicksSequence& Extrude::
      axisPicks ()
      {
        return this->axisPicks_;
      }

      void Extrude::
      axisPicks (const AxisPicksSequence& s)
      {
        this->axisPicks_ = s;
      }

      const Extrude::DirectionType& Extrude::
      direction () const
      {
        return this->direction_.get ();
      }

      Extrude::DirectionType& Extrude::
      direction ()
      {
        return this->direction_.get ();
      }

      void Extrude::
      direction (const DirectionType& x)
      {
        this->direction_.set (x);
      }

      void Extrude::
      direction (::std::unique_ptr< DirectionType > x)
      {
        this->direction_.set (std::move (x));
      }

      const Extrude::DirectionLabelType& Extrude::
      directionLabel () const
      {
        return this->directionLabel_.get ();
      }

      Extrude::DirectionLabelType& Extrude::
      directionLabel ()
      {
        return this->directionLabel_.get ();
      }

      void Extrude::
      directionLabel (const DirectionLabelType& x)
      {
        this->directionLabel_.set (x);
      }

      void Extrude::
      directionLabel (::std::unique_ptr< DirectionLabelType > x)
      {
        this->directionLabel_.set (std::move (x));
      }

      const Extrude::DistanceType& Extrude::
      distance () const
      {
        return this->distance_.get ();
      }

      Extrude::DistanceType& Extrude::
      distance ()
      {
        return this->distance_.get ();
      }

      void Extrude::
      distance (const DistanceType& x)
      {
        this->distance_.set (x);
      }

      void Extrude::
      distance (::std::unique_ptr< DistanceType > x)
      {
        this->distance_.set (std::move (x));
      }

      const Extrude::DistanceLabelType& Extrude::
      distanceLabel () const
      {
        return this->distanceLabel_.get ();
      }

      Extrude::DistanceLabelType& Extrude::
      distanceLabel ()
      {
        return this->distanceLabel_.get ();
      }

      void Extrude::
      distanceLabel (const DistanceLabelType& x)
      {
        this->distanceLabel_.set (x);
      }

      void Extrude::
      distanceLabel (::std::unique_ptr< DistanceLabelType > x)
      {
        this->distanceLabel_.set (std::move (x));
      }

      const Extrude::OffsetType& Extrude::
      offset () const
      {
        return this->offset_.get ();
      }

      Extrude::OffsetType& Extrude::
      offset ()
      {
        return this->offset_.get ();
      }

      void Extrude::
      offset (const OffsetType& x)
      {
        this->offset_.set (x);
      }

      void Extrude::
      offset (::std::unique_ptr< OffsetType > x)
      {
        this->offset_.set (std::move (x));
      }

      const Extrude::OffsetLabelType& Extrude::
      offsetLabel () const
      {
        return this->offsetLabel_.get ();
      }

      Extrude::OffsetLabelType& Extrude::
      offsetLabel ()
      {
        return this->offsetLabel_.get ();
      }

      void Extrude::
      offsetLabel (const OffsetLabelType& x)
      {
        this->offsetLabel_.set (x);
      }

      void Extrude::
      offsetLabel (::std::unique_ptr< OffsetLabelType > x)
      {
        this->offsetLabel_.set (std::move (x));
      }

      const Extrude::DirectionTypeType& Extrude::
      directionType () const
      {
        return this->directionType_.get ();
      }

      Extrude::DirectionTypeType& Extrude::
      directionType ()
      {
        return this->directionType_.get ();
      }

      void Extrude::
      directionType (const DirectionTypeType& x)
      {
        this->directionType_.set (x);
      }

      const Extrude::OriginalMapSequence& Extrude::
      originalMap () const
      {
        return this->originalMap_;
      }

      Extrude::OriginalMapSequence& Extrude::
      originalMap ()
      {
        return this->originalMap_;
      }

      void Extrude::
      originalMap (const OriginalMapSequence& s)
      {
        this->originalMap_ = s;
      }

      const Extrude::GeneratedMapSequence& Extrude::
      generatedMap () const
      {
        return this->generatedMap_;
      }

      Extrude::GeneratedMapSequence& Extrude::
      generatedMap ()
      {
        return this->generatedMap_;
      }

      void Extrude::
      generatedMap (const GeneratedMapSequence& s)
      {
        this->generatedMap_ = s;
      }

      const Extrude::LastMapSequence& Extrude::
      lastMap () const
      {
        return this->lastMap_;
      }

      Extrude::LastMapSequence& Extrude::
      lastMap ()
      {
        return this->lastMap_;
      }

      void Extrude::
      lastMap (const LastMapSequence& s)
      {
        this->lastMap_ = s;
      }

      const Extrude::OWireMapSequence& Extrude::
      oWireMap () const
      {
        return this->oWireMap_;
      }

      Extrude::OWireMapSequence& Extrude::
      oWireMap ()
      {
        return this->oWireMap_;
      }

      void Extrude::
      oWireMap (const OWireMapSequence& s)
      {
        this->oWireMap_ = s;
      }
    }
  }
}

#include <xsd/cxx/xml/dom/parsing-source.hxx>

namespace prj
{
  namespace srl
  {
    namespace exrs
    {
      // Extrude
      //

      Extrude::
      Extrude (const BaseType& base,
               const SeerShapeType& seerShape,
               const DirectionType& direction,
               const DirectionLabelType& directionLabel,
               const DistanceType& distance,
               const DistanceLabelType& distanceLabel,
               const OffsetType& offset,
               const OffsetLabelType& offsetLabel,
               const DirectionTypeType& directionType)
      : ::xml_schema::Type (),
        base_ (base, this),
        seerShape_ (seerShape, this),
        picks_ (this),
        axisPicks_ (this),
        direction_ (direction, this),
        directionLabel_ (directionLabel, this),
        distance_ (distance, this),
        distanceLabel_ (distanceLabel, this),
        offset_ (offset, this),
        offsetLabel_ (offsetLabel, this),
        directionType_ (directionType, this),
        originalMap_ (this),
        generatedMap_ (this),
        lastMap_ (this),
        oWireMap_ (this)
      {
      }

      Extrude::
      Extrude (::std::unique_ptr< BaseType > base,
               ::std::unique_ptr< SeerShapeType > seerShape,
               ::std::unique_ptr< DirectionType > direction,
               ::std::unique_ptr< DirectionLabelType > directionLabel,
               ::std::unique_ptr< DistanceType > distance,
               ::std::unique_ptr< DistanceLabelType > distanceLabel,
               ::std::unique_ptr< OffsetType > offset,
               ::std::unique_ptr< OffsetLabelType > offsetLabel,
               const DirectionTypeType& directionType)
      : ::xml_schema::Type (),
        base_ (std::move (base), this),
        seerShape_ (std::move (seerShape), this),
        picks_ (this),
        axisPicks_ (this),
        direction_ (std::move (direction), this),
        directionLabel_ (std::move (directionLabel), this),
        distance_ (std::move (distance), this),
        distanceLabel_ (std::move (distanceLabel), this),
        offset_ (std::move (offset), this),
        offsetLabel_ (std::move (offsetLabel), this),
        directionType_ (directionType, this),
        originalMap_ (this),
        generatedMap_ (this),
        lastMap_ (this),
        oWireMap_ (this)
      {
      }

      Extrude::
      Extrude (const Extrude& x,
               ::xml_schema::Flags f,
               ::xml_schema::Container* c)
      : ::xml_schema::Type (x, f, c),
        base_ (x.base_, f, this),
        seerShape_ (x.seerShape_, f, this),
        picks_ (x.picks_, f, this),
        axisPicks_ (x.axisPicks_, f, this),
        direction_ (x.direction_, f, this),
        directionLabel_ (x.directionLabel_, f, this),
        distance_ (x.distance_, f, this),
        distanceLabel_ (x.distanceLabel_, f, this),
        offset_ (x.offset_, f, this),
        offsetLabel_ (x.offsetLabel_, f, this),
        directionType_ (x.directionType_, f, this),
        originalMap_ (x.originalMap_, f, this),
        generatedMap_ (x.generatedMap_, f, this),
        lastMap_ (x.lastMap_, f, this),
        oWireMap_ (x.oWireMap_, f, this)
      {
      }

      Extrude::
      Extrude (const ::xercesc::DOMElement& e,
               ::xml_schema::Flags f,
               ::xml_schema::Container* c)
      : ::xml_schema::Type (e, f | ::xml_schema::Flags::base, c),
        base_ (this),
        seerShape_ (this),
        picks_ (this),
        axisPicks_ (this),
        direction_ (this),
        directionLabel_ (this),
        distance_ (this),
        distanceLabel_ (this),
        offset_ (this),
        offsetLabel_ (this),
        directionType_ (this),
        originalMap_ (this),
        generatedMap_ (this),
        lastMap_ (this),
        oWireMap_ (this)
      {
        if ((f & ::xml_schema::Flags::base) == 0)
        {
          ::xsd::cxx::xml::dom::parser< char > p (e, true, false, false);
          this->parse (p, f);
        }
      }

      void Extrude::
      parse (::xsd::cxx::xml::dom::parser< char >& p,
             ::xml_schema::Flags f)
      {
        for (; p.more_content (); p.next_content (false))
        {
          const ::xercesc::DOMElement& i (p.cur_element ());
          const ::xsd::cxx::xml::qualified_name< char > n (
            ::xsd::cxx::xml::dom::name< char > (i));

          // base
          //
          if (n.name () == "base" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< BaseType > r (
              BaseTraits::create (i, f, this));

            if (!base_.present ())
            {
              this->base_.set (::std::move (r));
              continue;
            }
          }

          // seerShape
          //
          if (n.name () == "seerShape" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< SeerShapeType > r (
              SeerShapeTraits::create (i, f, this));

            if (!seerShape_.present ())
            {
              this->seerShape_.set (::std::move (r));
              continue;
            }
          }

          // picks
          //
          if (n.name () == "picks" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< PicksType > r (
              PicksTraits::create (i, f, this));

            this->picks_.push_back (::std::move (r));
            continue;
          }

          // axisPicks
          //
          if (n.name () == "axisPicks" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< AxisPicksType > r (
              AxisPicksTraits::create (i, f, this));

            this->axisPicks_.push_back (::std::move (r));
            continue;
          }

          // direction
          //
          if (n.name () == "direction" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< DirectionType > r (
              DirectionTraits::create (i, f, this));

            if (!direction_.present ())
            {
              this->direction_.set (::std::move (r));
              continue;
            }
          }

          // directionLabel
          //
          if (n.name () == "directionLabel" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< DirectionLabelType > r (
              DirectionLabelTraits::create (i, f, this));

            if (!directionLabel_.present ())
            {
              this->directionLabel_.set (::std::move (r));
              continue;
            }
          }

          // distance
          //
          if (n.name () == "distance" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< DistanceType > r (
              DistanceTraits::create (i, f, this));

            if (!distance_.present ())
            {
              this->distance_.set (::std::move (r));
              continue;
            }
          }

          // distanceLabel
          //
          if (n.name () == "distanceLabel" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< DistanceLabelType > r (
              DistanceLabelTraits::create (i, f, this));

            if (!distanceLabel_.present ())
            {
              this->distanceLabel_.set (::std::move (r));
              continue;
            }
          }

          // offset
          //
          if (n.name () == "offset" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< OffsetType > r (
              OffsetTraits::create (i, f, this));

            if (!offset_.present ())
            {
              this->offset_.set (::std::move (r));
              continue;
            }
          }

          // offsetLabel
          //
          if (n.name () == "offsetLabel" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< OffsetLabelType > r (
              OffsetLabelTraits::create (i, f, this));

            if (!offsetLabel_.present ())
            {
              this->offsetLabel_.set (::std::move (r));
              continue;
            }
          }

          // directionType
          //
          if (n.name () == "directionType" && n.namespace_ ().empty ())
          {
            if (!directionType_.present ())
            {
              this->directionType_.set (DirectionTypeTraits::create (i, f, this));
              continue;
            }
          }

          // originalMap
          //
          if (n.name () == "originalMap" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< OriginalMapType > r (
              OriginalMapTraits::create (i, f, this));

            this->originalMap_.push_back (::std::move (r));
            continue;
          }

          // generatedMap
          //
          if (n.name () == "generatedMap" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< GeneratedMapType > r (
              GeneratedMapTraits::create (i, f, this));

            this->generatedMap_.push_back (::std::move (r));
            continue;
          }

          // lastMap
          //
          if (n.name () == "lastMap" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< LastMapType > r (
              LastMapTraits::create (i, f, this));

            this->lastMap_.push_back (::std::move (r));
            continue;
          }

          // oWireMap
          //
          if (n.name () == "oWireMap" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< OWireMapType > r (
              OWireMapTraits::create (i, f, this));

            this->oWireMap_.push_back (::std::move (r));
            continue;
          }

          break;
        }

        if (!base_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "base",
            "");
        }

        if (!seerShape_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "seerShape",
            "");
        }

        if (!direction_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "direction",
            "");
        }

        if (!directionLabel_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "directionLabel",
            "");
        }

        if (!distance_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "distance",
            "");
        }

        if (!distanceLabel_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "distanceLabel",
            "");
        }

        if (!offset_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "offset",
            "");
        }

        if (!offsetLabel_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "offsetLabel",
            "");
        }

        if (!directionType_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "directionType",
            "");
        }
      }

      Extrude* Extrude::
      _clone (::xml_schema::Flags f,
              ::xml_schema::Container* c) const
      {
        return new class Extrude (*this, f, c);
      }

      Extrude& Extrude::
      operator= (const Extrude& x)
      {
        if (this != &x)
        {
          static_cast< ::xml_schema::Type& > (*this) = x;
          this->base_ = x.base_;
          this->seerShape_ = x.seerShape_;
          this->picks_ = x.picks_;
          this->axisPicks_ = x.axisPicks_;
          this->direction_ = x.direction_;
          this->directionLabel_ = x.directionLabel_;
          this->distance_ = x.distance_;
          this->distanceLabel_ = x.distanceLabel_;
          this->offset_ = x.offset_;
          this->offsetLabel_ = x.offsetLabel_;
          this->directionType_ = x.directionType_;
          this->originalMap_ = x.originalMap_;
          this->generatedMap_ = x.generatedMap_;
          this->lastMap_ = x.lastMap_;
          this->oWireMap_ = x.oWireMap_;
        }

        return *this;
      }

      Extrude::
      ~Extrude ()
      {
      }
    }
  }
}

#include <istream>
#include <xsd/cxx/xml/sax/std-input-source.hxx>
#include <xsd/cxx/tree/error-handler.hxx>

namespace prj
{
  namespace srl
  {
    namespace exrs
    {
      ::std::unique_ptr< ::prj::srl::exrs::Extrude >
      extrude (const ::std::string& u,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::tree::error_handler< char > h;

        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            u, h, p, f));

        h.throw_if_failed< ::xsd::cxx::tree::parsing< char > > ();

        return ::std::unique_ptr< ::prj::srl::exrs::Extrude > (
          ::prj::srl::exrs::extrude (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::exrs::Extrude >
      extrude (const ::std::string& u,
               ::xml_schema::ErrorHandler& h,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            u, h, p, f));

        if (!d.get ())
          throw ::xsd::cxx::tree::parsing< char > ();

        return ::std::unique_ptr< ::prj::srl::exrs::Extrude > (
          ::prj::srl::exrs::extrude (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::exrs::Extrude >
      extrude (const ::std::string& u,
               ::xercesc::DOMErrorHandler& h,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            u, h, p, f));

        if (!d.get ())
          throw ::xsd::cxx::tree::parsing< char > ();

        return ::std::unique_ptr< ::prj::srl::exrs::Extrude > (
          ::prj::srl::exrs::extrude (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::exrs::Extrude >
      extrude (::std::istream& is,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is);
        return ::prj::srl::exrs::extrude (isrc, f, p);
      }

      ::std::unique_ptr< ::prj::srl::exrs::Extrude >
      extrude (::std::istream& is,
               ::xml_schema::ErrorHandler& h,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is);
        return ::prj::srl::exrs::extrude (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::exrs::Extrude >
      extrude (::std::istream& is,
               ::xercesc::DOMErrorHandler& h,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::sax::std_input_source isrc (is);
        return ::prj::srl::exrs::extrude (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::exrs::Extrude >
      extrude (::std::istream& is,
               const ::std::string& sid,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
        return ::prj::srl::exrs::extrude (isrc, f, p);
      }

      ::std::unique_ptr< ::prj::srl::exrs::Extrude >
      extrude (::std::istream& is,
               const ::std::string& sid,
               ::xml_schema::ErrorHandler& h,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
        return ::prj::srl::exrs::extrude (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::exrs::Extrude >
      extrude (::std::istream& is,
               const ::std::string& sid,
               ::xercesc::DOMErrorHandler& h,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
        return ::prj::srl::exrs::extrude (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::exrs::Extrude >
      extrude (::xercesc::InputSource& i,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::tree::error_handler< char > h;

        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            i, h, p, f));

        h.throw_if_failed< ::xsd::cxx::tree::parsing< char > > ();

        return ::std::unique_ptr< ::prj::srl::exrs::Extrude > (
          ::prj::srl::exrs::extrude (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::exrs::Extrude >
      extrude (::xercesc::InputSource& i,
               ::xml_schema::ErrorHandler& h,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            i, h, p, f));

        if (!d.get ())
          throw ::xsd::cxx::tree::parsing< char > ();

        return ::std::unique_ptr< ::prj::srl::exrs::Extrude > (
          ::prj::srl::exrs::extrude (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::exrs::Extrude >
      extrude (::xercesc::InputSource& i,
               ::xercesc::DOMErrorHandler& h,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            i, h, p, f));

        if (!d.get ())
          throw ::xsd::cxx::tree::parsing< char > ();

        return ::std::unique_ptr< ::prj::srl::exrs::Extrude > (
          ::prj::srl::exrs::extrude (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::exrs::Extrude >
      extrude (const ::xercesc::DOMDocument& doc,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        if (f & ::xml_schema::Flags::keep_dom)
        {
          ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
            static_cast< ::xercesc::DOMDocument* > (doc.cloneNode (true)));

          return ::std::unique_ptr< ::prj::srl::exrs::Extrude > (
            ::prj::srl::exrs::extrude (
              std::move (d), f | ::xml_schema::Flags::own_dom, p));
        }

        const ::xercesc::DOMElement& e (*doc.getDocumentElement ());
        const ::xsd::cxx::xml::qualified_name< char > n (
          ::xsd::cxx::xml::dom::name< char > (e));

        if (n.name () == "extrude" &&
            n.namespace_ () == "http://www.cadseer.com/prj/srl/exrs")
        {
          ::std::unique_ptr< ::prj::srl::exrs::Extrude > r (
            ::xsd::cxx::tree::traits< ::prj::srl::exrs::Extrude, char >::create (
              e, f, 0));
          return r;
        }

        throw ::xsd::cxx::tree::unexpected_element < char > (
          n.name (),
          n.namespace_ (),
          "extrude",
          "http://www.cadseer.com/prj/srl/exrs");
      }

      ::std::unique_ptr< ::prj::srl::exrs::Extrude >
      extrude (::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties&)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > c (
          ((f & ::xml_schema::Flags::keep_dom) &&
           !(f & ::xml_schema::Flags::own_dom))
          ? static_cast< ::xercesc::DOMDocument* > (d->cloneNode (true))
          : 0);

        ::xercesc::DOMDocument& doc (c.get () ? *c : *d);
        const ::xercesc::DOMElement& e (*doc.getDocumentElement ());

        const ::xsd::cxx::xml::qualified_name< char > n (
          ::xsd::cxx::xml::dom::name< char > (e));

        if (f & ::xml_schema::Flags::keep_dom)
          doc.setUserData (::xml_schema::dom::tree_node_key,
                           (c.get () ? &c : &d),
                           0);

        if (n.name () == "extrude" &&
            n.namespace_ () == "http://www.cadseer.com/prj/srl/exrs")
        {
          ::std::unique_ptr< ::prj::srl::exrs::Extrude > r (
            ::xsd::cxx::tree::traits< ::prj::srl::exrs::Extrude, char >::create (
              e, f, 0));
          return r;
        }

        throw ::xsd::cxx::tree::unexpected_element < char > (
          n.name (),
          n.namespace_ (),
          "extrude",
          "http://www.cadseer.com/prj/srl/exrs");
      }
    }
  }
}

#include <ostream>
#include <xsd/cxx/tree/error-handler.hxx>
#include <xsd/cxx/xml/dom/serialization-source.hxx>

namespace prj
{
  namespace srl
  {
    namespace exrs
    {
      void
      operator<< (::xercesc::DOMElement& e, const Extrude& i)
      {
        e << static_cast< const ::xml_schema::Type& > (i);

        // base
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "base",
              e));

          s << i.base ();
        }

        // seerShape
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "seerShape",
              e));

          s << i.seerShape ();
        }

        // picks
        //
        for (Extrude::PicksConstIterator
             b (i.picks ().begin ()), n (i.picks ().end ());
             b != n; ++b)
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "picks",
              e));

          s << *b;
        }

        // axisPicks
        //
        for (Extrude::AxisPicksConstIterator
             b (i.axisPicks ().begin ()), n (i.axisPicks ().end ());
             b != n; ++b)
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "axisPicks",
              e));

          s << *b;
        }

        // direction
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "direction",
              e));

          s << i.direction ();
        }

        // directionLabel
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "directionLabel",
              e));

          s << i.directionLabel ();
        }

        // distance
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "distance",
              e));

          s << i.distance ();
        }

        // distanceLabel
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "distanceLabel",
              e));

          s << i.distanceLabel ();
        }

        // offset
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "offset",
              e));

          s << i.offset ();
        }

        // offsetLabel
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "offsetLabel",
              e));

          s << i.offsetLabel ();
        }

        // directionType
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "directionType",
              e));

          s << i.directionType ();
        }

        // originalMap
        //
        for (Extrude::OriginalMapConstIterator
             b (i.originalMap ().begin ()), n (i.originalMap ().end ());
             b != n; ++b)
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "originalMap",
              e));

          s << *b;
        }

        // generatedMap
        //
        for (Extrude::GeneratedMapConstIterator
             b (i.generatedMap ().begin ()), n (i.generatedMap ().end ());
             b != n; ++b)
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "generatedMap",
              e));

          s << *b;
        }

        // lastMap
        //
        for (Extrude::LastMapConstIterator
             b (i.lastMap ().begin ()), n (i.lastMap ().end ());
             b != n; ++b)
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "lastMap",
              e));

          s << *b;
        }

        // oWireMap
        //
        for (Extrude::OWireMapConstIterator
             b (i.oWireMap ().begin ()), n (i.oWireMap ().end ());
             b != n; ++b)
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "oWireMap",
              e));

          s << *b;
        }
      }

      void
      extrude (::std::ostream& o,
               const ::prj::srl::exrs::Extrude& s,
               const ::xml_schema::NamespaceInfomap& m,
               const ::std::string& e,
               ::xml_schema::Flags f)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0);

        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::exrs::extrude (s, m, f));

        ::xsd::cxx::tree::error_handler< char > h;

        ::xsd::cxx::xml::dom::ostream_format_target t (o);
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          h.throw_if_failed< ::xsd::cxx::tree::serialization< char > > ();
        }
      }

      void
      extrude (::std::ostream& o,
               const ::prj::srl::exrs::Extrude& s,
               ::xml_schema::ErrorHandler& h,
               const ::xml_schema::NamespaceInfomap& m,
               const ::std::string& e,
               ::xml_schema::Flags f)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0);

        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::exrs::extrude (s, m, f));
        ::xsd::cxx::xml::dom::ostream_format_target t (o);
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      extrude (::std::ostream& o,
               const ::prj::srl::exrs::Extrude& s,
               ::xercesc::DOMErrorHandler& h,
               const ::xml_schema::NamespaceInfomap& m,
               const ::std::string& e,
               ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::exrs::extrude (s, m, f));
        ::xsd::cxx::xml::dom::ostream_format_target t (o);
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      extrude (::xercesc::XMLFormatTarget& t,
               const ::prj::srl::exrs::Extrude& s,
               const ::xml_schema::NamespaceInfomap& m,
               const ::std::string& e,
               ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::exrs::extrude (s, m, f));

        ::xsd::cxx::tree::error_handler< char > h;

        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          h.throw_if_failed< ::xsd::cxx::tree::serialization< char > > ();
        }
      }

      void
      extrude (::xercesc::XMLFormatTarget& t,
               const ::prj::srl::exrs::Extrude& s,
               ::xml_schema::ErrorHandler& h,
               const ::xml_schema::NamespaceInfomap& m,
               const ::std::string& e,
               ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::exrs::extrude (s, m, f));
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      extrude (::xercesc::XMLFormatTarget& t,
               const ::prj::srl::exrs::Extrude& s,
               ::xercesc::DOMErrorHandler& h,
               const ::xml_schema::NamespaceInfomap& m,
               const ::std::string& e,
               ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::exrs::extrude (s, m, f));
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      extrude (::xercesc::DOMDocument& d,
               const ::prj::srl::exrs::Extrude& s,
               ::xml_schema::Flags)
      {
        ::xercesc::DOMElement& e (*d.getDocumentElement ());
        const ::xsd::cxx::xml::qualified_name< char > n (
          ::xsd::cxx::xml::dom::name< char > (e));

        if (n.name () == "extrude" &&
            n.namespace_ () == "http://www.cadseer.com/prj/srl/exrs")
        {
          e << s;
        }
        else
        {
          throw ::xsd::cxx::tree::unexpected_element < char > (
            n.name (),
            n.namespace_ (),
            "extrude",
            "http://www.cadseer.com/prj/srl/exrs");
        }
      }

      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument >
      extrude (const ::prj::srl::exrs::Extrude& s,
               const ::xml_schema::NamespaceInfomap& m,
               ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::serialize< char > (
            "extrude",
            "http://www.cadseer.com/prj/srl/exrs",
            m, f));

        ::prj::srl::exrs::extrude (*d, s, f);
        return d;
      }
    }
  }
}

#include <xsd/cxx/post.hxx>

// Begin epilogue.
//
//
// End epilogue.


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

#include "prjsrlthksthicken.h"

namespace prj
{
  namespace srl
  {
    namespace thks
    {
      // Thicken
      // 

      const Thicken::BaseType& Thicken::
      base () const
      {
        return this->base_.get ();
      }

      Thicken::BaseType& Thicken::
      base ()
      {
        return this->base_.get ();
      }

      void Thicken::
      base (const BaseType& x)
      {
        this->base_.set (x);
      }

      void Thicken::
      base (::std::unique_ptr< BaseType > x)
      {
        this->base_.set (std::move (x));
      }

      const Thicken::PicksType& Thicken::
      picks () const
      {
        return this->picks_.get ();
      }

      Thicken::PicksType& Thicken::
      picks ()
      {
        return this->picks_.get ();
      }

      void Thicken::
      picks (const PicksType& x)
      {
        this->picks_.set (x);
      }

      void Thicken::
      picks (::std::unique_ptr< PicksType > x)
      {
        this->picks_.set (std::move (x));
      }

      const Thicken::DistanceType& Thicken::
      distance () const
      {
        return this->distance_.get ();
      }

      Thicken::DistanceType& Thicken::
      distance ()
      {
        return this->distance_.get ();
      }

      void Thicken::
      distance (const DistanceType& x)
      {
        this->distance_.set (x);
      }

      void Thicken::
      distance (::std::unique_ptr< DistanceType > x)
      {
        this->distance_.set (std::move (x));
      }

      const Thicken::SeerShapeType& Thicken::
      seerShape () const
      {
        return this->seerShape_.get ();
      }

      Thicken::SeerShapeType& Thicken::
      seerShape ()
      {
        return this->seerShape_.get ();
      }

      void Thicken::
      seerShape (const SeerShapeType& x)
      {
        this->seerShape_.set (x);
      }

      void Thicken::
      seerShape (::std::unique_ptr< SeerShapeType > x)
      {
        this->seerShape_.set (std::move (x));
      }

      const Thicken::DistanceLabelType& Thicken::
      distanceLabel () const
      {
        return this->distanceLabel_.get ();
      }

      Thicken::DistanceLabelType& Thicken::
      distanceLabel ()
      {
        return this->distanceLabel_.get ();
      }

      void Thicken::
      distanceLabel (const DistanceLabelType& x)
      {
        this->distanceLabel_.set (x);
      }

      void Thicken::
      distanceLabel (::std::unique_ptr< DistanceLabelType > x)
      {
        this->distanceLabel_.set (std::move (x));
      }

      const Thicken::SolidIdType& Thicken::
      solidId () const
      {
        return this->solidId_.get ();
      }

      Thicken::SolidIdType& Thicken::
      solidId ()
      {
        return this->solidId_.get ();
      }

      void Thicken::
      solidId (const SolidIdType& x)
      {
        this->solidId_.set (x);
      }

      void Thicken::
      solidId (::std::unique_ptr< SolidIdType > x)
      {
        this->solidId_.set (std::move (x));
      }

      const Thicken::SolidIdType& Thicken::
      solidId_default_value ()
      {
        return solidId_default_value_;
      }

      const Thicken::ShellIdType& Thicken::
      shellId () const
      {
        return this->shellId_.get ();
      }

      Thicken::ShellIdType& Thicken::
      shellId ()
      {
        return this->shellId_.get ();
      }

      void Thicken::
      shellId (const ShellIdType& x)
      {
        this->shellId_.set (x);
      }

      void Thicken::
      shellId (::std::unique_ptr< ShellIdType > x)
      {
        this->shellId_.set (std::move (x));
      }

      const Thicken::ShellIdType& Thicken::
      shellId_default_value ()
      {
        return shellId_default_value_;
      }

      const Thicken::FaceMapSequence& Thicken::
      faceMap () const
      {
        return this->faceMap_;
      }

      Thicken::FaceMapSequence& Thicken::
      faceMap ()
      {
        return this->faceMap_;
      }

      void Thicken::
      faceMap (const FaceMapSequence& s)
      {
        this->faceMap_ = s;
      }

      const Thicken::EdgeMapSequence& Thicken::
      edgeMap () const
      {
        return this->edgeMap_;
      }

      Thicken::EdgeMapSequence& Thicken::
      edgeMap ()
      {
        return this->edgeMap_;
      }

      void Thicken::
      edgeMap (const EdgeMapSequence& s)
      {
        this->edgeMap_ = s;
      }

      const Thicken::BoundaryMapSequence& Thicken::
      boundaryMap () const
      {
        return this->boundaryMap_;
      }

      Thicken::BoundaryMapSequence& Thicken::
      boundaryMap ()
      {
        return this->boundaryMap_;
      }

      void Thicken::
      boundaryMap (const BoundaryMapSequence& s)
      {
        this->boundaryMap_ = s;
      }

      const Thicken::OWireMapSequence& Thicken::
      oWireMap () const
      {
        return this->oWireMap_;
      }

      Thicken::OWireMapSequence& Thicken::
      oWireMap ()
      {
        return this->oWireMap_;
      }

      void Thicken::
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
    namespace thks
    {
      // Thicken
      //

      const Thicken::SolidIdType Thicken::solidId_default_value_ (
        "00000000-0000-0000-0000-000000000000");

      const Thicken::ShellIdType Thicken::shellId_default_value_ (
        "00000000-0000-0000-0000-000000000000");

      Thicken::
      Thicken (const BaseType& base,
               const PicksType& picks,
               const DistanceType& distance,
               const SeerShapeType& seerShape,
               const DistanceLabelType& distanceLabel,
               const SolidIdType& solidId,
               const ShellIdType& shellId)
      : ::xml_schema::Type (),
        base_ (base, this),
        picks_ (picks, this),
        distance_ (distance, this),
        seerShape_ (seerShape, this),
        distanceLabel_ (distanceLabel, this),
        solidId_ (solidId, this),
        shellId_ (shellId, this),
        faceMap_ (this),
        edgeMap_ (this),
        boundaryMap_ (this),
        oWireMap_ (this)
      {
      }

      Thicken::
      Thicken (::std::unique_ptr< BaseType > base,
               ::std::unique_ptr< PicksType > picks,
               ::std::unique_ptr< DistanceType > distance,
               ::std::unique_ptr< SeerShapeType > seerShape,
               ::std::unique_ptr< DistanceLabelType > distanceLabel,
               const SolidIdType& solidId,
               const ShellIdType& shellId)
      : ::xml_schema::Type (),
        base_ (std::move (base), this),
        picks_ (std::move (picks), this),
        distance_ (std::move (distance), this),
        seerShape_ (std::move (seerShape), this),
        distanceLabel_ (std::move (distanceLabel), this),
        solidId_ (solidId, this),
        shellId_ (shellId, this),
        faceMap_ (this),
        edgeMap_ (this),
        boundaryMap_ (this),
        oWireMap_ (this)
      {
      }

      Thicken::
      Thicken (const Thicken& x,
               ::xml_schema::Flags f,
               ::xml_schema::Container* c)
      : ::xml_schema::Type (x, f, c),
        base_ (x.base_, f, this),
        picks_ (x.picks_, f, this),
        distance_ (x.distance_, f, this),
        seerShape_ (x.seerShape_, f, this),
        distanceLabel_ (x.distanceLabel_, f, this),
        solidId_ (x.solidId_, f, this),
        shellId_ (x.shellId_, f, this),
        faceMap_ (x.faceMap_, f, this),
        edgeMap_ (x.edgeMap_, f, this),
        boundaryMap_ (x.boundaryMap_, f, this),
        oWireMap_ (x.oWireMap_, f, this)
      {
      }

      Thicken::
      Thicken (const ::xercesc::DOMElement& e,
               ::xml_schema::Flags f,
               ::xml_schema::Container* c)
      : ::xml_schema::Type (e, f | ::xml_schema::Flags::base, c),
        base_ (this),
        picks_ (this),
        distance_ (this),
        seerShape_ (this),
        distanceLabel_ (this),
        solidId_ (this),
        shellId_ (this),
        faceMap_ (this),
        edgeMap_ (this),
        boundaryMap_ (this),
        oWireMap_ (this)
      {
        if ((f & ::xml_schema::Flags::base) == 0)
        {
          ::xsd::cxx::xml::dom::parser< char > p (e, true, false, false);
          this->parse (p, f);
        }
      }

      void Thicken::
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

          // picks
          //
          if (n.name () == "picks" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< PicksType > r (
              PicksTraits::create (i, f, this));

            if (!picks_.present ())
            {
              this->picks_.set (::std::move (r));
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

          // solidId
          //
          if (n.name () == "solidId" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< SolidIdType > r (
              SolidIdTraits::create (i, f, this));

            if (!solidId_.present ())
            {
              this->solidId_.set (::std::move (r));
              continue;
            }
          }

          // shellId
          //
          if (n.name () == "shellId" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< ShellIdType > r (
              ShellIdTraits::create (i, f, this));

            if (!shellId_.present ())
            {
              this->shellId_.set (::std::move (r));
              continue;
            }
          }

          // faceMap
          //
          if (n.name () == "faceMap" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< FaceMapType > r (
              FaceMapTraits::create (i, f, this));

            this->faceMap_.push_back (::std::move (r));
            continue;
          }

          // edgeMap
          //
          if (n.name () == "edgeMap" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< EdgeMapType > r (
              EdgeMapTraits::create (i, f, this));

            this->edgeMap_.push_back (::std::move (r));
            continue;
          }

          // boundaryMap
          //
          if (n.name () == "boundaryMap" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< BoundaryMapType > r (
              BoundaryMapTraits::create (i, f, this));

            this->boundaryMap_.push_back (::std::move (r));
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

        if (!picks_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "picks",
            "");
        }

        if (!distance_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "distance",
            "");
        }

        if (!seerShape_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "seerShape",
            "");
        }

        if (!distanceLabel_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "distanceLabel",
            "");
        }

        if (!solidId_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "solidId",
            "");
        }

        if (!shellId_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "shellId",
            "");
        }
      }

      Thicken* Thicken::
      _clone (::xml_schema::Flags f,
              ::xml_schema::Container* c) const
      {
        return new class Thicken (*this, f, c);
      }

      Thicken& Thicken::
      operator= (const Thicken& x)
      {
        if (this != &x)
        {
          static_cast< ::xml_schema::Type& > (*this) = x;
          this->base_ = x.base_;
          this->picks_ = x.picks_;
          this->distance_ = x.distance_;
          this->seerShape_ = x.seerShape_;
          this->distanceLabel_ = x.distanceLabel_;
          this->solidId_ = x.solidId_;
          this->shellId_ = x.shellId_;
          this->faceMap_ = x.faceMap_;
          this->edgeMap_ = x.edgeMap_;
          this->boundaryMap_ = x.boundaryMap_;
          this->oWireMap_ = x.oWireMap_;
        }

        return *this;
      }

      Thicken::
      ~Thicken ()
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
    namespace thks
    {
      ::std::unique_ptr< ::prj::srl::thks::Thicken >
      thicken (const ::std::string& u,
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

        return ::std::unique_ptr< ::prj::srl::thks::Thicken > (
          ::prj::srl::thks::thicken (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::thks::Thicken >
      thicken (const ::std::string& u,
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

        return ::std::unique_ptr< ::prj::srl::thks::Thicken > (
          ::prj::srl::thks::thicken (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::thks::Thicken >
      thicken (const ::std::string& u,
               ::xercesc::DOMErrorHandler& h,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            u, h, p, f));

        if (!d.get ())
          throw ::xsd::cxx::tree::parsing< char > ();

        return ::std::unique_ptr< ::prj::srl::thks::Thicken > (
          ::prj::srl::thks::thicken (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::thks::Thicken >
      thicken (::std::istream& is,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is);
        return ::prj::srl::thks::thicken (isrc, f, p);
      }

      ::std::unique_ptr< ::prj::srl::thks::Thicken >
      thicken (::std::istream& is,
               ::xml_schema::ErrorHandler& h,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is);
        return ::prj::srl::thks::thicken (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::thks::Thicken >
      thicken (::std::istream& is,
               ::xercesc::DOMErrorHandler& h,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::sax::std_input_source isrc (is);
        return ::prj::srl::thks::thicken (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::thks::Thicken >
      thicken (::std::istream& is,
               const ::std::string& sid,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
        return ::prj::srl::thks::thicken (isrc, f, p);
      }

      ::std::unique_ptr< ::prj::srl::thks::Thicken >
      thicken (::std::istream& is,
               const ::std::string& sid,
               ::xml_schema::ErrorHandler& h,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
        return ::prj::srl::thks::thicken (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::thks::Thicken >
      thicken (::std::istream& is,
               const ::std::string& sid,
               ::xercesc::DOMErrorHandler& h,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
        return ::prj::srl::thks::thicken (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::thks::Thicken >
      thicken (::xercesc::InputSource& i,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::tree::error_handler< char > h;

        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            i, h, p, f));

        h.throw_if_failed< ::xsd::cxx::tree::parsing< char > > ();

        return ::std::unique_ptr< ::prj::srl::thks::Thicken > (
          ::prj::srl::thks::thicken (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::thks::Thicken >
      thicken (::xercesc::InputSource& i,
               ::xml_schema::ErrorHandler& h,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            i, h, p, f));

        if (!d.get ())
          throw ::xsd::cxx::tree::parsing< char > ();

        return ::std::unique_ptr< ::prj::srl::thks::Thicken > (
          ::prj::srl::thks::thicken (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::thks::Thicken >
      thicken (::xercesc::InputSource& i,
               ::xercesc::DOMErrorHandler& h,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            i, h, p, f));

        if (!d.get ())
          throw ::xsd::cxx::tree::parsing< char > ();

        return ::std::unique_ptr< ::prj::srl::thks::Thicken > (
          ::prj::srl::thks::thicken (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::thks::Thicken >
      thicken (const ::xercesc::DOMDocument& doc,
               ::xml_schema::Flags f,
               const ::xml_schema::Properties& p)
      {
        if (f & ::xml_schema::Flags::keep_dom)
        {
          ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
            static_cast< ::xercesc::DOMDocument* > (doc.cloneNode (true)));

          return ::std::unique_ptr< ::prj::srl::thks::Thicken > (
            ::prj::srl::thks::thicken (
              std::move (d), f | ::xml_schema::Flags::own_dom, p));
        }

        const ::xercesc::DOMElement& e (*doc.getDocumentElement ());
        const ::xsd::cxx::xml::qualified_name< char > n (
          ::xsd::cxx::xml::dom::name< char > (e));

        if (n.name () == "thicken" &&
            n.namespace_ () == "http://www.cadseer.com/prj/srl/thks")
        {
          ::std::unique_ptr< ::prj::srl::thks::Thicken > r (
            ::xsd::cxx::tree::traits< ::prj::srl::thks::Thicken, char >::create (
              e, f, 0));
          return r;
        }

        throw ::xsd::cxx::tree::unexpected_element < char > (
          n.name (),
          n.namespace_ (),
          "thicken",
          "http://www.cadseer.com/prj/srl/thks");
      }

      ::std::unique_ptr< ::prj::srl::thks::Thicken >
      thicken (::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d,
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

        if (n.name () == "thicken" &&
            n.namespace_ () == "http://www.cadseer.com/prj/srl/thks")
        {
          ::std::unique_ptr< ::prj::srl::thks::Thicken > r (
            ::xsd::cxx::tree::traits< ::prj::srl::thks::Thicken, char >::create (
              e, f, 0));
          return r;
        }

        throw ::xsd::cxx::tree::unexpected_element < char > (
          n.name (),
          n.namespace_ (),
          "thicken",
          "http://www.cadseer.com/prj/srl/thks");
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
    namespace thks
    {
      void
      operator<< (::xercesc::DOMElement& e, const Thicken& i)
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

        // picks
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "picks",
              e));

          s << i.picks ();
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

        // seerShape
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "seerShape",
              e));

          s << i.seerShape ();
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

        // solidId
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "solidId",
              e));

          s << i.solidId ();
        }

        // shellId
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "shellId",
              e));

          s << i.shellId ();
        }

        // faceMap
        //
        for (Thicken::FaceMapConstIterator
             b (i.faceMap ().begin ()), n (i.faceMap ().end ());
             b != n; ++b)
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "faceMap",
              e));

          s << *b;
        }

        // edgeMap
        //
        for (Thicken::EdgeMapConstIterator
             b (i.edgeMap ().begin ()), n (i.edgeMap ().end ());
             b != n; ++b)
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "edgeMap",
              e));

          s << *b;
        }

        // boundaryMap
        //
        for (Thicken::BoundaryMapConstIterator
             b (i.boundaryMap ().begin ()), n (i.boundaryMap ().end ());
             b != n; ++b)
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "boundaryMap",
              e));

          s << *b;
        }

        // oWireMap
        //
        for (Thicken::OWireMapConstIterator
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
      thicken (::std::ostream& o,
               const ::prj::srl::thks::Thicken& s,
               const ::xml_schema::NamespaceInfomap& m,
               const ::std::string& e,
               ::xml_schema::Flags f)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0);

        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::thks::thicken (s, m, f));

        ::xsd::cxx::tree::error_handler< char > h;

        ::xsd::cxx::xml::dom::ostream_format_target t (o);
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          h.throw_if_failed< ::xsd::cxx::tree::serialization< char > > ();
        }
      }

      void
      thicken (::std::ostream& o,
               const ::prj::srl::thks::Thicken& s,
               ::xml_schema::ErrorHandler& h,
               const ::xml_schema::NamespaceInfomap& m,
               const ::std::string& e,
               ::xml_schema::Flags f)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0);

        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::thks::thicken (s, m, f));
        ::xsd::cxx::xml::dom::ostream_format_target t (o);
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      thicken (::std::ostream& o,
               const ::prj::srl::thks::Thicken& s,
               ::xercesc::DOMErrorHandler& h,
               const ::xml_schema::NamespaceInfomap& m,
               const ::std::string& e,
               ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::thks::thicken (s, m, f));
        ::xsd::cxx::xml::dom::ostream_format_target t (o);
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      thicken (::xercesc::XMLFormatTarget& t,
               const ::prj::srl::thks::Thicken& s,
               const ::xml_schema::NamespaceInfomap& m,
               const ::std::string& e,
               ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::thks::thicken (s, m, f));

        ::xsd::cxx::tree::error_handler< char > h;

        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          h.throw_if_failed< ::xsd::cxx::tree::serialization< char > > ();
        }
      }

      void
      thicken (::xercesc::XMLFormatTarget& t,
               const ::prj::srl::thks::Thicken& s,
               ::xml_schema::ErrorHandler& h,
               const ::xml_schema::NamespaceInfomap& m,
               const ::std::string& e,
               ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::thks::thicken (s, m, f));
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      thicken (::xercesc::XMLFormatTarget& t,
               const ::prj::srl::thks::Thicken& s,
               ::xercesc::DOMErrorHandler& h,
               const ::xml_schema::NamespaceInfomap& m,
               const ::std::string& e,
               ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::thks::thicken (s, m, f));
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      thicken (::xercesc::DOMDocument& d,
               const ::prj::srl::thks::Thicken& s,
               ::xml_schema::Flags)
      {
        ::xercesc::DOMElement& e (*d.getDocumentElement ());
        const ::xsd::cxx::xml::qualified_name< char > n (
          ::xsd::cxx::xml::dom::name< char > (e));

        if (n.name () == "thicken" &&
            n.namespace_ () == "http://www.cadseer.com/prj/srl/thks")
        {
          e << s;
        }
        else
        {
          throw ::xsd::cxx::tree::unexpected_element < char > (
            n.name (),
            n.namespace_ (),
            "thicken",
            "http://www.cadseer.com/prj/srl/thks");
        }
      }

      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument >
      thicken (const ::prj::srl::thks::Thicken& s,
               const ::xml_schema::NamespaceInfomap& m,
               ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::serialize< char > (
            "thicken",
            "http://www.cadseer.com/prj/srl/thks",
            m, f));

        ::prj::srl::thks::thicken (*d, s, f);
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


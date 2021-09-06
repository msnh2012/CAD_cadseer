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

#include "prjsrlswssew.h"

namespace prj
{
  namespace srl
  {
    namespace sws
    {
      // Sew
      // 

      const Sew::BaseType& Sew::
      base () const
      {
        return this->base_.get ();
      }

      Sew::BaseType& Sew::
      base ()
      {
        return this->base_.get ();
      }

      void Sew::
      base (const BaseType& x)
      {
        this->base_.set (x);
      }

      void Sew::
      base (::std::unique_ptr< BaseType > x)
      {
        this->base_.set (std::move (x));
      }

      const Sew::PicksType& Sew::
      picks () const
      {
        return this->picks_.get ();
      }

      Sew::PicksType& Sew::
      picks ()
      {
        return this->picks_.get ();
      }

      void Sew::
      picks (const PicksType& x)
      {
        this->picks_.set (x);
      }

      void Sew::
      picks (::std::unique_ptr< PicksType > x)
      {
        this->picks_.set (std::move (x));
      }

      const Sew::SeerShapeType& Sew::
      seerShape () const
      {
        return this->seerShape_.get ();
      }

      Sew::SeerShapeType& Sew::
      seerShape ()
      {
        return this->seerShape_.get ();
      }

      void Sew::
      seerShape (const SeerShapeType& x)
      {
        this->seerShape_.set (x);
      }

      void Sew::
      seerShape (::std::unique_ptr< SeerShapeType > x)
      {
        this->seerShape_.set (std::move (x));
      }

      const Sew::SolidIdType& Sew::
      solidId () const
      {
        return this->solidId_.get ();
      }

      Sew::SolidIdType& Sew::
      solidId ()
      {
        return this->solidId_.get ();
      }

      void Sew::
      solidId (const SolidIdType& x)
      {
        this->solidId_.set (x);
      }

      void Sew::
      solidId (::std::unique_ptr< SolidIdType > x)
      {
        this->solidId_.set (std::move (x));
      }

      const Sew::SolidIdType& Sew::
      solidId_default_value ()
      {
        return solidId_default_value_;
      }

      const Sew::ShellIdType& Sew::
      shellId () const
      {
        return this->shellId_.get ();
      }

      Sew::ShellIdType& Sew::
      shellId ()
      {
        return this->shellId_.get ();
      }

      void Sew::
      shellId (const ShellIdType& x)
      {
        this->shellId_.set (x);
      }

      void Sew::
      shellId (::std::unique_ptr< ShellIdType > x)
      {
        this->shellId_.set (std::move (x));
      }

      const Sew::ShellIdType& Sew::
      shellId_default_value ()
      {
        return shellId_default_value_;
      }
    }
  }
}

#include <xsd/cxx/xml/dom/parsing-source.hxx>

namespace prj
{
  namespace srl
  {
    namespace sws
    {
      // Sew
      //

      const Sew::SolidIdType Sew::solidId_default_value_ (
        "00000000-0000-0000-0000-000000000000");

      const Sew::ShellIdType Sew::shellId_default_value_ (
        "00000000-0000-0000-0000-000000000000");

      Sew::
      Sew (const BaseType& base,
           const PicksType& picks,
           const SeerShapeType& seerShape,
           const SolidIdType& solidId,
           const ShellIdType& shellId)
      : ::xml_schema::Type (),
        base_ (base, this),
        picks_ (picks, this),
        seerShape_ (seerShape, this),
        solidId_ (solidId, this),
        shellId_ (shellId, this)
      {
      }

      Sew::
      Sew (::std::unique_ptr< BaseType > base,
           ::std::unique_ptr< PicksType > picks,
           ::std::unique_ptr< SeerShapeType > seerShape,
           const SolidIdType& solidId,
           const ShellIdType& shellId)
      : ::xml_schema::Type (),
        base_ (std::move (base), this),
        picks_ (std::move (picks), this),
        seerShape_ (std::move (seerShape), this),
        solidId_ (solidId, this),
        shellId_ (shellId, this)
      {
      }

      Sew::
      Sew (const Sew& x,
           ::xml_schema::Flags f,
           ::xml_schema::Container* c)
      : ::xml_schema::Type (x, f, c),
        base_ (x.base_, f, this),
        picks_ (x.picks_, f, this),
        seerShape_ (x.seerShape_, f, this),
        solidId_ (x.solidId_, f, this),
        shellId_ (x.shellId_, f, this)
      {
      }

      Sew::
      Sew (const ::xercesc::DOMElement& e,
           ::xml_schema::Flags f,
           ::xml_schema::Container* c)
      : ::xml_schema::Type (e, f | ::xml_schema::Flags::base, c),
        base_ (this),
        picks_ (this),
        seerShape_ (this),
        solidId_ (this),
        shellId_ (this)
      {
        if ((f & ::xml_schema::Flags::base) == 0)
        {
          ::xsd::cxx::xml::dom::parser< char > p (e, true, false, false);
          this->parse (p, f);
        }
      }

      void Sew::
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

        if (!seerShape_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "seerShape",
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

      Sew* Sew::
      _clone (::xml_schema::Flags f,
              ::xml_schema::Container* c) const
      {
        return new class Sew (*this, f, c);
      }

      Sew& Sew::
      operator= (const Sew& x)
      {
        if (this != &x)
        {
          static_cast< ::xml_schema::Type& > (*this) = x;
          this->base_ = x.base_;
          this->picks_ = x.picks_;
          this->seerShape_ = x.seerShape_;
          this->solidId_ = x.solidId_;
          this->shellId_ = x.shellId_;
        }

        return *this;
      }

      Sew::
      ~Sew ()
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
    namespace sws
    {
      ::std::unique_ptr< ::prj::srl::sws::Sew >
      sew (const ::std::string& u,
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

        return ::std::unique_ptr< ::prj::srl::sws::Sew > (
          ::prj::srl::sws::sew (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::sws::Sew >
      sew (const ::std::string& u,
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

        return ::std::unique_ptr< ::prj::srl::sws::Sew > (
          ::prj::srl::sws::sew (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::sws::Sew >
      sew (const ::std::string& u,
           ::xercesc::DOMErrorHandler& h,
           ::xml_schema::Flags f,
           const ::xml_schema::Properties& p)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            u, h, p, f));

        if (!d.get ())
          throw ::xsd::cxx::tree::parsing< char > ();

        return ::std::unique_ptr< ::prj::srl::sws::Sew > (
          ::prj::srl::sws::sew (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::sws::Sew >
      sew (::std::istream& is,
           ::xml_schema::Flags f,
           const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is);
        return ::prj::srl::sws::sew (isrc, f, p);
      }

      ::std::unique_ptr< ::prj::srl::sws::Sew >
      sew (::std::istream& is,
           ::xml_schema::ErrorHandler& h,
           ::xml_schema::Flags f,
           const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is);
        return ::prj::srl::sws::sew (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::sws::Sew >
      sew (::std::istream& is,
           ::xercesc::DOMErrorHandler& h,
           ::xml_schema::Flags f,
           const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::sax::std_input_source isrc (is);
        return ::prj::srl::sws::sew (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::sws::Sew >
      sew (::std::istream& is,
           const ::std::string& sid,
           ::xml_schema::Flags f,
           const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
        return ::prj::srl::sws::sew (isrc, f, p);
      }

      ::std::unique_ptr< ::prj::srl::sws::Sew >
      sew (::std::istream& is,
           const ::std::string& sid,
           ::xml_schema::ErrorHandler& h,
           ::xml_schema::Flags f,
           const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
        return ::prj::srl::sws::sew (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::sws::Sew >
      sew (::std::istream& is,
           const ::std::string& sid,
           ::xercesc::DOMErrorHandler& h,
           ::xml_schema::Flags f,
           const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
        return ::prj::srl::sws::sew (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::sws::Sew >
      sew (::xercesc::InputSource& i,
           ::xml_schema::Flags f,
           const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::tree::error_handler< char > h;

        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            i, h, p, f));

        h.throw_if_failed< ::xsd::cxx::tree::parsing< char > > ();

        return ::std::unique_ptr< ::prj::srl::sws::Sew > (
          ::prj::srl::sws::sew (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::sws::Sew >
      sew (::xercesc::InputSource& i,
           ::xml_schema::ErrorHandler& h,
           ::xml_schema::Flags f,
           const ::xml_schema::Properties& p)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            i, h, p, f));

        if (!d.get ())
          throw ::xsd::cxx::tree::parsing< char > ();

        return ::std::unique_ptr< ::prj::srl::sws::Sew > (
          ::prj::srl::sws::sew (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::sws::Sew >
      sew (::xercesc::InputSource& i,
           ::xercesc::DOMErrorHandler& h,
           ::xml_schema::Flags f,
           const ::xml_schema::Properties& p)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            i, h, p, f));

        if (!d.get ())
          throw ::xsd::cxx::tree::parsing< char > ();

        return ::std::unique_ptr< ::prj::srl::sws::Sew > (
          ::prj::srl::sws::sew (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::sws::Sew >
      sew (const ::xercesc::DOMDocument& doc,
           ::xml_schema::Flags f,
           const ::xml_schema::Properties& p)
      {
        if (f & ::xml_schema::Flags::keep_dom)
        {
          ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
            static_cast< ::xercesc::DOMDocument* > (doc.cloneNode (true)));

          return ::std::unique_ptr< ::prj::srl::sws::Sew > (
            ::prj::srl::sws::sew (
              std::move (d), f | ::xml_schema::Flags::own_dom, p));
        }

        const ::xercesc::DOMElement& e (*doc.getDocumentElement ());
        const ::xsd::cxx::xml::qualified_name< char > n (
          ::xsd::cxx::xml::dom::name< char > (e));

        if (n.name () == "sew" &&
            n.namespace_ () == "http://www.cadseer.com/prj/srl/sws")
        {
          ::std::unique_ptr< ::prj::srl::sws::Sew > r (
            ::xsd::cxx::tree::traits< ::prj::srl::sws::Sew, char >::create (
              e, f, 0));
          return r;
        }

        throw ::xsd::cxx::tree::unexpected_element < char > (
          n.name (),
          n.namespace_ (),
          "sew",
          "http://www.cadseer.com/prj/srl/sws");
      }

      ::std::unique_ptr< ::prj::srl::sws::Sew >
      sew (::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d,
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

        if (n.name () == "sew" &&
            n.namespace_ () == "http://www.cadseer.com/prj/srl/sws")
        {
          ::std::unique_ptr< ::prj::srl::sws::Sew > r (
            ::xsd::cxx::tree::traits< ::prj::srl::sws::Sew, char >::create (
              e, f, 0));
          return r;
        }

        throw ::xsd::cxx::tree::unexpected_element < char > (
          n.name (),
          n.namespace_ (),
          "sew",
          "http://www.cadseer.com/prj/srl/sws");
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
    namespace sws
    {
      void
      operator<< (::xercesc::DOMElement& e, const Sew& i)
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

        // seerShape
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "seerShape",
              e));

          s << i.seerShape ();
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
      }

      void
      sew (::std::ostream& o,
           const ::prj::srl::sws::Sew& s,
           const ::xml_schema::NamespaceInfomap& m,
           const ::std::string& e,
           ::xml_schema::Flags f)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0);

        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::sws::sew (s, m, f));

        ::xsd::cxx::tree::error_handler< char > h;

        ::xsd::cxx::xml::dom::ostream_format_target t (o);
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          h.throw_if_failed< ::xsd::cxx::tree::serialization< char > > ();
        }
      }

      void
      sew (::std::ostream& o,
           const ::prj::srl::sws::Sew& s,
           ::xml_schema::ErrorHandler& h,
           const ::xml_schema::NamespaceInfomap& m,
           const ::std::string& e,
           ::xml_schema::Flags f)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0);

        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::sws::sew (s, m, f));
        ::xsd::cxx::xml::dom::ostream_format_target t (o);
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      sew (::std::ostream& o,
           const ::prj::srl::sws::Sew& s,
           ::xercesc::DOMErrorHandler& h,
           const ::xml_schema::NamespaceInfomap& m,
           const ::std::string& e,
           ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::sws::sew (s, m, f));
        ::xsd::cxx::xml::dom::ostream_format_target t (o);
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      sew (::xercesc::XMLFormatTarget& t,
           const ::prj::srl::sws::Sew& s,
           const ::xml_schema::NamespaceInfomap& m,
           const ::std::string& e,
           ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::sws::sew (s, m, f));

        ::xsd::cxx::tree::error_handler< char > h;

        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          h.throw_if_failed< ::xsd::cxx::tree::serialization< char > > ();
        }
      }

      void
      sew (::xercesc::XMLFormatTarget& t,
           const ::prj::srl::sws::Sew& s,
           ::xml_schema::ErrorHandler& h,
           const ::xml_schema::NamespaceInfomap& m,
           const ::std::string& e,
           ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::sws::sew (s, m, f));
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      sew (::xercesc::XMLFormatTarget& t,
           const ::prj::srl::sws::Sew& s,
           ::xercesc::DOMErrorHandler& h,
           const ::xml_schema::NamespaceInfomap& m,
           const ::std::string& e,
           ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::sws::sew (s, m, f));
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      sew (::xercesc::DOMDocument& d,
           const ::prj::srl::sws::Sew& s,
           ::xml_schema::Flags)
      {
        ::xercesc::DOMElement& e (*d.getDocumentElement ());
        const ::xsd::cxx::xml::qualified_name< char > n (
          ::xsd::cxx::xml::dom::name< char > (e));

        if (n.name () == "sew" &&
            n.namespace_ () == "http://www.cadseer.com/prj/srl/sws")
        {
          e << s;
        }
        else
        {
          throw ::xsd::cxx::tree::unexpected_element < char > (
            n.name (),
            n.namespace_ (),
            "sew",
            "http://www.cadseer.com/prj/srl/sws");
        }
      }

      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument >
      sew (const ::prj::srl::sws::Sew& s,
           const ::xml_schema::NamespaceInfomap& m,
           ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::serialize< char > (
            "sew",
            "http://www.cadseer.com/prj/srl/sws",
            m, f));

        ::prj::srl::sws::sew (*d, s, f);
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


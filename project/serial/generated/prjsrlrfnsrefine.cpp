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

#include "prjsrlrfnsrefine.h"

namespace prj
{
  namespace srl
  {
    namespace rfns
    {
      // Refine
      // 

      const Refine::BaseType& Refine::
      base () const
      {
        return this->base_.get ();
      }

      Refine::BaseType& Refine::
      base ()
      {
        return this->base_.get ();
      }

      void Refine::
      base (const BaseType& x)
      {
        this->base_.set (x);
      }

      void Refine::
      base (::std::unique_ptr< BaseType > x)
      {
        this->base_.set (std::move (x));
      }

      const Refine::SeerShapeType& Refine::
      seerShape () const
      {
        return this->seerShape_.get ();
      }

      Refine::SeerShapeType& Refine::
      seerShape ()
      {
        return this->seerShape_.get ();
      }

      void Refine::
      seerShape (const SeerShapeType& x)
      {
        this->seerShape_.set (x);
      }

      void Refine::
      seerShape (::std::unique_ptr< SeerShapeType > x)
      {
        this->seerShape_.set (std::move (x));
      }

      const Refine::ShapeMapSequence& Refine::
      shapeMap () const
      {
        return this->shapeMap_;
      }

      Refine::ShapeMapSequence& Refine::
      shapeMap ()
      {
        return this->shapeMap_;
      }

      void Refine::
      shapeMap (const ShapeMapSequence& s)
      {
        this->shapeMap_ = s;
      }
    }
  }
}

#include <xsd/cxx/xml/dom/parsing-source.hxx>

namespace prj
{
  namespace srl
  {
    namespace rfns
    {
      // Refine
      //

      Refine::
      Refine (const BaseType& base,
              const SeerShapeType& seerShape)
      : ::xml_schema::Type (),
        base_ (base, this),
        seerShape_ (seerShape, this),
        shapeMap_ (this)
      {
      }

      Refine::
      Refine (::std::unique_ptr< BaseType > base,
              ::std::unique_ptr< SeerShapeType > seerShape)
      : ::xml_schema::Type (),
        base_ (std::move (base), this),
        seerShape_ (std::move (seerShape), this),
        shapeMap_ (this)
      {
      }

      Refine::
      Refine (const Refine& x,
              ::xml_schema::Flags f,
              ::xml_schema::Container* c)
      : ::xml_schema::Type (x, f, c),
        base_ (x.base_, f, this),
        seerShape_ (x.seerShape_, f, this),
        shapeMap_ (x.shapeMap_, f, this)
      {
      }

      Refine::
      Refine (const ::xercesc::DOMElement& e,
              ::xml_schema::Flags f,
              ::xml_schema::Container* c)
      : ::xml_schema::Type (e, f | ::xml_schema::Flags::base, c),
        base_ (this),
        seerShape_ (this),
        shapeMap_ (this)
      {
        if ((f & ::xml_schema::Flags::base) == 0)
        {
          ::xsd::cxx::xml::dom::parser< char > p (e, true, false, false);
          this->parse (p, f);
        }
      }

      void Refine::
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

          // shapeMap
          //
          if (n.name () == "shapeMap" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< ShapeMapType > r (
              ShapeMapTraits::create (i, f, this));

            this->shapeMap_.push_back (::std::move (r));
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
      }

      Refine* Refine::
      _clone (::xml_schema::Flags f,
              ::xml_schema::Container* c) const
      {
        return new class Refine (*this, f, c);
      }

      Refine& Refine::
      operator= (const Refine& x)
      {
        if (this != &x)
        {
          static_cast< ::xml_schema::Type& > (*this) = x;
          this->base_ = x.base_;
          this->seerShape_ = x.seerShape_;
          this->shapeMap_ = x.shapeMap_;
        }

        return *this;
      }

      Refine::
      ~Refine ()
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
    namespace rfns
    {
      ::std::unique_ptr< ::prj::srl::rfns::Refine >
      refine (const ::std::string& u,
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

        return ::std::unique_ptr< ::prj::srl::rfns::Refine > (
          ::prj::srl::rfns::refine (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::rfns::Refine >
      refine (const ::std::string& u,
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

        return ::std::unique_ptr< ::prj::srl::rfns::Refine > (
          ::prj::srl::rfns::refine (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::rfns::Refine >
      refine (const ::std::string& u,
              ::xercesc::DOMErrorHandler& h,
              ::xml_schema::Flags f,
              const ::xml_schema::Properties& p)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            u, h, p, f));

        if (!d.get ())
          throw ::xsd::cxx::tree::parsing< char > ();

        return ::std::unique_ptr< ::prj::srl::rfns::Refine > (
          ::prj::srl::rfns::refine (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::rfns::Refine >
      refine (::std::istream& is,
              ::xml_schema::Flags f,
              const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is);
        return ::prj::srl::rfns::refine (isrc, f, p);
      }

      ::std::unique_ptr< ::prj::srl::rfns::Refine >
      refine (::std::istream& is,
              ::xml_schema::ErrorHandler& h,
              ::xml_schema::Flags f,
              const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is);
        return ::prj::srl::rfns::refine (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::rfns::Refine >
      refine (::std::istream& is,
              ::xercesc::DOMErrorHandler& h,
              ::xml_schema::Flags f,
              const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::sax::std_input_source isrc (is);
        return ::prj::srl::rfns::refine (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::rfns::Refine >
      refine (::std::istream& is,
              const ::std::string& sid,
              ::xml_schema::Flags f,
              const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
        return ::prj::srl::rfns::refine (isrc, f, p);
      }

      ::std::unique_ptr< ::prj::srl::rfns::Refine >
      refine (::std::istream& is,
              const ::std::string& sid,
              ::xml_schema::ErrorHandler& h,
              ::xml_schema::Flags f,
              const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
        return ::prj::srl::rfns::refine (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::rfns::Refine >
      refine (::std::istream& is,
              const ::std::string& sid,
              ::xercesc::DOMErrorHandler& h,
              ::xml_schema::Flags f,
              const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
        return ::prj::srl::rfns::refine (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::rfns::Refine >
      refine (::xercesc::InputSource& i,
              ::xml_schema::Flags f,
              const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::tree::error_handler< char > h;

        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            i, h, p, f));

        h.throw_if_failed< ::xsd::cxx::tree::parsing< char > > ();

        return ::std::unique_ptr< ::prj::srl::rfns::Refine > (
          ::prj::srl::rfns::refine (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::rfns::Refine >
      refine (::xercesc::InputSource& i,
              ::xml_schema::ErrorHandler& h,
              ::xml_schema::Flags f,
              const ::xml_schema::Properties& p)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            i, h, p, f));

        if (!d.get ())
          throw ::xsd::cxx::tree::parsing< char > ();

        return ::std::unique_ptr< ::prj::srl::rfns::Refine > (
          ::prj::srl::rfns::refine (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::rfns::Refine >
      refine (::xercesc::InputSource& i,
              ::xercesc::DOMErrorHandler& h,
              ::xml_schema::Flags f,
              const ::xml_schema::Properties& p)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            i, h, p, f));

        if (!d.get ())
          throw ::xsd::cxx::tree::parsing< char > ();

        return ::std::unique_ptr< ::prj::srl::rfns::Refine > (
          ::prj::srl::rfns::refine (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::rfns::Refine >
      refine (const ::xercesc::DOMDocument& doc,
              ::xml_schema::Flags f,
              const ::xml_schema::Properties& p)
      {
        if (f & ::xml_schema::Flags::keep_dom)
        {
          ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
            static_cast< ::xercesc::DOMDocument* > (doc.cloneNode (true)));

          return ::std::unique_ptr< ::prj::srl::rfns::Refine > (
            ::prj::srl::rfns::refine (
              std::move (d), f | ::xml_schema::Flags::own_dom, p));
        }

        const ::xercesc::DOMElement& e (*doc.getDocumentElement ());
        const ::xsd::cxx::xml::qualified_name< char > n (
          ::xsd::cxx::xml::dom::name< char > (e));

        if (n.name () == "refine" &&
            n.namespace_ () == "http://www.cadseer.com/prj/srl/rfns")
        {
          ::std::unique_ptr< ::prj::srl::rfns::Refine > r (
            ::xsd::cxx::tree::traits< ::prj::srl::rfns::Refine, char >::create (
              e, f, 0));
          return r;
        }

        throw ::xsd::cxx::tree::unexpected_element < char > (
          n.name (),
          n.namespace_ (),
          "refine",
          "http://www.cadseer.com/prj/srl/rfns");
      }

      ::std::unique_ptr< ::prj::srl::rfns::Refine >
      refine (::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d,
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

        if (n.name () == "refine" &&
            n.namespace_ () == "http://www.cadseer.com/prj/srl/rfns")
        {
          ::std::unique_ptr< ::prj::srl::rfns::Refine > r (
            ::xsd::cxx::tree::traits< ::prj::srl::rfns::Refine, char >::create (
              e, f, 0));
          return r;
        }

        throw ::xsd::cxx::tree::unexpected_element < char > (
          n.name (),
          n.namespace_ (),
          "refine",
          "http://www.cadseer.com/prj/srl/rfns");
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
    namespace rfns
    {
      void
      operator<< (::xercesc::DOMElement& e, const Refine& i)
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

        // shapeMap
        //
        for (Refine::ShapeMapConstIterator
             b (i.shapeMap ().begin ()), n (i.shapeMap ().end ());
             b != n; ++b)
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "shapeMap",
              e));

          s << *b;
        }
      }

      void
      refine (::std::ostream& o,
              const ::prj::srl::rfns::Refine& s,
              const ::xml_schema::NamespaceInfomap& m,
              const ::std::string& e,
              ::xml_schema::Flags f)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0);

        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::rfns::refine (s, m, f));

        ::xsd::cxx::tree::error_handler< char > h;

        ::xsd::cxx::xml::dom::ostream_format_target t (o);
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          h.throw_if_failed< ::xsd::cxx::tree::serialization< char > > ();
        }
      }

      void
      refine (::std::ostream& o,
              const ::prj::srl::rfns::Refine& s,
              ::xml_schema::ErrorHandler& h,
              const ::xml_schema::NamespaceInfomap& m,
              const ::std::string& e,
              ::xml_schema::Flags f)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0);

        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::rfns::refine (s, m, f));
        ::xsd::cxx::xml::dom::ostream_format_target t (o);
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      refine (::std::ostream& o,
              const ::prj::srl::rfns::Refine& s,
              ::xercesc::DOMErrorHandler& h,
              const ::xml_schema::NamespaceInfomap& m,
              const ::std::string& e,
              ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::rfns::refine (s, m, f));
        ::xsd::cxx::xml::dom::ostream_format_target t (o);
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      refine (::xercesc::XMLFormatTarget& t,
              const ::prj::srl::rfns::Refine& s,
              const ::xml_schema::NamespaceInfomap& m,
              const ::std::string& e,
              ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::rfns::refine (s, m, f));

        ::xsd::cxx::tree::error_handler< char > h;

        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          h.throw_if_failed< ::xsd::cxx::tree::serialization< char > > ();
        }
      }

      void
      refine (::xercesc::XMLFormatTarget& t,
              const ::prj::srl::rfns::Refine& s,
              ::xml_schema::ErrorHandler& h,
              const ::xml_schema::NamespaceInfomap& m,
              const ::std::string& e,
              ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::rfns::refine (s, m, f));
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      refine (::xercesc::XMLFormatTarget& t,
              const ::prj::srl::rfns::Refine& s,
              ::xercesc::DOMErrorHandler& h,
              const ::xml_schema::NamespaceInfomap& m,
              const ::std::string& e,
              ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::rfns::refine (s, m, f));
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      refine (::xercesc::DOMDocument& d,
              const ::prj::srl::rfns::Refine& s,
              ::xml_schema::Flags)
      {
        ::xercesc::DOMElement& e (*d.getDocumentElement ());
        const ::xsd::cxx::xml::qualified_name< char > n (
          ::xsd::cxx::xml::dom::name< char > (e));

        if (n.name () == "refine" &&
            n.namespace_ () == "http://www.cadseer.com/prj/srl/rfns")
        {
          e << s;
        }
        else
        {
          throw ::xsd::cxx::tree::unexpected_element < char > (
            n.name (),
            n.namespace_ (),
            "refine",
            "http://www.cadseer.com/prj/srl/rfns");
        }
      }

      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument >
      refine (const ::prj::srl::rfns::Refine& s,
              const ::xml_schema::NamespaceInfomap& m,
              ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::serialize< char > (
            "refine",
            "http://www.cadseer.com/prj/srl/rfns",
            m, f));

        ::prj::srl::rfns::refine (*d, s, f);
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


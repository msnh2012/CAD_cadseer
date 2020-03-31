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

#include "prjsrlsmfssurfacemeshfill.h"

namespace prj
{
  namespace srl
  {
    namespace smfs
    {
      // SurfaceMeshFill
      // 

      const SurfaceMeshFill::BaseType& SurfaceMeshFill::
      base () const
      {
        return this->base_.get ();
      }

      SurfaceMeshFill::BaseType& SurfaceMeshFill::
      base ()
      {
        return this->base_.get ();
      }

      void SurfaceMeshFill::
      base (const BaseType& x)
      {
        this->base_.set (x);
      }

      void SurfaceMeshFill::
      base (::std::unique_ptr< BaseType > x)
      {
        this->base_.set (std::move (x));
      }

      const SurfaceMeshFill::MeshType& SurfaceMeshFill::
      mesh () const
      {
        return this->mesh_.get ();
      }

      SurfaceMeshFill::MeshType& SurfaceMeshFill::
      mesh ()
      {
        return this->mesh_.get ();
      }

      void SurfaceMeshFill::
      mesh (const MeshType& x)
      {
        this->mesh_.set (x);
      }

      void SurfaceMeshFill::
      mesh (::std::unique_ptr< MeshType > x)
      {
        this->mesh_.set (std::move (x));
      }

      const SurfaceMeshFill::AlgorithmType& SurfaceMeshFill::
      algorithm () const
      {
        return this->algorithm_.get ();
      }

      SurfaceMeshFill::AlgorithmType& SurfaceMeshFill::
      algorithm ()
      {
        return this->algorithm_.get ();
      }

      void SurfaceMeshFill::
      algorithm (const AlgorithmType& x)
      {
        this->algorithm_.set (x);
      }

      void SurfaceMeshFill::
      algorithm (::std::unique_ptr< AlgorithmType > x)
      {
        this->algorithm_.set (std::move (x));
      }

      const SurfaceMeshFill::AlgorithmLabelType& SurfaceMeshFill::
      algorithmLabel () const
      {
        return this->algorithmLabel_.get ();
      }

      SurfaceMeshFill::AlgorithmLabelType& SurfaceMeshFill::
      algorithmLabel ()
      {
        return this->algorithmLabel_.get ();
      }

      void SurfaceMeshFill::
      algorithmLabel (const AlgorithmLabelType& x)
      {
        this->algorithmLabel_.set (x);
      }

      void SurfaceMeshFill::
      algorithmLabel (::std::unique_ptr< AlgorithmLabelType > x)
      {
        this->algorithmLabel_.set (std::move (x));
      }
    }
  }
}

#include <xsd/cxx/xml/dom/parsing-source.hxx>

namespace prj
{
  namespace srl
  {
    namespace smfs
    {
      // SurfaceMeshFill
      //

      SurfaceMeshFill::
      SurfaceMeshFill (const BaseType& base,
                       const MeshType& mesh,
                       const AlgorithmType& algorithm,
                       const AlgorithmLabelType& algorithmLabel)
      : ::xml_schema::Type (),
        base_ (base, this),
        mesh_ (mesh, this),
        algorithm_ (algorithm, this),
        algorithmLabel_ (algorithmLabel, this)
      {
      }

      SurfaceMeshFill::
      SurfaceMeshFill (::std::unique_ptr< BaseType > base,
                       ::std::unique_ptr< MeshType > mesh,
                       ::std::unique_ptr< AlgorithmType > algorithm,
                       ::std::unique_ptr< AlgorithmLabelType > algorithmLabel)
      : ::xml_schema::Type (),
        base_ (std::move (base), this),
        mesh_ (std::move (mesh), this),
        algorithm_ (std::move (algorithm), this),
        algorithmLabel_ (std::move (algorithmLabel), this)
      {
      }

      SurfaceMeshFill::
      SurfaceMeshFill (const SurfaceMeshFill& x,
                       ::xml_schema::Flags f,
                       ::xml_schema::Container* c)
      : ::xml_schema::Type (x, f, c),
        base_ (x.base_, f, this),
        mesh_ (x.mesh_, f, this),
        algorithm_ (x.algorithm_, f, this),
        algorithmLabel_ (x.algorithmLabel_, f, this)
      {
      }

      SurfaceMeshFill::
      SurfaceMeshFill (const ::xercesc::DOMElement& e,
                       ::xml_schema::Flags f,
                       ::xml_schema::Container* c)
      : ::xml_schema::Type (e, f | ::xml_schema::Flags::base, c),
        base_ (this),
        mesh_ (this),
        algorithm_ (this),
        algorithmLabel_ (this)
      {
        if ((f & ::xml_schema::Flags::base) == 0)
        {
          ::xsd::cxx::xml::dom::parser< char > p (e, true, false, false);
          this->parse (p, f);
        }
      }

      void SurfaceMeshFill::
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

          // mesh
          //
          if (n.name () == "mesh" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< MeshType > r (
              MeshTraits::create (i, f, this));

            if (!mesh_.present ())
            {
              this->mesh_.set (::std::move (r));
              continue;
            }
          }

          // algorithm
          //
          if (n.name () == "algorithm" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< AlgorithmType > r (
              AlgorithmTraits::create (i, f, this));

            if (!algorithm_.present ())
            {
              this->algorithm_.set (::std::move (r));
              continue;
            }
          }

          // algorithmLabel
          //
          if (n.name () == "algorithmLabel" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< AlgorithmLabelType > r (
              AlgorithmLabelTraits::create (i, f, this));

            if (!algorithmLabel_.present ())
            {
              this->algorithmLabel_.set (::std::move (r));
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

        if (!mesh_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "mesh",
            "");
        }

        if (!algorithm_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "algorithm",
            "");
        }

        if (!algorithmLabel_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "algorithmLabel",
            "");
        }
      }

      SurfaceMeshFill* SurfaceMeshFill::
      _clone (::xml_schema::Flags f,
              ::xml_schema::Container* c) const
      {
        return new class SurfaceMeshFill (*this, f, c);
      }

      SurfaceMeshFill& SurfaceMeshFill::
      operator= (const SurfaceMeshFill& x)
      {
        if (this != &x)
        {
          static_cast< ::xml_schema::Type& > (*this) = x;
          this->base_ = x.base_;
          this->mesh_ = x.mesh_;
          this->algorithm_ = x.algorithm_;
          this->algorithmLabel_ = x.algorithmLabel_;
        }

        return *this;
      }

      SurfaceMeshFill::
      ~SurfaceMeshFill ()
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
    namespace smfs
    {
      ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill >
      surfacemeshfill (const ::std::string& u,
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

        return ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill > (
          ::prj::srl::smfs::surfacemeshfill (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill >
      surfacemeshfill (const ::std::string& u,
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

        return ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill > (
          ::prj::srl::smfs::surfacemeshfill (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill >
      surfacemeshfill (const ::std::string& u,
                       ::xercesc::DOMErrorHandler& h,
                       ::xml_schema::Flags f,
                       const ::xml_schema::Properties& p)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            u, h, p, f));

        if (!d.get ())
          throw ::xsd::cxx::tree::parsing< char > ();

        return ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill > (
          ::prj::srl::smfs::surfacemeshfill (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill >
      surfacemeshfill (::std::istream& is,
                       ::xml_schema::Flags f,
                       const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is);
        return ::prj::srl::smfs::surfacemeshfill (isrc, f, p);
      }

      ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill >
      surfacemeshfill (::std::istream& is,
                       ::xml_schema::ErrorHandler& h,
                       ::xml_schema::Flags f,
                       const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is);
        return ::prj::srl::smfs::surfacemeshfill (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill >
      surfacemeshfill (::std::istream& is,
                       ::xercesc::DOMErrorHandler& h,
                       ::xml_schema::Flags f,
                       const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::sax::std_input_source isrc (is);
        return ::prj::srl::smfs::surfacemeshfill (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill >
      surfacemeshfill (::std::istream& is,
                       const ::std::string& sid,
                       ::xml_schema::Flags f,
                       const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
        return ::prj::srl::smfs::surfacemeshfill (isrc, f, p);
      }

      ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill >
      surfacemeshfill (::std::istream& is,
                       const ::std::string& sid,
                       ::xml_schema::ErrorHandler& h,
                       ::xml_schema::Flags f,
                       const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
        return ::prj::srl::smfs::surfacemeshfill (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill >
      surfacemeshfill (::std::istream& is,
                       const ::std::string& sid,
                       ::xercesc::DOMErrorHandler& h,
                       ::xml_schema::Flags f,
                       const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
        return ::prj::srl::smfs::surfacemeshfill (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill >
      surfacemeshfill (::xercesc::InputSource& i,
                       ::xml_schema::Flags f,
                       const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::tree::error_handler< char > h;

        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            i, h, p, f));

        h.throw_if_failed< ::xsd::cxx::tree::parsing< char > > ();

        return ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill > (
          ::prj::srl::smfs::surfacemeshfill (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill >
      surfacemeshfill (::xercesc::InputSource& i,
                       ::xml_schema::ErrorHandler& h,
                       ::xml_schema::Flags f,
                       const ::xml_schema::Properties& p)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            i, h, p, f));

        if (!d.get ())
          throw ::xsd::cxx::tree::parsing< char > ();

        return ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill > (
          ::prj::srl::smfs::surfacemeshfill (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill >
      surfacemeshfill (::xercesc::InputSource& i,
                       ::xercesc::DOMErrorHandler& h,
                       ::xml_schema::Flags f,
                       const ::xml_schema::Properties& p)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            i, h, p, f));

        if (!d.get ())
          throw ::xsd::cxx::tree::parsing< char > ();

        return ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill > (
          ::prj::srl::smfs::surfacemeshfill (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill >
      surfacemeshfill (const ::xercesc::DOMDocument& doc,
                       ::xml_schema::Flags f,
                       const ::xml_schema::Properties& p)
      {
        if (f & ::xml_schema::Flags::keep_dom)
        {
          ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
            static_cast< ::xercesc::DOMDocument* > (doc.cloneNode (true)));

          return ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill > (
            ::prj::srl::smfs::surfacemeshfill (
              std::move (d), f | ::xml_schema::Flags::own_dom, p));
        }

        const ::xercesc::DOMElement& e (*doc.getDocumentElement ());
        const ::xsd::cxx::xml::qualified_name< char > n (
          ::xsd::cxx::xml::dom::name< char > (e));

        if (n.name () == "surfacemeshfill" &&
            n.namespace_ () == "http://www.cadseer.com/prj/srl/smfs")
        {
          ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill > r (
            ::xsd::cxx::tree::traits< ::prj::srl::smfs::SurfaceMeshFill, char >::create (
              e, f, 0));
          return r;
        }

        throw ::xsd::cxx::tree::unexpected_element < char > (
          n.name (),
          n.namespace_ (),
          "surfacemeshfill",
          "http://www.cadseer.com/prj/srl/smfs");
      }

      ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill >
      surfacemeshfill (::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d,
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

        if (n.name () == "surfacemeshfill" &&
            n.namespace_ () == "http://www.cadseer.com/prj/srl/smfs")
        {
          ::std::unique_ptr< ::prj::srl::smfs::SurfaceMeshFill > r (
            ::xsd::cxx::tree::traits< ::prj::srl::smfs::SurfaceMeshFill, char >::create (
              e, f, 0));
          return r;
        }

        throw ::xsd::cxx::tree::unexpected_element < char > (
          n.name (),
          n.namespace_ (),
          "surfacemeshfill",
          "http://www.cadseer.com/prj/srl/smfs");
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
    namespace smfs
    {
      void
      operator<< (::xercesc::DOMElement& e, const SurfaceMeshFill& i)
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

        // mesh
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "mesh",
              e));

          s << i.mesh ();
        }

        // algorithm
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "algorithm",
              e));

          s << i.algorithm ();
        }

        // algorithmLabel
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "algorithmLabel",
              e));

          s << i.algorithmLabel ();
        }
      }

      void
      surfacemeshfill (::std::ostream& o,
                       const ::prj::srl::smfs::SurfaceMeshFill& s,
                       const ::xml_schema::NamespaceInfomap& m,
                       const ::std::string& e,
                       ::xml_schema::Flags f)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0);

        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::smfs::surfacemeshfill (s, m, f));

        ::xsd::cxx::tree::error_handler< char > h;

        ::xsd::cxx::xml::dom::ostream_format_target t (o);
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          h.throw_if_failed< ::xsd::cxx::tree::serialization< char > > ();
        }
      }

      void
      surfacemeshfill (::std::ostream& o,
                       const ::prj::srl::smfs::SurfaceMeshFill& s,
                       ::xml_schema::ErrorHandler& h,
                       const ::xml_schema::NamespaceInfomap& m,
                       const ::std::string& e,
                       ::xml_schema::Flags f)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0);

        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::smfs::surfacemeshfill (s, m, f));
        ::xsd::cxx::xml::dom::ostream_format_target t (o);
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      surfacemeshfill (::std::ostream& o,
                       const ::prj::srl::smfs::SurfaceMeshFill& s,
                       ::xercesc::DOMErrorHandler& h,
                       const ::xml_schema::NamespaceInfomap& m,
                       const ::std::string& e,
                       ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::smfs::surfacemeshfill (s, m, f));
        ::xsd::cxx::xml::dom::ostream_format_target t (o);
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      surfacemeshfill (::xercesc::XMLFormatTarget& t,
                       const ::prj::srl::smfs::SurfaceMeshFill& s,
                       const ::xml_schema::NamespaceInfomap& m,
                       const ::std::string& e,
                       ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::smfs::surfacemeshfill (s, m, f));

        ::xsd::cxx::tree::error_handler< char > h;

        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          h.throw_if_failed< ::xsd::cxx::tree::serialization< char > > ();
        }
      }

      void
      surfacemeshfill (::xercesc::XMLFormatTarget& t,
                       const ::prj::srl::smfs::SurfaceMeshFill& s,
                       ::xml_schema::ErrorHandler& h,
                       const ::xml_schema::NamespaceInfomap& m,
                       const ::std::string& e,
                       ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::smfs::surfacemeshfill (s, m, f));
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      surfacemeshfill (::xercesc::XMLFormatTarget& t,
                       const ::prj::srl::smfs::SurfaceMeshFill& s,
                       ::xercesc::DOMErrorHandler& h,
                       const ::xml_schema::NamespaceInfomap& m,
                       const ::std::string& e,
                       ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::smfs::surfacemeshfill (s, m, f));
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      surfacemeshfill (::xercesc::DOMDocument& d,
                       const ::prj::srl::smfs::SurfaceMeshFill& s,
                       ::xml_schema::Flags)
      {
        ::xercesc::DOMElement& e (*d.getDocumentElement ());
        const ::xsd::cxx::xml::qualified_name< char > n (
          ::xsd::cxx::xml::dom::name< char > (e));

        if (n.name () == "surfacemeshfill" &&
            n.namespace_ () == "http://www.cadseer.com/prj/srl/smfs")
        {
          e << s;
        }
        else
        {
          throw ::xsd::cxx::tree::unexpected_element < char > (
            n.name (),
            n.namespace_ (),
            "surfacemeshfill",
            "http://www.cadseer.com/prj/srl/smfs");
        }
      }

      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument >
      surfacemeshfill (const ::prj::srl::smfs::SurfaceMeshFill& s,
                       const ::xml_schema::NamespaceInfomap& m,
                       ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::serialize< char > (
            "surfacemeshfill",
            "http://www.cadseer.com/prj/srl/smfs",
            m, f));

        ::prj::srl::smfs::surfacemeshfill (*d, s, f);
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

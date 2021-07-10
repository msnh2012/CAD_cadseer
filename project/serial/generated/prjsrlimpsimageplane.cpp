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

#include "prjsrlimpsimageplane.h"

namespace prj
{
  namespace srl
  {
    namespace imps
    {
      // ImagePlane
      // 

      const ImagePlane::BaseType& ImagePlane::
      base () const
      {
        return this->base_.get ();
      }

      ImagePlane::BaseType& ImagePlane::
      base ()
      {
        return this->base_.get ();
      }

      void ImagePlane::
      base (const BaseType& x)
      {
        this->base_.set (x);
      }

      void ImagePlane::
      base (::std::unique_ptr< BaseType > x)
      {
        this->base_.set (std::move (x));
      }

      const ImagePlane::ScaleType& ImagePlane::
      scale () const
      {
        return this->scale_.get ();
      }

      ImagePlane::ScaleType& ImagePlane::
      scale ()
      {
        return this->scale_.get ();
      }

      void ImagePlane::
      scale (const ScaleType& x)
      {
        this->scale_.set (x);
      }

      void ImagePlane::
      scale (::std::unique_ptr< ScaleType > x)
      {
        this->scale_.set (std::move (x));
      }

      const ImagePlane::CsysType& ImagePlane::
      csys () const
      {
        return this->csys_.get ();
      }

      ImagePlane::CsysType& ImagePlane::
      csys ()
      {
        return this->csys_.get ();
      }

      void ImagePlane::
      csys (const CsysType& x)
      {
        this->csys_.set (x);
      }

      void ImagePlane::
      csys (::std::unique_ptr< CsysType > x)
      {
        this->csys_.set (std::move (x));
      }

      const ImagePlane::CsysDraggerType& ImagePlane::
      csysDragger () const
      {
        return this->csysDragger_.get ();
      }

      ImagePlane::CsysDraggerType& ImagePlane::
      csysDragger ()
      {
        return this->csysDragger_.get ();
      }

      void ImagePlane::
      csysDragger (const CsysDraggerType& x)
      {
        this->csysDragger_.set (x);
      }

      void ImagePlane::
      csysDragger (::std::unique_ptr< CsysDraggerType > x)
      {
        this->csysDragger_.set (std::move (x));
      }

      const ImagePlane::ScaleLabelType& ImagePlane::
      scaleLabel () const
      {
        return this->scaleLabel_.get ();
      }

      ImagePlane::ScaleLabelType& ImagePlane::
      scaleLabel ()
      {
        return this->scaleLabel_.get ();
      }

      void ImagePlane::
      scaleLabel (const ScaleLabelType& x)
      {
        this->scaleLabel_.set (x);
      }

      void ImagePlane::
      scaleLabel (::std::unique_ptr< ScaleLabelType > x)
      {
        this->scaleLabel_.set (std::move (x));
      }

      const ImagePlane::CornerVecType& ImagePlane::
      cornerVec () const
      {
        return this->cornerVec_.get ();
      }

      ImagePlane::CornerVecType& ImagePlane::
      cornerVec ()
      {
        return this->cornerVec_.get ();
      }

      void ImagePlane::
      cornerVec (const CornerVecType& x)
      {
        this->cornerVec_.set (x);
      }

      void ImagePlane::
      cornerVec (::std::unique_ptr< CornerVecType > x)
      {
        this->cornerVec_.set (std::move (x));
      }

      const ImagePlane::ImagePathType& ImagePlane::
      imagePath () const
      {
        return this->imagePath_.get ();
      }

      ImagePlane::ImagePathType& ImagePlane::
      imagePath ()
      {
        return this->imagePath_.get ();
      }

      void ImagePlane::
      imagePath (const ImagePathType& x)
      {
        this->imagePath_.set (x);
      }

      void ImagePlane::
      imagePath (::std::unique_ptr< ImagePathType > x)
      {
        this->imagePath_.set (std::move (x));
      }
    }
  }
}

#include <xsd/cxx/xml/dom/parsing-source.hxx>

namespace prj
{
  namespace srl
  {
    namespace imps
    {
      // ImagePlane
      //

      ImagePlane::
      ImagePlane (const BaseType& base,
                  const ScaleType& scale,
                  const CsysType& csys,
                  const CsysDraggerType& csysDragger,
                  const ScaleLabelType& scaleLabel,
                  const CornerVecType& cornerVec,
                  const ImagePathType& imagePath)
      : ::xml_schema::Type (),
        base_ (base, this),
        scale_ (scale, this),
        csys_ (csys, this),
        csysDragger_ (csysDragger, this),
        scaleLabel_ (scaleLabel, this),
        cornerVec_ (cornerVec, this),
        imagePath_ (imagePath, this)
      {
      }

      ImagePlane::
      ImagePlane (::std::unique_ptr< BaseType > base,
                  ::std::unique_ptr< ScaleType > scale,
                  ::std::unique_ptr< CsysType > csys,
                  ::std::unique_ptr< CsysDraggerType > csysDragger,
                  ::std::unique_ptr< ScaleLabelType > scaleLabel,
                  ::std::unique_ptr< CornerVecType > cornerVec,
                  const ImagePathType& imagePath)
      : ::xml_schema::Type (),
        base_ (std::move (base), this),
        scale_ (std::move (scale), this),
        csys_ (std::move (csys), this),
        csysDragger_ (std::move (csysDragger), this),
        scaleLabel_ (std::move (scaleLabel), this),
        cornerVec_ (std::move (cornerVec), this),
        imagePath_ (imagePath, this)
      {
      }

      ImagePlane::
      ImagePlane (const ImagePlane& x,
                  ::xml_schema::Flags f,
                  ::xml_schema::Container* c)
      : ::xml_schema::Type (x, f, c),
        base_ (x.base_, f, this),
        scale_ (x.scale_, f, this),
        csys_ (x.csys_, f, this),
        csysDragger_ (x.csysDragger_, f, this),
        scaleLabel_ (x.scaleLabel_, f, this),
        cornerVec_ (x.cornerVec_, f, this),
        imagePath_ (x.imagePath_, f, this)
      {
      }

      ImagePlane::
      ImagePlane (const ::xercesc::DOMElement& e,
                  ::xml_schema::Flags f,
                  ::xml_schema::Container* c)
      : ::xml_schema::Type (e, f | ::xml_schema::Flags::base, c),
        base_ (this),
        scale_ (this),
        csys_ (this),
        csysDragger_ (this),
        scaleLabel_ (this),
        cornerVec_ (this),
        imagePath_ (this)
      {
        if ((f & ::xml_schema::Flags::base) == 0)
        {
          ::xsd::cxx::xml::dom::parser< char > p (e, true, false, false);
          this->parse (p, f);
        }
      }

      void ImagePlane::
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

          // scale
          //
          if (n.name () == "scale" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< ScaleType > r (
              ScaleTraits::create (i, f, this));

            if (!scale_.present ())
            {
              this->scale_.set (::std::move (r));
              continue;
            }
          }

          // csys
          //
          if (n.name () == "csys" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< CsysType > r (
              CsysTraits::create (i, f, this));

            if (!csys_.present ())
            {
              this->csys_.set (::std::move (r));
              continue;
            }
          }

          // csysDragger
          //
          if (n.name () == "csysDragger" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< CsysDraggerType > r (
              CsysDraggerTraits::create (i, f, this));

            if (!csysDragger_.present ())
            {
              this->csysDragger_.set (::std::move (r));
              continue;
            }
          }

          // scaleLabel
          //
          if (n.name () == "scaleLabel" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< ScaleLabelType > r (
              ScaleLabelTraits::create (i, f, this));

            if (!scaleLabel_.present ())
            {
              this->scaleLabel_.set (::std::move (r));
              continue;
            }
          }

          // cornerVec
          //
          if (n.name () == "cornerVec" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< CornerVecType > r (
              CornerVecTraits::create (i, f, this));

            if (!cornerVec_.present ())
            {
              this->cornerVec_.set (::std::move (r));
              continue;
            }
          }

          // imagePath
          //
          if (n.name () == "imagePath" && n.namespace_ ().empty ())
          {
            ::std::unique_ptr< ImagePathType > r (
              ImagePathTraits::create (i, f, this));

            if (!imagePath_.present ())
            {
              this->imagePath_.set (::std::move (r));
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

        if (!scale_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "scale",
            "");
        }

        if (!csys_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "csys",
            "");
        }

        if (!csysDragger_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "csysDragger",
            "");
        }

        if (!scaleLabel_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "scaleLabel",
            "");
        }

        if (!cornerVec_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "cornerVec",
            "");
        }

        if (!imagePath_.present ())
        {
          throw ::xsd::cxx::tree::expected_element< char > (
            "imagePath",
            "");
        }
      }

      ImagePlane* ImagePlane::
      _clone (::xml_schema::Flags f,
              ::xml_schema::Container* c) const
      {
        return new class ImagePlane (*this, f, c);
      }

      ImagePlane& ImagePlane::
      operator= (const ImagePlane& x)
      {
        if (this != &x)
        {
          static_cast< ::xml_schema::Type& > (*this) = x;
          this->base_ = x.base_;
          this->scale_ = x.scale_;
          this->csys_ = x.csys_;
          this->csysDragger_ = x.csysDragger_;
          this->scaleLabel_ = x.scaleLabel_;
          this->cornerVec_ = x.cornerVec_;
          this->imagePath_ = x.imagePath_;
        }

        return *this;
      }

      ImagePlane::
      ~ImagePlane ()
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
    namespace imps
    {
      ::std::unique_ptr< ::prj::srl::imps::ImagePlane >
      imageplane (const ::std::string& u,
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

        return ::std::unique_ptr< ::prj::srl::imps::ImagePlane > (
          ::prj::srl::imps::imageplane (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::imps::ImagePlane >
      imageplane (const ::std::string& u,
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

        return ::std::unique_ptr< ::prj::srl::imps::ImagePlane > (
          ::prj::srl::imps::imageplane (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::imps::ImagePlane >
      imageplane (const ::std::string& u,
                  ::xercesc::DOMErrorHandler& h,
                  ::xml_schema::Flags f,
                  const ::xml_schema::Properties& p)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            u, h, p, f));

        if (!d.get ())
          throw ::xsd::cxx::tree::parsing< char > ();

        return ::std::unique_ptr< ::prj::srl::imps::ImagePlane > (
          ::prj::srl::imps::imageplane (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::imps::ImagePlane >
      imageplane (::std::istream& is,
                  ::xml_schema::Flags f,
                  const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is);
        return ::prj::srl::imps::imageplane (isrc, f, p);
      }

      ::std::unique_ptr< ::prj::srl::imps::ImagePlane >
      imageplane (::std::istream& is,
                  ::xml_schema::ErrorHandler& h,
                  ::xml_schema::Flags f,
                  const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is);
        return ::prj::srl::imps::imageplane (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::imps::ImagePlane >
      imageplane (::std::istream& is,
                  ::xercesc::DOMErrorHandler& h,
                  ::xml_schema::Flags f,
                  const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::sax::std_input_source isrc (is);
        return ::prj::srl::imps::imageplane (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::imps::ImagePlane >
      imageplane (::std::istream& is,
                  const ::std::string& sid,
                  ::xml_schema::Flags f,
                  const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
        return ::prj::srl::imps::imageplane (isrc, f, p);
      }

      ::std::unique_ptr< ::prj::srl::imps::ImagePlane >
      imageplane (::std::istream& is,
                  const ::std::string& sid,
                  ::xml_schema::ErrorHandler& h,
                  ::xml_schema::Flags f,
                  const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0,
          (f & ::xml_schema::Flags::keep_dom) == 0);

        ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
        return ::prj::srl::imps::imageplane (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::imps::ImagePlane >
      imageplane (::std::istream& is,
                  const ::std::string& sid,
                  ::xercesc::DOMErrorHandler& h,
                  ::xml_schema::Flags f,
                  const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
        return ::prj::srl::imps::imageplane (isrc, h, f, p);
      }

      ::std::unique_ptr< ::prj::srl::imps::ImagePlane >
      imageplane (::xercesc::InputSource& i,
                  ::xml_schema::Flags f,
                  const ::xml_schema::Properties& p)
      {
        ::xsd::cxx::tree::error_handler< char > h;

        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            i, h, p, f));

        h.throw_if_failed< ::xsd::cxx::tree::parsing< char > > ();

        return ::std::unique_ptr< ::prj::srl::imps::ImagePlane > (
          ::prj::srl::imps::imageplane (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::imps::ImagePlane >
      imageplane (::xercesc::InputSource& i,
                  ::xml_schema::ErrorHandler& h,
                  ::xml_schema::Flags f,
                  const ::xml_schema::Properties& p)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            i, h, p, f));

        if (!d.get ())
          throw ::xsd::cxx::tree::parsing< char > ();

        return ::std::unique_ptr< ::prj::srl::imps::ImagePlane > (
          ::prj::srl::imps::imageplane (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::imps::ImagePlane >
      imageplane (::xercesc::InputSource& i,
                  ::xercesc::DOMErrorHandler& h,
                  ::xml_schema::Flags f,
                  const ::xml_schema::Properties& p)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::parse< char > (
            i, h, p, f));

        if (!d.get ())
          throw ::xsd::cxx::tree::parsing< char > ();

        return ::std::unique_ptr< ::prj::srl::imps::ImagePlane > (
          ::prj::srl::imps::imageplane (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      ::std::unique_ptr< ::prj::srl::imps::ImagePlane >
      imageplane (const ::xercesc::DOMDocument& doc,
                  ::xml_schema::Flags f,
                  const ::xml_schema::Properties& p)
      {
        if (f & ::xml_schema::Flags::keep_dom)
        {
          ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
            static_cast< ::xercesc::DOMDocument* > (doc.cloneNode (true)));

          return ::std::unique_ptr< ::prj::srl::imps::ImagePlane > (
            ::prj::srl::imps::imageplane (
              std::move (d), f | ::xml_schema::Flags::own_dom, p));
        }

        const ::xercesc::DOMElement& e (*doc.getDocumentElement ());
        const ::xsd::cxx::xml::qualified_name< char > n (
          ::xsd::cxx::xml::dom::name< char > (e));

        if (n.name () == "imageplane" &&
            n.namespace_ () == "http://www.cadseer.com/prj/srl/imps")
        {
          ::std::unique_ptr< ::prj::srl::imps::ImagePlane > r (
            ::xsd::cxx::tree::traits< ::prj::srl::imps::ImagePlane, char >::create (
              e, f, 0));
          return r;
        }

        throw ::xsd::cxx::tree::unexpected_element < char > (
          n.name (),
          n.namespace_ (),
          "imageplane",
          "http://www.cadseer.com/prj/srl/imps");
      }

      ::std::unique_ptr< ::prj::srl::imps::ImagePlane >
      imageplane (::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d,
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

        if (n.name () == "imageplane" &&
            n.namespace_ () == "http://www.cadseer.com/prj/srl/imps")
        {
          ::std::unique_ptr< ::prj::srl::imps::ImagePlane > r (
            ::xsd::cxx::tree::traits< ::prj::srl::imps::ImagePlane, char >::create (
              e, f, 0));
          return r;
        }

        throw ::xsd::cxx::tree::unexpected_element < char > (
          n.name (),
          n.namespace_ (),
          "imageplane",
          "http://www.cadseer.com/prj/srl/imps");
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
    namespace imps
    {
      void
      operator<< (::xercesc::DOMElement& e, const ImagePlane& i)
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

        // scale
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "scale",
              e));

          s << i.scale ();
        }

        // csys
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "csys",
              e));

          s << i.csys ();
        }

        // csysDragger
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "csysDragger",
              e));

          s << i.csysDragger ();
        }

        // scaleLabel
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "scaleLabel",
              e));

          s << i.scaleLabel ();
        }

        // cornerVec
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "cornerVec",
              e));

          s << i.cornerVec ();
        }

        // imagePath
        //
        {
          ::xercesc::DOMElement& s (
            ::xsd::cxx::xml::dom::create_element (
              "imagePath",
              e));

          s << i.imagePath ();
        }
      }

      void
      imageplane (::std::ostream& o,
                  const ::prj::srl::imps::ImagePlane& s,
                  const ::xml_schema::NamespaceInfomap& m,
                  const ::std::string& e,
                  ::xml_schema::Flags f)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0);

        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::imps::imageplane (s, m, f));

        ::xsd::cxx::tree::error_handler< char > h;

        ::xsd::cxx::xml::dom::ostream_format_target t (o);
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          h.throw_if_failed< ::xsd::cxx::tree::serialization< char > > ();
        }
      }

      void
      imageplane (::std::ostream& o,
                  const ::prj::srl::imps::ImagePlane& s,
                  ::xml_schema::ErrorHandler& h,
                  const ::xml_schema::NamespaceInfomap& m,
                  const ::std::string& e,
                  ::xml_schema::Flags f)
      {
        ::xsd::cxx::xml::auto_initializer i (
          (f & ::xml_schema::Flags::dont_initialize) == 0);

        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::imps::imageplane (s, m, f));
        ::xsd::cxx::xml::dom::ostream_format_target t (o);
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      imageplane (::std::ostream& o,
                  const ::prj::srl::imps::ImagePlane& s,
                  ::xercesc::DOMErrorHandler& h,
                  const ::xml_schema::NamespaceInfomap& m,
                  const ::std::string& e,
                  ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::imps::imageplane (s, m, f));
        ::xsd::cxx::xml::dom::ostream_format_target t (o);
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      imageplane (::xercesc::XMLFormatTarget& t,
                  const ::prj::srl::imps::ImagePlane& s,
                  const ::xml_schema::NamespaceInfomap& m,
                  const ::std::string& e,
                  ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::imps::imageplane (s, m, f));

        ::xsd::cxx::tree::error_handler< char > h;

        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          h.throw_if_failed< ::xsd::cxx::tree::serialization< char > > ();
        }
      }

      void
      imageplane (::xercesc::XMLFormatTarget& t,
                  const ::prj::srl::imps::ImagePlane& s,
                  ::xml_schema::ErrorHandler& h,
                  const ::xml_schema::NamespaceInfomap& m,
                  const ::std::string& e,
                  ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::imps::imageplane (s, m, f));
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      imageplane (::xercesc::XMLFormatTarget& t,
                  const ::prj::srl::imps::ImagePlane& s,
                  ::xercesc::DOMErrorHandler& h,
                  const ::xml_schema::NamespaceInfomap& m,
                  const ::std::string& e,
                  ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::prj::srl::imps::imageplane (s, m, f));
        if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
        {
          throw ::xsd::cxx::tree::serialization< char > ();
        }
      }

      void
      imageplane (::xercesc::DOMDocument& d,
                  const ::prj::srl::imps::ImagePlane& s,
                  ::xml_schema::Flags)
      {
        ::xercesc::DOMElement& e (*d.getDocumentElement ());
        const ::xsd::cxx::xml::qualified_name< char > n (
          ::xsd::cxx::xml::dom::name< char > (e));

        if (n.name () == "imageplane" &&
            n.namespace_ () == "http://www.cadseer.com/prj/srl/imps")
        {
          e << s;
        }
        else
        {
          throw ::xsd::cxx::tree::unexpected_element < char > (
            n.name (),
            n.namespace_ (),
            "imageplane",
            "http://www.cadseer.com/prj/srl/imps");
        }
      }

      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument >
      imageplane (const ::prj::srl::imps::ImagePlane& s,
                  const ::xml_schema::NamespaceInfomap& m,
                  ::xml_schema::Flags f)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          ::xsd::cxx::xml::dom::serialize< char > (
            "imageplane",
            "http://www.cadseer.com/prj/srl/imps",
            m, f));

        ::prj::srl::imps::imageplane (*d, s, f);
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


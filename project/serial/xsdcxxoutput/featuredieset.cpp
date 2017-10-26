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

#include "featuredieset.h"

namespace prj
{
  namespace srl
  {
    // FeatureDieSet
    // 

    const FeatureDieSet::FeatureBaseType& FeatureDieSet::
    featureBase () const
    {
      return this->featureBase_.get ();
    }

    FeatureDieSet::FeatureBaseType& FeatureDieSet::
    featureBase ()
    {
      return this->featureBase_.get ();
    }

    void FeatureDieSet::
    featureBase (const FeatureBaseType& x)
    {
      this->featureBase_.set (x);
    }

    void FeatureDieSet::
    featureBase (::std::unique_ptr< FeatureBaseType > x)
    {
      this->featureBase_.set (std::move (x));
    }

    const FeatureDieSet::LengthType& FeatureDieSet::
    length () const
    {
      return this->length_.get ();
    }

    FeatureDieSet::LengthType& FeatureDieSet::
    length ()
    {
      return this->length_.get ();
    }

    void FeatureDieSet::
    length (const LengthType& x)
    {
      this->length_.set (x);
    }

    void FeatureDieSet::
    length (::std::unique_ptr< LengthType > x)
    {
      this->length_.set (std::move (x));
    }

    const FeatureDieSet::LengthPaddingType& FeatureDieSet::
    lengthPadding () const
    {
      return this->lengthPadding_.get ();
    }

    FeatureDieSet::LengthPaddingType& FeatureDieSet::
    lengthPadding ()
    {
      return this->lengthPadding_.get ();
    }

    void FeatureDieSet::
    lengthPadding (const LengthPaddingType& x)
    {
      this->lengthPadding_.set (x);
    }

    void FeatureDieSet::
    lengthPadding (::std::unique_ptr< LengthPaddingType > x)
    {
      this->lengthPadding_.set (std::move (x));
    }

    const FeatureDieSet::WidthType& FeatureDieSet::
    width () const
    {
      return this->width_.get ();
    }

    FeatureDieSet::WidthType& FeatureDieSet::
    width ()
    {
      return this->width_.get ();
    }

    void FeatureDieSet::
    width (const WidthType& x)
    {
      this->width_.set (x);
    }

    void FeatureDieSet::
    width (::std::unique_ptr< WidthType > x)
    {
      this->width_.set (std::move (x));
    }

    const FeatureDieSet::WidthPaddingType& FeatureDieSet::
    widthPadding () const
    {
      return this->widthPadding_.get ();
    }

    FeatureDieSet::WidthPaddingType& FeatureDieSet::
    widthPadding ()
    {
      return this->widthPadding_.get ();
    }

    void FeatureDieSet::
    widthPadding (const WidthPaddingType& x)
    {
      this->widthPadding_.set (x);
    }

    void FeatureDieSet::
    widthPadding (::std::unique_ptr< WidthPaddingType > x)
    {
      this->widthPadding_.set (std::move (x));
    }

    const FeatureDieSet::OriginType& FeatureDieSet::
    origin () const
    {
      return this->origin_.get ();
    }

    FeatureDieSet::OriginType& FeatureDieSet::
    origin ()
    {
      return this->origin_.get ();
    }

    void FeatureDieSet::
    origin (const OriginType& x)
    {
      this->origin_.set (x);
    }

    void FeatureDieSet::
    origin (::std::unique_ptr< OriginType > x)
    {
      this->origin_.set (std::move (x));
    }

    const FeatureDieSet::AutoCalcType& FeatureDieSet::
    autoCalc () const
    {
      return this->autoCalc_.get ();
    }

    FeatureDieSet::AutoCalcType& FeatureDieSet::
    autoCalc ()
    {
      return this->autoCalc_.get ();
    }

    void FeatureDieSet::
    autoCalc (const AutoCalcType& x)
    {
      this->autoCalc_.set (x);
    }

    void FeatureDieSet::
    autoCalc (::std::unique_ptr< AutoCalcType > x)
    {
      this->autoCalc_.set (std::move (x));
    }
  }
}

#include <xsd/cxx/xml/dom/parsing-source.hxx>

namespace prj
{
  namespace srl
  {
    // FeatureDieSet
    //

    FeatureDieSet::
    FeatureDieSet (const FeatureBaseType& featureBase,
                   const LengthType& length,
                   const LengthPaddingType& lengthPadding,
                   const WidthType& width,
                   const WidthPaddingType& widthPadding,
                   const OriginType& origin,
                   const AutoCalcType& autoCalc)
    : ::xml_schema::Type (),
      featureBase_ (featureBase, this),
      length_ (length, this),
      lengthPadding_ (lengthPadding, this),
      width_ (width, this),
      widthPadding_ (widthPadding, this),
      origin_ (origin, this),
      autoCalc_ (autoCalc, this)
    {
    }

    FeatureDieSet::
    FeatureDieSet (::std::unique_ptr< FeatureBaseType > featureBase,
                   ::std::unique_ptr< LengthType > length,
                   ::std::unique_ptr< LengthPaddingType > lengthPadding,
                   ::std::unique_ptr< WidthType > width,
                   ::std::unique_ptr< WidthPaddingType > widthPadding,
                   ::std::unique_ptr< OriginType > origin,
                   ::std::unique_ptr< AutoCalcType > autoCalc)
    : ::xml_schema::Type (),
      featureBase_ (std::move (featureBase), this),
      length_ (std::move (length), this),
      lengthPadding_ (std::move (lengthPadding), this),
      width_ (std::move (width), this),
      widthPadding_ (std::move (widthPadding), this),
      origin_ (std::move (origin), this),
      autoCalc_ (std::move (autoCalc), this)
    {
    }

    FeatureDieSet::
    FeatureDieSet (const FeatureDieSet& x,
                   ::xml_schema::Flags f,
                   ::xml_schema::Container* c)
    : ::xml_schema::Type (x, f, c),
      featureBase_ (x.featureBase_, f, this),
      length_ (x.length_, f, this),
      lengthPadding_ (x.lengthPadding_, f, this),
      width_ (x.width_, f, this),
      widthPadding_ (x.widthPadding_, f, this),
      origin_ (x.origin_, f, this),
      autoCalc_ (x.autoCalc_, f, this)
    {
    }

    FeatureDieSet::
    FeatureDieSet (const ::xercesc::DOMElement& e,
                   ::xml_schema::Flags f,
                   ::xml_schema::Container* c)
    : ::xml_schema::Type (e, f | ::xml_schema::Flags::base, c),
      featureBase_ (this),
      length_ (this),
      lengthPadding_ (this),
      width_ (this),
      widthPadding_ (this),
      origin_ (this),
      autoCalc_ (this)
    {
      if ((f & ::xml_schema::Flags::base) == 0)
      {
        ::xsd::cxx::xml::dom::parser< char > p (e, true, false, false);
        this->parse (p, f);
      }
    }

    void FeatureDieSet::
    parse (::xsd::cxx::xml::dom::parser< char >& p,
           ::xml_schema::Flags f)
    {
      for (; p.more_content (); p.next_content (false))
      {
        const ::xercesc::DOMElement& i (p.cur_element ());
        const ::xsd::cxx::xml::qualified_name< char > n (
          ::xsd::cxx::xml::dom::name< char > (i));

        // featureBase
        //
        if (n.name () == "featureBase" && n.namespace_ ().empty ())
        {
          ::std::unique_ptr< FeatureBaseType > r (
            FeatureBaseTraits::create (i, f, this));

          if (!featureBase_.present ())
          {
            this->featureBase_.set (::std::move (r));
            continue;
          }
        }

        // length
        //
        if (n.name () == "length" && n.namespace_ ().empty ())
        {
          ::std::unique_ptr< LengthType > r (
            LengthTraits::create (i, f, this));

          if (!length_.present ())
          {
            this->length_.set (::std::move (r));
            continue;
          }
        }

        // lengthPadding
        //
        if (n.name () == "lengthPadding" && n.namespace_ ().empty ())
        {
          ::std::unique_ptr< LengthPaddingType > r (
            LengthPaddingTraits::create (i, f, this));

          if (!lengthPadding_.present ())
          {
            this->lengthPadding_.set (::std::move (r));
            continue;
          }
        }

        // width
        //
        if (n.name () == "width" && n.namespace_ ().empty ())
        {
          ::std::unique_ptr< WidthType > r (
            WidthTraits::create (i, f, this));

          if (!width_.present ())
          {
            this->width_.set (::std::move (r));
            continue;
          }
        }

        // widthPadding
        //
        if (n.name () == "widthPadding" && n.namespace_ ().empty ())
        {
          ::std::unique_ptr< WidthPaddingType > r (
            WidthPaddingTraits::create (i, f, this));

          if (!widthPadding_.present ())
          {
            this->widthPadding_.set (::std::move (r));
            continue;
          }
        }

        // origin
        //
        if (n.name () == "origin" && n.namespace_ ().empty ())
        {
          ::std::unique_ptr< OriginType > r (
            OriginTraits::create (i, f, this));

          if (!origin_.present ())
          {
            this->origin_.set (::std::move (r));
            continue;
          }
        }

        // autoCalc
        //
        if (n.name () == "autoCalc" && n.namespace_ ().empty ())
        {
          ::std::unique_ptr< AutoCalcType > r (
            AutoCalcTraits::create (i, f, this));

          if (!autoCalc_.present ())
          {
            this->autoCalc_.set (::std::move (r));
            continue;
          }
        }

        break;
      }

      if (!featureBase_.present ())
      {
        throw ::xsd::cxx::tree::expected_element< char > (
          "featureBase",
          "");
      }

      if (!length_.present ())
      {
        throw ::xsd::cxx::tree::expected_element< char > (
          "length",
          "");
      }

      if (!lengthPadding_.present ())
      {
        throw ::xsd::cxx::tree::expected_element< char > (
          "lengthPadding",
          "");
      }

      if (!width_.present ())
      {
        throw ::xsd::cxx::tree::expected_element< char > (
          "width",
          "");
      }

      if (!widthPadding_.present ())
      {
        throw ::xsd::cxx::tree::expected_element< char > (
          "widthPadding",
          "");
      }

      if (!origin_.present ())
      {
        throw ::xsd::cxx::tree::expected_element< char > (
          "origin",
          "");
      }

      if (!autoCalc_.present ())
      {
        throw ::xsd::cxx::tree::expected_element< char > (
          "autoCalc",
          "");
      }
    }

    FeatureDieSet* FeatureDieSet::
    _clone (::xml_schema::Flags f,
            ::xml_schema::Container* c) const
    {
      return new class FeatureDieSet (*this, f, c);
    }

    FeatureDieSet& FeatureDieSet::
    operator= (const FeatureDieSet& x)
    {
      if (this != &x)
      {
        static_cast< ::xml_schema::Type& > (*this) = x;
        this->featureBase_ = x.featureBase_;
        this->length_ = x.length_;
        this->lengthPadding_ = x.lengthPadding_;
        this->width_ = x.width_;
        this->widthPadding_ = x.widthPadding_;
        this->origin_ = x.origin_;
        this->autoCalc_ = x.autoCalc_;
      }

      return *this;
    }

    FeatureDieSet::
    ~FeatureDieSet ()
    {
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
    ::std::unique_ptr< ::prj::srl::FeatureDieSet >
    dieset (const ::std::string& u,
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

      return ::std::unique_ptr< ::prj::srl::FeatureDieSet > (
        ::prj::srl::dieset (
          std::move (d), f | ::xml_schema::Flags::own_dom, p));
    }

    ::std::unique_ptr< ::prj::srl::FeatureDieSet >
    dieset (const ::std::string& u,
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

      return ::std::unique_ptr< ::prj::srl::FeatureDieSet > (
        ::prj::srl::dieset (
          std::move (d), f | ::xml_schema::Flags::own_dom, p));
    }

    ::std::unique_ptr< ::prj::srl::FeatureDieSet >
    dieset (const ::std::string& u,
            ::xercesc::DOMErrorHandler& h,
            ::xml_schema::Flags f,
            const ::xml_schema::Properties& p)
    {
      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::xsd::cxx::xml::dom::parse< char > (
          u, h, p, f));

      if (!d.get ())
        throw ::xsd::cxx::tree::parsing< char > ();

      return ::std::unique_ptr< ::prj::srl::FeatureDieSet > (
        ::prj::srl::dieset (
          std::move (d), f | ::xml_schema::Flags::own_dom, p));
    }

    ::std::unique_ptr< ::prj::srl::FeatureDieSet >
    dieset (::std::istream& is,
            ::xml_schema::Flags f,
            const ::xml_schema::Properties& p)
    {
      ::xsd::cxx::xml::auto_initializer i (
        (f & ::xml_schema::Flags::dont_initialize) == 0,
        (f & ::xml_schema::Flags::keep_dom) == 0);

      ::xsd::cxx::xml::sax::std_input_source isrc (is);
      return ::prj::srl::dieset (isrc, f, p);
    }

    ::std::unique_ptr< ::prj::srl::FeatureDieSet >
    dieset (::std::istream& is,
            ::xml_schema::ErrorHandler& h,
            ::xml_schema::Flags f,
            const ::xml_schema::Properties& p)
    {
      ::xsd::cxx::xml::auto_initializer i (
        (f & ::xml_schema::Flags::dont_initialize) == 0,
        (f & ::xml_schema::Flags::keep_dom) == 0);

      ::xsd::cxx::xml::sax::std_input_source isrc (is);
      return ::prj::srl::dieset (isrc, h, f, p);
    }

    ::std::unique_ptr< ::prj::srl::FeatureDieSet >
    dieset (::std::istream& is,
            ::xercesc::DOMErrorHandler& h,
            ::xml_schema::Flags f,
            const ::xml_schema::Properties& p)
    {
      ::xsd::cxx::xml::sax::std_input_source isrc (is);
      return ::prj::srl::dieset (isrc, h, f, p);
    }

    ::std::unique_ptr< ::prj::srl::FeatureDieSet >
    dieset (::std::istream& is,
            const ::std::string& sid,
            ::xml_schema::Flags f,
            const ::xml_schema::Properties& p)
    {
      ::xsd::cxx::xml::auto_initializer i (
        (f & ::xml_schema::Flags::dont_initialize) == 0,
        (f & ::xml_schema::Flags::keep_dom) == 0);

      ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
      return ::prj::srl::dieset (isrc, f, p);
    }

    ::std::unique_ptr< ::prj::srl::FeatureDieSet >
    dieset (::std::istream& is,
            const ::std::string& sid,
            ::xml_schema::ErrorHandler& h,
            ::xml_schema::Flags f,
            const ::xml_schema::Properties& p)
    {
      ::xsd::cxx::xml::auto_initializer i (
        (f & ::xml_schema::Flags::dont_initialize) == 0,
        (f & ::xml_schema::Flags::keep_dom) == 0);

      ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
      return ::prj::srl::dieset (isrc, h, f, p);
    }

    ::std::unique_ptr< ::prj::srl::FeatureDieSet >
    dieset (::std::istream& is,
            const ::std::string& sid,
            ::xercesc::DOMErrorHandler& h,
            ::xml_schema::Flags f,
            const ::xml_schema::Properties& p)
    {
      ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
      return ::prj::srl::dieset (isrc, h, f, p);
    }

    ::std::unique_ptr< ::prj::srl::FeatureDieSet >
    dieset (::xercesc::InputSource& i,
            ::xml_schema::Flags f,
            const ::xml_schema::Properties& p)
    {
      ::xsd::cxx::tree::error_handler< char > h;

      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::xsd::cxx::xml::dom::parse< char > (
          i, h, p, f));

      h.throw_if_failed< ::xsd::cxx::tree::parsing< char > > ();

      return ::std::unique_ptr< ::prj::srl::FeatureDieSet > (
        ::prj::srl::dieset (
          std::move (d), f | ::xml_schema::Flags::own_dom, p));
    }

    ::std::unique_ptr< ::prj::srl::FeatureDieSet >
    dieset (::xercesc::InputSource& i,
            ::xml_schema::ErrorHandler& h,
            ::xml_schema::Flags f,
            const ::xml_schema::Properties& p)
    {
      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::xsd::cxx::xml::dom::parse< char > (
          i, h, p, f));

      if (!d.get ())
        throw ::xsd::cxx::tree::parsing< char > ();

      return ::std::unique_ptr< ::prj::srl::FeatureDieSet > (
        ::prj::srl::dieset (
          std::move (d), f | ::xml_schema::Flags::own_dom, p));
    }

    ::std::unique_ptr< ::prj::srl::FeatureDieSet >
    dieset (::xercesc::InputSource& i,
            ::xercesc::DOMErrorHandler& h,
            ::xml_schema::Flags f,
            const ::xml_schema::Properties& p)
    {
      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::xsd::cxx::xml::dom::parse< char > (
          i, h, p, f));

      if (!d.get ())
        throw ::xsd::cxx::tree::parsing< char > ();

      return ::std::unique_ptr< ::prj::srl::FeatureDieSet > (
        ::prj::srl::dieset (
          std::move (d), f | ::xml_schema::Flags::own_dom, p));
    }

    ::std::unique_ptr< ::prj::srl::FeatureDieSet >
    dieset (const ::xercesc::DOMDocument& doc,
            ::xml_schema::Flags f,
            const ::xml_schema::Properties& p)
    {
      if (f & ::xml_schema::Flags::keep_dom)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          static_cast< ::xercesc::DOMDocument* > (doc.cloneNode (true)));

        return ::std::unique_ptr< ::prj::srl::FeatureDieSet > (
          ::prj::srl::dieset (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      const ::xercesc::DOMElement& e (*doc.getDocumentElement ());
      const ::xsd::cxx::xml::qualified_name< char > n (
        ::xsd::cxx::xml::dom::name< char > (e));

      if (n.name () == "dieset" &&
          n.namespace_ () == "http://www.cadseer.com/prj/srl")
      {
        ::std::unique_ptr< ::prj::srl::FeatureDieSet > r (
          ::xsd::cxx::tree::traits< ::prj::srl::FeatureDieSet, char >::create (
            e, f, 0));
        return r;
      }

      throw ::xsd::cxx::tree::unexpected_element < char > (
        n.name (),
        n.namespace_ (),
        "dieset",
        "http://www.cadseer.com/prj/srl");
    }

    ::std::unique_ptr< ::prj::srl::FeatureDieSet >
    dieset (::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d,
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

      if (n.name () == "dieset" &&
          n.namespace_ () == "http://www.cadseer.com/prj/srl")
      {
        ::std::unique_ptr< ::prj::srl::FeatureDieSet > r (
          ::xsd::cxx::tree::traits< ::prj::srl::FeatureDieSet, char >::create (
            e, f, 0));
        return r;
      }

      throw ::xsd::cxx::tree::unexpected_element < char > (
        n.name (),
        n.namespace_ (),
        "dieset",
        "http://www.cadseer.com/prj/srl");
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
    void
    operator<< (::xercesc::DOMElement& e, const FeatureDieSet& i)
    {
      e << static_cast< const ::xml_schema::Type& > (i);

      // featureBase
      //
      {
        ::xercesc::DOMElement& s (
          ::xsd::cxx::xml::dom::create_element (
            "featureBase",
            e));

        s << i.featureBase ();
      }

      // length
      //
      {
        ::xercesc::DOMElement& s (
          ::xsd::cxx::xml::dom::create_element (
            "length",
            e));

        s << i.length ();
      }

      // lengthPadding
      //
      {
        ::xercesc::DOMElement& s (
          ::xsd::cxx::xml::dom::create_element (
            "lengthPadding",
            e));

        s << i.lengthPadding ();
      }

      // width
      //
      {
        ::xercesc::DOMElement& s (
          ::xsd::cxx::xml::dom::create_element (
            "width",
            e));

        s << i.width ();
      }

      // widthPadding
      //
      {
        ::xercesc::DOMElement& s (
          ::xsd::cxx::xml::dom::create_element (
            "widthPadding",
            e));

        s << i.widthPadding ();
      }

      // origin
      //
      {
        ::xercesc::DOMElement& s (
          ::xsd::cxx::xml::dom::create_element (
            "origin",
            e));

        s << i.origin ();
      }

      // autoCalc
      //
      {
        ::xercesc::DOMElement& s (
          ::xsd::cxx::xml::dom::create_element (
            "autoCalc",
            e));

        s << i.autoCalc ();
      }
    }

    void
    dieset (::std::ostream& o,
            const ::prj::srl::FeatureDieSet& s,
            const ::xml_schema::NamespaceInfomap& m,
            const ::std::string& e,
            ::xml_schema::Flags f)
    {
      ::xsd::cxx::xml::auto_initializer i (
        (f & ::xml_schema::Flags::dont_initialize) == 0);

      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::prj::srl::dieset (s, m, f));

      ::xsd::cxx::tree::error_handler< char > h;

      ::xsd::cxx::xml::dom::ostream_format_target t (o);
      if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
      {
        h.throw_if_failed< ::xsd::cxx::tree::serialization< char > > ();
      }
    }

    void
    dieset (::std::ostream& o,
            const ::prj::srl::FeatureDieSet& s,
            ::xml_schema::ErrorHandler& h,
            const ::xml_schema::NamespaceInfomap& m,
            const ::std::string& e,
            ::xml_schema::Flags f)
    {
      ::xsd::cxx::xml::auto_initializer i (
        (f & ::xml_schema::Flags::dont_initialize) == 0);

      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::prj::srl::dieset (s, m, f));
      ::xsd::cxx::xml::dom::ostream_format_target t (o);
      if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
      {
        throw ::xsd::cxx::tree::serialization< char > ();
      }
    }

    void
    dieset (::std::ostream& o,
            const ::prj::srl::FeatureDieSet& s,
            ::xercesc::DOMErrorHandler& h,
            const ::xml_schema::NamespaceInfomap& m,
            const ::std::string& e,
            ::xml_schema::Flags f)
    {
      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::prj::srl::dieset (s, m, f));
      ::xsd::cxx::xml::dom::ostream_format_target t (o);
      if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
      {
        throw ::xsd::cxx::tree::serialization< char > ();
      }
    }

    void
    dieset (::xercesc::XMLFormatTarget& t,
            const ::prj::srl::FeatureDieSet& s,
            const ::xml_schema::NamespaceInfomap& m,
            const ::std::string& e,
            ::xml_schema::Flags f)
    {
      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::prj::srl::dieset (s, m, f));

      ::xsd::cxx::tree::error_handler< char > h;

      if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
      {
        h.throw_if_failed< ::xsd::cxx::tree::serialization< char > > ();
      }
    }

    void
    dieset (::xercesc::XMLFormatTarget& t,
            const ::prj::srl::FeatureDieSet& s,
            ::xml_schema::ErrorHandler& h,
            const ::xml_schema::NamespaceInfomap& m,
            const ::std::string& e,
            ::xml_schema::Flags f)
    {
      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::prj::srl::dieset (s, m, f));
      if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
      {
        throw ::xsd::cxx::tree::serialization< char > ();
      }
    }

    void
    dieset (::xercesc::XMLFormatTarget& t,
            const ::prj::srl::FeatureDieSet& s,
            ::xercesc::DOMErrorHandler& h,
            const ::xml_schema::NamespaceInfomap& m,
            const ::std::string& e,
            ::xml_schema::Flags f)
    {
      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::prj::srl::dieset (s, m, f));
      if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
      {
        throw ::xsd::cxx::tree::serialization< char > ();
      }
    }

    void
    dieset (::xercesc::DOMDocument& d,
            const ::prj::srl::FeatureDieSet& s,
            ::xml_schema::Flags)
    {
      ::xercesc::DOMElement& e (*d.getDocumentElement ());
      const ::xsd::cxx::xml::qualified_name< char > n (
        ::xsd::cxx::xml::dom::name< char > (e));

      if (n.name () == "dieset" &&
          n.namespace_ () == "http://www.cadseer.com/prj/srl")
      {
        e << s;
      }
      else
      {
        throw ::xsd::cxx::tree::unexpected_element < char > (
          n.name (),
          n.namespace_ (),
          "dieset",
          "http://www.cadseer.com/prj/srl");
      }
    }

    ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument >
    dieset (const ::prj::srl::FeatureDieSet& s,
            const ::xml_schema::NamespaceInfomap& m,
            ::xml_schema::Flags f)
    {
      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::xsd::cxx::xml::dom::serialize< char > (
          "dieset",
          "http://www.cadseer.com/prj/srl",
          m, f));

      ::prj::srl::dieset (*d, s, f);
      return d;
    }
  }
}

#include <xsd/cxx/post.hxx>

// Begin epilogue.
//
//
// End epilogue.


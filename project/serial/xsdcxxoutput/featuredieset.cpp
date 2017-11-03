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

    const FeatureDieSet::LengthPLabelType& FeatureDieSet::
    lengthPLabel () const
    {
      return this->lengthPLabel_.get ();
    }

    FeatureDieSet::LengthPLabelType& FeatureDieSet::
    lengthPLabel ()
    {
      return this->lengthPLabel_.get ();
    }

    void FeatureDieSet::
    lengthPLabel (const LengthPLabelType& x)
    {
      this->lengthPLabel_.set (x);
    }

    void FeatureDieSet::
    lengthPLabel (::std::unique_ptr< LengthPLabelType > x)
    {
      this->lengthPLabel_.set (std::move (x));
    }

    const FeatureDieSet::WidthPLabelType& FeatureDieSet::
    widthPLabel () const
    {
      return this->widthPLabel_.get ();
    }

    FeatureDieSet::WidthPLabelType& FeatureDieSet::
    widthPLabel ()
    {
      return this->widthPLabel_.get ();
    }

    void FeatureDieSet::
    widthPLabel (const WidthPLabelType& x)
    {
      this->widthPLabel_.set (x);
    }

    void FeatureDieSet::
    widthPLabel (::std::unique_ptr< WidthPLabelType > x)
    {
      this->widthPLabel_.set (std::move (x));
    }

    const FeatureDieSet::LengthPaddingPLabelType& FeatureDieSet::
    lengthPaddingPLabel () const
    {
      return this->lengthPaddingPLabel_.get ();
    }

    FeatureDieSet::LengthPaddingPLabelType& FeatureDieSet::
    lengthPaddingPLabel ()
    {
      return this->lengthPaddingPLabel_.get ();
    }

    void FeatureDieSet::
    lengthPaddingPLabel (const LengthPaddingPLabelType& x)
    {
      this->lengthPaddingPLabel_.set (x);
    }

    void FeatureDieSet::
    lengthPaddingPLabel (::std::unique_ptr< LengthPaddingPLabelType > x)
    {
      this->lengthPaddingPLabel_.set (std::move (x));
    }

    const FeatureDieSet::WidthPaddingPLabelType& FeatureDieSet::
    widthPaddingPLabel () const
    {
      return this->widthPaddingPLabel_.get ();
    }

    FeatureDieSet::WidthPaddingPLabelType& FeatureDieSet::
    widthPaddingPLabel ()
    {
      return this->widthPaddingPLabel_.get ();
    }

    void FeatureDieSet::
    widthPaddingPLabel (const WidthPaddingPLabelType& x)
    {
      this->widthPaddingPLabel_.set (x);
    }

    void FeatureDieSet::
    widthPaddingPLabel (::std::unique_ptr< WidthPaddingPLabelType > x)
    {
      this->widthPaddingPLabel_.set (std::move (x));
    }

    const FeatureDieSet::OriginPLabelType& FeatureDieSet::
    originPLabel () const
    {
      return this->originPLabel_.get ();
    }

    FeatureDieSet::OriginPLabelType& FeatureDieSet::
    originPLabel ()
    {
      return this->originPLabel_.get ();
    }

    void FeatureDieSet::
    originPLabel (const OriginPLabelType& x)
    {
      this->originPLabel_.set (x);
    }

    void FeatureDieSet::
    originPLabel (::std::unique_ptr< OriginPLabelType > x)
    {
      this->originPLabel_.set (std::move (x));
    }

    const FeatureDieSet::AutoCalcPLabelType& FeatureDieSet::
    autoCalcPLabel () const
    {
      return this->autoCalcPLabel_.get ();
    }

    FeatureDieSet::AutoCalcPLabelType& FeatureDieSet::
    autoCalcPLabel ()
    {
      return this->autoCalcPLabel_.get ();
    }

    void FeatureDieSet::
    autoCalcPLabel (const AutoCalcPLabelType& x)
    {
      this->autoCalcPLabel_.set (x);
    }

    void FeatureDieSet::
    autoCalcPLabel (::std::unique_ptr< AutoCalcPLabelType > x)
    {
      this->autoCalcPLabel_.set (std::move (x));
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
                   const AutoCalcType& autoCalc,
                   const LengthPLabelType& lengthPLabel,
                   const WidthPLabelType& widthPLabel,
                   const LengthPaddingPLabelType& lengthPaddingPLabel,
                   const WidthPaddingPLabelType& widthPaddingPLabel,
                   const OriginPLabelType& originPLabel,
                   const AutoCalcPLabelType& autoCalcPLabel)
    : ::xml_schema::Type (),
      featureBase_ (featureBase, this),
      length_ (length, this),
      lengthPadding_ (lengthPadding, this),
      width_ (width, this),
      widthPadding_ (widthPadding, this),
      origin_ (origin, this),
      autoCalc_ (autoCalc, this),
      lengthPLabel_ (lengthPLabel, this),
      widthPLabel_ (widthPLabel, this),
      lengthPaddingPLabel_ (lengthPaddingPLabel, this),
      widthPaddingPLabel_ (widthPaddingPLabel, this),
      originPLabel_ (originPLabel, this),
      autoCalcPLabel_ (autoCalcPLabel, this)
    {
    }

    FeatureDieSet::
    FeatureDieSet (::std::unique_ptr< FeatureBaseType > featureBase,
                   ::std::unique_ptr< LengthType > length,
                   ::std::unique_ptr< LengthPaddingType > lengthPadding,
                   ::std::unique_ptr< WidthType > width,
                   ::std::unique_ptr< WidthPaddingType > widthPadding,
                   ::std::unique_ptr< OriginType > origin,
                   ::std::unique_ptr< AutoCalcType > autoCalc,
                   ::std::unique_ptr< LengthPLabelType > lengthPLabel,
                   ::std::unique_ptr< WidthPLabelType > widthPLabel,
                   ::std::unique_ptr< LengthPaddingPLabelType > lengthPaddingPLabel,
                   ::std::unique_ptr< WidthPaddingPLabelType > widthPaddingPLabel,
                   ::std::unique_ptr< OriginPLabelType > originPLabel,
                   ::std::unique_ptr< AutoCalcPLabelType > autoCalcPLabel)
    : ::xml_schema::Type (),
      featureBase_ (std::move (featureBase), this),
      length_ (std::move (length), this),
      lengthPadding_ (std::move (lengthPadding), this),
      width_ (std::move (width), this),
      widthPadding_ (std::move (widthPadding), this),
      origin_ (std::move (origin), this),
      autoCalc_ (std::move (autoCalc), this),
      lengthPLabel_ (std::move (lengthPLabel), this),
      widthPLabel_ (std::move (widthPLabel), this),
      lengthPaddingPLabel_ (std::move (lengthPaddingPLabel), this),
      widthPaddingPLabel_ (std::move (widthPaddingPLabel), this),
      originPLabel_ (std::move (originPLabel), this),
      autoCalcPLabel_ (std::move (autoCalcPLabel), this)
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
      autoCalc_ (x.autoCalc_, f, this),
      lengthPLabel_ (x.lengthPLabel_, f, this),
      widthPLabel_ (x.widthPLabel_, f, this),
      lengthPaddingPLabel_ (x.lengthPaddingPLabel_, f, this),
      widthPaddingPLabel_ (x.widthPaddingPLabel_, f, this),
      originPLabel_ (x.originPLabel_, f, this),
      autoCalcPLabel_ (x.autoCalcPLabel_, f, this)
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
      autoCalc_ (this),
      lengthPLabel_ (this),
      widthPLabel_ (this),
      lengthPaddingPLabel_ (this),
      widthPaddingPLabel_ (this),
      originPLabel_ (this),
      autoCalcPLabel_ (this)
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

        // lengthPLabel
        //
        if (n.name () == "lengthPLabel" && n.namespace_ ().empty ())
        {
          ::std::unique_ptr< LengthPLabelType > r (
            LengthPLabelTraits::create (i, f, this));

          if (!lengthPLabel_.present ())
          {
            this->lengthPLabel_.set (::std::move (r));
            continue;
          }
        }

        // widthPLabel
        //
        if (n.name () == "widthPLabel" && n.namespace_ ().empty ())
        {
          ::std::unique_ptr< WidthPLabelType > r (
            WidthPLabelTraits::create (i, f, this));

          if (!widthPLabel_.present ())
          {
            this->widthPLabel_.set (::std::move (r));
            continue;
          }
        }

        // lengthPaddingPLabel
        //
        if (n.name () == "lengthPaddingPLabel" && n.namespace_ ().empty ())
        {
          ::std::unique_ptr< LengthPaddingPLabelType > r (
            LengthPaddingPLabelTraits::create (i, f, this));

          if (!lengthPaddingPLabel_.present ())
          {
            this->lengthPaddingPLabel_.set (::std::move (r));
            continue;
          }
        }

        // widthPaddingPLabel
        //
        if (n.name () == "widthPaddingPLabel" && n.namespace_ ().empty ())
        {
          ::std::unique_ptr< WidthPaddingPLabelType > r (
            WidthPaddingPLabelTraits::create (i, f, this));

          if (!widthPaddingPLabel_.present ())
          {
            this->widthPaddingPLabel_.set (::std::move (r));
            continue;
          }
        }

        // originPLabel
        //
        if (n.name () == "originPLabel" && n.namespace_ ().empty ())
        {
          ::std::unique_ptr< OriginPLabelType > r (
            OriginPLabelTraits::create (i, f, this));

          if (!originPLabel_.present ())
          {
            this->originPLabel_.set (::std::move (r));
            continue;
          }
        }

        // autoCalcPLabel
        //
        if (n.name () == "autoCalcPLabel" && n.namespace_ ().empty ())
        {
          ::std::unique_ptr< AutoCalcPLabelType > r (
            AutoCalcPLabelTraits::create (i, f, this));

          if (!autoCalcPLabel_.present ())
          {
            this->autoCalcPLabel_.set (::std::move (r));
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

      if (!lengthPLabel_.present ())
      {
        throw ::xsd::cxx::tree::expected_element< char > (
          "lengthPLabel",
          "");
      }

      if (!widthPLabel_.present ())
      {
        throw ::xsd::cxx::tree::expected_element< char > (
          "widthPLabel",
          "");
      }

      if (!lengthPaddingPLabel_.present ())
      {
        throw ::xsd::cxx::tree::expected_element< char > (
          "lengthPaddingPLabel",
          "");
      }

      if (!widthPaddingPLabel_.present ())
      {
        throw ::xsd::cxx::tree::expected_element< char > (
          "widthPaddingPLabel",
          "");
      }

      if (!originPLabel_.present ())
      {
        throw ::xsd::cxx::tree::expected_element< char > (
          "originPLabel",
          "");
      }

      if (!autoCalcPLabel_.present ())
      {
        throw ::xsd::cxx::tree::expected_element< char > (
          "autoCalcPLabel",
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
        this->lengthPLabel_ = x.lengthPLabel_;
        this->widthPLabel_ = x.widthPLabel_;
        this->lengthPaddingPLabel_ = x.lengthPaddingPLabel_;
        this->widthPaddingPLabel_ = x.widthPaddingPLabel_;
        this->originPLabel_ = x.originPLabel_;
        this->autoCalcPLabel_ = x.autoCalcPLabel_;
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

      // lengthPLabel
      //
      {
        ::xercesc::DOMElement& s (
          ::xsd::cxx::xml::dom::create_element (
            "lengthPLabel",
            e));

        s << i.lengthPLabel ();
      }

      // widthPLabel
      //
      {
        ::xercesc::DOMElement& s (
          ::xsd::cxx::xml::dom::create_element (
            "widthPLabel",
            e));

        s << i.widthPLabel ();
      }

      // lengthPaddingPLabel
      //
      {
        ::xercesc::DOMElement& s (
          ::xsd::cxx::xml::dom::create_element (
            "lengthPaddingPLabel",
            e));

        s << i.lengthPaddingPLabel ();
      }

      // widthPaddingPLabel
      //
      {
        ::xercesc::DOMElement& s (
          ::xsd::cxx::xml::dom::create_element (
            "widthPaddingPLabel",
            e));

        s << i.widthPaddingPLabel ();
      }

      // originPLabel
      //
      {
        ::xercesc::DOMElement& s (
          ::xsd::cxx::xml::dom::create_element (
            "originPLabel",
            e));

        s << i.originPLabel ();
      }

      // autoCalcPLabel
      //
      {
        ::xercesc::DOMElement& s (
          ::xsd::cxx::xml::dom::create_element (
            "autoCalcPLabel",
            e));

        s << i.autoCalcPLabel ();
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

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

#include "featurenest.h"

namespace prj
{
  namespace srl
  {
    // FeatureNest
    // 

    const FeatureNest::FeatureBaseType& FeatureNest::
    featureBase () const
    {
      return this->featureBase_.get ();
    }

    FeatureNest::FeatureBaseType& FeatureNest::
    featureBase ()
    {
      return this->featureBase_.get ();
    }

    void FeatureNest::
    featureBase (const FeatureBaseType& x)
    {
      this->featureBase_.set (x);
    }

    void FeatureNest::
    featureBase (::std::unique_ptr< FeatureBaseType > x)
    {
      this->featureBase_.set (std::move (x));
    }

    const FeatureNest::GapType& FeatureNest::
    gap () const
    {
      return this->gap_.get ();
    }

    FeatureNest::GapType& FeatureNest::
    gap ()
    {
      return this->gap_.get ();
    }

    void FeatureNest::
    gap (const GapType& x)
    {
      this->gap_.set (x);
    }

    void FeatureNest::
    gap (::std::unique_ptr< GapType > x)
    {
      this->gap_.set (std::move (x));
    }

    const FeatureNest::FeedDirectionType& FeatureNest::
    feedDirection () const
    {
      return this->feedDirection_.get ();
    }

    FeatureNest::FeedDirectionType& FeatureNest::
    feedDirection ()
    {
      return this->feedDirection_.get ();
    }

    void FeatureNest::
    feedDirection (const FeedDirectionType& x)
    {
      this->feedDirection_.set (x);
    }

    void FeatureNest::
    feedDirection (::std::unique_ptr< FeedDirectionType > x)
    {
      this->feedDirection_.set (std::move (x));
    }

    const FeatureNest::GapLabelType& FeatureNest::
    gapLabel () const
    {
      return this->gapLabel_.get ();
    }

    FeatureNest::GapLabelType& FeatureNest::
    gapLabel ()
    {
      return this->gapLabel_.get ();
    }

    void FeatureNest::
    gapLabel (const GapLabelType& x)
    {
      this->gapLabel_.set (x);
    }

    void FeatureNest::
    gapLabel (::std::unique_ptr< GapLabelType > x)
    {
      this->gapLabel_.set (std::move (x));
    }

    const FeatureNest::FeedDirectionLabelType& FeatureNest::
    feedDirectionLabel () const
    {
      return this->feedDirectionLabel_.get ();
    }

    FeatureNest::FeedDirectionLabelType& FeatureNest::
    feedDirectionLabel ()
    {
      return this->feedDirectionLabel_.get ();
    }

    void FeatureNest::
    feedDirectionLabel (const FeedDirectionLabelType& x)
    {
      this->feedDirectionLabel_.set (x);
    }

    void FeatureNest::
    feedDirectionLabel (::std::unique_ptr< FeedDirectionLabelType > x)
    {
      this->feedDirectionLabel_.set (std::move (x));
    }

    const FeatureNest::PitchType& FeatureNest::
    pitch () const
    {
      return this->pitch_.get ();
    }

    FeatureNest::PitchType& FeatureNest::
    pitch ()
    {
      return this->pitch_.get ();
    }

    void FeatureNest::
    pitch (const PitchType& x)
    {
      this->pitch_.set (x);
    }
  }
}

#include <xsd/cxx/xml/dom/parsing-source.hxx>

namespace prj
{
  namespace srl
  {
    // FeatureNest
    //

    FeatureNest::
    FeatureNest (const FeatureBaseType& featureBase,
                 const GapType& gap,
                 const FeedDirectionType& feedDirection,
                 const GapLabelType& gapLabel,
                 const FeedDirectionLabelType& feedDirectionLabel,
                 const PitchType& pitch)
    : ::xml_schema::Type (),
      featureBase_ (featureBase, this),
      gap_ (gap, this),
      feedDirection_ (feedDirection, this),
      gapLabel_ (gapLabel, this),
      feedDirectionLabel_ (feedDirectionLabel, this),
      pitch_ (pitch, this)
    {
    }

    FeatureNest::
    FeatureNest (::std::unique_ptr< FeatureBaseType > featureBase,
                 ::std::unique_ptr< GapType > gap,
                 ::std::unique_ptr< FeedDirectionType > feedDirection,
                 ::std::unique_ptr< GapLabelType > gapLabel,
                 ::std::unique_ptr< FeedDirectionLabelType > feedDirectionLabel,
                 const PitchType& pitch)
    : ::xml_schema::Type (),
      featureBase_ (std::move (featureBase), this),
      gap_ (std::move (gap), this),
      feedDirection_ (std::move (feedDirection), this),
      gapLabel_ (std::move (gapLabel), this),
      feedDirectionLabel_ (std::move (feedDirectionLabel), this),
      pitch_ (pitch, this)
    {
    }

    FeatureNest::
    FeatureNest (const FeatureNest& x,
                 ::xml_schema::Flags f,
                 ::xml_schema::Container* c)
    : ::xml_schema::Type (x, f, c),
      featureBase_ (x.featureBase_, f, this),
      gap_ (x.gap_, f, this),
      feedDirection_ (x.feedDirection_, f, this),
      gapLabel_ (x.gapLabel_, f, this),
      feedDirectionLabel_ (x.feedDirectionLabel_, f, this),
      pitch_ (x.pitch_, f, this)
    {
    }

    FeatureNest::
    FeatureNest (const ::xercesc::DOMElement& e,
                 ::xml_schema::Flags f,
                 ::xml_schema::Container* c)
    : ::xml_schema::Type (e, f | ::xml_schema::Flags::base, c),
      featureBase_ (this),
      gap_ (this),
      feedDirection_ (this),
      gapLabel_ (this),
      feedDirectionLabel_ (this),
      pitch_ (this)
    {
      if ((f & ::xml_schema::Flags::base) == 0)
      {
        ::xsd::cxx::xml::dom::parser< char > p (e, true, false, false);
        this->parse (p, f);
      }
    }

    void FeatureNest::
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

        // gap
        //
        if (n.name () == "gap" && n.namespace_ ().empty ())
        {
          ::std::unique_ptr< GapType > r (
            GapTraits::create (i, f, this));

          if (!gap_.present ())
          {
            this->gap_.set (::std::move (r));
            continue;
          }
        }

        // feedDirection
        //
        if (n.name () == "feedDirection" && n.namespace_ ().empty ())
        {
          ::std::unique_ptr< FeedDirectionType > r (
            FeedDirectionTraits::create (i, f, this));

          if (!feedDirection_.present ())
          {
            this->feedDirection_.set (::std::move (r));
            continue;
          }
        }

        // gapLabel
        //
        if (n.name () == "gapLabel" && n.namespace_ ().empty ())
        {
          ::std::unique_ptr< GapLabelType > r (
            GapLabelTraits::create (i, f, this));

          if (!gapLabel_.present ())
          {
            this->gapLabel_.set (::std::move (r));
            continue;
          }
        }

        // feedDirectionLabel
        //
        if (n.name () == "feedDirectionLabel" && n.namespace_ ().empty ())
        {
          ::std::unique_ptr< FeedDirectionLabelType > r (
            FeedDirectionLabelTraits::create (i, f, this));

          if (!feedDirectionLabel_.present ())
          {
            this->feedDirectionLabel_.set (::std::move (r));
            continue;
          }
        }

        // pitch
        //
        if (n.name () == "pitch" && n.namespace_ ().empty ())
        {
          if (!pitch_.present ())
          {
            this->pitch_.set (PitchTraits::create (i, f, this));
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

      if (!gap_.present ())
      {
        throw ::xsd::cxx::tree::expected_element< char > (
          "gap",
          "");
      }

      if (!feedDirection_.present ())
      {
        throw ::xsd::cxx::tree::expected_element< char > (
          "feedDirection",
          "");
      }

      if (!gapLabel_.present ())
      {
        throw ::xsd::cxx::tree::expected_element< char > (
          "gapLabel",
          "");
      }

      if (!feedDirectionLabel_.present ())
      {
        throw ::xsd::cxx::tree::expected_element< char > (
          "feedDirectionLabel",
          "");
      }

      if (!pitch_.present ())
      {
        throw ::xsd::cxx::tree::expected_element< char > (
          "pitch",
          "");
      }
    }

    FeatureNest* FeatureNest::
    _clone (::xml_schema::Flags f,
            ::xml_schema::Container* c) const
    {
      return new class FeatureNest (*this, f, c);
    }

    FeatureNest& FeatureNest::
    operator= (const FeatureNest& x)
    {
      if (this != &x)
      {
        static_cast< ::xml_schema::Type& > (*this) = x;
        this->featureBase_ = x.featureBase_;
        this->gap_ = x.gap_;
        this->feedDirection_ = x.feedDirection_;
        this->gapLabel_ = x.gapLabel_;
        this->feedDirectionLabel_ = x.feedDirectionLabel_;
        this->pitch_ = x.pitch_;
      }

      return *this;
    }

    FeatureNest::
    ~FeatureNest ()
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
    ::std::unique_ptr< ::prj::srl::FeatureNest >
    nest (const ::std::string& u,
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

      return ::std::unique_ptr< ::prj::srl::FeatureNest > (
        ::prj::srl::nest (
          std::move (d), f | ::xml_schema::Flags::own_dom, p));
    }

    ::std::unique_ptr< ::prj::srl::FeatureNest >
    nest (const ::std::string& u,
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

      return ::std::unique_ptr< ::prj::srl::FeatureNest > (
        ::prj::srl::nest (
          std::move (d), f | ::xml_schema::Flags::own_dom, p));
    }

    ::std::unique_ptr< ::prj::srl::FeatureNest >
    nest (const ::std::string& u,
          ::xercesc::DOMErrorHandler& h,
          ::xml_schema::Flags f,
          const ::xml_schema::Properties& p)
    {
      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::xsd::cxx::xml::dom::parse< char > (
          u, h, p, f));

      if (!d.get ())
        throw ::xsd::cxx::tree::parsing< char > ();

      return ::std::unique_ptr< ::prj::srl::FeatureNest > (
        ::prj::srl::nest (
          std::move (d), f | ::xml_schema::Flags::own_dom, p));
    }

    ::std::unique_ptr< ::prj::srl::FeatureNest >
    nest (::std::istream& is,
          ::xml_schema::Flags f,
          const ::xml_schema::Properties& p)
    {
      ::xsd::cxx::xml::auto_initializer i (
        (f & ::xml_schema::Flags::dont_initialize) == 0,
        (f & ::xml_schema::Flags::keep_dom) == 0);

      ::xsd::cxx::xml::sax::std_input_source isrc (is);
      return ::prj::srl::nest (isrc, f, p);
    }

    ::std::unique_ptr< ::prj::srl::FeatureNest >
    nest (::std::istream& is,
          ::xml_schema::ErrorHandler& h,
          ::xml_schema::Flags f,
          const ::xml_schema::Properties& p)
    {
      ::xsd::cxx::xml::auto_initializer i (
        (f & ::xml_schema::Flags::dont_initialize) == 0,
        (f & ::xml_schema::Flags::keep_dom) == 0);

      ::xsd::cxx::xml::sax::std_input_source isrc (is);
      return ::prj::srl::nest (isrc, h, f, p);
    }

    ::std::unique_ptr< ::prj::srl::FeatureNest >
    nest (::std::istream& is,
          ::xercesc::DOMErrorHandler& h,
          ::xml_schema::Flags f,
          const ::xml_schema::Properties& p)
    {
      ::xsd::cxx::xml::sax::std_input_source isrc (is);
      return ::prj::srl::nest (isrc, h, f, p);
    }

    ::std::unique_ptr< ::prj::srl::FeatureNest >
    nest (::std::istream& is,
          const ::std::string& sid,
          ::xml_schema::Flags f,
          const ::xml_schema::Properties& p)
    {
      ::xsd::cxx::xml::auto_initializer i (
        (f & ::xml_schema::Flags::dont_initialize) == 0,
        (f & ::xml_schema::Flags::keep_dom) == 0);

      ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
      return ::prj::srl::nest (isrc, f, p);
    }

    ::std::unique_ptr< ::prj::srl::FeatureNest >
    nest (::std::istream& is,
          const ::std::string& sid,
          ::xml_schema::ErrorHandler& h,
          ::xml_schema::Flags f,
          const ::xml_schema::Properties& p)
    {
      ::xsd::cxx::xml::auto_initializer i (
        (f & ::xml_schema::Flags::dont_initialize) == 0,
        (f & ::xml_schema::Flags::keep_dom) == 0);

      ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
      return ::prj::srl::nest (isrc, h, f, p);
    }

    ::std::unique_ptr< ::prj::srl::FeatureNest >
    nest (::std::istream& is,
          const ::std::string& sid,
          ::xercesc::DOMErrorHandler& h,
          ::xml_schema::Flags f,
          const ::xml_schema::Properties& p)
    {
      ::xsd::cxx::xml::sax::std_input_source isrc (is, sid);
      return ::prj::srl::nest (isrc, h, f, p);
    }

    ::std::unique_ptr< ::prj::srl::FeatureNest >
    nest (::xercesc::InputSource& i,
          ::xml_schema::Flags f,
          const ::xml_schema::Properties& p)
    {
      ::xsd::cxx::tree::error_handler< char > h;

      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::xsd::cxx::xml::dom::parse< char > (
          i, h, p, f));

      h.throw_if_failed< ::xsd::cxx::tree::parsing< char > > ();

      return ::std::unique_ptr< ::prj::srl::FeatureNest > (
        ::prj::srl::nest (
          std::move (d), f | ::xml_schema::Flags::own_dom, p));
    }

    ::std::unique_ptr< ::prj::srl::FeatureNest >
    nest (::xercesc::InputSource& i,
          ::xml_schema::ErrorHandler& h,
          ::xml_schema::Flags f,
          const ::xml_schema::Properties& p)
    {
      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::xsd::cxx::xml::dom::parse< char > (
          i, h, p, f));

      if (!d.get ())
        throw ::xsd::cxx::tree::parsing< char > ();

      return ::std::unique_ptr< ::prj::srl::FeatureNest > (
        ::prj::srl::nest (
          std::move (d), f | ::xml_schema::Flags::own_dom, p));
    }

    ::std::unique_ptr< ::prj::srl::FeatureNest >
    nest (::xercesc::InputSource& i,
          ::xercesc::DOMErrorHandler& h,
          ::xml_schema::Flags f,
          const ::xml_schema::Properties& p)
    {
      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::xsd::cxx::xml::dom::parse< char > (
          i, h, p, f));

      if (!d.get ())
        throw ::xsd::cxx::tree::parsing< char > ();

      return ::std::unique_ptr< ::prj::srl::FeatureNest > (
        ::prj::srl::nest (
          std::move (d), f | ::xml_schema::Flags::own_dom, p));
    }

    ::std::unique_ptr< ::prj::srl::FeatureNest >
    nest (const ::xercesc::DOMDocument& doc,
          ::xml_schema::Flags f,
          const ::xml_schema::Properties& p)
    {
      if (f & ::xml_schema::Flags::keep_dom)
      {
        ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
          static_cast< ::xercesc::DOMDocument* > (doc.cloneNode (true)));

        return ::std::unique_ptr< ::prj::srl::FeatureNest > (
          ::prj::srl::nest (
            std::move (d), f | ::xml_schema::Flags::own_dom, p));
      }

      const ::xercesc::DOMElement& e (*doc.getDocumentElement ());
      const ::xsd::cxx::xml::qualified_name< char > n (
        ::xsd::cxx::xml::dom::name< char > (e));

      if (n.name () == "nest" &&
          n.namespace_ () == "http://www.cadseer.com/prj/srl")
      {
        ::std::unique_ptr< ::prj::srl::FeatureNest > r (
          ::xsd::cxx::tree::traits< ::prj::srl::FeatureNest, char >::create (
            e, f, 0));
        return r;
      }

      throw ::xsd::cxx::tree::unexpected_element < char > (
        n.name (),
        n.namespace_ (),
        "nest",
        "http://www.cadseer.com/prj/srl");
    }

    ::std::unique_ptr< ::prj::srl::FeatureNest >
    nest (::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d,
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

      if (n.name () == "nest" &&
          n.namespace_ () == "http://www.cadseer.com/prj/srl")
      {
        ::std::unique_ptr< ::prj::srl::FeatureNest > r (
          ::xsd::cxx::tree::traits< ::prj::srl::FeatureNest, char >::create (
            e, f, 0));
        return r;
      }

      throw ::xsd::cxx::tree::unexpected_element < char > (
        n.name (),
        n.namespace_ (),
        "nest",
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
    operator<< (::xercesc::DOMElement& e, const FeatureNest& i)
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

      // gap
      //
      {
        ::xercesc::DOMElement& s (
          ::xsd::cxx::xml::dom::create_element (
            "gap",
            e));

        s << i.gap ();
      }

      // feedDirection
      //
      {
        ::xercesc::DOMElement& s (
          ::xsd::cxx::xml::dom::create_element (
            "feedDirection",
            e));

        s << i.feedDirection ();
      }

      // gapLabel
      //
      {
        ::xercesc::DOMElement& s (
          ::xsd::cxx::xml::dom::create_element (
            "gapLabel",
            e));

        s << i.gapLabel ();
      }

      // feedDirectionLabel
      //
      {
        ::xercesc::DOMElement& s (
          ::xsd::cxx::xml::dom::create_element (
            "feedDirectionLabel",
            e));

        s << i.feedDirectionLabel ();
      }

      // pitch
      //
      {
        ::xercesc::DOMElement& s (
          ::xsd::cxx::xml::dom::create_element (
            "pitch",
            e));

        s << ::xml_schema::AsDouble(i.pitch ());
      }
    }

    void
    nest (::std::ostream& o,
          const ::prj::srl::FeatureNest& s,
          const ::xml_schema::NamespaceInfomap& m,
          const ::std::string& e,
          ::xml_schema::Flags f)
    {
      ::xsd::cxx::xml::auto_initializer i (
        (f & ::xml_schema::Flags::dont_initialize) == 0);

      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::prj::srl::nest (s, m, f));

      ::xsd::cxx::tree::error_handler< char > h;

      ::xsd::cxx::xml::dom::ostream_format_target t (o);
      if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
      {
        h.throw_if_failed< ::xsd::cxx::tree::serialization< char > > ();
      }
    }

    void
    nest (::std::ostream& o,
          const ::prj::srl::FeatureNest& s,
          ::xml_schema::ErrorHandler& h,
          const ::xml_schema::NamespaceInfomap& m,
          const ::std::string& e,
          ::xml_schema::Flags f)
    {
      ::xsd::cxx::xml::auto_initializer i (
        (f & ::xml_schema::Flags::dont_initialize) == 0);

      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::prj::srl::nest (s, m, f));
      ::xsd::cxx::xml::dom::ostream_format_target t (o);
      if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
      {
        throw ::xsd::cxx::tree::serialization< char > ();
      }
    }

    void
    nest (::std::ostream& o,
          const ::prj::srl::FeatureNest& s,
          ::xercesc::DOMErrorHandler& h,
          const ::xml_schema::NamespaceInfomap& m,
          const ::std::string& e,
          ::xml_schema::Flags f)
    {
      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::prj::srl::nest (s, m, f));
      ::xsd::cxx::xml::dom::ostream_format_target t (o);
      if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
      {
        throw ::xsd::cxx::tree::serialization< char > ();
      }
    }

    void
    nest (::xercesc::XMLFormatTarget& t,
          const ::prj::srl::FeatureNest& s,
          const ::xml_schema::NamespaceInfomap& m,
          const ::std::string& e,
          ::xml_schema::Flags f)
    {
      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::prj::srl::nest (s, m, f));

      ::xsd::cxx::tree::error_handler< char > h;

      if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
      {
        h.throw_if_failed< ::xsd::cxx::tree::serialization< char > > ();
      }
    }

    void
    nest (::xercesc::XMLFormatTarget& t,
          const ::prj::srl::FeatureNest& s,
          ::xml_schema::ErrorHandler& h,
          const ::xml_schema::NamespaceInfomap& m,
          const ::std::string& e,
          ::xml_schema::Flags f)
    {
      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::prj::srl::nest (s, m, f));
      if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
      {
        throw ::xsd::cxx::tree::serialization< char > ();
      }
    }

    void
    nest (::xercesc::XMLFormatTarget& t,
          const ::prj::srl::FeatureNest& s,
          ::xercesc::DOMErrorHandler& h,
          const ::xml_schema::NamespaceInfomap& m,
          const ::std::string& e,
          ::xml_schema::Flags f)
    {
      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::prj::srl::nest (s, m, f));
      if (!::xsd::cxx::xml::dom::serialize (t, *d, e, h, f))
      {
        throw ::xsd::cxx::tree::serialization< char > ();
      }
    }

    void
    nest (::xercesc::DOMDocument& d,
          const ::prj::srl::FeatureNest& s,
          ::xml_schema::Flags)
    {
      ::xercesc::DOMElement& e (*d.getDocumentElement ());
      const ::xsd::cxx::xml::qualified_name< char > n (
        ::xsd::cxx::xml::dom::name< char > (e));

      if (n.name () == "nest" &&
          n.namespace_ () == "http://www.cadseer.com/prj/srl")
      {
        e << s;
      }
      else
      {
        throw ::xsd::cxx::tree::unexpected_element < char > (
          n.name (),
          n.namespace_ (),
          "nest",
          "http://www.cadseer.com/prj/srl");
      }
    }

    ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument >
    nest (const ::prj::srl::FeatureNest& s,
          const ::xml_schema::NamespaceInfomap& m,
          ::xml_schema::Flags f)
    {
      ::xml_schema::dom::unique_ptr< ::xercesc::DOMDocument > d (
        ::xsd::cxx::xml::dom::serialize< char > (
          "nest",
          "http://www.cadseer.com/prj/srl",
          m, f));

      ::prj::srl::nest (*d, s, f);
      return d;
    }
  }
}

#include <xsd/cxx/post.hxx>

// Begin epilogue.
//
//
// End epilogue.


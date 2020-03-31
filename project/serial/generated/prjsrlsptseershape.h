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

#ifndef PRJ_SRL_SPT_PRJSRLSPTSEERSHAPE_H
#define PRJ_SRL_SPT_PRJSRLSPTSEERSHAPE_H

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
    namespace spt
    {
      class ShapeIdRecord;
      class EvolveRecord;
      class FeatureTagRecord;
      class DerivedRecord;
      class SeerShape;
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

namespace prj
{
  namespace srl
  {
    namespace spt
    {
      class ShapeIdRecord: public ::xml_schema::Type
      {
        public:
        // id
        //
        typedef ::xml_schema::String IdType;
        typedef ::xsd::cxx::tree::traits< IdType, char > IdTraits;

        const IdType&
        id () const;

        IdType&
        id ();

        void
        id (const IdType& x);

        void
        id (::std::unique_ptr< IdType > p);

        static const IdType&
        id_default_value ();

        // shapeOffset
        //
        typedef ::xml_schema::UnsignedLong ShapeOffsetType;
        typedef ::xsd::cxx::tree::traits< ShapeOffsetType, char > ShapeOffsetTraits;

        const ShapeOffsetType&
        shapeOffset () const;

        ShapeOffsetType&
        shapeOffset ();

        void
        shapeOffset (const ShapeOffsetType& x);

        static ShapeOffsetType
        shapeOffset_default_value ();

        // Constructors.
        //
        ShapeIdRecord (const IdType&,
                       const ShapeOffsetType&);

        ShapeIdRecord (const ::xercesc::DOMElement& e,
                       ::xml_schema::Flags f = 0,
                       ::xml_schema::Container* c = 0);

        ShapeIdRecord (const ShapeIdRecord& x,
                       ::xml_schema::Flags f = 0,
                       ::xml_schema::Container* c = 0);

        virtual ShapeIdRecord*
        _clone (::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0) const;

        ShapeIdRecord&
        operator= (const ShapeIdRecord& x);

        virtual 
        ~ShapeIdRecord ();

        // Implementation.
        //
        protected:
        void
        parse (::xsd::cxx::xml::dom::parser< char >&,
               ::xml_schema::Flags);

        protected:
        ::xsd::cxx::tree::one< IdType > id_;
        static const IdType id_default_value_;
        ::xsd::cxx::tree::one< ShapeOffsetType > shapeOffset_;
      };

      class EvolveRecord: public ::xml_schema::Type
      {
        public:
        // idIn
        //
        typedef ::xml_schema::String IdInType;
        typedef ::xsd::cxx::tree::traits< IdInType, char > IdInTraits;

        const IdInType&
        idIn () const;

        IdInType&
        idIn ();

        void
        idIn (const IdInType& x);

        void
        idIn (::std::unique_ptr< IdInType > p);

        static const IdInType&
        idIn_default_value ();

        // idOut
        //
        typedef ::xml_schema::String IdOutType;
        typedef ::xsd::cxx::tree::traits< IdOutType, char > IdOutTraits;

        const IdOutType&
        idOut () const;

        IdOutType&
        idOut ();

        void
        idOut (const IdOutType& x);

        void
        idOut (::std::unique_ptr< IdOutType > p);

        static const IdOutType&
        idOut_default_value ();

        // Constructors.
        //
        EvolveRecord (const IdInType&,
                      const IdOutType&);

        EvolveRecord (const ::xercesc::DOMElement& e,
                      ::xml_schema::Flags f = 0,
                      ::xml_schema::Container* c = 0);

        EvolveRecord (const EvolveRecord& x,
                      ::xml_schema::Flags f = 0,
                      ::xml_schema::Container* c = 0);

        virtual EvolveRecord*
        _clone (::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0) const;

        EvolveRecord&
        operator= (const EvolveRecord& x);

        virtual 
        ~EvolveRecord ();

        // Implementation.
        //
        protected:
        void
        parse (::xsd::cxx::xml::dom::parser< char >&,
               ::xml_schema::Flags);

        protected:
        ::xsd::cxx::tree::one< IdInType > idIn_;
        static const IdInType idIn_default_value_;
        ::xsd::cxx::tree::one< IdOutType > idOut_;
        static const IdOutType idOut_default_value_;
      };

      class FeatureTagRecord: public ::xml_schema::Type
      {
        public:
        // id
        //
        typedef ::xml_schema::String IdType;
        typedef ::xsd::cxx::tree::traits< IdType, char > IdTraits;

        const IdType&
        id () const;

        IdType&
        id ();

        void
        id (const IdType& x);

        void
        id (::std::unique_ptr< IdType > p);

        static const IdType&
        id_default_value ();

        // tag
        //
        typedef ::xml_schema::String TagType;
        typedef ::xsd::cxx::tree::traits< TagType, char > TagTraits;

        const TagType&
        tag () const;

        TagType&
        tag ();

        void
        tag (const TagType& x);

        void
        tag (::std::unique_ptr< TagType > p);

        static const TagType&
        tag_default_value ();

        // Constructors.
        //
        FeatureTagRecord (const IdType&,
                          const TagType&);

        FeatureTagRecord (const ::xercesc::DOMElement& e,
                          ::xml_schema::Flags f = 0,
                          ::xml_schema::Container* c = 0);

        FeatureTagRecord (const FeatureTagRecord& x,
                          ::xml_schema::Flags f = 0,
                          ::xml_schema::Container* c = 0);

        virtual FeatureTagRecord*
        _clone (::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0) const;

        FeatureTagRecord&
        operator= (const FeatureTagRecord& x);

        virtual 
        ~FeatureTagRecord ();

        // Implementation.
        //
        protected:
        void
        parse (::xsd::cxx::xml::dom::parser< char >&,
               ::xml_schema::Flags);

        protected:
        ::xsd::cxx::tree::one< IdType > id_;
        static const IdType id_default_value_;
        ::xsd::cxx::tree::one< TagType > tag_;
        static const TagType tag_default_value_;
      };

      class DerivedRecord: public ::xml_schema::Type
      {
        public:
        // idSet
        //
        typedef ::xml_schema::String IdSetType;
        typedef ::xsd::cxx::tree::sequence< IdSetType > IdSetSequence;
        typedef IdSetSequence::iterator IdSetIterator;
        typedef IdSetSequence::const_iterator IdSetConstIterator;
        typedef ::xsd::cxx::tree::traits< IdSetType, char > IdSetTraits;

        const IdSetSequence&
        idSet () const;

        IdSetSequence&
        idSet ();

        void
        idSet (const IdSetSequence& s);

        // id
        //
        typedef ::xml_schema::String IdType;
        typedef ::xsd::cxx::tree::traits< IdType, char > IdTraits;

        const IdType&
        id () const;

        IdType&
        id ();

        void
        id (const IdType& x);

        void
        id (::std::unique_ptr< IdType > p);

        static const IdType&
        id_default_value ();

        // Constructors.
        //
        DerivedRecord (const IdType&);

        DerivedRecord (const ::xercesc::DOMElement& e,
                       ::xml_schema::Flags f = 0,
                       ::xml_schema::Container* c = 0);

        DerivedRecord (const DerivedRecord& x,
                       ::xml_schema::Flags f = 0,
                       ::xml_schema::Container* c = 0);

        virtual DerivedRecord*
        _clone (::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0) const;

        DerivedRecord&
        operator= (const DerivedRecord& x);

        virtual 
        ~DerivedRecord ();

        // Implementation.
        //
        protected:
        void
        parse (::xsd::cxx::xml::dom::parser< char >&,
               ::xml_schema::Flags);

        protected:
        IdSetSequence idSet_;
        ::xsd::cxx::tree::one< IdType > id_;
        static const IdType id_default_value_;
      };

      class SeerShape: public ::xml_schema::Type
      {
        public:
        // rootShapeId
        //
        typedef ::xml_schema::String RootShapeIdType;
        typedef ::xsd::cxx::tree::traits< RootShapeIdType, char > RootShapeIdTraits;

        const RootShapeIdType&
        rootShapeId () const;

        RootShapeIdType&
        rootShapeId ();

        void
        rootShapeId (const RootShapeIdType& x);

        void
        rootShapeId (::std::unique_ptr< RootShapeIdType > p);

        static const RootShapeIdType&
        rootShapeId_default_value ();

        // shapeIdContainer
        //
        typedef ::prj::srl::spt::ShapeIdRecord ShapeIdContainerType;
        typedef ::xsd::cxx::tree::sequence< ShapeIdContainerType > ShapeIdContainerSequence;
        typedef ShapeIdContainerSequence::iterator ShapeIdContainerIterator;
        typedef ShapeIdContainerSequence::const_iterator ShapeIdContainerConstIterator;
        typedef ::xsd::cxx::tree::traits< ShapeIdContainerType, char > ShapeIdContainerTraits;

        const ShapeIdContainerSequence&
        shapeIdContainer () const;

        ShapeIdContainerSequence&
        shapeIdContainer ();

        void
        shapeIdContainer (const ShapeIdContainerSequence& s);

        // evolveContainer
        //
        typedef ::prj::srl::spt::EvolveRecord EvolveContainerType;
        typedef ::xsd::cxx::tree::sequence< EvolveContainerType > EvolveContainerSequence;
        typedef EvolveContainerSequence::iterator EvolveContainerIterator;
        typedef EvolveContainerSequence::const_iterator EvolveContainerConstIterator;
        typedef ::xsd::cxx::tree::traits< EvolveContainerType, char > EvolveContainerTraits;

        const EvolveContainerSequence&
        evolveContainer () const;

        EvolveContainerSequence&
        evolveContainer ();

        void
        evolveContainer (const EvolveContainerSequence& s);

        // featureTagContainer
        //
        typedef ::prj::srl::spt::FeatureTagRecord FeatureTagContainerType;
        typedef ::xsd::cxx::tree::sequence< FeatureTagContainerType > FeatureTagContainerSequence;
        typedef FeatureTagContainerSequence::iterator FeatureTagContainerIterator;
        typedef FeatureTagContainerSequence::const_iterator FeatureTagContainerConstIterator;
        typedef ::xsd::cxx::tree::traits< FeatureTagContainerType, char > FeatureTagContainerTraits;

        const FeatureTagContainerSequence&
        featureTagContainer () const;

        FeatureTagContainerSequence&
        featureTagContainer ();

        void
        featureTagContainer (const FeatureTagContainerSequence& s);

        // derivedContainer
        //
        typedef ::prj::srl::spt::DerivedRecord DerivedContainerType;
        typedef ::xsd::cxx::tree::sequence< DerivedContainerType > DerivedContainerSequence;
        typedef DerivedContainerSequence::iterator DerivedContainerIterator;
        typedef DerivedContainerSequence::const_iterator DerivedContainerConstIterator;
        typedef ::xsd::cxx::tree::traits< DerivedContainerType, char > DerivedContainerTraits;

        const DerivedContainerSequence&
        derivedContainer () const;

        DerivedContainerSequence&
        derivedContainer ();

        void
        derivedContainer (const DerivedContainerSequence& s);

        // Constructors.
        //
        SeerShape (const RootShapeIdType&);

        SeerShape (const ::xercesc::DOMElement& e,
                   ::xml_schema::Flags f = 0,
                   ::xml_schema::Container* c = 0);

        SeerShape (const SeerShape& x,
                   ::xml_schema::Flags f = 0,
                   ::xml_schema::Container* c = 0);

        virtual SeerShape*
        _clone (::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0) const;

        SeerShape&
        operator= (const SeerShape& x);

        virtual 
        ~SeerShape ();

        // Implementation.
        //
        protected:
        void
        parse (::xsd::cxx::xml::dom::parser< char >&,
               ::xml_schema::Flags);

        protected:
        ::xsd::cxx::tree::one< RootShapeIdType > rootShapeId_;
        static const RootShapeIdType rootShapeId_default_value_;
        ShapeIdContainerSequence shapeIdContainer_;
        EvolveContainerSequence evolveContainer_;
        FeatureTagContainerSequence featureTagContainer_;
        DerivedContainerSequence derivedContainer_;
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
    namespace spt
    {
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
    namespace spt
    {
      void
      operator<< (::xercesc::DOMElement&, const ShapeIdRecord&);

      void
      operator<< (::xercesc::DOMElement&, const EvolveRecord&);

      void
      operator<< (::xercesc::DOMElement&, const FeatureTagRecord&);

      void
      operator<< (::xercesc::DOMElement&, const DerivedRecord&);

      void
      operator<< (::xercesc::DOMElement&, const SeerShape&);
    }
  }
}

#include <xsd/cxx/post.hxx>

// Begin epilogue.
//
//
// End epilogue.

#endif // PRJ_SRL_SPT_PRJSRLSPTSEERSHAPE_H
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

#ifndef PRJ_SRL_SPT_PRJSRLSPTSHAPEHISTORY_H
#define PRJ_SRL_SPT_PRJSRLSPTSHAPEHISTORY_H

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
      class HistoryVertex;
      class HistoryEdge;
      class ShapeHistory;
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
      class HistoryVertex: public ::xml_schema::Type
      {
        public:
        // featureId
        //
        typedef ::xml_schema::String FeatureIdType;
        typedef ::xsd::cxx::tree::traits< FeatureIdType, char > FeatureIdTraits;

        const FeatureIdType&
        featureId () const;

        FeatureIdType&
        featureId ();

        void
        featureId (const FeatureIdType& x);

        void
        featureId (::std::unique_ptr< FeatureIdType > p);

        static const FeatureIdType&
        featureId_default_value ();

        // shapeId
        //
        typedef ::xml_schema::String ShapeIdType;
        typedef ::xsd::cxx::tree::traits< ShapeIdType, char > ShapeIdTraits;

        const ShapeIdType&
        shapeId () const;

        ShapeIdType&
        shapeId ();

        void
        shapeId (const ShapeIdType& x);

        void
        shapeId (::std::unique_ptr< ShapeIdType > p);

        static const ShapeIdType&
        shapeId_default_value ();

        // Constructors.
        //
        HistoryVertex (const FeatureIdType&,
                       const ShapeIdType&);

        HistoryVertex (const ::xercesc::DOMElement& e,
                       ::xml_schema::Flags f = 0,
                       ::xml_schema::Container* c = 0);

        HistoryVertex (const HistoryVertex& x,
                       ::xml_schema::Flags f = 0,
                       ::xml_schema::Container* c = 0);

        virtual HistoryVertex*
        _clone (::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0) const;

        HistoryVertex&
        operator= (const HistoryVertex& x);

        virtual 
        ~HistoryVertex ();

        // Implementation.
        //
        protected:
        void
        parse (::xsd::cxx::xml::dom::parser< char >&,
               ::xml_schema::Flags);

        protected:
        ::xsd::cxx::tree::one< FeatureIdType > featureId_;
        static const FeatureIdType featureId_default_value_;
        ::xsd::cxx::tree::one< ShapeIdType > shapeId_;
        static const ShapeIdType shapeId_default_value_;
      };

      class HistoryEdge: public ::xml_schema::Type
      {
        public:
        // sourceShapeId
        //
        typedef ::xml_schema::String SourceShapeIdType;
        typedef ::xsd::cxx::tree::traits< SourceShapeIdType, char > SourceShapeIdTraits;

        const SourceShapeIdType&
        sourceShapeId () const;

        SourceShapeIdType&
        sourceShapeId ();

        void
        sourceShapeId (const SourceShapeIdType& x);

        void
        sourceShapeId (::std::unique_ptr< SourceShapeIdType > p);

        static const SourceShapeIdType&
        sourceShapeId_default_value ();

        // targetShapeId
        //
        typedef ::xml_schema::String TargetShapeIdType;
        typedef ::xsd::cxx::tree::traits< TargetShapeIdType, char > TargetShapeIdTraits;

        const TargetShapeIdType&
        targetShapeId () const;

        TargetShapeIdType&
        targetShapeId ();

        void
        targetShapeId (const TargetShapeIdType& x);

        void
        targetShapeId (::std::unique_ptr< TargetShapeIdType > p);

        static const TargetShapeIdType&
        targetShapeId_default_value ();

        // Constructors.
        //
        HistoryEdge (const SourceShapeIdType&,
                     const TargetShapeIdType&);

        HistoryEdge (const ::xercesc::DOMElement& e,
                     ::xml_schema::Flags f = 0,
                     ::xml_schema::Container* c = 0);

        HistoryEdge (const HistoryEdge& x,
                     ::xml_schema::Flags f = 0,
                     ::xml_schema::Container* c = 0);

        virtual HistoryEdge*
        _clone (::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0) const;

        HistoryEdge&
        operator= (const HistoryEdge& x);

        virtual 
        ~HistoryEdge ();

        // Implementation.
        //
        protected:
        void
        parse (::xsd::cxx::xml::dom::parser< char >&,
               ::xml_schema::Flags);

        protected:
        ::xsd::cxx::tree::one< SourceShapeIdType > sourceShapeId_;
        static const SourceShapeIdType sourceShapeId_default_value_;
        ::xsd::cxx::tree::one< TargetShapeIdType > targetShapeId_;
        static const TargetShapeIdType targetShapeId_default_value_;
      };

      class ShapeHistory: public ::xml_schema::Type
      {
        public:
        // vertices
        //
        typedef ::prj::srl::spt::HistoryVertex VerticesType;
        typedef ::xsd::cxx::tree::sequence< VerticesType > VerticesSequence;
        typedef VerticesSequence::iterator VerticesIterator;
        typedef VerticesSequence::const_iterator VerticesConstIterator;
        typedef ::xsd::cxx::tree::traits< VerticesType, char > VerticesTraits;

        const VerticesSequence&
        vertices () const;

        VerticesSequence&
        vertices ();

        void
        vertices (const VerticesSequence& s);

        // edges
        //
        typedef ::prj::srl::spt::HistoryEdge EdgesType;
        typedef ::xsd::cxx::tree::sequence< EdgesType > EdgesSequence;
        typedef EdgesSequence::iterator EdgesIterator;
        typedef EdgesSequence::const_iterator EdgesConstIterator;
        typedef ::xsd::cxx::tree::traits< EdgesType, char > EdgesTraits;

        const EdgesSequence&
        edges () const;

        EdgesSequence&
        edges ();

        void
        edges (const EdgesSequence& s);

        // Constructors.
        //
        ShapeHistory ();

        ShapeHistory (const ::xercesc::DOMElement& e,
                      ::xml_schema::Flags f = 0,
                      ::xml_schema::Container* c = 0);

        ShapeHistory (const ShapeHistory& x,
                      ::xml_schema::Flags f = 0,
                      ::xml_schema::Container* c = 0);

        virtual ShapeHistory*
        _clone (::xml_schema::Flags f = 0,
                ::xml_schema::Container* c = 0) const;

        ShapeHistory&
        operator= (const ShapeHistory& x);

        virtual 
        ~ShapeHistory ();

        // Implementation.
        //
        protected:
        void
        parse (::xsd::cxx::xml::dom::parser< char >&,
               ::xml_schema::Flags);

        protected:
        VerticesSequence vertices_;
        EdgesSequence edges_;
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
      operator<< (::xercesc::DOMElement&, const HistoryVertex&);

      void
      operator<< (::xercesc::DOMElement&, const HistoryEdge&);

      void
      operator<< (::xercesc::DOMElement&, const ShapeHistory&);
    }
  }
}

#include <xsd/cxx/post.hxx>

// Begin epilogue.
//
//
// End epilogue.

#endif // PRJ_SRL_SPT_PRJSRLSPTSHAPEHISTORY_H
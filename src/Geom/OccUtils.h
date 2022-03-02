/***************************************************************************
 *   Copyright (c) 2019 Qingfeng Xia  <qingfeng.xia@ukaea.uk>              *
 *   This file is part of parallel-preprocessor CAx development system.    *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef PPP_OCCT_UTILS_H
#define PPP_OCCT_UTILS_H

#include "GeometryTypes.h"
#include "OpenCascadeAll.h"
#include "PPP/PreCompiled.h"


namespace Geom
{
    using namespace PPP;

    /// \ingroup Geom
    /* static utility metheds, wrapping around OpenCASCADE API
     */
    namespace OccUtils
    {
        /// save shape to brep file
        GeomExport void saveShape(const std::vector<TopoDS_Shape>& shapes, const std::string file_name);
        GeomExport void saveShape(const TopoDS_Shape& shape, const std::string file_name);

        /// save to buffer in memory, avoid disk IO, to be sent over network
        /// is there any Endianness issue for utf8 text stream?
        GeomExport std::shared_ptr<std::stringstream> saveShapeToStream(const std::vector<TopoDS_Shape>& shapes,
                                                                        const std::string& fileType = "brep");

        GeomExport TopoDS_Shape loadShape(const std::string file_name);
        GeomExport TopoDS_Shape loadShape(std::shared_ptr<std::stringstream> stream);

        /// this API will not count on shared geometry, like shared faces in a compsolid
        GeomExport unsigned long countSubShapes(const TopoDS_Shape& _shape, const TopAbs_ShapeEnum Type);
        /// shared geometry will be counted twice
        GeomExport std::map<std::string, int> exploreShape(const TopoDS_Shape& aShape);

        /// based on axis-aligned boundbox matching and surface area equivalence
        GeomExport Standard_Boolean isCoincidentDomain(const TopoDS_Shape& shape, const TopoDS_Shape& shape2);

        /// two-step unifying: first unify edges, secondly unify faces,
        /// deprecated: use glueFaces() instead
        GeomExport TopoDS_Shape unifyFaces(const TopoDS_Shape& _shape);

        /// the input shapes to be glued should be a compound
        /// adapted from Geom module of Salome platform (LGPL v2.1)
        /// see the function `glueFaces` in GeomImpl_GlueDriver.cxx of Salome platform
        GeomExport TopoDS_Shape glueFaces(const TopoDS_Shape& _shape, const Standard_Real tolerance = 0.0);


        GeomExport TopoDS_Compound
        createCompound(const ItemContainerType theSolids,
                       std::shared_ptr<const MapType<ItemHashType, ShapeErrorType>> suppressed = nullptr);
        GeomExport TopoDS_CompSolid
        createCompSolid(const ItemContainerType theSolids,
                        std::shared_ptr<const MapType<ItemHashType, ShapeErrorType>> suppressed = nullptr);
        GeomExport TopoDS_Shape createCompound(std::vector<TopoDS_Shape> shapes);


        GeomExport Bnd_Box calcBndBox(const TopoDS_Shape& s);
        GeomExport json jsonifyBndBox(const Bnd_Box& p);
        GeomExport bool isBndBoxOverlapped(const Bnd_OBB& thisBox, const Bnd_OBB& otherBox);
        /** gap can control overlapping, near or contacting, by given an absolute gap */
        GeomExport bool isBndBoxOverlapped(const Bnd_Box& thisBox, const Bnd_Box& otherBox, Standard_Real gap = 1e-2);
        /** detect if two shapes taking up the same space/boundbox, by relative gap (0~1.0) */
        GeomExport bool isBndBoxCoincident(const Bnd_Box& thisBox, const Bnd_Box& otherBox,
                                           Standard_Real reltolerance = 1e-2);
        GeomExport bool isBndBoxCoincident(const TopoDS_Shape& s, const Bnd_Box& otherBox,
                                           Standard_Real reltolerance = 1e-2);

        GeomExport bool floatEqual(double a, double b, double reltol = 1e-4);
        GeomExport GeometryProperty geometryProperty(const TopoDS_Shape& s);
        GeomExport Standard_Real area(const TopoDS_Shape& s);
        GeomExport Standard_Real perimeter(const TopoDS_Shape& s);
        GeomExport Standard_Real volume(const TopoDS_Shape& s);
        GeomExport Standard_Real tolerance(const TopoDS_Shape& s);
        GeomExport Standard_Real distance(const TopoDS_Shape& s1, const TopoDS_Shape& s2);
        GeomExport UniqueIdType uniqueId(const GeometryProperty& p);
        GeomExport UniqueIdType uniqueId(const TopoDS_Shape& s);


        int countDeletedShape(const std::shared_ptr<BRepAlgoAPI_BuilderAlgo>& mkGFA, TopAbs_ShapeEnum stype);
        int countModifiedShape(const std::shared_ptr<BRepAlgoAPI_BuilderAlgo>& mkGFA, TopAbs_ShapeEnum stype);
        GeomExport void summarizeBuilderAlgo(std::shared_ptr<BRepAlgoAPI_BuilderAlgo> mkGFA);

        /** boolean fragments
         * the caller has full control on multiple threading the `builder` pointer
         * internal multiple thread can be turn off: internalMultiThreading = false */
        GeomExport std::vector<TopoDS_Shape> generalFuse(const std::vector<TopoDS_Shape>& shapes,
                                                         const Standard_Real tolerance,
                                                         std::shared_ptr<BRepAlgoAPI_BuilderAlgo> builder);
        GeomExport TopoDS_Shape commonShape(const VectorType<TopoDS_Shape> v, bool occInternalParallel = true);
        GeomExport TopoDS_Shape fuseShape(const VectorType<TopoDS_Shape> v, bool occInternalParallel = true);
        GeomExport TopoDS_Shape cutShape(const TopoDS_Shape& from, const TopoDS_Shape& substractor);

        /// surface mesh
        GeomExport std::shared_ptr<BRepMesh_IncrementalMesh> meshShape(const TopoDS_Shape& aShape,
                                                                       double resolution = 0.01);
        /// save to STL mesh format
        GeomExport Standard_Boolean saveMesh(TopoDS_Shape& aShape, const std::string file_name);

        /// scale up or down shape
        GeomExport TopoDS_Shape scaleShape(const TopoDS_Shape& from, double scale, const gp_Pnt origin = gp_Pnt());

    } // namespace OccUtils

} // namespace Geom

#endif // header firewall
/*
Copyright(C) 2017 Shing Liu(eryar@163.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Visual Studio 2013 & OpenCASCADE7.1.0, // 2017-04-18  20:52
// https://www.cnblogs.com/opencascade/p/6804446.html

// changelog:  adapted to Linux, OCCT 7.3

#include <BRepExtrema_TriangleSet.hxx>

#include <BRepTools.hxx>
#include <BRep_Builder.hxx>

#include <TopoDS.hxx>

#include <TopExp_Explorer.hxx>

#include <BRepPrimAPI_MakeBox.hxx>

#include <BRepMesh_IncrementalMesh.hxx>

#if defined(WIN32)
#pragma comment(lib, "TKernel.lib")
#pragma comment(lib, "TKMath.lib")

#pragma comment(lib, "TKG2d.lib")
#pragma comment(lib, "TKG3d.lib")
#pragma comment(lib, "TKGeomBase.lib")
#pragma comment(lib, "TKGeomAlgo.lib")

#pragma comment(lib, "TKBRep.lib")
#pragma comment(lib, "TKPrim.lib")
#pragma comment(lib, "TKMesh.lib")
#pragma comment(lib, "TKTopAlgo.lib")
#endif

void testBvh(const std::string& theFileName)
{
    BRep_Builder aBuilder;
    TopoDS_Compound aCompound;

    TopoDS_Shape aShape;
    BRepTools::Read(aShape, theFileName.c_str(), aBuilder);

    BRepExtrema_ShapeList aFaceList;
    for (TopExp_Explorer i(aShape, TopAbs_FACE); i.More(); i.Next())
    {
        aFaceList.Append(TopoDS::Face(i.Current()));
    }

    aBuilder.MakeCompound(aCompound);

    // `BRepMesh_IncrementalMesh` API has gone in OCCT 7.4
    BRepMesh_IncrementalMesh aMesher(aShape, 1.0);

    BRepExtrema_TriangleSet aTriangleSet(aFaceList);
    // API changed for `BVH()` return type
    const opencascade::handle<BVH_Tree<Standard_Real, 3>>& aBvh = aTriangleSet.BVH();
    for (int i = 0; i < aBvh->Length(); i++)
    {
        const BVH_Vec4i& aNodeData = aBvh->NodeInfoBuffer()[i];

        if (aNodeData.x() == 0)
        {
            // inner node.
            const BVH_Vec3d& aP1 = aBvh->MinPoint(aNodeData.y());
            const BVH_Vec3d& aP2 = aBvh->MaxPoint(aNodeData.y());

            const BVH_Vec3d& aQ1 = aBvh->MinPoint(aNodeData.z());
            const BVH_Vec3d& aQ2 = aBvh->MaxPoint(aNodeData.z());

            try
            {
                BRepPrimAPI_MakeBox aBoxMaker(gp_Pnt(aP1.x(), aP1.y(), aP1.z()), gp_Pnt(aP2.x(), aP2.y(), aP2.z()));
                for (TopExp_Explorer i(aBoxMaker.Shape(), TopAbs_EDGE); i.More(); i.Next())
                {
                    aBuilder.Add(aCompound, i.Current());
                }
            }
            catch (Standard_Failure f)
            {
            }

            try
            {
                BRepPrimAPI_MakeBox aBoxMaker(gp_Pnt(aQ1.x(), aQ1.y(), aQ1.z()), gp_Pnt(aQ2.x(), aQ2.y(), aQ2.z()));
                for (TopExp_Explorer i(aBoxMaker.Shape(), TopAbs_EDGE); i.More(); i.Next())
                {
                    aBuilder.Add(aCompound, i.Current());
                }
            }
            catch (Standard_Failure f)
            {
            }
        }
        else
        {
            // leaves node.
            const BVH_Vec3d& aP1 = aBvh->MinPoint(i);
            const BVH_Vec3d& aP2 = aBvh->MaxPoint(i);

            try
            {
                BRepPrimAPI_MakeBox aBoxMaker(gp_Pnt(aP1.x(), aP1.y(), aP1.z()), gp_Pnt(aP2.x(), aP2.y(), aP2.z()));

                aBuilder.Add(aCompound, aBoxMaker.Shape());
            }
            catch (Standard_Failure f)
            {
            }
        }
    }

    BRepTools::Write(aCompound, "./bvh.brep");
}

int main(int argc, char* argv[])
{
    if (argc > 1)
    {
        testBvh(argv[1]);
    }

    return 0;
}
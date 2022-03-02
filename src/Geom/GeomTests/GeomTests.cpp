// usually include the user cpp file directly, not the header
//#include "Geom/OccShapeUtils.cpp"
#include "Geom/CollisionDetector.h"
#include "Geom/OccUtils.h"

#undef INFO
/// CATCH_CONFIG_MAIN should be put before `#include "catch2/catch.hpp"ls`
#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cmake target
#include "catch2/catch.hpp"


using namespace Geom;

/// catch2 test
TEST_CASE("OccUtilsTest")
{
    using namespace Geom::OccUtils;
    SECTION("hasCollision_tests")
    {

        float R = 2;
        float H = 10;
        gp_Ax2 anAxis, anotherAxis;
        anAxis.SetLocation(gp_Pnt(0.0, 0, 0.0));
        anotherAxis.SetLocation(gp_Pnt(0.0, 0.0, -H));

        // if Processor derived classes have not use globalParameter() etc. in constructor
        // it is possible to test only static class method in this cpp unit test setup
        // however, python integration test fully the program in realistic condition
        auto cd = CollisionDetector();

        TopoDS_Shape aCylinder = BRepPrimAPI_MakeCylinder(anAxis, R, H).Shape();
        TopoDS_Shape aCylinder2 = BRepPrimAPI_MakeCylinder(anAxis, R, H).Shape();

        REQUIRE(CollisionDetector::hasCollision(aCylinder, aCylinder));
        // "identical shape, some object in memory, fusion is not done, using reference =="
        REQUIRE(
            CollisionDetector::hasCollision(aCylinder, aCylinder2)); // "equal parameter shapes count as coincidence"

        // completely inside
        TopoDS_Shape aCylinder3 = BRepPrimAPI_MakeCylinder(anAxis, R * 0.5, H * 0.5).Shape();
        REQUIRE(CollisionDetector::hasCollision(aCylinder, aCylinder3));

        // radius confusion()
        TopoDS_Shape aCylinder4 = BRepPrimAPI_MakeCylinder(anAxis, R + Precision::Confusion(), H).Shape();
        REQUIRE(CollisionDetector::hasCollision(aCylinder, aCylinder4));

        // contact by planer surface,
        TopoDS_Shape aCylinder5 = BRepPrimAPI_MakeCylinder(anotherAxis, R, H).Shape();
        REQUIRE(CollisionDetector::hasCollision(aCylinder, aCylinder5));
    }

    SECTION("collisionType_tests")
    {
        gp_Ax2 anAxis;
        anAxis.SetLocation(gp_Pnt(0.0, 0.0, 0.0));

        float R = 2;
        float H = 10; // height z+ axis
        TopoDS_Shape aCylinder = BRepPrimAPI_MakeCylinder(anAxis, R, H).Shape();
        TopoDS_Shape aCylinder2 = BRepPrimAPI_MakeCylinder(anAxis, R, H).Shape();
        const std::vector<Standard_Real> vols; // empty

        // "identical shape, should be counted as Coincidence",
        REQUIRE(CollisionDetector::detectCollision(aCylinder, aCylinder, vols).type == CollisionType::Coincidence);
        // if using detectCollisionType(), just inline wrap of detectCollision(), this failed, why?

        // "equal parameter shapes count as coincidence"
        REQUIRE(CollisionDetector::detectCollisionType(aCylinder, aCylinder2, vols) == CollisionType::Coincidence);

        // radius + confusion()
        TopoDS_Shape aCylinder3 = BRepPrimAPI_MakeCylinder(anAxis, R + Precision::Confusion(), H).Shape();
        REQUIRE(CollisionDetector::detectCollisionType(aCylinder, aCylinder3, vols) == CollisionType::Coincidence);

        TopoDS_Shape aCylinder4 = BRepPrimAPI_MakeCylinder(anAxis, R + 1e-3, H).Shape();
        REQUIRE(CollisionDetector::detectCollisionType(aCylinder, aCylinder4, vols, 2e-3) ==
                CollisionType::Coincidence);
        // tolerance must be much bigger than geometry radius error, X2 is not enough, or not working
        REQUIRE(CollisionDetector::detectCollisionType(aCylinder, aCylinder4, vols, 5e-4) !=
                CollisionType::Coincidence);

        // not completely inside, enclosure, but share some faces
        TopoDS_Shape aCylinder5 = BRepPrimAPI_MakeCylinder(anAxis, R * 0.5, H * 0.5).Shape();
        REQUIRE(CollisionDetector::detectCollisionType(aCylinder, aCylinder5, vols) == CollisionType::Enclosure);

        // completely inside, no share faces
        anAxis.SetLocation(gp_Pnt(0.0, 0.0, H * 0.1));
        TopoDS_Shape aCylinder6 = BRepPrimAPI_MakeCylinder(anAxis, R * 0.5, H * 0.5).Shape();
        REQUIRE(CollisionDetector::detectCollisionType(aCylinder, aCylinder6, vols) == CollisionType::Enclosure);

        TopoDS_Shape aCylinder7 = BRepPrimAPI_MakeCylinder(anAxis, R * 0.5, H).Shape();
        REQUIRE(CollisionDetector::detectCollisionType(aCylinder, aCylinder7, vols) == CollisionType::Interference);

        TopoDS_Shape aCylinder8 = BRepPrimAPI_MakeCylinder(anAxis, R, H).Shape();
        REQUIRE(CollisionDetector::detectCollisionType(aCylinder, aCylinder8, vols) == CollisionType::Interference);

        gp_Ax2 anotherAxis; // to simulate face contact becomes weak interference  due to data exchange error
        anotherAxis.SetLocation(gp_Pnt(0.0, 0.0, H * 0.95));
        TopoDS_Shape aCylinder9 = BRepPrimAPI_MakeCylinder(anotherAxis, R, H).Shape();
        // saveShape({aCylinder, aCylinder9}, "fs::temp_directory_path() / "weakInterference.brep");
        REQUIRE(CollisionDetector::detectCollisionType(aCylinder, aCylinder9, vols) == CollisionType::WeakInterference);
    }

    SECTION("test_gfa_modification")
    {
        // does that must be Solid() not Shape()
        TopoDS_Solid box1 = BRepPrimAPI_MakeBox(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(10.0, 10.0, 10.0)).Solid();
        // face contact
        // TopoDS_Solid box2 = BRepPrimAPI_MakeBox(gp_Pnt(10.0, 0.0, 0.0), gp_Pnt(20.0, 10.0, 10.0)).Solid();
        // edge contact, share the z-axis edge
        TopoDS_Solid box2 = BRepPrimAPI_MakeBox(gp_Pnt(10.0, 10.0, 0.0), gp_Pnt(20.0, 20.0, 10.0)).Solid();
        std::vector<TopoDS_Shape> shapes = {box1, box2};
        size_t NumberOfFaces = shapes.size() * 6;

        auto mkGFA = std::make_shared<BRepAlgoAPI_BuilderAlgo>();
        mkGFA->SetNonDestructive(true);
        // mkGFA->SetGlue(BOPAlgo_GlueShift); can accelerate in case of overlapping, but not interference
        mkGFA->SetRunParallel(false);
        mkGFA->SetUseOBB(true);
        generalFuse(shapes, 0.0, mkGFA);

        std::vector<TopoDS_Shape> mapOriginal;
        std::vector<TopTools_ListOfShape> mapModified, mapGenerated;
        std::vector<bool> mapDeleted;
        // TopTools_ListIteratorOfListOfShape it(objectShapes);
        // for (; it.More(); it.Next())
        for (const auto& s : shapes)
        {
            // mapOriginal.push_back(it.Value());
            mapModified.push_back(mkGFA->Modified(s));
            mapDeleted.push_back(mkGFA->IsDeleted(s)); // Solid deleted?
            mapGenerated.push_back(mkGFA->Generated(s));
        }
        INFO("number of modified : " + std::to_string(mapModified.size()));
        REQUIRE(std::all_of(mapDeleted.begin(), mapDeleted.end(), [](bool v) { return v; }) == false);

        const TopoDS_Shape& compSolid = mkGFA->Shape();
        REQUIRE(compSolid.ShapeType() == TopAbs_COMPOUND); // result is compound for General Fusion

        REQUIRE(countSubShapes(compSolid, TopAbs_FACE) == NumberOfFaces);
        REQUIRE(countSubShapes(compSolid, TopAbs_COMPOUND) == 1);
    }

    SECTION("contact_type_tests")
    {
        Standard_Real L = 10.0;
        gp_Pnt o = gp_Pnt(0, 0, 0);
        const std::vector<Standard_Real> vols; // empty
        TopoDS_Shape box = BRepPrimAPI_MakeBox(o, gp_Pnt(L, L, L)).Shape();
        TopoDS_Shape box1 = BRepPrimAPI_MakeBox(o, gp_Pnt(-L, L, L)).Shape();

        auto mkGFA = std::make_shared<BRepAlgoAPI_BuilderAlgo>();
        auto res = generalFuse({box, box1}, 0.0, mkGFA);
        auto ctype = CollisionDetector::calcCollisionType(mkGFA);
        std::cout << "Result V, S, L = " << volume(mkGFA->Shape()) << ", " << area(mkGFA->Shape()) << ", "
                  << perimeter(mkGFA->Shape()) << std::endl;
        REQUIRE(floatEqual(area(mkGFA->Shape()), L * L * 11));
        REQUIRE(floatEqual(perimeter(mkGFA->Shape()), L * 20));
        REQUIRE(ctype == CollisionType::FaceContact);
        // REQUIRE(detectCollisionType(aCylinder, aCylinder5, vols, 1e-3) == CollisionType::FaceContact);

        // todo:
        TopoDS_Shape box2 = BRepPrimAPI_MakeBox(o, gp_Pnt(L, -L, -L)).Shape();
        mkGFA = std::make_shared<BRepAlgoAPI_BuilderAlgo>();
        res = generalFuse({box, box2}, 0.0, mkGFA);
        ctype = CollisionDetector::calcCollisionType(mkGFA);
        std::cout << "Result V, S, L = " << volume(mkGFA->Shape()) << ", " << area(mkGFA->Shape()) << ", "
                  << perimeter(mkGFA->Shape()) << std::endl;
        REQUIRE(floatEqual(perimeter(mkGFA->Shape()), L * 23));
        REQUIRE(ctype == CollisionType::EdgeContact);

        TopoDS_Shape box3 = BRepPrimAPI_MakeBox(o, gp_Pnt(-L, -L, -L)).Shape();
        REQUIRE(CollisionDetector::detectCollisionType(box, box3, vols) == CollisionType::VertexContact);
    }

    SECTION("curved_surface_contact_tests")
    {
        double R = 10;
        double R1 = R * 0.8;
        auto s = BRepPrimAPI_MakeSphere(R).Shape();
        const std::vector<Standard_Real> vols;
        auto box1 = BRepPrimAPI_MakeBox(R1, R1, R1).Shape();
        auto box2 = BRepPrimAPI_MakeBox(-R1, R1, R1).Shape();

        TopoDS_Shape s1 = BRepAlgoAPI_Cut(box1, s);
        TopoDS_Shape s2 = BRepAlgoAPI_Cut(box2, s);

        REQUIRE(CollisionDetector::detectCollisionType(s, s1, vols) ==
                CollisionType::FaceContact); // contact is kind of collision
    }

    SECTION("boundbox_tests")
    {
        gp_Ax2 anAxis, anotherAxis;
        anAxis.SetLocation(gp_Pnt(0.0, 30.0, 0.0));
        anotherAxis.SetLocation(gp_Pnt(0.0, -30.0, 0.0));
        float R = 3;
        float H = 5;

        TopoDS_Shape s = BRepPrimAPI_MakeCylinder(anAxis, R, H).Shape();
        // completely inside itself? no
        Bnd_OBB obb;
        BRepBndLib::AddOBB(s, obb);
        REQUIRE(obb.IsCompletelyInside(obb) == false);
        REQUIRE(obb.IsOut(obb) == false);
    }

    SECTION("JsonConversionTest")
    {
        json j{0, 0, 0, 10, 10, 10};
        Bnd_Box b = j;

        json mj = json{{"name", "materialName"}, {"density", 1000.0}};
        Geom::Material m = mj;
        REQUIRE(m.name == "materialName");
    }

    SECTION("test_geometry_hash")
    {
        float R = 10;
        auto s = BRepPrimAPI_MakeSphere(R).Shape();
        auto s2 = BRepBuilderAPI_Copy(s).Shape();
        REQUIRE(s.HashCode(ItemHashMax) != s2.HashCode(ItemHashMax));
    }

    SECTION("test_geometry_tolerance")
    {
        // detect collision type, by fuse curved surface with diff tolerance
    }
}


TEST_CASE("GeometryImprintTest")
{
    using namespace Geom::OccUtils;
    SECTION("test_glueCoincidentDomain")
    {
        // shape to added into CompSolid must be TopoDS_Solid not any shape
        TopoDS_Solid box1 = BRepPrimAPI_MakeBox(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(10.0, 10.0, 10.0)).Solid();
        TopoDS_Solid box2 = BRepPrimAPI_MakeBox(gp_Pnt(10.0, 0.0, 0.0), gp_Pnt(20.0, 10.0, 10.0)).Solid();
        TopoDS_Solid box3 = BRepPrimAPI_MakeBox(gp_Pnt(0.0, 10.0, 0.0), gp_Pnt(10.0, 20.0, 10.0)).Solid();
        std::vector<TopoDS_Shape> shapes = {box1, box2, box3};
        int NumberOfFaces = shapes.size() * 6 - (shapes.size() - 1);

        TopoDS_CompSolid compSolid;
        TopoDS_Builder cBuilder;

        cBuilder.MakeCompSolid(compSolid);
        cBuilder.Add(compSolid, box1);
        cBuilder.Add(compSolid, box2);
        cBuilder.Add(compSolid, box3);

        // without bool_fragments, shared faces are not removed, even build `cBuilder.MakeCompSolid`
        REQUIRE(countSubShapes(compSolid, TopAbs_FACE) == shapes.size() * 6);
        REQUIRE(countSubShapes(compSolid, TopAbs_COMPSOLID) == 1);

        REQUIRE(countSubShapes(glueFaces(compSolid), TopAbs_FACE) == shapes.size() * 6 - 2);
    }

    SECTION("test_shape_explore")
    {
        std::string tf = "../data/test_geometry/compsolid_with_8faces.brep";
        if (fs::exists(tf))
        {
            const TopoDS_Shape compSolid = OccUtils::loadShape(tf);

            auto info = OccUtils::exploreShape(compSolid);
            for (const auto& k : info)
                INFO("____Number of " + k.first + std::to_string(k.second) + "____\n");
            REQUIRE(info["faces"] != countSubShapes(compSolid, TopAbs_FACE)); // =9
            REQUIRE(info["compsolids"] == 1);

            REQUIRE(countSubShapes(compSolid, TopAbs_FACE) == 8);
            REQUIRE(countSubShapes(compSolid, TopAbs_COMPSOLID) == 1);
        }
    }


    /* merge all solids in a line by one general fuse, must correct     */
    SECTION("test_build_compsolid")
    {
        int length = 10;
        int Nboxes = 6;
        std::vector<TopoDS_Shape> shapes;
        for (int i = 0; i < Nboxes; i++)
        {
            shapes.push_back(
                BRepPrimAPI_MakeBox(gp_Pnt(0.0 + i * length, 0.0, 0.0), gp_Pnt((i + 1) * length, 10.0, 10.0)).Solid());
        }
        size_t NumberOfFaces = shapes.size() * 6 - (shapes.size() - 1);

        auto mkGFA = std::make_shared<BRepAlgoAPI_BuilderAlgo>();
        mkGFA->SetNonDestructive(true);
        // mkGFA->SetGlue(BOPAlgo_GlueShift); can accelerate in case of overlapping, but not interference
        mkGFA->SetRunParallel(false);
        mkGFA->SetUseOBB(true);
        auto res = generalFuse(shapes, 0.0, mkGFA);

        // explode solids from the fragments and rebuild a new compSolid from it
        TopoDS_CompSolid compSolid;
        TopoDS_Builder cBuilder;
        TopExp_Explorer xp;
        cBuilder.MakeCompSolid(compSolid);
#if 0 // make no difference
        for (xp.Init(mkGFA->Shape(), TopAbs_SOLID); xp.More(); xp.Next()) // limited to only SOLID type
        {
            // const TopoDS_Shape& s = BRepBuilderAPI_Copy(xp.Current(), false).Shape(); // copyGeometry must be false
            cBuilder.Add(compSolid, xp.Current());
        }
#else
        for (const auto& s : res)
            cBuilder.Add(compSolid, s);
#endif
        REQUIRE(countSubShapes(compSolid, TopAbs_FACE) == NumberOfFaces);
        REQUIRE(countSubShapes(compSolid, TopAbs_COMPSOLID) == 1);

        //  test read back, data structure
        std::string tf = "_tmp.brep";
        if (fs::exists(tf))
            fs::remove(tf);
        BRepTools::Write(compSolid, tf.c_str());
        REQUIRE(fs::exists(tf));

        const TopoDS_Shape readback = OccUtils::loadShape(tf);
        fs::remove(tf); // after usage, clean up

        REQUIRE(countSubShapes(readback, TopAbs_FACE) == NumberOfFaces);
        REQUIRE(countSubShapes(readback, TopAbs_COMPSOLID) == 1);
    }

    /// boxes in 3D stacking configuration
    SECTION("test_merge_stacked_cubes")
    {
        std::vector<TopoDS_Shape> shapes;
        auto loaded_shape = loadShape("../data/test_geometry/test_cubes.stp");
        for (TopExp_Explorer anExp(loaded_shape, TopAbs_SOLID); anExp.More(); anExp.Next())
        {
            shapes.push_back(anExp.Current());
        }
        int Nboxes = shapes.size();
        int NumberOfFaces = Nboxes * 6 - 12;

        float tolerance = 0; // it is crucially important, even 1e-7 does not work!
        REQUIRE(Nboxes == 8);

        // it is correct to merge multiple solids by this generalFuse()
        auto fuser = std::make_shared<BRepAlgoAPI_BuilderAlgo>();
        generalFuse(shapes, tolerance, fuser);
        summarizeBuilderAlgo(fuser);
        auto merged = fuser->Shape();
        REQUIRE(countSubShapes(merged, TopAbs_FACE) == NumberOfFaces);

        for (int i = 0; i < Nboxes; i++)
        {
            for (int j = i + 1; j < Nboxes; j++)
            {
                std::vector<TopoDS_Shape> ss = {shapes[i], shapes[j]};
                auto fuser = std::make_shared<BRepAlgoAPI_BuilderAlgo>();
                auto res = generalFuse(ss, tolerance, fuser);
                shapes[i] = res[0];
                shapes[j] = res[1];
                if (i == 0 and j == 1)
                    summarizeBuilderAlgo(fuser);
            }
        }
        auto merged2 = createCompound(shapes);
        REQUIRE(countSubShapes(glueFaces(merged2), TopAbs_FACE) == NumberOfFaces);
    }

    SECTION("test_merge_boxes_in_a_matrix")
    {
        float tolerance = 0; // it is crucially important, even 1e-7 does not work!
        float length = 10;   // box edge size
        std::vector<TopoDS_Shape> shapes;

        // matrix layout, some are shared faces, some share only edges
        int Nboxes = 4;
        float L = length;
        shapes.push_back(BRepPrimAPI_MakeBox(gp_Pnt(0.0, 0.0, 0.0), gp_Pnt(L, L, L)).Solid());
        shapes.push_back(BRepPrimAPI_MakeBox(gp_Pnt(L, 0.0, 0.0), gp_Pnt(2 * L, L, L)).Solid());
        shapes.push_back(BRepPrimAPI_MakeBox(gp_Pnt(L, L, 0.0), gp_Pnt(2 * L, 2 * L, L)).Solid());
        shapes.push_back(BRepPrimAPI_MakeBox(gp_Pnt(0, L, 0.0), gp_Pnt(L, 2 * L, L)).Solid());

        int batchSize = 2; // set as 2 to simulate GeomemtryMerger
        int Nbatch = Nboxes;
        int NumberOfFaces = Nboxes * 6 - Nboxes;

        // it is confirmed that unifyFaces work only on solids after generalfuse()
        std::vector<std::shared_ptr<BRepAlgoAPI_BuilderAlgo>> fusers;
        // fusers.resize(Nbatch);
        for (int i = 0; i < Nbatch; i++)
        {
            fusers.push_back(std::make_shared<BRepAlgoAPI_BuilderAlgo>());
            fusers[i]->SetRunParallel(false);
            // fusers[i]->SetNonDestructive(true);  // it is always true
        }

        for (int i = 0; i < Nbatch; i++)
        {
            std::vector<TopoDS_Shape> ss;
            for (int j = 0; j < batchSize; j++)
            {
                auto index = (i + j) % Nboxes;
                ss.push_back(shapes[index]);
            }
            auto res = generalFuse(ss, tolerance, fusers[i]);
            for (int j = 0; j < batchSize; j++)
            {
                auto index = (i + j) % Nboxes;
                shapes[index] = res[j];
            }
        }

        // explode solids from the fragments and rebuild a new compSolid from it
        TopoDS_Builder cBuilder;
        TopExp_Explorer xp;
        // compound and compsolid does not make diff
        TopoDS_CompSolid compSolid;
        cBuilder.MakeCompSolid(compSolid);

        for (const auto& res : fusers)
        {
            for (TopExp_Explorer anExp(res->Shape(), TopAbs_SOLID); anExp.More(); anExp.Next())
            {
                cBuilder.Add(compSolid, anExp.Current());
            }
        }

        // if boolean fragments are not done, flueFaces() will not work
        REQUIRE(countSubShapes(glueFaces(compSolid), TopAbs_FACE) == NumberOfFaces);

        auto tp = fs::temp_directory_path() / "_tmp_matrixboxes.step";
        saveShape(glueFaces(compSolid), tp.string());
        REQUIRE(countSubShapes(unifyFaces(loadShape(tp.string())), TopAbs_FACE) != NumberOfFaces);
        // this test confirm that shared faces can be saved to file and loaded to memory in brep format
        tp = fs::temp_directory_path() / "_tmp_matrixboxes.brep";
        saveShape(glueFaces(compSolid), tp.string());
        REQUIRE(countSubShapes(loadShape(tp.string()), TopAbs_FACE) == NumberOfFaces);
    }

    SECTION("test_merge_boxes_in_a_line")
    {
        float tolerance = 0; // it is crucially important, even 1e-7 does not work!
        float length = 10;   // box edge size
        std::vector<TopoDS_Shape> shapes;

        int Nboxes = 8;
        for (int i = 0; i < Nboxes; i++)
        {
            double yOffset = 0.0;
            if (i % 2 == 1)
                yOffset = 5.0;
            auto b = BRepPrimAPI_MakeBox(gp_Pnt(0.0 + i * length, yOffset, 0.0),
                                         gp_Pnt((i + 1) * length, 10.0 + yOffset, 10.0));
            shapes.push_back(b.Solid());
        }

        int batchSize = 2; // set as 2 to simulate GeomemtryMerger
        int Nbatch = Nboxes - (batchSize - 1);
        // int NumberOfFaces = Nboxes * 6 - (Nboxes - 1);
        int NumberOfFaces = Nboxes * 6 + (Nboxes - 1); // if there is yOffset


        // it is confirmed that unifyFaces work only on solids after generalfuse()
        std::vector<std::shared_ptr<BRepAlgoAPI_BuilderAlgo>> fusers;
        // fusers.resize(Nbatch);
        for (int i = 0; i < Nbatch; i++)
        {
            fusers.push_back(std::make_shared<BRepAlgoAPI_BuilderAlgo>());
            fusers[i]->SetRunParallel(false);
            // fusers[i]->SetNonDestructive(true);  // it is always true
        }

        // batch size = 2 only, to simulate GeometryImprinter class
        for (int i = 0; i < Nboxes; i += batchSize)
        {
            std::vector<TopoDS_Shape> ss;
            for (int j = 0; j < batchSize; j++)
                ss.push_back(shapes[i + j]);
            auto res = generalFuse(ss, tolerance, nullptr);
            for (int j = 0; j < batchSize; j++)
                shapes[i + j] = res[j];
        }
        // merge 1 and 2, 3 and 4, ...
        for (int i = 1; i < Nboxes - batchSize; i += batchSize)
        {
            std::vector<TopoDS_Shape> ss;
            for (int j = 0; j < batchSize; j++)
                ss.push_back(shapes[i + j]);
            auto res = generalFuse(ss, tolerance, nullptr);
            for (int j = 0; j < batchSize; j++)
                shapes[i + j] = res[j];
        }

        // explode solids from the fragments and rebuild a new compSolid from it
        TopoDS_Builder cBuilder;
        TopExp_Explorer xp;
        // compound and compsolid does not make diff
        TopoDS_CompSolid compSolid;
        cBuilder.MakeCompSolid(compSolid);

        for (const auto& s : shapes)
            cBuilder.Add(compSolid, s);

        // if boolean fragments are not done, flueFaces() will not work
        REQUIRE(countSubShapes(glueFaces(compSolid), TopAbs_FACE) == NumberOfFaces);
        // REQUIRE(countSubShapes(unifyFaces(compSolid), TopAbs_FACE) == NumberOfFaces);


        auto tp = fs::temp_directory_path() / "_tmp_lineboxes.step";
        saveShape(glueFaces(compSolid), tp.string());
        REQUIRE(countSubShapes(unifyFaces(loadShape(tp.string())), TopAbs_FACE) != NumberOfFaces);
        // this test confirm that shared faces can be saved to file and loaded to memory in brep format
        tp = fs::temp_directory_path() / "_tmp_lineboxes.brep";
        saveShape(glueFaces(compSolid), tp.string());
        REQUIRE(countSubShapes(loadShape(tp.string()), TopAbs_FACE) == NumberOfFaces);
    }

    SECTION("test_shape_distance")
    {
        auto b1 = BRepPrimAPI_MakeBox(gp_Pnt(0, 0, 0), gp_Pnt(10, 10, 10)).Shape();
        auto b2 = BRepPrimAPI_MakeBox(gp_Pnt(20, 0, 0), gp_Pnt(30, 10, 10)).Shape();

        auto d = distance(b1, b2);
        std::cout << "distance between two shapes (min between 2 points) is " << d << std::endl;
        REQUIRE(floatEqual(d, 10.0));

        auto b3 = BRepPrimAPI_MakeBox(gp_Pnt(10, 0, 0), gp_Pnt(20, 10, 10)).Shape();
        REQUIRE(floatEqual(distance(b1, b3), 0.0)); // face contact

        auto b4 = BRepPrimAPI_MakeBox(gp_Pnt(5, 0, 0), gp_Pnt(15, 10, 10)).Shape();
        REQUIRE(floatEqual(distance(b1, b4), 0.0)); // interference
    }
}



#if OCC_VERSION_HEX >= 0x070400
/// there is a bug in and before OCCT 7.3, it will hang forever
/// in BOP check and general fuse boolean operation
/// a commit fixed this in June 2019, so OCCT 7.4 is working
TEST_CASE("BOPCheck", "BOPCheck")
{
    // using namespace std;
    BRep_Builder builder;
    TopoDS_Shape shape;
    const char* shape_path = "../data/test_geometry/test_bop_check.brep";
    bool success = BRepTools::Read(shape, shape_path, builder);
    REQUIRE(not shape.IsNull());

    TopTools_IndexedMapOfShape map;
    TopExp::MapShapes(shape, TopAbs_FACE, map);

    int n_faces = map.Extent();
    std::cout << n_faces << " faces in theloaded shape" << std::endl;

    BOPAlgo_ArgumentAnalyzer BOPCheck;
    TopoDS_Shape BOPCopy = BRepBuilderAPI_Copy(shape).Shape();
    BOPCheck.SetShape1(BOPCopy);

    //   BOPCheck.SetShape1(shape);
    BOPCheck.ArgumentTypeMode() = true;
    BOPCheck.SelfInterMode() = true;
    BOPCheck.SmallEdgeMode() = true;
    BOPCheck.RebuildFaceMode() = true;
    BOPCheck.ContinuityMode() = false;
    BOPCheck.SetParallelMode(false);
    BOPCheck.SetRunParallel(false);
    BOPCheck.TangentMode() = true;
    BOPCheck.MergeVertexMode() = true;
    BOPCheck.MergeEdgeMode() = true;
    BOPCheck.CurveOnSurfaceMode() = false;

    BOPCheck.Perform();

    REQUIRE(not BOPCheck.HasFaulty()); // it is a problematic geometry file in OCCT 7.3
}
#endif

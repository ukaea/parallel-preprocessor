
#ifndef PPP_GEOMETRY_READER_H
#define PPP_GEOMETRY_READER_H

#include "GeometryData.h"
#include "GeometryProcessor.h"
#include "PPP/Reader.h"

#include "OpenCascadeAll.h"


namespace Geom
{
    using namespace PPP;

    /// \ingroup Geom
    /**
     * read geometry file and generate GeometryData object
     * must be in sequential mode, should derived from App::Reader,
     * for the input manifest of multiple geometry files list, parallel reading is possible.
     *
     * Aupported geometry file formats
     * + step/stp AP214(with material and color meta data) OpenCASCADE XCAF
     * + IGES/igs: OpenCASCADE XCAF reader
     * + FreeCAD native format  *.FCStd
     * + parallel preproessor output: *.brep + *_metadata.json(*.metadata.json)
     * + manifest.json textual format: a list of filename-materail json objects
     *     this json file must ended with "manifest.json", here is an example:
     *    ```json
     *    [{
     *       "material": "one_material",
     *       "filename": "absolute_path or path_relative_to_this_json_file/something.stp"
     *    },
     *    {
     *       "material": "another material",
     *       "filename": "absolute_path or path_relative_to_this_json_file/something.stp"
     *    }]
     *    ```
     *    Note: 1. filename key can be any string contains substring "filename" such as "stp_filename"
     *           but recommend to use just "filename".
     *          2. currently all input geometry format must be of the same format.
     */
    class GeometryReader : public Reader
    {
        TYPESYSTEM_HEADER();

    private:
        // some info/objects should be obtained from static methods of object/singleton app/context class
        // const PipelineController* myProcessor;

        /// @{
        Handle(XCAFApp_Application) hApp; // not quite necessary
        Handle(TDocStd_Document) hDoc;    // leave it uninitialized, IsNull()

        Handle(XCAFDoc_ShapeTool) myShapeTool;
        Handle(XCAFDoc_ColorTool) myColorTool;
        Handle(XCAFDoc_MaterialTool) myMaterialTool;

        std::vector<TDF_Label> myShapeLabels;
        std::set<ItemHashType> myRefShapes;
        /// @}

        /// @{
        /// load from CAFDoc, then std::move into propertyContainer
        // std::map is not thread-safe for modification, even not at the same time
        // MapType
        ItemContainerType mySolids;
        ItemContainerType myShells;
        ItemContainerType myCompounds;
        ItemContainerType myOtherShapes; // toplevel free
        /// STEP meta data
        MapType<ItemHashType, Quantity_Color> myColorMap;
        MapType<ItemHashType, Material> myMaterialMap;
        MapType<ItemHashType, std::string> myNameMap;
        // MapType<ItemHashType, ItemHashType> myParentMap;
        // MapType<ItemHashType, bool> mySuppressionMap;
        /// @}

    public:
        virtual void process() override
        {
            std::string file_name = myConfig["dataFileName"];
            read(file_name);
        }

        virtual void prepareOutput() override
        {
            myOutputData = std::make_shared<GeometryData>();
            this->moveShapeIntoPropertyContainer();
        }

    protected:
        void read(std::string file_name)
        {
            if (not fs::exists(file_name))
            {
                auto fp = fs::absolute(fs::path(file_name)).string().c_str();
                LOG_F(ERROR, "input file %s does not exist", fp);
                return;
            }

            // todo: HDF5 or zipped container, unzip first
            if (Utilities::hasFileExt(file_name, "stp") || Utilities::hasFileExt(file_name, "step") ||
                Utilities::hasFileExt(file_name, "igs") || Utilities::hasFileExt(file_name, "iges"))
            {
                this->readXCAFDoc(file_name);
                this->loadXCAFDoc();
            }
            else if (Utilities::hasFileExt(file_name, "FCStd"))
            {
                this->readFreeCADFile(file_name);
            }
            else if (Utilities::hasFileExt(file_name, "json"))
            {
                this->readManifestFile(file_name);
            }
            else if (Utilities::hasFileExt(file_name, "brep") || Utilities::hasFileExt(file_name, "brp"))
            {
                readBrep(file_name);
            }
            else
            {
                LOG_F(ERROR, "only file suffix .stp/.step, .iges/.igs, .brep, *manifest.json, *.FCStd are supported");
            }
            this->summary();
        }

        /**
         * filename must has this pattern: projectname_manifest.json or projectname.manifest.json
         */
        void readManifestFile(std::string filename)
        {
            json manifest;
            fs::path manifest_dir = fs::path(filename).parent_path();
            std::ifstream i(filename);
            i >> manifest;
            for (const auto& p : manifest)
            {
                /// NOTE: requested by ISSUE 12, filename key now accepts this pattern  "*filename"
                std::string filename_key = "filename";
                if (not p.contains(filename_key))
                {
                    for (json::const_iterator it = p.begin(); it != p.end(); ++it)
                    {
                        if (it.key().find(filename_key) != std::string::npos)
                        {
                            filename_key = it.key();
                            break; /// break the for loop, use the first key found with the pattern "*filename"
                        }
                    }
                    if (not p.contains(filename_key))
                    {
                        LOG_F(ERROR, "key `%s` does not exist `%s` ", filename_key.c_str(), p.dump(4).c_str());
                    }
                }

                std::string file_name = p[filename_key].get<std::string>();
                std::string file_path = file_name;
                if (fs::path(file_name).is_relative())
                    file_path = (manifest_dir / file_name).string();
                // calc the abs path, by refer to manifest file
                if (fs::exists(fs::path(file_path)))
                {
                    LOG_F(INFO, "read file: %s", file_path.c_str());
                    read(file_path, p);
                }
                else
                {
                    LOG_F(ERROR, "input file: `%s` does not exist", file_path.c_str());
                }
            }
        }

        /**
         * used by readManifestFile()
         */
        void read(std::string file_name, const json& metadata)
        {
            if (Utilities::hasFileExt(file_name, "stp") || Utilities::hasFileExt(file_name, "step") ||
                Utilities::hasFileExt(file_name, "igs") || Utilities::hasFileExt(file_name, "iges"))
            {
                this->readXCAFDoc(file_name);
                this->loadXCAFDoc(metadata);
            }
            else if (Utilities::hasFileExt(file_name, "FCStd"))
            {
                this->readFreeCADFile(file_name);
            }
            else if (Utilities::hasFileExt(file_name, "brep") || Utilities::hasFileExt(file_name, "brp"))
            {
                readBrep(file_name, metadata);
            }
            else
            {
                LOG_F(ERROR, "only file suffix .stp or .step, .iges, .igs, .brep, *.FCStd are supported");
            }
        }

        /// composolid and compoundshare, only for FreeCAD file reader
        /// material, color, name+id
        /// todo: assembly structure, parent relationship
        void registerCompoundShape(const TopoDS_Shape& shape, const json& p)
        {
            if (shape.ShapeType() == TopAbs_COMPOUND)
            {
                int solidCount = 0;
                TopExp_Explorer Ex(shape, TopAbs_SOLID);
                while (Ex.More())
                {
                    const TopoDS_Shape& s = Ex.Current();
                    json pp = p;
                    pp["name"] = p["name"].get<std::string>() + "_component" + std::to_string(solidCount);
                    registerShape(s, pp);
                    solidCount++;
                    Ex.Next();
                }
                if (solidCount) // debugging
                {
                    const char* ns = p["name"].get<std::string>().c_str();
                    LOG_F(INFO, "compound shape `%s` has %d components loaded ", ns, solidCount);
                }
            }
        }

        /// can register FreeCAD metadata and PPP output brep + json
        void registerShape(const TopoDS_Shape& s, const json p)
        {
            if (s.ShapeType() == TopAbs_SOLID)
            {
                ItemHashType hid = s.HashCode(ItemHashMax);

                mySolids.insert(std::make_pair(hid, s));

                // todo: step subassembly structure,  `parent`
                if (p.contains("name") and (not p["name"].is_null()))
                {
                    myNameMap.insert(std::make_pair(hid, p["name"].get<std::string>()));
                }
                else
                {
                    myNameMap.insert(std::make_pair(hid, "shape" + std::to_string(hid)));
                }
                if (p.contains("color") and p["color"].is_array())
                {
                    Quantity_Color c = p["color"];
                    myColorMap.insert(std::make_pair(hid, c));
                }
                if (p.contains("material") and not p["material"].empty()) /// in case of ppp output meta data
                {
                    Material m = p["material"].get<Material>();
                    myMaterialMap.insert(std::make_pair(hid, m));
                }
                if (p.contains("groups")) /// in case freecad groups means material
                {
                    std::vector<std::string> mat = p["groups"];
                    if (mat.size() > 0) // this is list type, but can be empty
                    {
                        Material m;
                        m.name = mat[0];
                        myMaterialMap.insert(std::make_pair(hid, m));
                    }
                }
            }
            else
            {
                const char* ns = p["name"].get<std::string>().c_str();
                LOG_F(WARNING, "shape `%s` is not solid type, will not be loaded ", ns);
            }
        }

        /// UniqueIdValidation.cpp
        void readBrep(std::string filename, const json default_metadata = json())
        {
            typedef uint64_t UniqueIdType;
            json metadata;
            std::unordered_map<uint64_t, json> gpMap;
            if (this->myConfig.contains("metadataFileName")
                && !this->myConfig["metadataFileName"].is_null())
            {
                std::string mfile = this->myConfig["metadataFileName"];
                if (not fs::exists(mfile))
                {
                    LOG_F(WARNING, "metadataFileName `%s` provided config does not exist!", mfile.c_str());
                }
                else
                {
                    std::ifstream i(mfile);
                    i >> metadata;
                    for (const auto& p : metadata)
                    {
                        gpMap.insert_or_assign(p["uniqueId"].get<UniqueIdType>(), p);
                        // idSet.insert(p["uniqueId"].get<uint64_t>());
                    }
                }
            }
            else
            {
                if (default_metadata.is_null())
                {
                    LOG_F(WARNING, "only shape are read, no meta data (json file) found or provided as default");
                    LOG_F(WARNING, " default geometry metadata like material color, etc should be provided here");
                }
            }
            // read a Compound or CompSolid, return a vector of TopoDS_Shape of Solid type

            BRep_Builder cBuilder;
            TopoDS_Shape shape;
            BRepTools::Read(shape, filename.c_str(), cBuilder);
            if (shape.IsNull())
                throw std::runtime_error("brep read data is Null for:" + filename);

            // explore solids and calc Id
            int solidCount = 0;
            TopExp_Explorer Ex(shape, TopAbs_SOLID);
            while (Ex.More())
            {
                const TopoDS_Shape& s = Ex.Current();
                UniqueIdType gid = OccUtils::uniqueId(s);
                if (gpMap.size() > 0)
                {
                    const auto& p = gpMap[gid];
                    bool suppressed = false;
                    if (p.contains("suppressed")) /// in case ppp output, mark it as suppressed
                    {
                        suppressed = p["suppressed"].get<bool>();
                    }
                    if (not suppressed)
                        registerShape(s, p);
                    else
                    {
                        const char* ns = p["name"].get<std::string>().c_str();
                        LOG_F(WARNING, "shape `%s` has been marked as suppressed, so it will not be loaded ", ns);
                    }
                }
                else
                {
                    json mdata = json{{"name", "geometry"}, {"material", "unknown"}};
                    // if default meta data can be generated, it is possible to read
                    registerShape(s, mdata);
                }
                solidCount++; // sequenceID, started with zero
                Ex.Next();
            }
        }
        /// const json default_metadata = json() not used for FreeCAD data type
        void readFreeCADFile(std::string filename)
        {
            fs::path output_dir = fs::temp_directory_path() / "test_fc_reader";
            fs::path fc_parser = "pppFreeCADparser.py"; //
            fs::path fc_parser_path = Processor::globalParameter<std::string>("scriptFolder") / fc_parser;
            if (not fs::exists(fc_parser_path))
            {
                throw ProcessorError("Python script is not found: " + fc_parser_path.string());
            }
            Utilities::runCommand("python3 " + fc_parser_path.string() + " " + filename + " " + output_dir.string());
            fs::path metadata_filepath = fs::path(output_dir) / "metadata.json";
            json metadata;
            if (fs::exists(metadata_filepath))
            {
                std::ifstream i(metadata_filepath);
                i >> metadata;
                loadFreeCADShapes(output_dir.string(), metadata);
            }
            else
            {
                throw ProcessorError("metadata file does not exists, check the output dir: " + output_dir.string());
            }
        }

        /*  not completed code
        void placeShape(TopoDS_Shape s)
        {
            // Mod/Part/App/GeomFeature.cpp
            Base::Placement pl = this->Placement.getValue();
            Base::Rotation rot(pl.getRotation());
            Base::Vector3d axis;
            double angle;
            rot.getValue(axis, angle);

            gp_Trsf trf;
            trf.SetRotation(gp_Ax1(gp_Pnt(), gp_Dir(axis.x, axis.y, axis.z)), angle);
            trf.SetTranslationPart(gp_Vec(pl.getPosition().x,pl.getPosition().y,pl.getPosition().z));
            return TopLoc_Location(trf);

        }
        */

        void loadFreeCADShapes(std::string data_dir, const json& metadata)
        {
            for (const auto& p : metadata)
            {
                std::string f = p["filename"].get<std::string>();
                auto fp = fs::path(data_dir) / f;
                auto s = OccUtils::loadShape(fp.string());
                // placeShape(s); //  not needed

                bool suppressed = false;
                if (p.contains("visible")) /// in case FreeCAD file format
                {
                    suppressed = not p["visible"].get<bool>();
                }
                if (not suppressed)
                {
                    if (s.ShapeType() == TopAbs_COMPOUND or s.ShapeType() == TopAbs_COMPSOLID)
                        registerCompoundShape(s, p);
                    else
                        registerShape(s, p);
                }
                else
                {
                    const char* ns = p["name"].get<std::string>().c_str();
                    LOG_F(WARNING, "shape `%s` has been marked as invisible, so it will not be loaded ", ns);
                }
            }
        }

        ///////////////////// step and iges XCAF format ////////////////////////////////
        /**
         * for step and iges only, using OpenCASCADE OCAF reader
         */

        /** get a basic reader STEPControl_Reader */
        void checkUnits(STEPControl_Reader& reader)
        {
            TColStd_SequenceOfAsciiString theUnitLengthNames, theUnitAngleNames, theUnitSolidAngleNames;
            reader.FileUnits(theUnitLengthNames, theUnitAngleNames, theUnitSolidAngleNames);
            for (const auto& s : theUnitLengthNames)
            {
                LOG_F(INFO, "length unit of input file is %s", s.ToCString());
            }
        }


        /** This function is adapted from FreeCAD project
         * FreeCAD/src/Mod/Part/App/ImportOCAF.cpp
         */
        bool readXCAFDoc(std::string file_name)
        {
            // app =  Handle(TDocStd_Application)::DownCast(doc->Application());
            // Handle(TDocStd_Dataset) hDoc;
            hApp = XCAFApp_Application::GetApplication();
            /*
            if (!CDF_Session::Exists()) {
                Handle(CDF_Session) S = CDF_Session::CurrentSession();
                if (!S->HasCurrentApplication())
                Standard_DomainError::Raise("DDocStd::Find no applicative session");
                app = Handle(TDocStd_Application)::DownCast(S->CurrentApplication());
            }
            else {
                // none active application
                std::printf("")
            }
            */

            if (hDoc)
            {
                if (!this->hDoc->IsEmpty())
                {
                    if (this->hDoc->IsValid())
                    {
                        this->hApp->Close(this->hDoc);
                    }
                    else
                    {
                        LOG_F(ERROR, "document to be closed is not valid");
                    }
                }
                else
                {
                    LOG_F(INFO, "document should be empty when open function is called");
                }
            }
            // "MDTV-CAF" is deprecated, readonly, using xml or binary format
            hApp->NewDocument(TCollection_ExtendedString("MDTV-CAF"), hDoc); //

            try
            {
                bool ret = false;
                if (Utilities::hasFileExt(file_name, "stp") || Utilities::hasFileExt(file_name, "step"))
                {
                    STEPCAFControl_Reader aReader;
                    aReader.SetColorMode(true);
                    aReader.SetNameMode(true);
                    aReader.SetMatMode(true);
                    // aReader.SetLayerMode(true);
                    if (aReader.ReadFile((Standard_CString)(file_name.c_str())) != IFSelect_RetDone)
                    {
                        throw OSD_Exception("cannot read STEP file");
                    }
                    auto reader = aReader.Reader();
                    checkUnits(reader);
                    // IFSelect_PrintCount mode = IFSelect_CountByItem;
                    // STEPControl_Reader has the check method:PrintCheckLoad(failsonly, mode);

                    // Handle(Message_ProgressIndicator) pi = new ProgressIndicator(100);
                    // aReader.Reader().WS()->MapReader()->SetProgress(pi);  // do I must have a progress?
                    // pi->NewScope(100, "Reading STEP file...\n");
                    // pi->Show();
                    ret = aReader.Transfer(hDoc);
                    // pi->EndScope();
                }
                else if (Utilities::hasFileExt(file_name, "igs") || Utilities::hasFileExt(file_name, "iges"))
                {
                    // IGESControl_Controller::Init();
                    IGESCAFControl_Reader aReader;
                    aReader.SetColorMode(true);
                    aReader.SetNameMode(true);
                    // aReader.SetMatMode(true);   // no such method

                    // IGESControl_Reader aReader;
                    if (aReader.ReadFile((Standard_CString)(file_name.c_str())) != IFSelect_RetDone)
                    {
                        throw OSD_Exception("cannot read IGES file");
                    }
                    // Handle(Message_ProgressIndicator) pi = new ProgressIndicator(100);
                    // aReader.Reader().WS()->MapReader()->SetProgress(pi);  // do I must have a progress?
                    // pi->NewScope(100, "Reading STEP file...\n");
                    // pi->Show();
                    // Standard_Integer nbRootsForTransfer = aReader.NbRootsForTransfer();
                    aReader.NbRootsForTransfer();
                    ret = aReader.Transfer(hDoc); // translate from step/iges to XDE Dataset
                    // pi->EndScope();

                    /// load shapes without XCAF document is possible ,see FreeCAD source code
                }
                else
                {
                    std::cout << "only step file with suffix .stp or .step, igs, iges are supported" << std::endl;
                    return false;
                }

                return ret;
            }
            catch (OSD_Exception& e)
            {
                LOG_F(ERROR, "Error during reading step file in OCCT: %s\n",
                      e.GetMessageString()); // todo: a better log
                return false;
            }
        }

        /** all prepration before traversing and processing
         * todo: overloading by providing default material property via json meta data
         */
        void loadXCAFDoc(const json default_metadata = json())
        {
            // collect sequence of labels to display
            myShapeTool = XCAFDoc_DocumentTool::ShapeTool(hDoc->Main());
            myColorTool = XCAFDoc_DocumentTool::ColorTool(hDoc->Main());
            myMaterialTool = XCAFDoc_DocumentTool::MaterialTool(hDoc->Main());

            TDF_LabelSequence shapeLabels;
            myShapeTool->GetFreeShapes(shapeLabels); // get all toplevel, not referred by other

            // set presentations and show
            LOG_F(INFO, "load the toplevel free shapes (not referred) nb #%d", shapeLabels.Length());
            for (Standard_Integer i = 1; i <= shapeLabels.Length(); i++)
            {
                // get the shapes and attributes
                const TDF_Label& label = shapeLabels.Value(i);
                loadShapes(label, default_metadata);
            }
        }

        void extractXCAFMetadata(const TDF_Label& label, const TopoDS_Shape& aShape, const json metadata = json())
        {
            // getting material
            Handle(XCAFDoc_Material) MatAttr;
            if (label.FindAttribute(XCAFDoc_Material::GetID(), MatAttr))
            {
                Material m;
                m.name = MatAttr->GetName()->ToCString();
                m.density = MatAttr->GetDensity();
                myMaterialMap[aShape.HashCode(ItemHashMax)] = m;
            }
            else
            {
                if (not metadata.is_null())
                {
                    Material m = metadata["material"];
                    myMaterialMap[aShape.HashCode(ItemHashMax)] = m;
                }
            }

            //  XCAFDoc_ColorGen is generic
            Quantity_Color col; // default to yellow color
            if (myColorTool->GetColor(label, XCAFDoc_ColorGen, col) ||
                myColorTool->GetColor(label, XCAFDoc_ColorSurf, col) ||
                myColorTool->GetColor(label, XCAFDoc_ColorCurv, col))
            {
                // add defined color
                myColorMap[aShape.HashCode(ItemHashMax)] = col;
            }
            /*  getting color of solids instead of all other subshapes
            else
            {
                // http://www.opencascade.org/org/forum/thread_17107/
                TopoDS_Iterator it;
                for (it.Initialize(aShape); it.More(); it.Next())
                {
                    if (myColorTool->GetColor(it.Value(), XCAFDoc_ColorGen, col) ||
                        myColorTool->GetColor(it.Value(), XCAFDoc_ColorSurf, col) ||
                        myColorTool->GetColor(it.Value(), XCAFDoc_ColorCurv, col))
                    {
                        // add defined color
                        myColorMap[it.Value().HashCode(ItemHashMax)] = col;
                    }
                }
            }
            */

            // getting names, shall we just get top level names?
            Handle(TDataStd_Name) name;
            if (label.FindAttribute(TDataStd_Name::GetID(), name))
            {
                TCollection_ExtendedString extstr = name->Get();
                char* str = new char[extstr.LengthOfCString() + 1];
                extstr.ToUTF8CString(str);
                std::string label(str);
                if (!label.empty()) //  'Open CASCADE STEP translator 7.3 5' not quite useful
                    myNameMap[aShape.HashCode(ItemHashMax)] = label;
                delete[] str;
            }

            if (true)
            {
                if (not metadata.is_null())
                {
                    std::string pname = "";
                    if (metadata.contains("filename"))
                        pname = metadata["filename"]; // todo: component id, to give unique name
                    else if (metadata.contains("name"))
                        pname = metadata["name"];
                    else
                    {
                        /* left is empty */
                    }
                    myNameMap[aShape.HashCode(ItemHashMax)] = pname;
                }
            }
        }

        void loadShapes(const TDF_Label& label, const json metadata = json())
        {
            TopoDS_Shape aShape;
            if (myShapeTool->GetShape(label, aShape))
            {
                // if (myShapeTool->IsReference(label)) {
                //    TDF_Label reflabel;
                //    if (myShapeTool->GetReferredShape(label, reflabel)) {
                //        loadShapes(reflabel);
                //    }
                //}
                myShapeLabels.push_back(label); // the container stores the copy not reference
                if (myShapeTool->IsTopLevel(label))
                {
                    int ctSolids = 0, ctShells = 0, ctComps = 0;
                    // add the shapes
                    TopExp_Explorer xp;
                    for (xp.Init(aShape, TopAbs_SOLID); xp.More(); xp.Next(), ctSolids++)
                    {
                        //
                        auto hKey = xp.Current().HashCode(ItemHashMax);
                        if (mySolids.find(hKey) == mySolids.end())
                        {
                            this->mySolids[hKey] = (xp.Current());
                            // todo: register metadata
                            // todo: assembly parent relationship
                        }
                        else
                            LOG_F(ERROR, "shape hash key has existed");
                    }
                    for (xp.Init(aShape, TopAbs_SHELL); xp.More(); xp.Next(), ctShells++)
                        this->myShells[xp.Current().HashCode(ItemHashMax)] = (xp.Current());
                    // if no solids and no shells were found then go for compounds
                    if (ctSolids == 0 && ctShells == 0)
                    {
                        for (xp.Init(aShape, TopAbs_COMPOUND); xp.More(); xp.Next(), ctComps++)
                        {
                            this->myCompounds[xp.Current().HashCode(ItemHashMax)] = (xp.Current());
                            // solids should have been extracted
                        }
                    }
                    if (ctComps == 0)
                    { // why not   `&& ctSolids == 0 && ctShells == 0`
                        for (xp.Init(aShape, TopAbs_FACE, TopAbs_SHELL); xp.More(); xp.Next())
                            this->myOtherShapes[xp.Current().HashCode(ItemHashMax)] = (xp.Current());
                        for (xp.Init(aShape, TopAbs_WIRE, TopAbs_FACE); xp.More(); xp.Next())
                            this->myOtherShapes[xp.Current().HashCode(ItemHashMax)] = (xp.Current());
                        for (xp.Init(aShape, TopAbs_EDGE, TopAbs_WIRE); xp.More(); xp.Next())
                            this->myOtherShapes[xp.Current().HashCode(ItemHashMax)] = (xp.Current());
                        for (xp.Init(aShape, TopAbs_VERTEX, TopAbs_EDGE); xp.More(); xp.Next())
                            this->myOtherShapes[xp.Current().HashCode(ItemHashMax)] = (xp.Current());
                    }

                    gp_Pnt pos; // not in used, also like volume, can be used to validate geometry
                    Handle(XCAFDoc_Centroid) C;
                    label.FindAttribute(XCAFDoc_Centroid::GetID(), C);
                    if (!C.IsNull())
                        pos = C->Get();
                }

                extractXCAFMetadata(label, aShape, metadata);

                // assembly supported, while the processed lose structure info
                // sub-assembly can be natural boundary for geometry decomposition
                if (myShapeTool->IsAssembly(label))
                {
                    TDF_LabelSequence shapeLabels;
                    myShapeTool->GetComponents(label, shapeLabels);
                    Standard_Integer nbShapes = shapeLabels.Length();
                    for (Standard_Integer i = 1; i <= nbShapes; i++)
                    {
                        loadShapes(shapeLabels.Value(i), metadata);
                        // todo: assembly parent relationship
                    }
                }

                if (label.HasChild())
                {
                    TDF_ChildIterator it;
                    for (it.Initialize(label); it.More(); it.Next())
                    {
                        loadShapes(it.Value(), metadata); // recursive call to this function
                        // todo: assembly parent relationship
                    }
                }
            }
        }


        /** print loaded shapes statistics, before moved into propertyContainer
         * this info could be output to log stream,*/
        void summary()
        {
            std::stringstream sout;
            sout << " ======= input geometry summary =======" << std::endl;
            sout << " length of solids: " << mySolids.size() << std::endl;
            sout << " length of shells: " << myShells.size() << std::endl;
            sout << " length of compounds: " << myCompounds.size() << std::endl;
            sout << " length of name map: " << myNameMap.size() << std::endl;
            sout << " length of color map: " << myColorMap.size() << std::endl;
            sout << " length of material map: " << myMaterialMap.size() << std::endl;
            LOG_F(INFO, "%s", sout.str().c_str());
        }

        /** only items of ShapeType::Solid will be move
         * after move, private members such as mySolids are empty,
         * using the public property get and set API
         */
        void moveShapeIntoPropertyContainer()
        {
            std::vector<ItemHashType> mySolidIDs;
            MapType<ItemHashType, ShapeErrorType> myShapeErrors;
            for (auto const& item : mySolids) // what is the sequence?
            {
                mySolidIDs.push_back(item.first); // the sequence
                myShapeErrors[item.first] = ShapeErrorType::NoError;
            }

            myOutputData->setValue<ShapeType>("myShapeType", ShapeType::Solid);
            myOutputData->setItemCount(mySolids.size());
            // emplace equal to the two step above
            myOutputData->emplace("mySolids", std::move(mySolids));
            myOutputData->emplace("mySolidIDs", std::move(mySolidIDs));
            myOutputData->emplace("myShapeErrors", std::move(myShapeErrors));

            // those three types are not used in this PPP
            myOutputData->emplace("myShells", std::move(myShells));
            myOutputData->emplace("myCompounds", std::move(myCompounds));
            myOutputData->emplace("myOtherShapes", std::move(myOtherShapes));

            // STEP214 meta data
            myOutputData->emplace("myColorMap", std::move(myColorMap));
            myOutputData->emplace("myMaterialMap", std::move(myMaterialMap));
            myOutputData->emplace<MapType<ItemHashType, std::string>>("myNameMap", std::move(myNameMap));
        }

    }; // end of class

} // namespace Geom

#endif

#ifndef PPP_GEOMETRY_WRITER_H
#define PPP_GEOMETRY_WRITER_H


#include "GeometryProcessor.h"
#include "OccUtils.h"
#include "PPP/Writer.h"


namespace Geom
{
    using namespace PPP;

    /// \ingroup Geom
    /**
     * \brief write out processed geometry into files
     */
    class GeometryWriter : public Writer
    {
        TYPESYSTEM_HEADER();

    private:
        /// @{

        Handle(XCAFDoc_ShapeTool) myShapeTool;
        Handle(XCAFDoc_ColorTool) myColorTool;
        Handle(XCAFDoc_MaterialTool) myMaterialTool;

        std::vector<TDF_Label> myShapeLabels;
        std::set<Standard_Integer> myRefShapes;
        /// @}

        bool mergeResultShapes = true;
        std::shared_ptr<const ItemContainerType> mySolids;
        std::shared_ptr<const MapType<ItemHashType, ShapeErrorType>> myShapeErrors;

    public:
        virtual void prepareInput() override final
        {
            /// NOTE: currently only deal with solid shapes
            if (myInputData->contains("mySolids"))
                mySolids = myInputData->getConst<ItemContainerType>("mySolids");
            else
                LOG_F(ERROR, "there is no mySolids property in myGeometryData");

            // GeometryWriter is not derived from GeometryProcessor, so "myShapeErrors" is not available
            if (myInputData->contains("myShapeErrors"))
                myShapeErrors = myInputData->getConst<MapType<ItemHashType, ShapeErrorType>>("myShapeErrors");
            else
                LOG_F(ERROR, "myShapeErrors data entry is not available in the inputData");
        }

        virtual void process() override final
        {
            mergeResultShapes = parameterValue<bool>("mergeResultShapes", true);

            std::string file_name = parameterValue<std::string>("dataFileName");
            if (not fs::path(file_name).is_absolute())
                file_name = dataStoragePath(file_name);
            if (file_name.size())
            {
                exportGeometry(file_name);
                LOG_F(INFO, "output the processed geometry as %s \n", file_name.c_str());
            }
            else
                LOG_F(INFO, "output file name in configuration is empty, skip save geometry. \n");
        }

    protected:
        void exportCompSolid(const std::string& file_name)
        {
            summary();
            // actual merge happends here, instead of GeometryImprinter
            TopoDS_Shape finalShape;
            if (mergeResultShapes)
                finalShape = OccUtils::glueFaces(OccUtils::createCompSolid(*mySolids, myShapeErrors));
            else
            {
                LOG_F(INFO, "result is not merged (duplicated face removed) for result brep file");
                finalShape = OccUtils::createCompSolid(*mySolids, myShapeErrors);
            }

            BRepTools::Write(finalShape, file_name.c_str()); //  progress reporter can be the last arg
            /// NOTE: exportMetaData() is done in the second GeometryPropertyBuilder processor
            LOG_F(INFO, "save the processed geometry compoSolid into file: %s", file_name.c_str());
        }

        /** print loaded shapes statistics, before moved into propertyContainer
         * this info could be output to log stream,*/
        void summary()
        {
            auto count = 0UL;
            for (const auto& p : (*myShapeErrors))
                if (p.second == ShapeErrorType::NoError)
                    count++;

            if (count == 0UL)
                LOG_F(WARNING, "result solid count is zero");

            std::stringstream sout;
            sout << " ======= write geometry summary =======" << std::endl;
            sout << "count of result solids is " << count << " out of total " << mySolids->size() << std::endl;

            LOG_F(INFO, "%s", sout.str().c_str());
        }

        /// can export floating shapes which can not be compoSolid
        void exportCompound(const std::string& file_name)
        {
            summary();

            // actual merge happends here, instead of GeometryImprinter
            TopoDS_Shape finalShape;
            if (mergeResultShapes)
            {
                finalShape = OccUtils::glueFaces(OccUtils::createCompound(*mySolids, myShapeErrors));
                VLOG_F(LOGLEVEL_DEBUG, "result shap is merged (duplicated face removed)");
            }
            else
            {
                LOG_F(INFO, "result is not merged (duplicated face removed) for result brep file");
                finalShape = OccUtils::createCompound(*mySolids, myShapeErrors);
            }
            /// NOTE: exportMetaData()  is done in the second GeometryPropertyBuilder processor
            BRepTools::Write(finalShape, file_name.c_str()); //  progress reporter can be the last arg
        }

        /** export result shape, only brep format can keep shared face topology during imprinting
         * #export is a c++ keyword can not be used as function name
         * */
        bool exportGeometry(const std::string file_name)
        {
            if (Utilities::hasFileExt(file_name, "brp") || Utilities::hasFileExt(file_name, "brep"))
            {
                exportCompound(file_name);
                return true;
            }
            else if (Utilities::hasFileExt(file_name, "stp") || Utilities::hasFileExt(file_name, "step"))
            {
                // LOG_F(INFO, "export Dataset pointed by member hDoc");
                Handle(TDocStd_Document) aDoc = createDocument();

                /// the user can work with an already prepared WorkSession or create a new one
                Standard_Boolean scratch = Standard_False;
                Handle(XSControl_WorkSession) WS = new XSControl_WorkSession();
                /// NOTE: length unit is controlled here, default "MM", PrecisionMode

                STEPControl_StepModelType mode = STEPControl_AsIs;
                // recommended value, others shape mode are available
                // Interface_Static::SetCVal("write.step.schema", "AP214IS");
                Interface_Static::SetIVal("write.step.assembly", 1); // global variable
                // "write.precision.val" = 0.0001 is the default value

                STEPCAFControl_Writer writer(WS, scratch);
                // this writer contains a STEPControl_Writer class, not by inheritance
                // writer.SetColorMode(mode);
                if (!writer.Transfer(aDoc, mode))
                {
                    LOG_F(ERROR, "The Dataset cannot be translated or gives no result");
                    // abandon ..
                }

                IFSelect_ReturnStatus stat = writer.Write(file_name.c_str());
                return (IFSelect_RetDone == stat);
            }
            else
            {
                LOG_F(ERROR, "The Dataset cannot be exported in the suffix format");
                return false;
            }

            return false;
        }

        Handle(TDocStd_Document) createDocument()
        {

            Handle(XCAFApp_Application) hApp = XCAFApp_Application::GetApplication();

            Handle(TDocStd_Document) newDoc; // the new doc
            /// NOTE: "MDTV-CAF" is deprecated, readonly, using xml or binary format
            hApp->NewDocument(TCollection_ExtendedString("MDTV-CAF"), newDoc);
            Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(newDoc->Main());
            Handle(XCAFDoc_ColorTool) colorTool = XCAFDoc_DocumentTool::ColorTool(newDoc->Main());
            Handle(XCAFDoc_MaterialTool) materialTool = XCAFDoc_DocumentTool::MaterialTool(newDoc->Main());

            auto myColorMap = myInputData->get<MapType<Standard_Integer, Quantity_Color>>("myColorMap");
            auto myNameMap = myInputData->get<MapType<Standard_Integer, std::string>>("myNameMap");
            ///  material is not supported yet:  auto myMaterialMap =

            /// see: https://www.opencascade.com/content/exporting-step-assembly-what-am-i-doing-wrong
            size_t i = 0U;
            for (auto& item : *mySolids)
            {
                TDF_Label partLabel = shapeTool->NewShape();
                shapeTool->SetShape(partLabel, item.second);
                colorTool->SetColor(partLabel, (*myColorMap)[item.first], XCAFDoc_ColorGen);
                TDataStd_Name::Set(partLabel, TCollection_ExtendedString((*myNameMap)[item.first].c_str(), true));
                ///  Material not yet supported
                i++;
            }
            shapeTool->UpdateAssemblies(); // OCCT 7.2 will not automatically update, so must explicitly call this func

            return newDoc;
        }
    }; // end of class

} // namespace Geom

#endif
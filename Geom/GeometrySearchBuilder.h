#pragma once


#include "GeometryProcessor.h"
#include "OccUtils.h"

namespace Geom
{
    using namespace PPP;

    /**
     * to specify search input criteria and also decide the search input values
     *
     * CONSIDER: boundbox tree may be build to accelerate research by boundbox
     *       need a relative tolerance like to judge identical or similarity
     * */
    enum class ShapeSearchType
    {
        UniqueId,     ///!< geometry ID hashed from center of mass and volume for solid
        GeometryFile, ///!< read the geometry from file and match
        BoundBox,     ///!<  axis align bound box,
    };
    NLOHMANN_JSON_SERIALIZE_ENUM(ShapeSearchType, {
                                                      {ShapeSearchType::UniqueId, "UniqueId"},
                                                      {ShapeSearchType::GeometryFile, "GeometryFile"},
                                                      {ShapeSearchType::BoundBox, "BoundBox"},
                                                  });
    enum class ShapeSimilarity
    {
        Unknown,   ///!< not calculated/intialized, or has error during similarity matching
        Identical, ///!< idential shapes
        Similar,   ///!< need some criteria to define similarity, volume, topology
        Insimilar, ///!< all the rest, not identical, not similar
    };

    /// \ingroup Geom
    /**
     * \brief search shapes by given criteria (searchType and searchValues) in config
     *  see ShapeSearchType
     * + UniqueId is mainly for internal usage, to filtered out problematic shapes
     * + BoundBox is used to search parts inside a given box region
     * + GeometryFile is used to find identical/similar parts regardless of placement/position
     *
     */
    class GeometrySearchBuilder : public GeometryProcessor
    {
        TYPESYSTEM_HEADER();

    private:
        ShapeSearchType myShapeSearchType = ShapeSearchType::UniqueId;
        size_t myFilterCount = 0;

        std::vector<UniqueIdType> myUniqueIds;
        std::vector<Bnd_Box> myBoundBoxes;
        /// filename holding geometry to search, or just a list of TopoDS_Shape
        std::vector<std::string> myGeometryFiles;

        bool suppressMatched = false;

        std::shared_ptr<VectorType<Bnd_Box>> myShapeBoundBoxes;
        std::shared_ptr<VectorType<GeometryProperty>> myGeometryProperties;
        std::shared_ptr<VectorType<Bnd_OBB>> myShapeOrientedBoundBoxes;

        // std::string myTargetShapeFilename;
        std::vector<VectorType<bool>> myMatchedResults;

    public:
        GeometrySearchBuilder()
        {
            // boundbox is optional, if boundbox tree is going to be build then bbox is required
            myCharacteristics["requiredProperties"] = {"myGeometryProperties"};
            // myCharacteristics["producedProperties"] = {"myCollisionInfos"};
        }
        ~GeometrySearchBuilder() = default;

        /**
         * \brief preparing work in serial mode
         */
        virtual void prepareInput() override final
        {
            // todo: collect all parameters, currently, are default except uniqueId

            GeometryProcessor::prepareInput();

            myShapeBoundBoxes = myInputData->get<VectorType<Bnd_Box>>("myShapeBoundBoxes");
            GeometryProcessor::prepareInput();
            if (myInputData->contains("myShapeOrientedBoundBoxes"))
                myShapeOrientedBoundBoxes = myInputData->get<VectorType<Bnd_OBB>>("myShapeOrientedBoundBoxes");

            myGeometryProperties = myInputData->get<VectorType<GeometryProperty>>("myGeometryProperties");

            parseSearchInput();
            /// prepare private properties like `std::vector<T>.resize(myInputData->itemCount());`
            /// therefore accessing item will not cause memory reallocation and items copying
            for (size_t i = 0; i < myFilterCount; i++)
            {
                auto m = VectorType<bool>(myInputData->itemCount(), false);
                myMatchedResults.emplace_back(m);
            }
        }

        /**
         * \brief preparing work in serial mode, write report, move data into `myOutputData`
         */
        virtual void prepareOutput() override final
        {

            if (myConfig.contains("output"))
            {
                auto file_name = dataStoragePath(parameter<std::string>("output"));
                writeResult(file_name);
            }
            else
            {
                LOG_F(WARNING, "user must provided output filename for matched shapes");
            }
            /*
            if (suppressMatched)
            {
                for (size_t r = 0; r < myFilterCount; r++)
                {
                    const auto& matched = myMatchedResults[r];
                    {
                        // done in writeResult()
                    }
                }
            }
            */
            myOutputData->emplace<decltype(myMatchedResults)>("myMatchedResults", std::move(myMatchedResults));
        }

        /**
         * \brief process single data item in parallel without affecting other data items
         * @param index: index to get/set by item(index)/setItem(index, newDataItem)
         */
        virtual void processItem(const ItemIndexType index) override final
        {
            matchItem(index);
        }

    private:
        void parseSearchInput()
        {
            suppressMatched = parameterValue<bool>("suppressMatched", false);
            myShapeSearchType = parameterValue<decltype(myShapeSearchType)>("searchType");
            if (myShapeSearchType == ShapeSearchType::UniqueId)
            {
                myUniqueIds = parameterValue<decltype(myUniqueIds)>("searchValues");
                myFilterCount = myUniqueIds.size();
            }
            else if (myShapeSearchType == ShapeSearchType::BoundBox)
            {
                json values = parameterJson("searchValues");
                // myBoundBoxes = parameterValue<decltype(myBoundBoxes)>("searchValues");
                for (const json& value : values)
                {
                    Bnd_Box b = value;
                    myBoundBoxes.emplace_back(b);
                }
                myFilterCount = myBoundBoxes.size();
            }
            else if (myShapeSearchType == ShapeSearchType::GeometryFile)
            {
                // todo: single filename full path or a list of filenames?
            }
            else
            {
                LOG_F(WARNING, "Geometry search input type is not recognized");
            }

            if (myFilterCount == 0)
            {
                LOG_F(WARNING, "user must provided at least one searchValue");
            }
        }

        void matchItem(const ItemIndexType index)
        {
            const auto& s = item(index);
            for (size_t r = 0; r < myFilterCount; r++)
            {
                auto& matched = myMatchedResults[r];
                // LOG_F(INFO, "matched vector size %lu", matched.size());
                if (myShapeSearchType == ShapeSearchType::UniqueId)
                {
                    const UniqueIdType uid = myUniqueIds[r];
                    matched[index] = matchUniqueId(s, uid);
                }
                else if (myShapeSearchType == ShapeSearchType::BoundBox)
                {
                    const Bnd_Box& box = myBoundBoxes[r];
                    matched[index] = matchBoundBox(index, box);
                }
                else if (myShapeSearchType == ShapeSearchType::GeometryFile)
                {
                    // todo
                }
                else
                {
                    LOG_F(WARNING, "Geometry search input type is not recognized");
                }
            }
        }

        bool matchUniqueId(const ItemType& s, const UniqueIdType& uid) const
        {
            UniqueIdType id = OccUtils::uniqueId(s);
            return id == uid; // todo: math with tolerance, or make UniqueID a class
        }

        bool matchBoundBox(const ItemIndexType index, const Bnd_Box& bbox) const
        {
            const Bnd_Box& box = (*myShapeBoundBoxes)[index];
            return OccUtils::isBndBoxCoincident(bbox, box);
        }

        void writeResult(const std::string filename)
        {
            for (size_t r = 0; r < myFilterCount; r++)
            {
                const auto& matched = myMatchedResults[r];
                auto pos = filename.rfind(".");
                std::string suffix = filename.substr(pos, filename.size() - pos);
                std::string stem = filename.substr(0, pos);
                std::string outputfilename = stem + "_" + std::to_string(r) + suffix;
                std::vector<ItemType> shapes;

                for (size_t i = 0; i < itemCount(); i++)
                {
                    if (matched[i])
                    {
                        shapes.push_back(item(i));
                        if (suppressMatched)
                            suppressItem(i, ShapeErrorType::UnknownError);
                    }
                }
                if (shapes.size() > 0)
                    OccUtils::saveShape(shapes, outputfilename);
                else
                    LOG_F(INFO, "search has no match for research entry #%lu", r);
            }
        }
    }; // namespace Geom
} // namespace Geom

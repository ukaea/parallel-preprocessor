#pragma once

#include "GeometryProcessor.h"
#include "GeometryTypes.h"
#include "OccUtils.h"

#pragma once

namespace Geom
{
    using namespace PPP;

    /// \ingroup Geom
    /**
     * generate geometrical properties for meta data writting, for downstream processors
     */
    class GeometryPropertyBuilder : public GeometryProcessor
    {
        TYPESYSTEM_HEADER();

    private:
        VectorType<GeometryProperty> myGeometryProperties;
        VectorType<ItemIndexType> myGeometryUniqueIds;
        double myMininumVolumeThreshold;
        // thread_local ItemIndexType myCurrentIndex;

    public:
        GeometryPropertyBuilder()
        {
            // The parent's default ctor be called automatically (implicitely)
            myCharacteristics["producedProperties"] = {"myGeometryProperties", "myGeometryUniqueIds"};
            // std::cout << myCharacteristics;
        }

        /**
         * for builders, it is essential to reserve itemCount to avoid resize
         * in order to achieve lockfree parallel writing into std::vector
         */
        virtual void prepareInput() override
        {
            GeometryProcessor::prepareInput();
            myGeometryProperties.resize(myInputData->itemCount());
            myGeometryUniqueIds.resize(myInputData->itemCount());

            myMininumVolumeThreshold = 1; // todo: length unit mm, a part with volume of 1 cubic mm is an error
        }

        virtual void prepareOutput() override
        {
            // volumeCheck();
            // dump(dataStoragePath("gproperties_dump.json")); // debug dump
            // report, save and display erroneous shape
            if (myConfig.contains("output"))
            {
                auto file_name = dataStoragePath(parameter<std::string>("output"));
                writeMetaData(file_name);
            }
            else
            {
                LOG_F(WARNING, "user must provided output filename for meta data");
            }
            myOutputData->emplace("myGeometryProperties", std::move(myGeometryProperties));
            myOutputData->emplace("myGeometryUniqueIds", std::move(myGeometryUniqueIds));
        }

        virtual void processItem(const std::size_t index) override
        {
            const TopoDS_Shape& aShape = item(index);
            myGeometryProperties[index] = OccUtils::geometryProperty(aShape); // no way for std::move, but obj is small
            itemPropertyCheck(index);
            myGeometryUniqueIds[index] = OccUtils::uniqueId(myGeometryProperties[index]);
        }

    protected:
        void volumeCheck()
        {
            // max and min volume check!
            double max_volume = 0;
            double min_volume = 1e100;  //  unit is mm^3
            double max_volume_threshold = 1e16; // todo: get from config parameter
            for (size_t i = 0; i < myInputData->itemCount(); i++)
            {
                const auto& p = myGeometryProperties[i];
                if (p.volume > max_volume and p.volume < max_volume_threshold)
                    max_volume = p.volume;
                if (p.volume < min_volume and p.volume > myMininumVolumeThreshold)
                    min_volume = p.volume;
                if (p.volume > max_volume_threshold)
                    LOG_F(ERROR, "maximum volume in solids surpass the threshold %f", max_volume_threshold);
            }
        }

        /// suppress and dump solid if the volume is too small
        void itemPropertyCheck(const std::size_t i)
        {
            const GeometryProperty& p = myGeometryProperties[i];
            if (p.volume < myMininumVolumeThreshold)
            {
                if (not itemSuppressed(i)) // this can be run as a second time
                {
                    LOG_F(ERROR, "item volume %f < threshold %f mm^3, so suppress item %lu", p.volume,
                          myMininumVolumeThreshold, i);
                    suppressItem(i, ShapeErrorType::VolumeTooSmall);
                    auto df = generateDumpName("dump_smallVolume", {i}) + ".brep";
                    OccUtils::saveShape({item(i)}, df);
                }
            }
        }

        /// this dump only geometry properties for debugging
        void dump(const std::string file_name) const
        {
            std::ofstream o(file_name);
            o << '[' << std::endl;
            for (auto& p : myGeometryProperties)
            {
                o << std::setw(4);
                json j = p;
                o << j << ',' << std::endl;
            }
            o << ']';
            // auto save and close the file
        }

        /// write meta data in json for solids extracted from input file
        /// suppressed Item will not have meta data write out
        /// if there is no name information, it is empty string or just sequence
        /// if there is no coloar or material, write `null` in json file
        void writeMetaData(const std::string file_name) const
        {
            std::ofstream o(file_name);
            o << '[' << std::endl;
            Quantity_Color defaultColor;
            json jm, jc; // this will write null value in json file
            for (ItemIndexType i = 0; i < itemCount(); i++)
            {
                if (not itemSuppressed(i))
                {
                    const auto c = itemColor(i);
                    if (c)
                    {
                        jc = c.value();
                        // Quantity_Color qc = c.value();
                        // to_json(jc, qc); // why = operator does not work?
                    }

                    const auto m = itemMaterial(i);
                    if (m)
                    {
                        jm = m.value();
                    }
                    else
                    {
                        jm = "unknown"; // if no material information from input geometry
                    }

                    json j{{"property", myGeometryProperties[i]},
                           {"uniqueId", myGeometryUniqueIds[i]},
                           {"sequenceId", i}, // writing sequence, to check if this sequence keep in readback
                           {"color", jc},
                           {"material", jm},
                           {"name", itemName(i)},
                           {"suppressed", itemSuppressed(i)},
                           {"shapeError", itemError(i)}};
                    o << std::setw(4);
                    o << j;
                    if (i < itemCount() - 1)
                        o << ','; // python json module does not accept comma without further data after it
                    o << std::endl;
                }
            }
            o << ']';
            // auto save and close the file
        }
    };
} // namespace Geom

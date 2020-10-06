
#pragma once

#include "Processor.h"

namespace PPP
{

    /// \ingroup PPP
    /**
     * \brief base class for all writer in module like Geom
     *
     * In this base class implementation of `process()`, write a manifest file with filename for each line
     * */
    class AppExport Writer : public Processor
    {
#if PPP_BUILD_TYPE
        TYPESYSTEM_HEADER();
#endif
    public:
        virtual void process() override
        {
            std::string file_name = myConfig["dataFileName"];
            if (!fs::path(file_name).is_absolute())
                file_name = dataStoragePath(file_name);
            if (file_name.size() && myOutputData->contains("myItemOutputs"))
            {
                auto outputs = myOutputData->getConst<VectorType<std::string>>("myItemOutputs");
                std::ofstream ofs(file_name);
                for (const auto& s : *outputs)
                {
                    ofs << s << std::endl;
                }
            }
        }
    };

} // namespace PPP


#pragma once

#include "DataObject.h"
#include "Processor.h"
#include "Utilities.h"

namespace PPP
{

    /// \ingroup PPP
    /**
     * \brief base class for all reader in modules like Geom
     *
     * in this base class's `process()`, read a manifest file with filename for each line, item data type is FilePath
     * */
    class AppExport Reader : public Processor
    {
#if PPP_BUILD_TYPE
        TYPESYSTEM_HEADER();
#endif
    private:
        VectorType<std::string> myFilePaths;

    public:
        virtual void process() override
        {
            std::string file_name = myConfig["dataFileName"];
            std::ifstream ifs(file_name);
            Utilities::getFileContent(file_name, myFilePaths);
        }

        virtual void prepareOutput() override
        {
            myOutputData = std::make_shared<DataObject>();
            myOutputData->setItemCount(myFilePaths.size()); // must get size() before move()
            myOutputData->emplace("myFilePaths", std::move(myFilePaths));
        }
    };

} // namespace PPP
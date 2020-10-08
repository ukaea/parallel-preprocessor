#pragma once

#include "./PreCompiled.h"
#include "./TypeDefs.h"

#include "./Logger.h"
#include "./Utilities.h"

namespace PPP
{
    /// \ingroup PPP

    /** Data storage type,  Folder is the default and only implemented type.
     *  Database is dramatically different from the other, without the concept of path
     * */
    enum AppExport DataStorageType
    {
        Folder, ///< can be zipped into single file
        HDF5,   ///< useful for mesh data storage
        NetCFD  ///< some solver input file uses this format
    };

    /**
     * Base class for DataStorage, implementing local file system storage into a folder.
     * providing name conflicting detection, etc.
     * locking is not necessary since writing only in the main thread
     * */
    class AppExport DataStorage
    {
    public:
        DataStorage() = default;

        /// if existed, remove_all content there
        virtual void setStoragePath(const std::string pathname)
        {
            fs::path _path{pathname};

            /// TODO: generate a unique name, and backup the existing result folder
            if (fs::exists(_path))
            {
                LOG_F(WARNING, "result storage path exists, remove that folder");
                fs::remove_all(_path);
                fs::create_directory(_path);
            }
            else
            {
                fs::create_directory(_path);
            }
            myStoragePath = fs::absolute(_path).string();
            LOG_F(INFO, "all result data will be saved to folder: %s", myStoragePath.c_str());
        }

        std::string generateStoragePath(std::string input_name)
        {
            return input_name + "_processed";
        }

        virtual std::string storagePath()
        {
            return myStoragePath;
        }

        DataStorageType storageType() const
        {
            return myStorageType;
        }

        /// get full path from relative filename
        virtual std::string getFullPath(const std::string& filename)
        {
            fs::path _path{filename};
            if (_path.is_absolute())
            {
                LOG_F(WARNING, "path `%s` is absolute, return it as it is", filename.c_str());
                return filename;
            }
            auto target = fs::path(myStoragePath) / filename;
            return target.string();
        }

        /// maybe useful for PropertyContainer with serializer function
        virtual std::shared_ptr<std::ostream> createFileStream(const std::string& filename)
        {
            auto target = fs::path(myStoragePath + filename);
            if (fs::exists(target))
            {
                LOG_F(WARNING, "path `%s` exists, overriding", target.c_str());
            }
            return std::make_shared<std::ofstream>(filename);
        }

        /// file path or HDF5 path name, relative to the storage root
        virtual bool save(std::string& pathName, const json& json_stuff)
        {
            (*createFileStream(pathName)) << json_stuff;
            return true;
        }

    private:
        /// HDF5 file path or Folder path, even URI as the std::string type
        std::string myStoragePath;
        DataStorageType myStorageType = DataStorageType::Folder;
    };
} // namespace PPP
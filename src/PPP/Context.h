#ifndef PPP_Context_H
#define PPP_Context_H

#include "DataStorage.h"
#define SINGLETON_CONTEXT 0

namespace PPP
{

    /// \ingroup PPP
    /** singleton provides MPI, type, log initialization
     *
     * CONSIDER: remove singleton pattern, back to normal class
     */
    class AppExport Context
    {
    public:
        static void initialize(int argc, char** argv);
        static void finalize();
        /// get the configuration data
        inline static const Config& config()
        {
            return Context::singleton()->myConfig;
        }

        /// absolute path, C++17 std::filesystem::current_path()
        inline static const std::string workingDir()
        {
            return Context::singleton()->myConfig["storageConfig"]["workingDir"];
        }

        /// get the data storage,  folder, zipped file container, or HDF5/netCDF
        inline static DataStorage& dataStorage()
        {
            return *(Context::singleton()->myDataStorage);
        }

        /// is MPI distributive parallel
        inline static bool distributive()
        {
            return false;
        }
        inline static std::size_t threadCount()
        {
            return Context::singleton()->myConfig["parallelism"]["threadsOnNode"];
        }


    private:
        Config myConfig;
        std::string myLogFile;
        std::string myConfigFile;

        bool isDistributive = false;
        bool isParallel = true;

        std::shared_ptr<DataStorage> myDataStorage;

        static Context* mySingleton; // the one and only pointer to the Context object
        static Context* singleton()
        {
            return Context::mySingleton;
        }

        /// private Constructor for singleton pattern
        Context(const std::string& config_filename)
        {
            setupConfig(config_filename);
        }
        /// as singleton is created by new, so it must be explicitly destroy/delete
        ~Context() = default;

        Context(const Context&) = delete;            // Prevent copy-construction
        Context& operator=(const Context&) = delete; // Prevent assignment

    protected:
        void setupConfig(const std::string& config_filename);
        void setupAppInfo(const Config& config);
        void setupDataStorage(const Config& config);
        void setupParallelism(const Config& config);
        void setupLogger(const Config& config);
    };

} // namespace PPP
#endif
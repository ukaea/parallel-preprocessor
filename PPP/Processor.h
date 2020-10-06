
#pragma once

// for visual C++ to support not, and, or keywords
#include <iso646.h>

#include "./PreCompiled.h"
#include "./Utilities.h"
#include "Context.h"
#include "DataObject.h"
#include "OperatorProxy.h"
#include "Parameter.h"
#include "ProcessorError.h"
#include "ProcessorResult.h"

#if PPP_BUILD_TYPE
#include "Base/BaseClass.h"
// just use TYPESYSTEM_HEADER()
#else // declare nothing, just ignore that line/macro
#define TYPESYSTEM_HEADER()
#endif


namespace PPP
{

    /// \ingroup PPP
    /**
     * \brief base class for all processors, mappable to vtkAlgorithm,  opKernel of tensorflow
     * see API of vtkAlgorithm: <https://vtk.org/doc/nightly/html/classvtkAlgorithm.html>
     * Multiple input and multiple ouput, is not supported, GetNumberOfInputPorts()
     * register input parameters, given the doc and default values by the processor developer
     * register generated data properties (downstream processors may depend on)
     * register required properties
     * those registered meta info would be used by pipeline validator in cpp or python
     * each processor must have a ctor without any parameter, all parameters are from config
     */
#if PPP_BUILD_TYPE
    class AppExport Processor : public Base::BaseClass
    {
        TYPESYSTEM_HEADER();
#else
    class AppExport Processor
    {
#endif
    public:
        Processor() = default;
        virtual ~Processor() = default;

    public:
        /// @{ configuration group
        // for multiple input port:  int port = 0
        void setInputData(std::shared_ptr<DataObject> data)
        {
            myInputData = data;
            myOutputData = myInputData; // set a default link, in case you forget to do that
        }
        /// get some information, use with care (only after process.prepareInput() )
        std::shared_ptr<const DataObject> inputData() const
        {
            return myInputData;
        }
        std::shared_ptr<DataObject> outputData()
        {
            return myOutputData;
        }
        /// get processor result, in addition to processed input data
        std::shared_ptr<ProcessorResult> result()
        {
            return myResult;
        }
        /// configration (controlling parameters) input
        inline void setConfig(const Config& config)
        {
            myConfig = config;
        }
        inline const Config config()
        {
            return myConfig;
        }

        /**
         * intrinsic characteristics of the processor, read only
         */
        inline const Config characteristics()
        {
            return myCharacteristics;
        }

        /// deprecated this in favour of ProcessorResult
        void setInputInformation(std::shared_ptr<Information> info)
        {
            myInfo = info;
        }
        std::shared_ptr<Information> getOutputInformation()
        {
            return myInfo;
        }

        /// set by builder
        void setOperator(std::shared_ptr<OperatorProxy> v)
        {
            myOperator = v;
        }

        /**
         * notify operator and wait for instruction
         */
        virtual void notify(){};

        /// maybe accessed directly by `attribute<IndexPattern>("indexPattern")`
        inline IndexPattern indexPattern() const
        {
            if (myCharacteristics.contains("indexPattern"))
                return static_cast<IndexPattern>(myCharacteristics["indexPattern"]);
            else
                return IndexPattern::Linear;
        }

        inline bool isCoupledOperation()
        {
            return myCharacteristics.contains("coupled") && myCharacteristics["coupled"];
        }
        /// @}

        /// @{ API for pipe controller
        /// default imp: reserve memery for item report vector
        virtual void prepareInput()
        {
            myItemReports.resize(myInputData->itemCount());
        };

        /// default imp: do nothing
        virtual void prepareOutput(){};


        /**  use it only if parallel process is not supported, such as reader and writer
         * mappable to VTK function
        `int ProcessRequest (vtkInformation *request, vtkInformationVector **inInfo,
                             vtkInformationVector *outInfo)`
        */
        virtual void process(){};
        /*
        {
            LOG_F(ERROR, "you should call processItem() instead, API for reader and writer only");
        };
        */

        /// check if item(i) need to processed by processItem() for IndexPattern::FilteredVector
        /// this function should be light-weighted, while processItem() is time-consuming
        /// todo: reconsider this function name
        virtual bool required(const ItemIndexType)
        {
            return true;
        }

        /// pipecontroller can schedule the work in a finer grain parallelism
        virtual void processItem(const ItemIndexType){}; // or make it pure virtual
        /// @}

        /// @{ API for coupled data item processing
        /// will processing item I affect/modify item J?
        virtual bool isCoupledPair(const ItemIndexType, const ItemIndexType)
        {
            return false;
        };

        /// run in parallelism with the assistance of ParallelAccessor on coupled data
        virtual void processItemPair(const ItemIndexType, const ItemIndexType){};
        /// @}


        /// @{ result group

        /**
         * report after successfully processsing all items, according to verbosity level
         * write/append into output Information, may also reportor to operator
         * myItemReports
         */
        virtual void report()
        {
            Information report;
            const std::size_t NShapes = myItemReports.size();
            size_t errorCount = 0;
            for (std::size_t i = 0; i < NShapes; i++)
            {
                if (hasItemReport(i))
                {
                    const std::string& s = itemReport(i).str();
                    // report[itemName(i)] = std::string(s);
                    report[std::to_string(i)] = std::string(s);
                    errorCount++;
                }
            }
            if (errorCount)
            {
                auto outFile = dataStoragePath(parameter<std::string>("report"));
                std::ofstream o(outFile);
                o << report;
            }
            // todo: instanceID unique name to distinguish instances, or just
            // (*myInfo)[className()] = report;
        }

        void setItemReport(const ItemIndexType i, std::stringstream&& msg)
        {
            myItemReports[i] = std::make_shared<std::stringstream>(std::move(msg));
        }
        void writeItemReport(const ItemIndexType i, const std::string msg)
        {
            itemReport(i) << msg;
        }

        bool hasItemReport(const ItemIndexType i) const
        {
            return myItemReports[i] and myItemReports[i]->str().size();
        }

        /// must check before use this method
        inline const std::stringstream& itemReport(const ItemIndexType i) const
        {
            return *myItemReports[i];
        }
        /// use it as s stream `itemReport(i) << msg;`, create if nullptr
        inline std::stringstream& itemReport(const ItemIndexType i)
        {
            if (not hasItemReport(i))
                myItemReports[i] = std::make_shared<std::stringstream>();
            return *myItemReports[i];
        }

        /**
         * checkpoint and dump data property, configration, and information
         * useful for debugging when there is an error, or interrupted by Ctrl-C signal
         * this should be done by pipeline's signal handler
         */
        virtual void dump() const
        {
#if PPP_BUILD_TYPE
            auto p = dataStoragePath(className() + "_dump.json");
#else
            auto p = dataStoragePath("processor_dump.json"); // datastorage class will generate uniq
#endif
            std::ofstream file(p);
            file << (*myInfo); // todo: dump myResult
        }

        /// for debugging output into result data storage folder
        virtual void dumpJson(const json& j, std::string filename)
        {
            auto p = dataStoragePath(filename);
            std::ofstream file(p); // todo: a better unique name
            file << j;
        }

        /** dump items in case of error
         * TODO: appending processor `className()`, otherwise may be name clashing
         * */
        std::string generateDumpName(const std::string prefix, const std::vector<ItemIndexType> args) const
        {
            std::stringstream ss;
            ss << prefix;
            for (auto i : args)
            {
                ss << "_" << i;
            }
            return dataStoragePath(ss.str());
        }

        /// if file name has existed, generate a unique name by timestamp
        std::string dataStoragePath(const std::string& filename) const
        {
            auto f = Context::dataStorage().getFullPath(filename);
            if (not fs::exists(f))
                return f;
            else
            {
                auto f_ts = Utilities::timeStampFileName(filename);
                return Context::dataStorage().getFullPath(f_ts);
            }
        }
        /// @}

        std::size_t threadCount()
        {
            return Context::threadCount();
        }

    public:
        /// get intrinsic attribute of the processor, used internally by executor, controller classes
        template <typename T> const T attribute(const std::string& name, T defaultValue = T())
        {
            if (myCharacteristics.contains(name))
            {
                json& a = myCharacteristics[name];
                if (a.contains("value"))
                {
                    return a["value"].get<T>(); // parameter format
                }
                else
                {
                    return a.get<T>();
                }
            }
            else
            {
                /// issue a warning by using default attribute value
                // std::cout << myCharacteristics << std::endl;
                LOG_F(WARNING, "attribute `%s` is not found in myCharacteristics, using default value", name.c_str());
                return defaultValue;
            }
        }

        /** Dependency management meta data registration, used in pipeline validation
        some processor must run after dependent processed has completed, to generate some properties
        if some missing properties found in pipeline check, the checker can give hint which processor is missed
        this API cooperate with the `producedProperties()` method.
        todo: register meta information in dependencies, products in `myCharacteristics` in constructor
        */
        std::vector<std::string> requiredProperties()
        {
            std::vector<std::string> myDependencies;
            if (myCharacteristics.contains("requiredProperties"))
            {
                myDependencies = myCharacteristics["requiredProperties"].get<std::vector<std::string>>();
            }
            return myDependencies; // this can be empty vector
        }

        /// a vector of names for properties produced by this processor
        std::vector<std::string> producedProperties()
        {
            std::vector<std::string> myProducts;
            if (myCharacteristics.contains("producedProperties"))
            {
                myProducts = myCharacteristics["producedProperties"].get<std::vector<std::string>>();
            }
            return myProducts;
        }

        /// @{ parameter
        /// return registered parameters to control this processor
        const std::vector<Config> Parameters()
        {
            std::vector<Config> myParameters;
            if (myCharacteristics.contains("producedProperties"))
            {
                for (const auto& p : myCharacteristics["parameters"])
                    myParameters.push_back(p);
            }
            return myParameters;
        }

        /// check only in processor's own config
        inline bool hasParameter(const std::string& name)
        {
            return (myConfig.contains(name));
        }


        /// get parameter from user configuration input, otherwise use the default
        template <typename T> const T parameterValue(const std::string& name, T defaultValue = T())
        {
            // unit conversion may be necessary later
            if (myConfig.contains(name))
            {
                json& a = myConfig[name];
                if (a.contains("value"))
                {
                    return a["value"].get<T>(); /// parameter format with extra information
                }
                else
                {
                    return a.get<T>(); /// key-value pair
                }
            }
            else
            {
                /// issue a warning by using default attribute value: not needed
                return defaultValue;
            }
        }

        /// get the raw json without auto-conversion, from processor config
        /// then complicate  raw json can be parsed manually
        /// equal to `parameterValue<json>(name)` but more efficiently
        const json parameterJson(const std::string& name)
        {
            // unit conversion may be necessary later
            if (myConfig.contains(name))
            {
                json& a = myConfig[name];
                if (a.contains("value"))
                {
                    return a["value"]; /// parameter format with extra information
                }
                else
                {
                    return a;
                }
            }
            else
            {
                LOG_F(ERROR,
                      "parameter `%s` is not found in config\n"
                      "try the overloded version with default value instead",
                      name.c_str());
                throw ProcessorError("parameter is not found in processor configuration");
            }
        }

        /// parameter is compulsory in config
        template <typename T> const T parameter(const std::string& name)
        {
            Parameter<T> p;
            if (myConfig.contains(name))
            {
                const json& a = myConfig[name];
                p = Parameter<T>::fromJson(a);
            }
            else
            {
                LOG_F(ERROR,
                      "parameter `%s` is not found in config\n"
                      "try the overloded version with default value instead",
                      name.c_str());
                throw ProcessorError("parameter is not found in processor configuration");
            }
            return p.value;
        }

        /// if parameter is not provided in config, use the second argument provided
        template <typename T> const T parameter(const std::string& name, T defaultValue)
        {
            Parameter<T> p;
            if (myConfig.contains(name))
            {
                const json& a = myConfig[name];
                p = Parameter<T>::fromJson(a);
            }
            else
            {
                LOG_F(WARNING, "parameter `%s` is not found in config, using default value", name.c_str());
                p.value = defaultValue;
            }
            return p.value;
        }


        // add a group name
        template <typename T> const T globalConfig(const std::string& group_name, const std::string& name)
        {
            const Config& gConfig = Context::config()[group_name];
            if (gConfig.contains(name))
            {
                const json& a = gConfig[name];
                return Parameter<T>::fromJson(a).value;
            }
        }

        /** for global parameters, it should fill into GeometryProcessor
         *  make it static to be used by utility functions
         */
        template <typename T> static const T globalParameter(const std::string name)
        {
            Parameter<T> p;
            const Config& gConfig = Context::config()["globalParameters"];
            if (gConfig.contains(name))
            {
                const json& a = gConfig[name];
                p = Parameter<T>::fromJson(a);
            }
            else
            {
                LOG_F(ERROR, "parameter %s is not found in global parameters", name.c_str());
                throw ProcessorError("parameter is not found in global parameters");
                // p.value = defaultValue;
            }
            return p.value;
        }
        /// @}

        /// A graphic progressor starts in an external process, only for time-comsuming job
        /// this method will fail safely if fine not find or python Qt module are not installed
        static void startExternalProgressor()
        {
            const std::string log_filename = Context::config()["logger"]["logFileName"];
            const std::string title = "parallel-preprocessor processing progress";
            fs::path py_monitor = "pppMonitorProgress.py"; //
            try
            {
                fs::path py_monitor_path = Processor::globalParameter<std::string>("scriptFolder") / py_monitor;
                if (not fs::exists(py_monitor_path))
                {
                    VLOG_F(LOGLEVEL_DEBUG, "Python script `%s` is not found, ignore", py_monitor_path.string().c_str());
                }
                else
                {
                    // by appending `&`, then the command is nonblocking /detached from the main process
                    Utilities::runCommand("python3 " + py_monitor_path.string() + " " + log_filename + " " + title +
                                          " &");
                }
            }
            catch (...)
            {
                // globalParameter<std::string>("scriptFolder")
            }
        }

    protected:
        /// intrinsic attribute of the process operation, parallel without affact other item? readonly?
        /// set myCharacteristics in constructor may make ctor is not default
        Config myCharacteristics;
        Config myConfig; /// treated as value type

        /// chained up intput -> output, can be saved to file,each processor appends to myInfo
        std::shared_ptr<Information> myInfo;
        std::shared_ptr<ProcessorResult> myResult;
        VectorType<std::shared_ptr<std::stringstream>> myItemReports;

        std::shared_ptr<OperatorProxy> myOperator = nullptr; // should be created with nullptr?

        /// make it a std::vector<> for multiple input
        std::shared_ptr<DataObject> myInputData;  /// it is shared_pointer<>
        std::shared_ptr<DataObject> myOutputData; /// input data, or cloned/checkpointed and then modified
    };


} // namespace PPP

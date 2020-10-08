#include "Context.h"

#include "./Logger.h"
#include "./Utilities.h"

#if PPP_BUILD_TYPE
#include "Base/BaseClass.h"
#endif

#if PPP_BUILD_MPI
#include <mpi.h>
#endif

using namespace PPP;

/// kind of redeclare static member data, but without `static` keyword
/// to give it storage and initial value, otherwise "undefined variable" in linkage
/// if the static data is const, it can be defined in header without the source file
/// static inline var in header , means the var will appear in each compilation unit

Context* Context::mySingleton = nullptr;

void Context::initialize(int argc, char** argv)
{
    /// work on char** is tricky
    std::vector<std::string> arguments;
    for (int i = 0; i < argc; i++)
    {
        arguments.push_back(std::string(argv[i]));
    }
#if DEBUG // debug print before loguru is ready to write
    std::cout << "print all argv with argc = " << argc << std::endl;
    for (int i = 0; i < argc; i++)
    {
        std::cout << arguments[i] << std::endl;
    }
#endif
    std::string input_file = "../python/config.json";
    const char* USAGE = "Usage: program input_config_file \n mpirun -np 2 program input_config_file";

    if (argc >= 2)
    { // the first arg is the program name itself, once MPI parameter prepended, then?
        input_file = arguments[1];
    }
    else
    {
        std::cout << USAGE;
    }

    // detect from argv, if "mpirun" is find, then it is distributive()

    Context::mySingleton = new Context(input_file); // call setup
    Context::mySingleton->myConfigFile = input_file;

    // loguru::init() is optional, to detect -v argument, also setup SIGNAL handler
    int log_argc = 1;
    char appString[] = "PPP";
    char* log_argv[] = {appString, nullptr};
    loguru::init(log_argc, log_argv); // it requires argv[argc] == nullptr

#if PPP_BUILD_TYPE
    using namespace Base;
    Type::init(); // init the type system
    // then init each class (actually register this class type), including the BaseClass
    BaseClass::init(); // this root class must be initialized before user types
    VLOG_F(LOGLEVEL_DEBUG, "Type system and BaseClass has been initialized ");
#endif

    VLOG_F(LOGLEVEL_DEBUG, "PPP has been initialized \n");
    // result data set
    // message queue
}

void Context::finalize()
{
    // pack result, config, log into zip file, dump, each DataObject has a dump folder
    auto logFile = mySingleton->myDataStorage->getFullPath(mySingleton->myLogFile);
    VLOG_F(LOGLEVEL_PROGRESS, "log file copy %s to %s ", mySingleton->myLogFile.c_str(), logFile.c_str());
    fs::copy_file(mySingleton->myLogFile, logFile);
    fs::remove(mySingleton->myLogFile);

    std::string savedConfig = mySingleton->myDataStorage->getFullPath("config.json");
    VLOG_F(LOGLEVEL_PROGRESS, "config has been dump to file: %s ", savedConfig.c_str());
    // fs::copy_file(mySingleton->myConfigFile, savedConfig);
    std::ofstream cf(savedConfig); // must using std::ofstream instead of std::fstream
    cf << std::setw(4) << (mySingleton->myConfig);

    delete Context::mySingleton;
    Context::mySingleton = nullptr;

#if PPP_BUILD_TYPE
    Base::Type::destruct();
#endif

#if PPP_USE_MPI
    MPI_Finalize();
#endif
    VLOG_F(LOGLEVEL_DEBUG, "PPP has been finalized \n");
}


/// Singleton getter of the Context
void Context::setupConfig(const std::string& config_filename)
{
    if (fs::exists(config_filename) && Utilities::hasFileExt(config_filename, ".json"))
    {
        auto input = std::ifstream(config_filename);
        input >> myConfig;
        // setupAppInfo(myConfig);
        setupParallelism(myConfig);
        setupDataStorage(myConfig); // it uses log functions
        setupLogger(myConfig);
    }
    else
    {
        std::cout << "configuration json file `" << config_filename
                  << "` does not exists, terminate check filename and relative path \n";
        std::terminate();
    }
}


void Context::setupAppInfo(const Config&)
{
    Config storageInfo;
}


void Context::setupDataStorage(const Config& config)
{
    Config storageInfo;
    if (config.contains("dataStorage"))
    {
        storageInfo = config["dataStorage"];
    }
    else
    {
        std::cout << "configuration file does not have `dataStorage` section \n";
    }

    if (!storageInfo.contains("workingDir"))
    {
        storageInfo["workingDir"] = fs::current_path().string(); // boost has no u8string() method!
    }
    else
    {
        fs::current_path(storageInfo["workingDir"].get<std::string>());
    }

    if (!storageInfo.contains("dataStorageType"))
    {
        storageInfo["dataStorageType"] = "Folder";
    }

    if (!storageInfo.contains("dataStoragePath"))
    {
        fs::path _path{config["readers"][0]["dataFileName"].get<std::string>()};
        std::string caseName = fs::absolute(_path).stem().string(); // extract only a name
        if (storageInfo["dataStorageType"] == "Folder")
        {
            auto p = fs::path(storageInfo["workingDir"].get<std::string>()) / caseName;
            storageInfo["dataStoragePath"] = p.string();
        }
        else
        {
            std::cout << "dataStorageType = " << storageInfo["dataStorageType"] << "is not supported;";
        }
        VLOG_F(LOGLEVEL_DEBUG, " folder file: %s, working dir: %s", caseName.c_str(),
               storageInfo["workingDir"].get<std::string>().c_str());
    }
    myDataStorage = std::make_shared<DataStorage>();
    // myDataStorage->setStoragePath(storageInfo["dataStoragePath"]);  // set in pipecontroller::compute()
    myConfig["dataStorage"] = storageInfo;
}

void Context::setupLogger(const Config& config)
{
    Config logConfig;
    if (config.contains("logger"))
    {
        logConfig = config["logger"];
        // it is better to check contains
        if (!(logConfig.contains("logFileName") && logConfig.contains("logLevel")))
        {
            logConfig["logFileName"] = "debug_info.log";
            logConfig["logLevel"] = "INFO";
            std::cout << "logger config is invalid/incomplete, using default setup" << logConfig << std::endl;
        }
    }
    else
    {
        logConfig["logFileName"] = "debug_info.log";
        logConfig["logLevel"] = "INFO";
        std::cout << "no logger configuration provideusing default setup" << logConfig << std::endl;
    }

    // todo: setup logger for MPI, only the first worker print
    // std::cout << "get<loguru::NamedVerbosity>()" <<
    auto logLevel = loguru::Verbosity_2; // 0: info, 1: progress 2: verbosity/debug
    if (logConfig.contains("logLevel"))
        logLevel = logConfig["logLevel"].get<loguru::NamedVerbosity>();
    auto verbosity = loguru::Verbosity_INFO; // 0: INFO, -1 WARNING
    if (logConfig.contains("verbosity"))
        verbosity = logConfig["verbosity"].get<loguru::NamedVerbosity>();

    // dataStorage is not init yet,  log will be copied into dataStorage at the end of processing
    myLogFile = logConfig["logFileName"].get<std::string>();
    // myLogFile = mySingleton->dataStorage().getFullPath(myLogFile);  // storagePath is yet known
    // myLogFile = fs::absolute(myLogFile).string();
    // workingDir may be set different from pwd

    // Only show most relevant things on stderr
    loguru::g_stderr_verbosity = verbosity; // loguru::NamedVerbosity::Verbosity_1,  progress

    loguru::g_preamble_date = false;
    loguru::g_preamble_thread = false;
    loguru::g_preamble_file = false;
    // if (logLevel < loguru::NamedVerbosity::Verbosity_2)
    //{
    //    loguru::g_preamble_file = false;
    //}

    loguru::add_file(myLogFile.c_str(), loguru::Truncate, logLevel);

    myConfig["logger"] = logConfig;
}

// todo: may merge into storageInfo
void Context::setupParallelism(const Config& config)
{
    Config myParallism;
    if (config.contains("parallelism"))
    {
        myParallism = config["parallelism"];
        auto nCores = std::thread::hardware_concurrency();
        if (myParallism["threadsOnNode"] < 1 || myParallism["threadsOnNode"] > nCores)
        {
            myParallism["threadsOnNode"] = nCores;
        }
    }
    else
    {
        VLOG_F(LOGLEVEL_DEBUG, "no parallelism configuration provided, using default setup");
    }
    myConfig["parallelism"] = myParallism;
#if PPP_BUILD_MPI
    MPI_Init(&argc, &argv);
#else
    VLOG_F(LOGLEVEL_DEBUG, "This program is not compiled with MPI supported");
#endif
}

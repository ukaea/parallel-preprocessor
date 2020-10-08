
#include "PipelineController.h"

#include "ConsoleOperator.h"
#include "Context.h"
#include "Logger.h"
#include "PreCompiled.h"
#include "Utilities.h"

#include <csignal>

#define PPP_USE_THREADING 1
#if PPP_USE_THREADING
#include "./ThreadPoolExecutor.h"
#else
#include "./Executor.h"
#endif

#if PPP_USE_BOOST_PROCESS
#include "CommandLineProcessor.h"
#endif

#include "CouplingMatrixBuilder.h"
#include "Processor.h"
#include "Reader.h"
#include "Writer.h"

#if PPP_BUILD_TYPE
// this type system macro must be put into source file to store static type data
TYPESYSTEM_SOURCE(PPP::Processor, Base::BaseClass);
TYPESYSTEM_SOURCE(PPP::Reader, PPP::Processor);
TYPESYSTEM_SOURCE(PPP::Writer, PPP::Processor);
TYPESYSTEM_SOURCE(PPP::CouplingMatrixBuilder, PPP::Processor);
#if PPP_USE_BOOST_PROCESS
TYPESYSTEM_SOURCE(PPP::CommandLineProcessor, PPP::Processor);
#endif
#endif

std::function<void(int)> callback_wrapper;
static void ppp_signal_callback_handler(int signum)
{
    callback_wrapper(signum);
}

namespace PPP
{
    void PipelineController::initTypes()
    {
#if PPP_BUILD_TYPE
        Processor::init();
        Reader::init();
        Writer::init();
        CouplingMatrixBuilder::init();
#if PPP_USE_BOOST_PROCESS
        CommandLineProcessor::init();
#endif
#endif
    }

    PipelineController::PipelineController(int argc, char** argv)
    {
        Context::initialize(argc, argv);
        setConfig(Context::config()); // Context class can add new content into config
    }

    PipelineController::~PipelineController()
    {
        Context::finalize();
    }

    PipelineController::PipelineController(const std::string& config_filename)
    {
        if (fs::exists(config_filename) && Utilities::hasFileExt(config_filename, ".json"))
        {
#if _DEBUG
            // logging is not yet ready, to confirm the correct configuration file is used
            std::cout << "configuration json filename :" << config_filename << std::endl;
#endif
            std::vector<char*> args(3);
            std::string prog = "PPP";

            args[0] = const_cast<char*>(prog.c_str());
            args[1] = const_cast<char*>(config_filename.c_str());
            args[2] = nullptr;

            Context::initialize(2, args.data());
            setConfig(Context::config()); // application can add new content into config
        }
        else
        {
            std::cout << "configuration json file does not exist: \n"
                      << config_filename << "\n , program terminated, check filename and relative path \n";
            std::terminate();
        }
    }


    const Config PipelineController::config() const
    {
        return myConfig;
    }

    void PipelineController::setConfig(const json& config)
    {
        myConfig = config;
    }

    void _checkConfigSection(const Config& myConfig, const std::string& section)
    {
        if (!myConfig.contains(section))
            LOG_F(ERROR, "configuration has no `%s` section", section.c_str());
        else
        {
            if (!(myConfig[section].is_array() && myConfig[section].size() > 0))
                LOG_F(ERROR, "`%s` section is not an array or it is zero length", section.c_str());
        }
    }

    /**
     * check if input config is well-structured without missing key
     */
    void PipelineController::checkConfig() const
    {
        /// log check has been done in setupLogger()
        _checkConfigSection(myConfig, "readers");
        _checkConfigSection(myConfig, "processors");
        /// NOTE: not necessory to check "writers" section
    }

    void PipelineController::initialize()
    {
        PipelineController::initTypes(); // if moved to ctors, must copy into each ctor
        checkConfig();

        // before process, if operator is not set, set a default one
        if (!myOperator)
        {
            std::shared_ptr<AbstractOperator> op = std::make_shared<ConsoleOperator>();
            std::shared_ptr<OperatorProxy> proxy = std::make_shared<OperatorProxy>(op);
            setOperator(proxy);
        }
    }

    void PipelineController::finalize()
    {
    }


    std::shared_ptr<Processor> PipelineController::createProcessor(const Config& p)
    {
        std::string pname = p["className"];
#if PPP_BUILD_TYPE
        Base::BaseClass* bp = nullptr;
        bp = static_cast<Base::BaseClass*>(Base::Type::createInstanceByName(pname.c_str()));
        if (not bp)
        {
            LOG_F(ERROR, "create instance by name failed for `%s`\n namespace should be given to the the class name \n",
                  pname.c_str());
        }
        PPP::Processor* pp = Base::type_dynamic_cast<PPP::Processor>(bp);
        if (not pp)
        {
            LOG_F(ERROR, "`Base::type_dynamic_cast<PPP::Processor>(bc)` failed\n");
        }
        std::shared_ptr<Processor> sp(pp);
        return sp;
#else
        LOG_F(ERROR, "TypeSystem is not available to create Processor instance from class name\n");
        return nullptr;
#endif
    }


    void PipelineController::build()
    {
#if PPP_BUILD_TYPE
        for (auto& p : myConfig["processors"])
        {
            std::string pname = p["className"];
            auto sp = createProcessor(p);
            if (sp)
            {
                myProcessors.push_back(sp);
                VLOG_F(LOGLEVEL_DEBUG, "created a processor with name: %s \n", pname.c_str());
            }
        }
#endif
    }

    void PipelineController::computeAll()
    {

        for (size_t iData = 0; iData < myConfig["readers"].size(); iData++)
        {
            // set diff result path for each input file
            auto inputName = fs::path(myConfig["readers"][iData]["dataFileName"].get<std::string>());
            std::string storagePath = Context::dataStorage().generateStoragePath(inputName.stem().string());
            Context::dataStorage().setStoragePath(storagePath);
#if PPP_BUILD_TYPE
            std::shared_ptr<Processor> reader = createProcessor(myConfig["readers"][iData]);
#else
            std::shared_ptr<Reader> reader = std::make_shared<Reader>();
#endif
            reader->setConfig(myConfig["readers"][iData]);
            reader->process();
            reader->prepareOutput();
            auto data = reader->outputData();
            if (!data)
            {
                std::cout << "Failed to load the  data from file: \n"
                          << " Here is the reader config: \n"
                          << myConfig["readers"][iData][""] << std::endl;
            }

            auto info = std::make_shared<Information>();
            (*info)["parallelism"] = myConfig["parallelism"];

            compute(data, info);

            // report writing, todo: get output name from configuration
            std::string outfile = Context::dataStorage().getFullPath("processed_info.json");
            std::fstream report_info(outfile);

            // processsed data writing, writer is not always needed
            bool hasWriter = !myConfig["writers"].empty();
            if (hasWriter)
            {
#if PPP_BUILD_TYPE
                std::shared_ptr<Processor> writer = createProcessor(myConfig["writers"][iData]);
#else
                std::shared_ptr<Writer> writer = std::make_shared<Writer>();
#endif
                writer->setConfig(myConfig["writers"][iData]);
                if (myProcessors.size() == 0) // if there is no processor, do data IO format translation
                {
                    writer->setInputData(data);
                    writer->setInputInformation(info);
                }
                else
                {
                    writer->setInputData(myProcessors[myProcessors.size() - 1]->outputData());
                    writer->setInputInformation(myProcessors[myProcessors.size() - 1]->getOutputInformation());
                }
                writer->prepareInput();
                writer->process();
            }
            report_info << (*info);
        }
    }

    void PipelineController::process()
    {
        /// CONSIDER: estimate the problem scale, cpu cores, then print the estimated processing time

        // 1. open the remain doc, reader
        this->initialize(); // setup, check config, output name

        // 2. build the pipeline
#if PPP_BUILD_TYPE
        build(); // building pipeline from json configuraton file
#else
// this is example of hardcoded pipeline building
#if PPP_USE_BOOST_PROCESS
        myProcessors.push_back(std::make_shared<CommandLineProcessor>());
#endif
#endif
        // 3. execute
        computeAll();

        // 4. close and clean up, not necessary, no raw pointers are used
        this->finalize();
    }

    void PipelineController::handleSignal(int signum)
    {
        myCurrentProcessor->dump(); // get the current running processor
        LOG_F(WARNING, "Caught interrupt signal %d, call clean up function", signum);
        std::exit(signum); // Terminate program
    }

    void PipelineController::compute(std::shared_ptr<DataObject> data, std::shared_ptr<Information> info)
    {

        /// NOTE:the ctrl-C signal handler must be installed to the main thread
        /// it is expected more error message can be capture like data storage dump,
        /// if SIGSEGV (invalid access storage) happened on other threads
        callback_wrapper = std::bind(&PipelineController::handleSignal, this, std::placeholders::_1);
#ifndef _WIN32
        struct sigaction sigIntHandler;
        sigIntHandler.sa_handler = ppp_signal_callback_handler;
        sigemptyset(&sigIntHandler.sa_mask); // posix API
        sigIntHandler.sa_flags = 0;
        sigaction(SIGINT, &sigIntHandler, NULL); // more signal handler can be added for diff signal
#endif

#if PPP_USE_THREADING
        size_t nCores = myConfig["parallelism"]["threadsOnNode"];
        auto threadPool = std::make_shared<ThreadPoolType>();
        if (data->itemCount() < nCores)
        {
            VLOG_F(LOGLEVEL_DEBUG, "item count is smaller than thread count!, is this a test data?");
        }
#endif

        for (std::size_t i = 0; i < myProcessors.size(); i++)
        {
            std::string pname = myConfig["processors"][i]["className"];
            VLOG_F(LOGLEVEL_PROGRESS, " ========processor #%lu %s started=======", i, pname.c_str());
            myCurrentProcessor = myProcessors[i]; // current processor index is needed in handleSignal()
            auto start = std::chrono::steady_clock::now();
            json thisConfig = myConfig["processors"][i];
            myProcessors[i]->setConfig(thisConfig);
            if (i == 0)
            {
                myProcessors[i]->setInputData(data);
                myProcessors[i]->setInputInformation(info);
            }
            else
            {
                myProcessors[i]->setInputData(myProcessors[i - 1]->outputData());
                myProcessors[i]->setInputInformation(myProcessors[i - 1]->getOutputInformation());
            }
            myProcessors[i]->setOperator(myOperator);
#if PPP_USE_THREADING
            auto aExecutor = std::make_shared<ThreadPoolExecutor>(myProcessors[i], nCores, threadPool);
#else
            auto aExecutor = std::make_shared<SequentialExecutor>(myProcessors[i]);
#endif
            aExecutor->process(); // must be declared as pointer, otherwise no polymorphism!
            std::chrono::duration<double, std::milli> duration = std::chrono::steady_clock::now() - start;
            VLOG_F(LOGLEVEL_PROGRESS, " ====== processor #%lu  %s completed in %lf seconds =====", i, pname.c_str(),
                   duration.count() / 1000);
        }
    }

} // namespace PPP

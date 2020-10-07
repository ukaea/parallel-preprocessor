#include "DataObject.h"
#include "OperatorProxy.h"
#include "PreCompiled.h"
#include "Processor.h"
#include "Utilities.h"
#include "WorkflowController.h"

#pragma once


namespace PPP
{
    /// \ingroup PPP
    /**
     * The control class for the data processing in pipeline topology,
     * one processor followed by another processor.
     *
     * As a WorkflowController derived class, this class is the most important class in this module,
     * TypeSystem registration for all types in this module should be done by this class.
     */
    class AppExport PipelineController : public WorkflowController
    {
    public:
        /// constructed from a json config filename
        PipelineController(const std::string& config_filename);
        /// constructed from command line argurments
        PipelineController(int argc, char** argv);

        virtual ~PipelineController();

        void setConfig(const Config& json_config);
        const Config config() const;
        void checkConfig() const;

        void setOperator(std::shared_ptr<OperatorProxy> v)
        {
            myOperator = v;
        }

        virtual void process();

    protected:
        /// TypeSystem registration for all types in this module
        static void initTypes(); /// NOTE: static function can not be virtual

        /** prepration before processing, init log, set default names */
        virtual void initialize();
        virtual void computeAll();
        /** save result, log, etc */
        virtual void finalize();

        std::shared_ptr<Processor> createProcessor(const Config& p);

        /// build the processors topopology from configuration file
        virtual void build() override;
        /// process all processors in the pipeline
        void compute(std::shared_ptr<DataObject> data, std::shared_ptr<Information>);
        /// process Ctrl-C signal
        void handleSignal(int signum);

    protected:
        Config myConfig;

        std::vector<std::shared_ptr<Processor>> myProcessors;
        /// current running processor
        std::shared_ptr<Processor> myCurrentProcessor;

        /// the only operator proxy shared by all processors
        std::shared_ptr<OperatorProxy> myOperator;
    };
} // namespace PPP

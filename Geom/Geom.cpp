#include "GeometryPipelineController.h"
#include "GeometryProcessor.h"

/// tmp solution: for undefined external symbol, to export loguru.hpp for visual C++
/// loguru.cpp has been compiled into pppApp.dll, but symbol not exported
#if defined(_WIN32)
#include "../third-party/loguru/loguru.cpp"
#endif

#include "PPP/Utilities.h"

#include "BoundBoxBuilder.h"
#include "CollisionDetector.h"
#include "GeometryImprinter.h"
#include "GeometryPropertyBuilder.h"
#include "GeometrySearchBuilder.h"
#include "GeometryShapeChecker.h"

#include "GeometryReader.h"
#include "GeometryWriter.h"

#include "GeometryOperatorProxy.h"

/// Those files are not removed for the time being
//#include "GeometryDecomposer.h"
//#include "GeometryEnclosureBuilder.h"
//#include "GeometryFaceter.h"
//#include "GeometryFixer.h"


// source file to store static type data, also add these into `initTypes()`
TYPESYSTEM_SOURCE(Geom::GeometryProcessor, PPP::Processor);
TYPESYSTEM_SOURCE(Geom::GeometryReader, PPP::Reader);
TYPESYSTEM_SOURCE(Geom::GeometryWriter, PPP::Writer);

TYPESYSTEM_SOURCE(Geom::GeometryShapeChecker, Geom::GeometryProcessor);
TYPESYSTEM_SOURCE(Geom::GeometryPropertyBuilder, Geom::GeometryProcessor);
TYPESYSTEM_SOURCE(Geom::GeometrySearchBuilder, Geom::GeometryProcessor);
TYPESYSTEM_SOURCE(Geom::BoundBoxBuilder, Geom::GeometryProcessor);

TYPESYSTEM_SOURCE(Geom::CollisionDetector, Geom::GeometryProcessor);
TYPESYSTEM_SOURCE(Geom::GeometryImprinter, Geom::CollisionDetector);

// TYPESYSTEM_SOURCE(Geom::GeometryFaceter, Geom::GeometryProcessor);
// TYPESYSTEM_SOURCE(Geom::GeometryDecomposer, Geom::GeometryProcessor);
// TYPESYSTEM_SOURCE(Geom::GeometryEnclosureBuilder, Geom::GeometryProcessor);
// TYPESYSTEM_SOURCE(Geom::GeometryFixer, Geom::GeometryProcessor);

namespace Geom
{
    using namespace PPP;
    // this should be refactored into Geom::initTypes()
    void GeometryPipelineController::initTypes()
    {

        GeometryProcessor::init();
        GeometryReader::init();
        GeometryWriter::init();

        GeometryPropertyBuilder::init();
        GeometrySearchBuilder::init();
        BoundBoxBuilder::init();
        GeometryShapeChecker::init();
        CollisionDetector::init();
        GeometryImprinter::init();
        // GeometryFaceter::init();
        // GeometryEnclosureBuilder::init();
        // GeometryDecomposer::init();
        // GeometryFixer::init();
        VLOG_F(LOGLEVEL_DEBUG, "Processor types in geom module has been registered \n");
    }


    bool GeometryPipelineController::GuiUp()
    {
#if PPP_BUILD_GUI
        return true;
#else
        return false;
#endif
    }


    void GeometryPipelineController::initialize()
    {
        PipelineController::initialize();
        GeometryPipelineController::initTypes();
    }


    void GeometryPipelineController::process()
    {

        // 1. open the step doc, reader
        this->initialize(); // setup, check config, output name

        // 2. build the pipeline
#if PPP_BUILD_TYPE
        build(); // building pipeline from json configuraton file
#else
        // this is example of hardcoded pipeline building
        myProcessors.push_back(std::make_shared<GeometryShapeChecker>());
        // setConfig()
        myProcessors.push_back(std::make_shared<GeometryPropertyBuilder>());
        myProcessors.push_back(std::make_shared<BoundBoxBuilder>());
        myProcessors.push_back(std::make_shared<CollisionDetector>());
        myProcessors.push_back(std::make_shared<GeometryImprinter>());
#endif
        // 3. execute
        computeAll();

        // 4. close and clean up, not necessary, no raw pointers are used
        this->finalize();
    }

} // namespace Geom
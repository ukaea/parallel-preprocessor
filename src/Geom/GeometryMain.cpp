

#include "GeometryPipelineController.h"
#include <cstdlib>
#include <iostream>
#include <string>

/// the program running without Qt Gui
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "Error: input config file is not provided" << std::endl;
        std::cout << "Usage: pppGeomPipeline config.json" << std::endl;
        std::terminate();
    }
    auto p = new Geom::GeometryPipelineController(argc, argv);
    p->process();

    return 0;
}

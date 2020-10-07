
#ifndef PPP_PREPROCESSOR_H
#define PPP_PREPROCESSOR_H

#include "GeometryData.h"
#include "PPP/PipelineController.h"


namespace Geom
{
    using namespace PPP;

    /**
     * class derived from PipelineController class to deal with geometry data
     */
    class GeomExport GeometryPipelineController : public PipelineController
    {
    public:
        using PipelineController::PipelineController;

        virtual void process() override;

        static bool GuiUp();

    protected:
        static void initTypes();
        virtual void initialize() override;
        // virtual void finalize() override;

    private:
    };


} // namespace Geom

#endif // header firewall

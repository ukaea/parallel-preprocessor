#pragma once
#include "TypeDefs.h"

namespace PPP
{

    /// dataflow topology type for workflow
    enum TopologyType
    {
        Unknown = 0,  ///< default value, unkown, unspecified
        Pipeline = 1, ///< single data source, single output pipeline, no fork
        Tree = 2,     ///< single data source, multiple pipelines/forks/outputs
        Graph = 3     ///< Directed acyclic graph, most complicate
    };

    NLOHMANN_JSON_SERIALIZE_ENUM(TopologyType, {
                                                   {TopologyType::Unknown, "Unknown"},
                                                   {TopologyType::Pipeline, "Pipeline"},
                                                   {TopologyType::Tree, "True"},
                                                   {TopologyType::Graph, "Graph"},
                                               });

    /// \ingroup PPP
    /**
     * base class for all processor control class, building processor data flow topology.
     *
     * It is generalized from `PipelineController` to deal with other data flow topologies.
     * In the future, maybe workflow with a graph topology can be supported.
     */
    class AppExport WorkflowController
    {
    protected:
        TopologyType myTopologyType;

        virtual void build() = 0;
    };

} // namespace PPP
#pragma once

namespace PPP
{
    /**
     * \brief result of processor, json serializable
     *
     * write to report file, specified by `report` parameter in the processor config
     * checkpoint, serializable to PropertyContainer/DataObject.
     * */
    class AppExport ProcessorResult
    {
    public:
        bool status;        /// status OK or not,  CONSIDER: enum class ProcessStatus
        Information result; /// CONSIDER: const shared pointer
    };

    inline void to_json(json& j, const ProcessorResult& p)
    {
        j = json{{"status", p.status}, {"result", p.result}};
    }

} // namespace PPP
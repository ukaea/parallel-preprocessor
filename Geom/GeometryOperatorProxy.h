#ifndef PPP_GEOMETRY_VIEWER_H
#define PPP_GEOMETRY_VIEWER_H


#include "../PPP/OperatorProxy.h"
#include "./GeometryData.h"
#include "./OccUtils.h"

namespace Geom
{
    using namespace PPP;

    /** abstract Proxy class to remote viewer
     * to be implemented in each module, such GeometryOperatorProxy
     */
    class GeometryOperatorProxy : public OperatorProxy
    {
    public:
        using OperatorProxy::OperatorProxy;

        virtual bool interactive() override
        {
            return false;
        }

        virtual void display(const json& msg) override
        {
            for (auto& op : myOperators)
            {
                // send request to show, then send file
                op->display(msg);
            }
            /// NOTE: tmp solutoin: block for 20 seconds to mimic user response
            using namespace std::literals::chrono_literals;
            std::this_thread::sleep_for(20s);
        }

        // virtual void show(const ItemIndexType index) override
        // virtual void printMessage(const std::string msg) override;
        // virtual void getStatus() override;
    protected:
        virtual bool init() override
        {
            return false;
        }

        virtual Response request(const Transaction&) override
        {
            Response s;
            return s;
        }

    protected:
    private:
        // ViewClient
    };
} // namespace Geom

#endif
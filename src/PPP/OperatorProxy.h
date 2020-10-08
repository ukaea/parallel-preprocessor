#ifndef PPP_OBSERVER_PROXY_H
#define PPP_OBSERVER_PROXY_H

#include "./AbstractOperator.h"
#include "./TypeDefs.h"
#include <string>

namespace PPP
{

    /// \ingroup PPP
    /**  Proxy class for Operator, implements AbstractOperator and behaves like a operator
     *
     * Processor will interact with only with this ProxyOperator, which forwards to multiple concrete operator
     * ConsoleOperator is the default operator, it will print json data without actuall caring the content,
     * while each module should implement Graphic and Web Operator to view result and confirm user choice.
     * see wiki on user interface design section
     */
    class AppExport OperatorProxy : public AbstractOperator
    {
    public:
        OperatorProxy() = default;
        virtual ~OperatorProxy() = default;

        OperatorProxy(const std::shared_ptr<AbstractOperator> op)
        {
            myOperators.push_back(op);
        }
        OperatorProxy(const Config& conf)
        {
            myConfig = conf;
        }

        Config config()
        {
            return myConfig;
        }
        void setConfig(const Config& conf)
        {
            myConfig = conf;
        }

        /// add subscribers/operators, note `operator` is a c++ keyword
        void addOperator(const std::shared_ptr<AbstractOperator> op)
        {
            myOperators.push_back(op);
        }

        // @{
        /// check if it is block mode operator
        virtual bool interactive()
        {
            return false;
        }

        virtual bool secure() override
        {
            return m_secure;
        }

        /// display the content
        virtual void display(const json& msg) override
        {
            for (auto& op : myOperators)
            {
                // send request to show, then send file
                op->display(msg);
            }
        }

        /// report the content
        virtual void report(const json& msg) override
        {
            for (auto& op : myOperators)
            {
                // send request to show, then send file
                op->report(msg);
            }
        }

        // @}

    protected:
        /// prepare the viewer, e.g. connect to the remote message server
        virtual bool init()
        {
            return false; // not yet done
        }

        /// client and server communication
        virtual Response request(const Transaction&)
        {
            json msg;
            return msg;
        }

        std::vector<std::shared_ptr<AbstractOperator>> myOperators;

    private:
        bool m_secure = true;
        Config myConfig;
    };
} // namespace PPP

#endif
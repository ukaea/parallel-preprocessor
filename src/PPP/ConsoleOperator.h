#pragma once

#include "./AbstractOperator.h"
#include <iostream>

namespace PPP
{

    /// \ingroup PPP

    /** can be directed to file, as the default operator interface
     * */
    class AppExport ConsoleOperator : public AbstractOperator
    {

    public:
        ConsoleOperator()
                : myInput(std::cin)
                , myOutput(std::cout)
        {
        }
        ~ConsoleOperator() = default;

        ConsoleOperator(std::ostream& out, std::istream& ins)
                : myInput(ins)
                , myOutput(out)
        {
        }

        /*
        virtual inline void setOutputStream(std::ostream& os)
        {
            myOutput = os;

        }

        virtual inline void setInputStream(std::istream& ins)
        {
            myInput = ins;
        }
        */

        virtual void display(const json& msg) override
        {
            myOutput << msg;
        }
        virtual void report(const json& msg) override
        {
            myOutput << msg;
        }
        virtual bool secure() override
        {
            return m_secure;
        }

    protected:
        bool m_secure = true;
        std::istream& myInput;
        std::ostream& myOutput;

    private:
    };

} // namespace PPP

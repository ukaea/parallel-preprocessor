/***************************************************************************
 *   Copyright (c) 2020 Qingfeng Xia  <qingfeng.xia@iesensor.com>          *
 *   This file is part of parallel-preprocessor CAx development system.    *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

/** copyright note:
    this class (NOT in the funded project work scope) is completed and tested,
    in Qingfeng Xia's personal time (weekend of March 21, 2020).
*/

#pragma once

#include "Process.h"
#include "Processor.h"

#if __has_include(<format>)
#include <format>
using format = std::format;
// modern format lib support C++20, python format,  https://github.com/fmtlib/fmt
#elif __has_include(<fmt/format>)
#include <fmt/format>
using format = fmt::format;
#else
//#error "C++20 format, <boost/format.hpp> <fmt/format.h> can not be found"
#endif


namespace PPP
{

    /// \ingroup PPP
    /**
     * \brief launch external process on ditributive worker nodes
     *
     * this approach is not efficient for the cost of launch new process, but flexible and powerful
     * e.g. you can write processor in any language for pipeline working.
     *
     * File IO is another constraint, so shared memery and pipe(string stream),
     * assuming the external program get input from stream and output to stream like
     * `echo input|myprog > output`
     *
     * ThreadPoolExector for blocking/synchronous executing
     * ProcessPoolExecutor for nonblocking/async, future
     */
    class AppExport CommandLineProcessor : public Processor
    {
        TYPESYSTEM_HEADER();

    protected:
        bool blocking = true;
        bool usingPipeStream = false;

        std::string myCommandLinePattern; // todo: std::format C++20

    private:
        /// std::variant of filename,  stream, or just declear new variables
        VectorType<std::string> myItemInputs;
        VectorType<std::string> myItemOutputs;

    public:
        CommandLineProcessor() = default;
        ~CommandLineProcessor() = default;

        /**
         * \brief preparing work in serial mode
         */
        virtual void prepareInput() override final
        {
            Processor::prepareInput();
            myCommandLinePattern = parameter<std::string>("commandLinePattern");
            blocking = parameter<bool>("blocking");
            // input and output preparation

            /// prepare private properties like `std::vector<T>.resize(myInputData->itemCount());`
            /// therefore accessing item will not cause memory reallocation and items copying
            myItemInputs.resize(myInputData->itemCount());
            myItemOutputs.resize(myInputData->itemCount());
            // for (ItemIndexType i = 0; i < myInputData->itemCount(); i++)
        }

        /**
         * \brief preparing work in serial mode, write report, move data into `myOutputData`
         */
        virtual void prepareOutput() override
        {
            myOutputData->emplace<decltype(myItemOutputs)>("myItemOutputs", std::move(myItemOutputs));
        }

        /**
         * \brief process data item in parallel without affecting other data items in blocking mode
         *
         * @param index: index to get input data (file or stream) and output filename or stream
         */
        virtual void processItem(const ItemIndexType index) override final
        {
            myItemInputs[index] = prepareItemInput(index);
            myItemOutputs[index] = "itemOutput_" + std::to_string(index);
            if (blocking)
            {
                if (usingPipeStream)
                {
#if PPP_USE_BOOST_PROCESS
                    // boost process and pipe
#else
                    LOG_F(WARNING, "boost::process is not available");
#endif
                }
                else
                    Utilities::runCommand(generateCommandLine(index)); // run in blocking mode
            }
            // for non-blocking execution, using ProcessPoolExectutor and its derived class
        }

    protected:
        /// derive class should override this class
        /// e.g. dump item into file and return file name as input argument
        virtual std::string prepareItemInput(const ItemIndexType i)
        {
            return "item" + std::to_string(i);
        }

        /// process result generated by external process
        virtual bool processItemOutput(const ItemIndexType i)
        {
            return true; // do nothing
        }

        virtual std::string generateCommandLine(const ItemIndexType i)
        {
            std::string input_file = myItemInputs[i];
#if __has_include(<format>)
            return format(myCommandLinePattern, input_file);
#else
            return myCommandLinePattern + " " + input_file; // dummy example
#endif
        }
    };
} // namespace PPP
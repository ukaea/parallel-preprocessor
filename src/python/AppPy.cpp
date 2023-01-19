// cmake has set the path, or pybind11 has been installed `/usr/include`
#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "third-party/pybind_json.hpp"

#include "../Geom/GeometryPipelineController.h"


namespace py = pybind11;

PYBIND11_MODULE(ppp, m)
{
    m.doc() = R"pbdoc(
        Pybind11 wrapper of the parallel preprocessor
        -----------------------
        .. currentmodule:: cmake_example
        .. autosummary::
           :toctree: _generate
    )pbdoc";

    typedef PPP::PipelineController pc;
    py::class_<pc>(m, "PipelineController")
        .def(py::init<const std::string&>(), "constructor with configuration file path")
        .def("config", &pc::config, "get configuration json")
        .def("process", [](pc& self) { // C++ std::cout does not redirected to python properly, so do this
            py::scoped_ostream_redirect stream(std::cout,                               // std::ostream&
                                               py::module::import("sys").attr("stdout") // Python output
            );
            self.process();
        }); // semicolon at the end of the chain

    using namespace Geom;
    /// ctor() binding sequence has some impact, string can be match to `std::vector<std::string>`
    typedef GeometryPipelineController mt;
    py::class_<mt>(m, "GeometryPipeline")
        //.def(py::init<>(), "default constructor, without config input")
        .def(py::init<const std::string&>(), "constructor with configuration file path")
        .def(py::init([](std::vector<std::string> argv) {
                 std::vector<char*> args;
                 args.reserve(argv.size() + 1);
                 for (auto& s : argv)
                     args.push_back(const_cast<char*>(s.c_str()));
                 // args.push_back(nullptr);
                 return new mt(args.size(), args.data());
             }),
             "constructor with argument list such as sys.argv")

        .def("config", &mt::config, "get configuration json")
        .def("process", [](mt& self) { // C++ std::cout does not redirected to python properly, so do this
            py::scoped_ostream_redirect stream(std::cout,                               // std::ostream&
                                               py::module::import("sys").attr("stdout") // Python output
            );
            self.process();
        }); // semicolon at the end of the chain


#ifdef PPP_VERSION_INFO // compiler definition
    m.attr("__version__") = PPP_VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}
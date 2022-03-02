#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Together with generate_site.sh, CMake config, 
this script will generate html site for doxygen doc, coverage and other QA report/

adapted from EERA: https://github.com/ScottishCovidResponse/Covid19_EERAModel/blob/dev/scripts/site_generation.py
with hand-crafted: flawfinder, similarity, clang_tidy parsers

All generated html file are located in repo_root/site/, 
so all other resource path paths should be relative to repo_root/site/

Changelog: 
    - coverage_report_folder by default is located in `build` folder
    - generalized by extract out `my_project_name` as a variable 
    - this python script works with generate_site.sh, run in build folder for local test
    - github CI works in project root folder, while build folder must be called `build`


Todo: flowfinder.html page is missing
      CI badge URL is hardcoded to this repo, 

"""

import os
import re
import os.path

my_project_name = "parallel-preprocessor"
if os.path.exists("src"):  # CI mode. all paths should be relative to repo_root/site/
    coverage_report_folder = "../build/{}_coverage/index.html".format(my_project_name)
    doxygen_html_index = "../doxygen/html/index.html"
else:  # local build test mode, run in build folder
    coverage_report_folder = "{}_coverage/index.html".format(my_project_name)
    doxygen_html_index = "doxygen/html/index.html"

class HTMLFileBuilder(object):
    def __init__(self, name=my_project_name):
        self._clang_tidy_data = {}
        self._name = name
        self._page_list = {
            "Documentation": {
                "Doxygen Code Documentation": {
                    "filename": "doxygen-docs.html",
                    "source": doxygen_html_index,
                },
                "Developer Guide": {
                    "filename": "developer_documentation.html",
                    "source": doxygen_html_index,
                },
            },
            "CodeTools": {
                "CPP Check": {
                    "filename": "cppcheck.html",
                    "source": "cppcheck/index.html",
                },
                "Code Coverage": {
                    "filename": "code-coverage.html",
                    "source": coverage_report_folder,
                },
                "Flawfinder": {
                    "filename": "flawfinder.html",
                    "source": "flawfinder.html",
                },
                "Sim C++": {"filename": "simcpp.html", "source": "simcpp.html"},
                "Clang Tidy": {
                    "filename": "clang_tidy.html",
                    "source": "clang_tidy.html",
                },
            },
        }
        self._extern_pages = [
            "Doxygen Code Documentation",
            "Developer Guide",
            "CPP Check",
            "Code Coverage",
        ]

        self._doc_imports = [
            '\t\t\t\t\t<a class="dropdown-item" href="site/{}">{}</a>'.format(
                self._page_list["Documentation"][n]["filename"], n
            )
            for n in self._page_list["Documentation"]
        ]
        self._code_imports = [
            '\t\t\t\t\t<a class="dropdown-item" href="site/{}">{}</a>'.format(
                self._page_list["CodeTools"][n]["filename"], n
            )
            for n in self._page_list["CodeTools"]
        ]
        self._header = """
    <head>
    <meta charset="UTF-8">
    <!-- Redirect -->
    <!--<meta http-equiv="refresh" content="1;url=doxygen/html/index.html">-->
    <meta name="viewport" content="width=device-width, initial-scale=1">
    
    <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/4.5.0/css/bootstrap.min.css">
    <script src="https://code.jquery.com/jquery-3.5.1.slim.min.js" integrity="sha384-DfXdz2htPH0lsSSs5nCTpuj/zy4C+OGpamoFVy38MVBnE+IbbVYUew+OrCXaRkfj" crossorigin="anonymous"></script>
    <script src="https://cdn.jsdelivr.net/npm/popper.js@1.16.0/dist/umd/popper.min.js" integrity="sha384-Q6E9RHvbIyZFJoft+2mJbHaEWldlvI9IOYy5n3zV9zzTtmI3UksdQRVvoxMfooAo" crossorigin="anonymous"></script>
    <script src="https://stackpath.bootstrapcdn.com/bootstrap/4.5.0/js/bootstrap.min.js" integrity="sha384-OgVRvuATP1z7JjHLkuOU7Xw704+h835Lr+6QL9UvYjZE3Ipu6Tp75j7Bh/kR0JKI" crossorigin="anonymous"></script>
    
    <title>{site_name}</title>

    <nav class="navbar navbar-expand-sm bg-dark navbar-dark">
      <!-- Brand -->
      <a class="navbar-brand" href="{index_addr}">{project_name}</a>
    
      <!-- Links -->
      <ul class="navbar-nav">
        <!-- Dropdown -->
      <li class="nav-item dropdown">
        <a class="nav-link dropdown-toggle" href="#" id="navbardrop" data-toggle="dropdown">
          Documentation
        </a>
        <div class="dropdown-menu">
          {document_imports}
        </div>
      </li>
        <!-- Dropdown -->
        <li class="nav-item dropdown">
          <a class="nav-link dropdown-toggle" href="#" id="navbardrop" data-toggle="dropdown">
            Code Checks
          </a>
          <div class="dropdown-menu">
            {code_imports}
          </div>
        </li>
      </ul>
    </nav> 
        
</head>
        """.format(
            site_name=self._name,
            index_addr="index.html",
            project_name=my_project_name,
            document_imports="\n".join(self._doc_imports),
            code_imports="\n".join(self._code_imports),
        )
        self._flawfinder_data = {}
        self._simcpp_data = []

    def parse_simcpp(self, input_file):
        # parse similarity analysis report from simc++
        _entry = {"file_1_lines": [], "file_2_lines": []}
        if not os.path.exists(input_file):
            print("Warning: input file is not found ", os.path.abspath(input_file))
        with open(input_file) as f:
            for line in f:
                try:
                    if "src" in line:
                        if "file_1" in _entry:
                            self._simcpp_data.append(_entry)
                            _entry = {"file_1_lines": [], "file_2_lines": []}
                        _file_addr_1, _file_addr_2 = line.split("|")
                        _file_addr_1 = _file_addr_1.strip()
                        _file_addr_2 = _file_addr_2.strip()
                        _entry["file_1"] = _file_addr_1
                        _entry["file_2"] = _file_addr_2
                    else:
                        _l1, _l2 = line.split("|")
                        _l1 = _l1.strip()
                        _l2 = _l2.strip()
                        _entry["file_1_lines"].append(_l1)
                        _entry["file_2_lines"].append(_l2)
                except ValueError:
                    continue

    def parse_clangtidy(self, input_file):
        if not os.path.exists(input_file):
            print("Warning: input file is not found ", os.path.abspath(input_file))
        with open(input_file) as f:
            for line in f:
                try:
                    # search for line blocks with regex, but message span several lines
                    entry = re.findall(r"(.+:\d.+:.+)", line)[0]  # bug: always index error
                    entry = entry.split(": ", 1)
                    try:
                        file_name, address = entry[0].split(":", 1)
                    except ValueError:
                        file_name = entry[0]
                        address = ""
                    library = re.findall(r"\[(.+)\]", entry[1])
                    message = (
                        entry[1]
                        if len(library) == 0
                        else entry[1].replace(" [{}]".format(library[0]), "")
                    )
                    if file_name not in self._clang_tidy_data:
                        self._clang_tidy_data[file_name] = {}
                    self._clang_tidy_data[file_name][address] = {
                        "message": message,
                        "library": "" if len(library) == 0 else library[0],
                    }
                except IndexError:
                    continue

    def parse_flawfinder(self, input_file):
        if not os.path.exists(input_file):
            print("Warning: input file is not found ", os.path.abspath(input_file))
        _lines_str = ""
        with open(input_file) as f:
            for line in f:
                # search for that has 3 colons, but message span multiple lines
                if re.findall(r"(.+:\d.+:.+)", line):
                    if _lines_str:
                        _key, _message = _lines_str.split(":  ", 1)
                        _library, _message = _message.split(":\n", 1)
                        _file, _address = _key.split(":")
                        if _file not in self._flawfinder_data:
                            self._flawfinder_data[_file] = {}
                        self._flawfinder_data[_file][_address] = {
                            "message": _message,
                            "library": _library,
                            "file": _file,
                        }
                    _lines_str = line
                elif "ANALYSIS" in line:
                    break
                elif re.findall("  .+", line):
                    _lines_str += line

    def build_index(self):
        _code_buttons = [
            '<a href="site/{}" class="btn btn-primary" role="button">{}</a>'.format(
                self._page_list["CodeTools"][n]["filename"], n
            )
            for n in self._page_list["CodeTools"]
        ]
        _doc_buttons = [
            '<a href="site/{}" class="btn btn-primary" role="button">{}</a>'.format(
                self._page_list["Documentation"][n]["filename"], n
            )
            for n in self._page_list["Documentation"]
        ]
        _index_str = """
<!DOCTYPE HTML>
<html lang="en-GB">
    {header}
    <body>
        <div class="container">
            <div class="jumbotron">
            <h1>{project_name}</h1>      
            <p>This is the documentation website for the {project_name} repository on GitHub.</p>
            </div>
        </div>
        <div class="container">
          <h2>Documentation</h2>
            {doc_buttons}    
         </div>
        <div class="container">
          <h2>Code Check Reports</h2> 
            {code_buttons}  
         </div>
        <div class="container">
            <p></p>
            <h2>Builds</h2>           
            <table class="table table-striped">
              <thead>
                <tr>
                  <th>Branch</th>
                  <th>Status</th>
                </tr>
              </thead>
              <tbody>
                <tr>
                  <td>master branch on UKAEA</td>
                  <td> <img src="https://github.com/ukaea/parallel-preprocessor/workflows/ubuntu-macos/badge.svg?branch=master&event=push" class="img-rounded" alt="Master Status"> </td>
                </tr>
                <tr>
                    <td>dev of Qingfeng Xia fork</td>
                    <td> <img src="https://github.com/qingfengxia/parallel-preprocessor/workflows/ubuntu-macos/badge.svg?branch=dev&event=push" class="img-rounded" alt="Dev Status"> </td>
                </tr>
                <tr>
                    <td>gh-pages</td>
                    <td> <img src="https://github.com/ukaea/parallel-preprocessor/workflows/ubuntu-macos/badge.svg?branch=gh-pages&event=push" class="img-rounded" alt="GitHub Pages Status"> </td>
                </tr>
              </tbody>
            </table>
          </div>
          
    </body>
</html>
      """.format(
            header=self._header.replace("index", "../index"),
            site_name=self._name,
            project_name=my_project_name,
            code_buttons="\n".join(_code_buttons),
            doc_buttons="\n".join(_doc_buttons),
        )

        with open("index.html", "w") as f:
            f.write(_index_str)

    def build_wrapper_pages(self):

        template = """
<!DOCTYPE HTML>
<html lang="en-GB">
  {head}
    <body>
        <div class="container">
            <div class="jumbotron">
            <h1>{title}</h1>      
            <p></p>
            </div>
            <div>
                <center><object data="{address_of_source}" width=1100 height=900></object></center>
            </div>
        </div>
    </body>
</html>
      """

        for categ in self._page_list:
            for other_page in self._page_list[categ]:
                if other_page in self._extern_pages:
                    with open(
                        "site/" + self._page_list[categ][other_page]["filename"], "w") as f:
                        f.write(
                            template.format(
                                title=other_page,
                                head=self._header.replace("site/", "").replace(
                                    "index", "../index"
                                ),
                                address_of_source=self._page_list[categ][other_page]["source"],
                            )
                        )

    def build_flawfinder_html(self):
        _body = """
<!DOCTYPE HTML>
<html lang="en-GB">
    {header}
    <body>
    <div class="container">
    <div class="jumbotron">
        <h1>Flawfinder</h1>
        <p>Listed below are the check results from Flawfinder.</p>
    </div>
    <p></p>
    </div>
    <div class="container">
    <table class="table table-striped">
    <thead>
      <tr>
        <th>Line</th>
        <th>Library</th>
        <th>Message</th>
      </tr>
    </thead>
    {body}
    </container>
    </body>
</html>
      """
        _table_body = ""
        for entry in self._flawfinder_data:
            entry_str = """
              <tr class="table-dark text-dark">
              <td colspan="4" style="text-align:center"><strong>{file}</strong></td>
              </tr>
              """.format(
                file=entry
            )
            for address in sorted(self._flawfinder_data[entry].keys()):
                entry_str += """
                <tr>
                    <td>{address}</td>
                    <td>{library}</td>
                    <td>{message}</td>
                </tr>
                """.format(
                    file=entry,
                    address=address,
                    library=self._flawfinder_data[entry][address]["library"],
                    message=self._flawfinder_data[entry][address]["message"],
                )
            _table_body += entry_str

        wf = "site/{}".format(self._page_list["CodeTools"]["Flawfinder"]["filename"])
        with open(wf, "w") as f:
            f.write(_body.format(
                    body=_table_body,
                    site_name=self._name,
                    header=self._header.replace("index", "../index").replace(
                        "site/", ""
                    ),
                )
            )

    def build_clang_html(self):
        _body = """
<!DOCTYPE HTML>
<html lang="en-GB">
    {header}
    <body>
    <div class="container">
    <div class="jumbotron">
        <h1>Clang Tidy</h1>
        <p>Listed below are the warnings and suggestions made by Clang Tidy during compilation.</p>
    </div>
    <p></p>
    </div>
    <div class="container">
    <table class="table table-striped">
    <thead>
      <tr>
        <th>Type</th>
        <th>Address</th>
        <th>Library</th>
        <th>Message</th>
      </tr>
    </thead>
    {body}
    </container>
    </body>
</html>
"""
        _buttons = {
            "warning": '<button type="button" class="btn btn-warning"><strong>Warning</strong></button>',
            "info": '<button type="button" class="btn btn-info"><strong>Suggestion</strong></button>',
        }

        _table_body = ""

        for entry in self._clang_tidy_data:
            entry_str = """
            <tr class="table-dark text-dark">
            <td colspan="4" style="text-align:center"><strong>{file}</strong></td>
            </tr>
            """.format(
                file=entry
            )
            for address in sorted(self._clang_tidy_data[entry].keys()):
                is_warning = (
                    "warning"
                    in self._clang_tidy_data[entry][address]["message"].lower()
                )
                button = _buttons["warning"] if is_warning else _buttons["info"]

                entry_str += """
                <tr>
                    <td>{button}</td>
                    <td>{address}</td>
                    <td>{library}</td>
                    <td>{message}</td>
                </tr>
                """.format(
                    button=button,
                    file=entry,
                    address=address,
                    library=self._clang_tidy_data[entry][address]["library"],
                    message=self._clang_tidy_data[entry][address]["message"],
                )
            _table_body += entry_str

        with open(
            "site/{}".format(self._page_list["CodeTools"]["Clang Tidy"]["filename"]),
            "w",
        ) as f:
            f.write(
                _body.format(
                    body=_table_body,
                    site_name=self._name,
                    header=self._header.replace("site/", "").replace(
                        "index", "../index"
                    ),
                )
            )

    def build_simcpp_html(self):
        _body = """
<!DOCTYPE HTML>
<html lang="en-GB">
    {header}
    <body>
    <div class="container">
    <div class="jumbotron">
        <h1>Sim C++</h1>
        <p>Sim C++ finds similarities within the code files.</p>
    </div>
    <p></p>
    </div>
    <div class="container">
    <table class="table table-striped">
    <thead>
      <tr>
        <th>File 1</th>
        <th>File 2</th>
      </tr>
    </thead>
    {body}
    </container>
    </body>
</html>
      """
        _table_body = ""
        for i, entry in enumerate(self._simcpp_data):
            entry_str = """
        <tr class="table-dark text-dark">
        <td style="text-align:center"><strong>{}</strong></td>
        <td style="text-align:center"><strong>{}</strong></td>
        </tr>
        <tr>
          <td>\t<code>{}</code></td>
          <td>\t<code>{}</code></td>
        </tr>
        """.format(
                entry["file_1"],
                entry["file_2"],
                "\n\t".join(entry["file_1_lines"]),
                "\n\t".join(entry["file_2_lines"]),
            )
            _table_body += entry_str

        with open(
            "site/{}".format(self._page_list["CodeTools"]["Sim C++"]["filename"]), "w"
        ) as f:
            f.write(
                _body.format(
                    body=_table_body,
                    header=self._header.replace("site/", "").replace(
                        "index", "../index"
                    ),
                )
            )

    def Run(self):
        self.build_index()
        self.build_wrapper_pages()
        self.build_clang_html()
        self.build_flawfinder_html()
        self.build_simcpp_html()


if __name__ in "__main__":
    import argparse

    parser = argparse.ArgumentParser("ClangTidyParser")

    parser.add_argument("build_log", help="Build log file")
    parser.add_argument("flawfinder_log", help="Flaw Finder log file")
    parser.add_argument("simcpp_log", help="sim_c++ output file")

    args = parser.parse_args()

    cl_parser = HTMLFileBuilder()
    # fixed arg sequences
    cl_parser.parse_clangtidy(args.build_log)
    cl_parser.parse_flawfinder(args.flawfinder_log)
    cl_parser.parse_simcpp(args.simcpp_log)
    cl_parser.Run()


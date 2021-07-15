#include "utils.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
unordered_map<string, string> build_cfg;
unordered_map<string, string> libs_map;
}

void parse_build_cfg(const string& install_dir, const string& config)
{
    // parse the build.cfg file
    stringstream ss;
    ss << install_dir << DIR_SEP << "lib" << DIR_SEP << config << DIR_SEP << "build.cfg";

    if(!filesystem::exists(ss.str())) {
        cerr << "could not open configuration file: " << ss.str() << endl;
        exit(1);
    }

    ifstream infile(ss.str());
    string line;

    string major_version;
    string minor_version;
    string cxxflags;
    string compiler;
    constexpr int EXPECTED_ARGS = 5;
    while(getline(infile, line)) {
        trim(line);
        string key = before_first(line, "=");
        string value = after_first(line, "=");

        if(key == "WXVER_MAJOR") {
            build_cfg.insert({ "WXVER_MAJOR", value });
        } else if(key == "WXVER_MINOR") {
            build_cfg.insert({ "WXVER_MINOR", value });
        } else if(key == "CXXFLAGS") {
            build_cfg.insert({ "CXXFLAGS", value });
        } else if(key == "COMPILER") {
            build_cfg.insert({ "COMPILER", value });
        } else if(key == "BUILD") {
            build_cfg.insert({ "BUILD", value });
        }

        // stop processing once we got everything we need
        if(build_cfg.size() == EXPECTED_ARGS) {
            break;
        }
    }

    if(build_cfg.size() != EXPECTED_ARGS) {
        cerr << "failed to parse build.cfg file: " << ss.str() << endl;
        exit(1);
    }
}

int main(int argc, char** argv)
{
    CommandLineParser parser(argc, argv);
    parser.parse_args(true);
    auto prefix = parser.get_prefix();
    auto config = parser.get_config();

    replace(config.begin(), config.end(), '\\', DIR_SEP);
    replace(prefix.begin(), prefix.end(), '\\', DIR_SEP);
    trim(prefix, true, " \t\\/");

    parse_build_cfg(prefix, config);
    // populate the lib map

    string version_num = build_cfg["WXVER_MAJOR"] + build_cfg["WXVER_MINOR"];
    string cxxflags = build_cfg["CXXFLAGS"];
    string compiler = build_cfg["COMPILER"];
    string build = build_cfg["BUILD"];

    const auto& libs = parser.get_libs();
    for(const auto& lib : libs) {
        if(lib == "base") {
            libs_map.insert({ lib, "wx" + lib + version_num + "u" });
        } else if(lib == "net" || lib == "xml") {
            libs_map.insert({ lib, "wxbase" + version_num + "u" + "_" + lib });
        } else {
            libs_map.insert({ lib, "wxmsw" + version_num + "u" + "_" + lib });
        }
    }

    stringstream ss;
    if(parser.is_cxxflags_set()) {
        // print compile flags
        ss << "-I" << prefix << DIR_SEP << "lib" << DIR_SEP << config << " ";
        ss << "-I" << prefix << DIR_SEP << "include"
           << " ";

        // clang does not know `-mthreads`
        if(compiler != "clang") {
            ss << "-mthreads ";
        }

        ss << "-D_FILE_OFFSET_BITS=64 ";
        ss << "-DWXUSINGDLL ";
        ss << "-D__WXMSW__ ";
        ss << "-DHAVE_W32API_H ";
        ss << "-D_UNICODE ";
        ss << "-fmessage-length=0 ";
        ss << "-pipe ";
        if(!cxxflags.empty()) {
            ss << cxxflags << " ";
        }
        if(build == "release") {
            ss << "-DwxDEBUG_LEVEL=0 ";
        }

    } else if(parser.is_rcflags_set()) {
        // print resource compiler flags
        ss << "--include-dir " << prefix << DIR_SEP << "lib" << DIR_SEP << config << " ";
        ss << "--include-dir " << prefix << DIR_SEP << "include"
           << " ";
        ss << "--define __WXMSW__ ";
        ss << "--define _UNICODE ";
        ss << "--define WXUSINGDLL ";
    } else {
        // print linker flags
        const auto& libs = parser.get_libs();

        ss << "-L" << prefix << DIR_SEP << "lib" << DIR_SEP << before_first(config, DIR_SEP_STR) << " "
           << "-pipe -Wl,--subsystem,windows -mwindows ";

        // translate lib name to file name
        for(const auto& lib : libs) {
            if(libs_map.contains(lib)) {
                ss << "-l" << libs_map[lib] << " ";
            }
        }
    }
    cout << ss.str() << endl;
    return 0;
}

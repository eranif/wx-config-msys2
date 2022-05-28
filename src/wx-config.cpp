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

/**
 * @brief are we using monolithic build of wxWidgets?
 */
bool is_monolithic() { return build_cfg.count("MONOLITHIC") == 1 && build_cfg["MONOLITHIC"] == "1"; }
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
    unordered_set<string> keys = { "WXVER_MAJOR", "WXVER_MINOR", "WXVER_RELEASE", "CXXFLAGS", "CXXFLAGS", "BUILD",
        "MONOLITHIC", "VENDOR", "COMPILER" };

    string line;
    while(getline(infile, line) && !keys.empty()) {
        trim(line);
        string key = before_first(line, "=");
        string value = after_first(line, "=");

        if(keys.count(key)) {
            keys.erase(key);
            build_cfg.insert({ key, value });
        }
    }

    if(!keys.empty()) {
        // not all keys were processed
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
    string vendor = build_cfg["VENDOR"];

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
        ss << "-L" << prefix << DIR_SEP << "lib" << DIR_SEP << before_first(config, DIR_SEP_STR) << " "
           << "-pipe ";

        if(is_monolithic()) {
            // monolithic lib
            stringstream libname;
            // example: libwxmsw31u.a
            libname << "wxmsw" << build_cfg["WXVER_MAJOR"] << build_cfg["WXVER_MINOR"] << "u";
            ss << "-l" << libname.str() << " ";
        } else {
            // translate lib name to file name
            const auto& libs = parser.get_libs();
            for(const auto& lib : libs) {
                if(libs_map.count(lib)) {
                    ss << "-l" << libs_map[lib] << " ";
                }
            }
        }
    }
    cout << ss.str() << endl;
    return 0;
}

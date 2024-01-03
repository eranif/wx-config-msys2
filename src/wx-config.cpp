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

/// Add include path to the output
void add_include_dir(const CommandLineParser& parser, stringstream& ss, const string& path)
{
    if(parser.is_create_cmake_file()) {
        ss << "include_directories(" << path << ")\n";
    } else if(parser.is_cxxflags_set()) {
        ss << "-I" << path << " ";
    }
}

/// Add compiler definition to the output (can be -D or some other flag like -fPIC)
void add_macros(const CommandLineParser& parser, stringstream& ss)
{
    string compiler = build_cfg["COMPILER"];
    vector<string> macros;
    if(compiler != "clang") {
        macros.push_back("-mthreads");
    } else {
        // when compiled with clang, wxWidgets generates tones of these errors
        // lets disable them...
        macros.push_back("-Wno-ignored-attributes");
        macros.push_back("-Wno-unknown-pragmas");
        macros.push_back("-Wno-unused-private-field");
    }

    macros.push_back("-D_FILE_OFFSET_BITS=64");
    macros.push_back("-DWXUSINGDLL");
    macros.push_back("-D__WXMSW__");
    macros.push_back("-DHAVE_W32API_H");
    macros.push_back("-D_UNICODE");
    macros.push_back("-fmessage-length=0");
    macros.push_back("-pipe");

    if(build_cfg["BUILD"] == "release") {
        macros.push_back("-DwxDEBUG_LEVEL=0");
    }

    for(const auto& def : macros) {
        if(parser.is_create_cmake_file()) {
            ss << "add_definitions(" << def << ")\n";
        } else if(parser.is_cxxflags_set()) {
            ss << def << " ";
        }
    }
}

/// Build the libs string
void add_libs(const CommandLineParser& parser, const string& config, const string& prefix, stringstream& out_stream)
{
    string build = build_cfg["BUILD"];
    string debug_lib_suffix;
    if(build == "debug")
        debug_lib_suffix = "d";

    // for non monolithic libs, the release lib name could be: wxmsw32u_base or wxmsw32u_xml
    // the debug lib name could be: wxmsw32ud_base or wxmsw32ud_xml
    string unicode_suffix = "u";
    unicode_suffix += debug_lib_suffix;

    stringstream ss;

    // print linker flags
    ss << "-L" << prefix << DIR_SEP << "lib" << DIR_SEP << before_first(config, DIR_SEP_STR) << " "
       << "-pipe ";

    if(is_monolithic()) {
        // monolithic lib
        stringstream libname;
        // example: libwxmsw31u.a or libwxmsw31ud.a
        libname << "wxmsw" << build_cfg["WXVER_MAJOR"] << build_cfg["WXVER_MINOR"] << unicode_suffix;
        ss << "-l" << libname.str() << " ";
        // in the monolithic mode, there are usually two lib files, the common is libwxmsw32u.a, the other file is libwxmsw32u_gl.a
        // this means the wx's opengl support library is always a seperate library, so check to see whether the "gl" option is added
        // finally, we got the linker option line such as: "-lwxmsw32u -lwxmsw32u_gl"
        const auto& libs = parser.get_libs();
        for(const auto& lib : libs) {
            if(lib == "gl") {
                ss << "-l" << libname.str() << "_gl" << " ";
            } 
        }
    } else {
        // translate lib name to file name
        const auto& libs = parser.get_libs();
        for(const auto& lib : libs) {
            if(libs_map.count(lib)) {
                ss << "-l" << libs_map[lib] << " ";
            }
        }
    }

    if(parser.is_create_cmake_file()) {
        string libs = ss.str();
        trim(libs);

        out_stream << "set(wxWidgets_LIBRARIES \"" << libs << "\")\n";
    } else {
        out_stream << ss.str();
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

    string debug_lib_suffix;
    if(build == "debug")
        debug_lib_suffix = "d";

    // for non monolithic libs, the release lib name could be: wxmsw32u_base or wxmsw32u_xml
    // the debug lib name could be: wxmsw32ud_base or wxmsw32ud_xml
    string unicode_suffix = "u";
    unicode_suffix += debug_lib_suffix;

    const auto& libs = parser.get_libs();
    for(const auto& lib : libs) {
        if(lib == "base") {
            libs_map.insert({ lib, "wx" + lib + version_num + unicode_suffix });
        } else if(lib == "net" || lib == "xml") {
            libs_map.insert({ lib, "wxbase" + version_num + unicode_suffix + "_" + lib });
        } else {
            libs_map.insert({ lib, "wxmsw" + version_num + unicode_suffix + "_" + lib });
        }
    }

    stringstream ss;
    if(parser.is_create_cmake_file()) {
        // When --cmake is passed, we generate a wxWidgets.cmake file to be included
        // in the user CMakeLists
        ss << "## Auto Generated by wx-config: https://github.com/eranif/wx-config-msys2\n";
        ss << "## Include this file in your CMakeLists.txt:\n";
        ss << "## include(wxWidgets.cmake)\n";
        ss << "## ..\n";
        ss << "## And add this variable to link againt wxWidgets libraries:\n";
        ss << "## target_link_libraries(... ${wxWidgets_LIBRARIES}\n\n";

        add_include_dir(parser, ss, prefix + DIR_SEP + "lib" + DIR_SEP + config);
        add_include_dir(parser, ss, prefix + DIR_SEP + "include");
        add_macros(parser, ss);
        add_libs(parser, config, prefix, ss);

        ofstream out_file("wxWidgets.cmake", ios_base::out | ios_base::trunc);
        if(!out_file.good()) {
            cerr << "failed to open file wxWidgets.cmake for write" << endl;
            exit(1);
        }

        out_file << ss.str() << endl;
        out_file.close();
        std::filesystem::path cwd = std::filesystem::current_path() / "wxWidgets.cmake";
        string path = cwd.string();
        std::replace(path.begin(), path.end(), '\\', '/');
        cout << path << endl;

    } else {
        if(parser.is_cxxflags_set()) {
            add_include_dir(parser, ss, prefix + DIR_SEP + "lib" + DIR_SEP + config);
            add_include_dir(parser, ss, prefix + DIR_SEP + "include");
            add_macros(parser, ss);

            if(!cxxflags.empty()) {
                ss << cxxflags << " ";
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
            add_libs(parser, config, prefix, ss);
        }
        cout << ss.str() << endl;
    }
    return 0;
}

#include "utils.hpp"

string after_first(const string& str, const string& needle)
{
    auto where = str.find(needle);
    if(where == string::npos) {
        return "";
    }
    return str.substr(where + needle.length());
}

string before_first(const string& str, const string& needle)
{
    auto where = str.find(needle);
    if(where == string::npos) {
        return "";
    }
    return str.substr(0, where);
}

void trim(string& str, bool from_right, const string& trim_chars)
{
    if(from_right) {
        // trim from right
        str.erase(str.find_last_not_of(trim_chars) + 1);
    } else {
        // trim from left
        str.erase(0, str.find_first_not_of(trim_chars));
    }
}

string safe_getenv(const string& name)
{
    char* e = ::getenv(name.c_str());
    if(!e) {
        return "";
    }
    return e;
}

/// CommandLineParser
void CommandLineParser::parse_args(bool require_wxcfg)
{
    reset();
    for(int i = 1; i < m_argc; ++i) {
        string arg = m_argv[i];
        if(arg.starts_with("--prefix")) {
            set_prefix(after_first(arg, "="));
            if(m_prefix.empty()) {
                print_usage();
                exit(1);
            }
        } else if(arg.starts_with("--wxcfg")) {
            set_config(after_first(arg, "="));
            if(m_config.empty()) {
                print_usage();
                exit(2);
            }
        } else if(arg.starts_with("--libs")) {
            if((i + 1) < m_argc) {
                string arg = m_argv[i + 1];
                // ignore the next arg if it starts with `--`
                if(!arg.starts_with("--")) {
                    parse_libs(arg);
                    ++i;
                } else {
                    parse_libs("std");
                }
            } else {
                parse_libs("std");
            }

        } else if(arg.starts_with("--cflags") || arg.starts_with("--cxxflags")) {
            set_is_cxxflags();
        } else if(arg.starts_with("--rcflags")) {
            set_is_rcflags();
        } else if(arg.starts_with("--debug")) {
            set_is_debug();
        } else if(arg.starts_with("--cmake")) {
            set_is_cmake();
        }
    }

    if(m_prefix.empty()) {
        m_prefix = safe_getenv("WXWIN");
        if(m_prefix.empty()) {
            cerr << "Missing prefix. Please use environment variable WXWIN or --prefix=..." << endl;
            print_usage();
            exit(3);
        }
    }
    if(require_wxcfg && m_config.empty()) {
        m_config = safe_getenv("WXCFG");
        if(m_config.empty()) {
            cerr << "Missing config. Please use environment variable WXCFG or --wxcfg=..." << endl;
            print_usage();
            exit(4);
        }
    }
}

void CommandLineParser::print_usage()
{
    cout << "Print wxWidgets build & link flags for Windows / MinGW installed using MSYS2" << endl << endl;
    cout << "Please set WXWIN to point to the installation location of wxWidgets" << endl;
    cout << "set WXWIN=C:/msys64/mingw64" << endl;
    cout << "set WXCFG=clang_x64_dll/mswu" << endl;
    cout << "Optionally, you can use the --prefix and --wxcfg switchs: --prefix=C:/msys64/mingw64 "
            "--wxcfg=clang_x64_dll/mswu"
         << endl;
    cout << "wx-config [--prefix=<install_dir>] [--wxcfg=config-dir] [--libs|--cflags|--cxxflags|--rcflags "
            "[...]] [--debug]"
         << endl;
    cout << "wx-config --cmake [--prefix=<install_dir>] [--wxcfg=config-dir]" << endl;
    cout << "Example usage:" << endl;
    cout << endl;
    cout << "To print the default list of link flags + libraries:" << endl;
    cout << "  wx-config --libs" << endl;
    cout << endl;
    cout << "To print a specific list of libraries + link flags:" << endl;
    cout << "  wx-config --libs=std,aui" << endl;
    cout << endl;
    cout << "To print compiler flags:" << endl;
    cout << "  wx-config --cflags" << endl;
    cout << endl;
}

void CommandLineParser::parse_libs(const string& libs)
{
    auto vlibs = split_by_comma(libs);
    if(vlibs.empty()) {
        vlibs.push_back("std");
    }

    // --libs std
    // -lwx_mswu_xrc-3.0 -lwx_mswu_webview-3.0 -lwx_mswu_html-3.0 -lwx_mswu_qa-3.0 -lwx_mswu_adv-3.0 -lwx_mswu_core-3.0
    // -lwx_baseu_xml-3.0 -lwx_baseu_net-3.0 -lwx_baseu-3.0
    auto default_libs = { "xrc", "webview", "html", "qa", "adv", "core", "xml", "net", "base" };

    // --libs all
    // -lwx_mswu_xrc-3.0 -lwx_mswu_webview-3.0 -lwx_mswu_stc-3.0 -lwx_mswu_richtext-3.0 -lwx_mswu_ribbon-3.0
    // -lwx_mswu_propgrid-3.0 -lwx_mswu_aui-3.0 -lwx_mswu_gl-3.0 -lwx_mswu_html-3.0 -lwx_mswu_qa-3.0 -lwx_mswu_adv-3.0
    // -lwx_mswu_core-3.0 -lwx_baseu_xml-3.0 -lwx_baseu_net-3.0 -lwx_baseu-3.0
    auto all_libs = {
        "xrc",
        "webview",
        "stc",
        "richtext",
        "ribbon",
        "propgrid",
        "aui",
        "gl",
        "html",
        "qa",
        "adv",
        "core",
        "xml",
        "net",
        "base",
        "media",
    };

    // construct a map from the "all" list
    unordered_set<string> S { all_libs.begin(), all_libs.end() };

    // always make sure that "base" is included
    auto has_base = find_if(
        vlibs.begin(), vlibs.end(), [](const string& lib) { return lib == "base" || lib == "all" || lib == "std"; });
    if(has_base == vlibs.end()) {
        vlibs.push_back("base");
    }

    for(const auto& lib : vlibs) {
        // handle special cases
        if(lib == "std") {
            m_libs.insert(m_libs.end(), default_libs.begin(), default_libs.end());
        } else if(lib == "all") {
            m_libs.reserve(all_libs.size());
            m_libs.insert(m_libs.end(), all_libs.begin(), all_libs.end());
        } else {
            // search by name
            if(S.contains(lib)) {
                m_libs.emplace_back(lib);
            }
        }
    }
}

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
    for(int i = 0; i < m_argc; ++i) {
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
            parse_libs(after_first(arg, "="));
        } else if(arg.starts_with("--cflags") || arg.starts_with("--cxxflags")) {
            set_is_cxxflags();
        } else if(arg.starts_with("--rcflags")) {
            set_is_rcflags();
        } else if(arg.starts_with("--debug")) {
            set_is_debug();
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

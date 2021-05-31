#include <filesystem>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

namespace
{
string after_first(const string& str, const string& needle)
{
    auto where = str.find(needle);
    if(where == string::npos) {
        return "";
    }
    return str.substr(where + needle.length());
}

vector<pair<string, string>> all_libs = {
    { "xrc", "wx_mswu_xrc" },
    { "webview", "wx_mswu_webview" },
    { "stc", "wx_mswu_stc" },
    { "richtext", "wx_mswu_richtext" },
    { "ribbon", "wx_mswu_ribbon" },
    { "propgrid", "wx_mswu_propgrid" },
    { "aui", "wx_mswu_aui" },
    { "gl", "wx_mswu_gl" },
    { "html", "wx_mswu_html" },
    { "qa", "wx_mswu_qa" },
    { "core", "wx_mswu_core" },
    { "xml", "wx_baseu_xml" },
    { "net", "wx_baseu_net" },
    { "base", "wx_baseu" },
};

unordered_map<string, string> libs_map;

string safe_getenv(const string& name)
{
    char* e = ::getenv(name.c_str());
    if(!e) {
        return "";
    }
    return e;
}

string find_wx_version(const string& install_dir)
{
    // check if the user provided us with a version to use
    string user_ver = safe_getenv("WXVER");
    if(!user_ver.empty()) {
        return user_ver;
    }

    regex re("libwx_baseu\\-([\\d]+)[\\.]{1}([\\d]+)");
    size_t cur_weight = 0;
    string major, minor;
    for(const auto& entry : filesystem::directory_iterator(install_dir)) {
        auto path = entry.path().string();
        if(path.find("libwx_baseu-") != -1) {
            smatch m;
            if(regex_search(path, m, re)) {
                string tmp_major, tmp_minor;
                tmp_major = m[1];
                tmp_minor = m[2];
                size_t weight = (atoi(tmp_major.c_str()) * 100) + (atoi(tmp_minor.c_str()) * 10);
                if(weight > cur_weight) {
                    major.swap(tmp_major);
                    minor.swap(tmp_minor);
                    cur_weight = weight;
                }
            }
        }
    }

    if(cur_weight == 0) {
        cerr << "Could not determine wxWidgets version installed. Please set WXVER or ensure that the --prefix "
                "provided is correct"
             << endl;
        exit(3);
    }
    return major + "." + minor;
}
}

class CommandLineParser
{
protected:
    int& m_argc;
    char** m_argv;

    vector<string> m_libs;
    string m_mode;   // --libs | --cflags
    string m_prefix; // --prefix or the value from WXWIN
    size_t m_flags = 0;

protected:
    enum eFlags {
        kIsRcFlags = (1 << 0),
        kIsCxxFlags = (1 << 1),
        kIsDebug = (1 << 2),
    };

protected:
    void reset()
    {
        m_libs.clear();
        m_mode.clear();
        m_prefix.clear();
        m_flags = 0;
    }

    void set_prefix(const string& prefix) { m_prefix = prefix; }

    /**
     * @brief parse the libs and populate the m_libs member
     */
    void parse_libs(const string& libs)
    {
        auto vlibs = split_by_comma(libs);
        if(vlibs.empty()) {
            vlibs.push_back("std");
        }

        auto default_libs = { "xrc", "html", "qa", "core", "xml", "net", "base" };

        // always make sure that "base" is included
        auto has_base = find_if(vlibs.begin(), vlibs.end(),
            [](const string& lib) { return lib == "base" || lib == "all" || lib == "std"; });
        if(has_base == vlibs.end()) {
            vlibs.push_back("base");
        }

        for(const auto& lib : vlibs) {
            // handle special cases
            if(lib == "std") {
                m_libs.insert(m_libs.end(), default_libs.begin(), default_libs.end());
            } else if(lib == "all") {
                m_libs.reserve(m_libs.size() + all_libs.size());
                for(const auto& p : all_libs) {
                    m_libs.push_back(p.first);
                }
            } else {
                // search by name
                if(libs_map.contains(lib)) {
                    m_libs.push_back(lib);
                }
            }
        }
    }

    void set_is_cxxflags() { m_flags |= kIsCxxFlags; }
    void set_is_rcflags() { m_flags |= kIsRcFlags; }
    void set_is_debug() { m_flags |= kIsDebug; }

    /**
     * @brief split input string by command and return vector of the results
     */
    vector<string> split_by_comma(const string& str) const
    {
        vector<string> result;
        string token;
        istringstream iss(str);
        while(getline(iss, token, ',')) {
            token.erase(token.find_last_not_of(" \n\r\t") + 1);
            if(token.empty()) {
                continue;
            }
            result.push_back(token);
        }
        return result;
    }

public:
    CommandLineParser(int& argc, char** argv)
        : m_argc(argc)
        , m_argv(argv)
    {
    }

    void parse_args()
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
                exit(2);
            }
        }
    }

    void print_usage()
    {
        cout << "Print wxWidgets build & link flags for Windows / MinGW installed using MSYS2" << endl << endl;
        cout << "Please set WXWIN to point to the installation location of wxWidgets" << endl;
        cout << "set WXWIN=C:\\msys64\\mingw64" << endl;
        cout << "Optionally, you can use the --prefix switch: --prefix=C:\\msys64\\mingw64" << endl;
        cout << "wx-config-msys2 [--prefix=<install_dir>] [--libs|--cflags|--cxxflags|--rcflags [...]] [--debug]"
             << endl;
        cout << "Example usage:" << endl;
        cout << endl;
        cout << "To print the default list of link flags + libraries:" << endl;
        cout << "  wx-config-msys2 --libs" << endl;
        cout << endl;
        cout << "To print a specific list of libraries + link flags:" << endl;
        cout << "  wx-config-msys2 --libs=std,aui" << endl;
        cout << endl;
        cout << "To print compiler flags:" << endl;
        cout << "  wx-config-msys2 --cflags" << endl;
        cout << endl;
    }

    const auto& get_libs() const { return m_libs; }
    const auto& get_prefix() const { return m_prefix; }
    bool is_rcflags_set() const { return m_flags & kIsRcFlags; }
    bool is_cxxflags_set() const { return m_flags & kIsCxxFlags; }
    bool is_debug() const { return m_flags & kIsDebug; }
};

int main(int argc, char** argv)
{
    CommandLineParser parser(argc, argv);
    parser.parse_args();
    const auto& prefix = parser.get_prefix();

    // ----------------------------------------
    // append the wx version to all the libs
    // ----------------------------------------
    string wx_ver = find_wx_version(prefix + "\\lib");
    wx_ver.insert(0, "-");
    for(auto& p : all_libs) {
        p.second.append(wx_ver);
        libs_map.insert(p);
    }

    // Now that we updated libs_map, re-parse the data
    parser.parse_args();

    stringstream ss;
    if(parser.is_cxxflags_set()) {
        // print compile flags
        ss << "-I" << prefix << "\\lib\\wx\\include\\msw-unicode" << wx_ver << " ";
        ss << "-I" << prefix << "\\include\\wx" << wx_ver << " ";
        ss << "-mthreads ";
        ss << "-D_FILE_OFFSET_BITS=64 ";
        ss << "-DWXUSINGDLL ";
        ss << "-D__WXMSW__ ";
        ss << "-DHAVE_W32API_H ";
        ss << "-D_UNICODE ";
        ss << "-fmessage-length=0 ";
        ss << "-pipe ";
        if(parser.is_debug()) {
            ss << "-g -O0";
        } else {
            ss << "-O2 ";
            ss << "-DNDEBUG ";
        }

    } else if(parser.is_rcflags_set()) {
        // print resource compiler flags
        ss << "--include-dir " << prefix << "\\lib\\wx\\include\\msw-unicode" << wx_ver << " ";
        ss << "--include-dir " << prefix << "\\include\\wx" << wx_ver << " ";
        ss << "--define __WXMSW__ ";
        ss << "--define _UNICODE ";
        ss << "--define WXUSINGDLL ";
    } else {
        // print linker flags
        const auto& libs = parser.get_libs();

        ss << "-L" << prefix << "\\lib -pipe -Wl,--subsystem,windows -mwindows ";
        for(const auto& lib : libs) {
            ss << "-l" << libs_map[lib] << " ";
        }
    }
    cout << ss.str() << endl;
    return 0;
}

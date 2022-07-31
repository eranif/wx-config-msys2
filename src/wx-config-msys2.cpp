#include "utils.hpp"
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
    { "adv", "wx_mswu_adv" },
};

unordered_map<string, string> libs_map;

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
    if(!filesystem::is_directory(install_dir)) {
        cerr << "Directory: " << install_dir << " does not exist" << endl;
        cerr << "Could not determine wxWidgets version installed. Please set WXVER or ensure that the --prefix "
                "provided is correct"
             << endl;
        exit(4);
    }

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

int main(int argc, char** argv)
{
    CommandLineParser parser(argc, argv);
    parser.parse_args();
    auto prefix = parser.get_prefix();
    trim(prefix, true, " \t\\/");
    replace(prefix.begin(), prefix.end(), '\\', DIR_SEP);

    // ----------------------------------------
    // append the wx version to all the libs
    // ----------------------------------------
    string wx_ver = find_wx_version(prefix + "/lib");
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
        ss << "-I" << prefix << "/lib/wx/include/msw-unicode" << wx_ver << " ";
        ss << "-I" << prefix << "/include/wx" << wx_ver << " ";
        ss << "-D_FILE_OFFSET_BITS=64 ";
        ss << "-DWXUSINGDLL ";
        ss << "-D__WXMSW__ ";
        ss << "-DHAVE_W32API_H ";
        ss << "-D_UNICODE ";
        ss << "-fmessage-length=0 ";
        ss << "-pipe ";

    } else if(parser.is_rcflags_set()) {
        // print resource compiler flags
        ss << "--include-dir " << prefix << "/lib/wx/include/msw-unicode" << wx_ver << " ";
        ss << "--include-dir " << prefix << "/include/wx" << wx_ver << " ";
        ss << "--define __WXMSW__ ";
        ss << "--define _UNICODE ";
        ss << "--define WXUSINGDLL ";
    } else {
        // print linker flags
        const auto& libs = parser.get_libs();

        ss << "-L" << prefix << "/lib -pipe ";
        for(const auto& lib : libs) {
            ss << "-l" << libs_map[lib] << " ";
        }
    }
    cout << ss.str() << endl;
    return 0;
}

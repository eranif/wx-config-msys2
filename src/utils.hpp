#ifndef UTILS_HPP
#define UTILS_HPP

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

string after_first(const string& str, const string& needle);
string before_first(const string& str, const string& needle);
void trim(string& str, bool from_right = true, const string& trim_chars = "\r\n\t\v ");
string safe_getenv(const string& name);

#define DIR_SEP '/'
#define DIR_SEP_STR "/"

class CommandLineParser
{
protected:
    int& m_argc;
    char** m_argv;

    vector<string> m_libs;
    string m_mode;   // --libs | --cflags
    string m_prefix; // --prefix or the value from WXWIN
    string m_config; // --config or the value read from WXCFG
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
    void set_config(const string& cfg) { m_config = cfg; }

    /**
     * @brief parse the libs and populate the m_libs member
     */
    void parse_libs(const string& libs);
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

    void parse_args(bool require_wxcfg = false);
    void print_usage();

    const auto& get_libs() const { return m_libs; }
    const auto& get_prefix() const { return m_prefix; }
    const auto& get_config() const { return m_config; }
    bool is_rcflags_set() const { return m_flags & kIsRcFlags; }
    bool is_cxxflags_set() const { return m_flags & kIsCxxFlags; }
    bool is_debug() const { return m_flags & kIsDebug; }
};

#endif // UTILS_HPP

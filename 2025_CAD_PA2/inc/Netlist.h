#include "CellLib.h"

// ===================== Netlist =====================
class Netlist{
public:
    struct Gate{
        string type, name; 
        unordered_map<string, string> pins; 
    };

    string module_name;
    vector<string> PIs, POs;
    vector<Gate> gates;
    unordered_map<string, string> outpin;
    unordered_map<string, vector<string>> inpins;

    static string strip_comments(const string &s);      // remove /* ... */ or // (like CellLib)

    static void skip_ws(const string& s, size_t& i);    // skip white space, tabs, new lines (like CellLib)
    static string read_ident(const string& s, size_t& i);   // skip white space and read an identifier
    static void skip_to_semi(const string& s, size_t& i);   // skip to next ';'

    static void read_name_list(const string& s, size_t &i, vector<string>& out);    // read a list of names separated by ','

    bool parse(const string &path, const CellLib &lib);    // parse netlist from file
};
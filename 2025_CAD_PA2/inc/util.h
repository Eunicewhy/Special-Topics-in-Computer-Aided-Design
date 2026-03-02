#include <bits/stdc++.h>
using namespace std;

// ===================== helpers =====================
namespace util{
    // remove leading/trailing whitespaces, tabs, newlines
    static inline string trim(const string &s){
        size_t a = s.find_first_not_of(" \t\r\n");
        if(a == string::npos) return "";
        size_t b = s.find_last_not_of(" \t\r\n");
        return s.substr(a, b-a+1);
    }
    // split by whitespace, comma, tab
    static inline vector<string> split_ws(const string &s){
        vector<string> out; 
        string cur;
        for(size_t i = 0; i < s.size(); ++i){
            char c = s[i];
            if(c == ' ' || c ==',' || c =='\t'){
                if(!cur.empty()){
                    out.push_back(cur);
                    cur.clear();
                }
            }
            else cur.push_back(c);
        }
        if(!cur.empty()) out.push_back(cur);
        return out;
    }
    // convert to lower case
    static inline string lower(string s){
        for(size_t i = 0; i < s.size(); ++i)
            s[i] = (char)tolower((unsigned char)s[i]);
        return s;
    }
    // get basename without extension
    static inline string basename_wo_ext(string path){
        size_t p = path.find_last_of("/\\");    // get last '/' or '\'
        string f = (p == string::npos)? path : path.substr(p+1);    // get filename
        size_t d = f.find_last_of('.');   // remove extension
        if(d != string::npos) f = f.substr(0, d);
        return f;
    }
    // extract trailing number from a string
    static inline int inst_num(const string &name){
        int n = 0; 
        bool has = false;
        for(int i = (int)name.size()-1; i >= 0; --i){ // scan backwards
            // if digit, mark has = true
            if(isdigit((unsigned char)name[i])) 
                has = true;
            else {
                // if not digit and has digit before, extract number
                if(has){ 
                    n = stoi(name.substr((size_t)i+1)); 
                    return n; 
                } 
                else break; 
            }
        }
        return INT_MAX/2;
    }
}
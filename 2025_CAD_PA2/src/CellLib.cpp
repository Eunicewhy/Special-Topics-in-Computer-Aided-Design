// 1. 尚未做多個 library
#include "../inc/CellLib.h"


// --- added helper: parse_number_list (extract doubles from a quoted list) ---
static inline vector<double> parse_number_list(const string& in){
    vector<double> out; out.reserve(64);
    string t;
    for(size_t i=0;i<in.size();++i){
        char c=in[i];
        if((c>='0'&&c<='9')||c=='.'||c=='-'||c=='e'||c=='E') t.push_back(c);
        else{ if(!t.empty()){ out.push_back(strtod(t.c_str(), nullptr)); t.clear(); } }
    }
    if(!t.empty()) out.push_back(strtod(t.c_str(), nullptr));
    return out;
}

// --- added method: member version that uses template indices ---
void CellLib::fill_table_from_block(const string &tblk, const string &name, Table2D &out){
    // pattern: e.g., cell_rise ( table10 ) { ... }
    string pat = name + string(R"(\s*\(\s*([^\)\s]+)\s*\)\s*\{)");
    regex rtb(pat);
    smatch mm;
    if(regex_search(tblk, mm, rtb)){
        string tpl = util::trim(mm[1].str());
        size_t bs = (size_t)mm.position(0);
        int d = 0; 
        size_t be = bs;
        for(size_t i = bs; i < tblk.size(); ++i){
            if(tblk[i] == '{') d++;
            else if(tblk[i] == '}'){ 
                d--;
                if(d == 0){
                    be=i; 
                    break; 
                } 
            } 
        }
        string b = tblk.substr(bs, be-bs+1);

        // choose indices by template; fallback to top-level idx1/idx2 if not found
        const vector<double>* p1 = &idx1;
        const vector<double>* p2 = &idx2;
        unordered_map<string,TableTemplate>::const_iterator it = templates.find(tpl);
        if(it != templates.end()){
            if(!it->second.idx1.empty()) p1 = &it->second.idx1;
            if(!it->second.idx2.empty()) p2 = &it->second.idx2;
        }

        out.idx1 = *p1;
        out.idx2 = *p2;
        out.v = parse_value_matrix(b, (int)out.idx1.size(), (int)out.idx2.size());
    }
}

const CellLib::Cell* CellLib::getCell(const string &t) const{
    unordered_map<string, Cell>::const_iterator it = cells.find(t);
    if(it != cells.end()) return &it->second;
    // ? find lowercase name
    for(unordered_map<string,Cell>::const_iterator it2 = cells.begin(); it2 != cells.end(); ++it2){
        if(util::lower(it2->first) == util::lower(t)) 
            return &it2->second;
    }
    return NULL;
}

string CellLib::strip_comments(const string &s){
    string o; 
    o.reserve(s.size());
    bool sl = false, ml = false;    // sl: single line, ml: multi line
    for(size_t i = 0; i < s.size(); ++i){
        // check start of single line comment (//)
        if(!ml && !sl && i+1 < s.size() && s[i]=='/' && s[i+1]=='/') { 
            sl = true; 
            ++i; 
            continue; 
        }
        // check start of multi line comment (/*)
        if(!ml && !sl && i+1 < s.size() && s[i]=='/' && s[i+1]=='*') { 
            ml = true; 
            ++i; 
            continue; 
        }
        // check end of single line comment (\n)
        if(sl && s[i] == '\n'){ 
            sl = false; 
            o.push_back(s[i]); 
            continue; 
        }
        // check end of multi line comment (*/)
        if(ml && i+1 < s.size() && s[i] == '*' && s[i+1] == '/'){ 
            ml = false; 
            ++i; 
            continue; 
        }
        if(!ml && !sl) o.push_back(s[i]);
    }
    return o;
}

vector<vector<double>> CellLib::parse_value_matrix(const string &blk, int cols, int rows){
    vector<double> vals;
    vals.reserve(cols * rows);
    static const regex r(R"(values?\s*\(\s*([^;]*?)\s*\)\s*;)");    // match values(...) ;
    smatch m;    // match result
    if(!regex_search(blk, m, r)) return vector<vector<double>>();   // not found
    string in = m[1].str();
    string c;
    c.reserve(in.size());
    // remove '"' and '\'
    for(size_t i = 0; i < in.size(); ++i){ 
        char ch = in[i]; 
        if(ch != '"' && ch != '\\') c.push_back(ch); 
    }
    // tokenize numbers
    string tok;
    for(size_t i = 0; i < c.size(); ++i){
        char ch = c[i];
        if((ch >= '0' && ch <= '9') || ch == '.' || ch == '-' || ch== 'e' || ch== 'E') 
            tok.push_back(ch);
        else{ 
            if(!tok.empty()){ 
                vals.push_back(stod(tok)); 
                tok.clear(); 
            } 
        }
    }
    if(!tok.empty()) vals.push_back(stod(tok));
    vector<vector<double>> M(rows, vector<double>(cols, 0.0));
    if(cols == 0) return vector<vector<double>>();
    // adjust rows if not enough values
    if((int)vals.size() < cols * rows){ 
        rows = (int)vals.size() / cols; 
        M.assign(rows, vector<double>(cols, 0.0)); 
    }
    // fill matrix(M)
    int k = 0;
    for(int r = 0; r < rows && k < (int)vals.size(); ++r){
        for(int c2 = 0; c2 < cols && k < (int)vals.size(); ++c2){
            M[r][c2] = vals[k++];
        }
    }
    return M;
}

bool CellLib::parse(const string &path){
    ifstream fin(path.c_str()); 
    if(!fin) return false;
    string s((istreambuf_iterator<char>(fin)), istreambuf_iterator<char>());
    s = strip_comments(s);

    
    // === added: capture library(name) ===
    {
        static const regex rlib(R"(library\s*\(\s*([^\)]+)\s*\)\s*\{)");
        smatch mlib;
        if(regex_search(s, mlib, rlib)){
            library_name = util::trim(mlib[1].str());
        }
        else{
            library_name.clear();
        }
    }

    // === added: parse all lu_table_template(name){ index_1("..."); index_2("..."); } ===
    {
        static const regex rtpl(R"(lu_table_template\s*\(\s*([^\)]+)\s*\)\s*\{)");
        string::const_iterator ittpl = s.cbegin();
        smatch mt;
        while(regex_search(ittpl, s.cend(), mt, rtpl)){
            string tname = util::trim(string(mt[1].first, mt[1].second));
            size_t st = (size_t)mt.position(0) + (size_t)(ittpl - s.cbegin());
            int d=0; size_t ed=st;
            for(size_t i = st; i < s.size(); ++i){
                if(s[i] == '{') d++;
                else if(s[i] == '}'){ 
                    d--; 
                    if(d==0){ 
                        ed=i; 
                        break; 
                    } 
                }
            }
            string blk = s.substr(st, ed-st+1);
            ittpl = s.cbegin() + (ptrdiff_t)ed + 1;

            smatch m1,m2;
            static const regex r1(R"(index_1\s*\(\s*\"([^\)]*)\"\s*\)\s*;)");
            static const regex r2(R"(index_2\s*\(\s*\"([^\)]*)\"\s*\)\s*;)");
            TableTemplate TT;
            if(regex_search(blk, m1, r1)) TT.idx1 = parse_number_list(m1[1].str());
            if(regex_search(blk, m2, r2)) TT.idx2 = parse_number_list(m2[1].str());
            if(!TT.idx1.empty() || !TT.idx2.empty())
                templates[tname] = TT;
        }
    }

    // === added: parse top-level index_1/index_2 as fallback ===
    {
        smatch m;
        static const regex r1(R"(index_1\s*\(\s*\"([^\)]*)\"\s*\)\s*;)");
        if(regex_search(s, m, r1)) idx1 = parse_number_list(m[1].str());
        static const regex r2(R"(index_2\s*\(\s*\"([^\)]*)\"\s*\)\s*;)");
        if(regex_search(s, m, r2)) idx2 = parse_number_list(m[1].str());
    }

    // cells
    static const regex rc(R"(cell\s*\(\s*([^\)]+)\s*\)\s*\{)");
    string::const_iterator it = s.cbegin(); 
    smatch sm;
    while(regex_search(it, s.cend(), sm, rc)){
        string cname = util::trim(string(sm[1].first, sm[1].second));       // get cell name (NOR2X1, INVX1, NANDX1, ...)
        size_t st = (size_t)sm.position(0) + (size_t)(it - s.cbegin());
        int d = 0;
        size_t ed = st;
        for(size_t i = st; i < s.size(); ++i){ 
            if(s[i] == '{') d++; 
            else if(s[i] == '}'){ 
                d--; 
                if(d == 0){ 
                    ed = i; 
                    break; 
                } 
            } 
        }
        string blk = s.substr(st, ed-st+1);    // cell block (cell (cell_name){...})
        it = s.cbegin() + (ptrdiff_t)ed + 1;

        Cell c; 
        c.name = cname; 
        c.output_pin.clear();

        // pins
        static const regex rpin(R"(pin\s*\(\s*([^\)]+)\s*\)\s*\{)");
        string::const_iterator itp = blk.cbegin(); 
        smatch mp;
        while(regex_search(itp, blk.cend(), mp, rpin)){
            string pname = util::trim(string(mp[1].first, mp[1].second));    // get pin name
            size_t pst = (size_t)mp.position(0) + (size_t)(itp - blk.cbegin());
            int d2 = 0; 
            size_t ped = pst;
            for(size_t i = pst; i < blk.size(); ++i){ 
                if(blk[i] == '{') d2++; 
                else if(blk[i]=='}'){ 
                    d2--; 
                    if(d2 == 0){ 
                        ped = i; 
                        break; 
                    } 
                }
            }
            string pblk = blk.substr(pst, ped-pst+1);   // pin block (pin (pin_name){...})
            itp = blk.cbegin() + (ptrdiff_t)ped + 1;

            Pin p; 
            p.name = pname; 
            p.dir = ""; 
            p.cap = 0.0;
            // get pin direction
            static const regex rd(R"(direction\s*:\s*([a-zA-Z]+)\s*;)");
            smatch md; 
            if(regex_search(pblk, md, rd)) p.dir = md[1].str();
            // get pin capacitance
            static const regex rcx(R"(capacitance\s*:\s*([0-9eE\.\-\+]+)\s*;)");
            smatch mc; 
            if(regex_search(pblk, mc, rcx)) p.cap = stod(mc[1].str());
            // store pin
            if(p.dir == "output") c.output_pin = pname;    // store output pin name
            c.pins[pname] = p;
        }

        // timing()
        static const regex rtim(R"(timing\s*\(\s*\)\s*\{)");
        smatch mt;
        if(regex_search(blk, mt, rtim)){
            size_t ts = (size_t)mt.position(0);
            int d3 = 0; 
            size_t te = ts;
            for(size_t i = ts; i < blk.size(); ++i){ 
                if(blk[i] == '{') d3++; 
                else if(blk[i] == '}'){ 
                    d3--; 
                    if(d3 == 0){ 
                        te=i; 
                        break; 
                    } 
                } 
            }
            string tblk = blk.substr(ts, te-ts+1);
            fill_table_from_block(tblk, "cell_rise",       c.cell_rise);
            fill_table_from_block(tblk, "cell_fall",       c.cell_fall);
            fill_table_from_block(tblk, "rise_transition", c.rise_tran);
            fill_table_from_block(tblk, "fall_transition", c.fall_tran);
        }
        cells[c.name] = c;
    }
    return !cells.empty();
}
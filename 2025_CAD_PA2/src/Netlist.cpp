#include "../inc/Netlist.h"

string Netlist::strip_comments(const string &s){
    string o; 
    o.reserve(s.size());
    bool sl = false, ml = false;
    for(size_t i = 0; i < s.size(); ++i){
        if(!ml && !sl && i+1 < s.size() && s[i] == '/' && s[i+1] == '/') { 
            sl = true; 
            ++i; 
            continue; 
        }
        if(!ml && !sl && i+1 < s.size() && s[i] == '/' && s[i+1] == '*') { 
            ml = true;
            ++i;
            continue; 
        }
        if(sl && s[i] == '\n'){ 
            sl = false;
            o.push_back(s[i]);
            continue;
        }
        if(ml && i+1 < s.size() && s[i] == '*' && s[i+1] == '/'){ 
            ml = false; 
            ++i; 
            continue; 
        }
        if(!ml && !sl) o.push_back(s[i]);
    }
    return o;
}

void Netlist::skip_ws(const string& s, size_t& i){ 
    while(i < s.size() && isspace((unsigned char)s[i])) 
        ++i;
}

string Netlist::read_ident(const string& s, size_t& i){
    skip_ws(s,i);
    size_t st = i;
    if(i < s.size() && (isalpha((unsigned char)s[i]) || s[i] == '_' )){   // first char
        ++i; 
        while(i < s.size() && (isalnum((unsigned char)s[i]) || s[i] == '_'))  // rest chars
            ++i;
        return s.substr(st, i-st); 
    }
    return string();
}

void Netlist::skip_to_semi(const string& s, size_t& i){ 
    while(i < s.size() && s[i] != ';') 
        ++i; 
    if(i < s.size())   // skip ';'
        ++i;
}

void Netlist::read_name_list(const string& s, size_t &i, vector<string>& out){
    while(i < s.size()){
        skip_ws(s, i);
        if(i >= s.size()) 
            break;
        if(s[i] == ';'){ 
            ++i; 
            break; 
        }
        if(s[i]=='['){    // skip bus range (option)
            while(i < s.size() && s[i] != ']') 
                ++i; 
            if(i < s.size()) 
                ++i; 
            continue; 
        }
        string id = read_ident(s, i);
        if(id.empty()){ 
            ++i; 
            continue;
        }
        string lid = util::lower(id);
        if(lid == "input" || lid == "output" || lid == "wire" || lid == "reg") 
            continue;
        out.push_back(id);
        skip_ws(s, i);
        if(i < s.size() && s[i] == ','){ 
            ++i; 
            continue; 
        }
    }
}

bool Netlist::parse(const string &path, const CellLib &lib){
    ifstream fin(path.c_str()); 
    if(!fin) 
        return false;
    string s((istreambuf_iterator<char>(fin)), istreambuf_iterator<char>());
    s = strip_comments(s);

    // module name
    size_t pos = s.find("module");
    if(pos != string::npos){ 
        pos += 6; 
        module_name = read_ident(s, pos);
        if(module_name.empty()) 
            module_name = util::basename_wo_ext(path);
    }
    else module_name = util::basename_wo_ext(path);

    size_t i = 0;
    while(i < s.size()){
        skip_ws(s, i); 
        if(i >= s.size()) 
            break;
        if(s.compare(i, 6, "module") == 0){     // skip module declaration
            skip_to_semi(s, i); 
            continue; 
        }
        if(s.compare(i, 3, "end") == 0)     // skip endmodule
            break; 
        if(s.compare(i, 5, "input") == 0){  // read input pins
            i += 5; 
            read_name_list(s, i, PIs); 
            continue; 
        }
        if(s.compare(i, 6, "output") == 0){   // read output pins 
            i += 6;
            read_name_list(s, i, POs); 
            continue; 
        }
        if(s.compare(i, 4, "wire") == 0){   // ? skip wires 
            i += 4; 
            skip_to_semi(s,i); 
            continue; 
        }
        if(s.compare(i, 6, "assign") == 0){     // skip assign
            i += 6; 
            skip_to_semi(s,i); 
            continue; 
        }

        // read gate type (NOR2X1, INVX1, ...)
        size_t save_i = i;
        string t = read_ident(s, i);
        if(t.empty()){ 
            ++i; 
            continue; 
        }
        // read gate name (U8, U10, ...)
        skip_ws(s, i);
        string n = read_ident(s, i);
        if(n.empty()){ 
            i = save_i + 1; 
            continue; 
        }

        // skip to '('
        skip_ws(s,i);
        if(i >= s.size() || s[i] != '('){ 
            i = save_i; 
            skip_to_semi(s,i); 
            continue; 
        }
        ++i; // '('

        Gate g; 
        g.type = t; 
        g.name = n; 
        g.pins.clear();
        while(i < s.size()){
            skip_ws(s, i);
            if(i < s.size() && s[i] == ')'){ 
                ++i; 
                break; 
            }
            if(i < s.size() && s[i] == '.'){
                ++i;
                // read pin name (A1, A2, I, ZN, ...)
                string pin = read_ident(s,i);
                // read net name (N1, N2, n8, n9, ...)
                skip_ws(s, i);
                if(i < s.size() && s[i] == '('){
                    ++i; 
                    skip_ws(s, i);
                    string net = read_ident(s,i);   // get net name
                    while(i < s.size() && s[i] != ')')  // skip to ')'
                        ++i;
                    if(i < s.size() && s[i] == ')') 
                        ++i;
                    g.pins[pin] = net;
                    skip_ws(s, i);
                    if(i < s.size() && s[i] == ',') ++i;
                    continue;
                }
                else{ 
                    skip_to_semi(s, i); 
                    break; 
                }
            }
            else 
                ++i;
        }
        if(i < s.size() && s[i] == ';') 
            ++i;
        if(util::lower(g.type) == "module")     // skip module
            continue;
        gates.push_back(g);
    }

    // pin directions from lib (based on the name from the liberty)
    for(size_t gi = 0; gi < gates.size(); ++gi){
        const Gate &g = gates[gi];
        const CellLib::Cell *c = lib.getCell(g.type);
        string op = c ? c->output_pin : string("ZN");   // if no cell info, assume default output pin is "ZN"
        outpin[g.type] = op;    // store output pin name (e.g., "ZN", "Y", ...)
        vector<string> ins;
        if(c){
            for(unordered_map<string,CellLib::Pin>::const_iterator pk = c->pins.begin(); pk != c->pins.end(); ++pk){ 
                if(pk->second.dir == "input") 
                    ins.push_back(pk->first); 
            }
        }else{
            for(unordered_map<string,string>::const_iterator kv = g.pins.begin(); kv != g.pins.end(); ++kv){ 
                if(kv->first != op) 
                    ins.push_back(kv->first); 
            }
        }
        sort(ins.begin(), ins.end());
        inpins[g.type] = ins;
    }
    return !gates.empty();
}
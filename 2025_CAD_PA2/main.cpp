#include "STA.h"

// ===================== App =====================
class StaApp{
public:
    int run(int argc, char** argv){
        if(argc < 2) return 1;
        string net = argv[1], libp, patp;
        for(int i=2; i < argc; i++){
            string a = argv[i];
            if(a == "-l" && i+1 < argc) 
                libp = argv[++i];
            else if(a == "-i" && i+1 < argc) 
                patp = argv[++i];
        }
        if(libp.empty()) return 2;

        CellLib lib; 
        if(!lib.parse(libp)) return 3;
        string libname = !lib.library_name.empty() ? lib.library_name : util::basename_wo_ext(libp);
        Netlist nl; 
        if(!nl.parse(net, lib)) return 4;

        vector<string> pi_order; 
        vector<vector<int>> patterns; 
        bool have_patterns = false;
        if(!patp.empty()){
            ifstream fin(patp.c_str());
            if(fin){
                string line;
                while(getline(fin, line)){
                    line = util::trim(line);
                    if(line.empty()) continue;
                    if(!line.empty() && line[0] == '.') break;
                    if(line.find("input") == 0){
                        size_t pos = line.find("input"); 
                        string rest = line.substr(pos+5);
                        for(size_t i = 0; i < rest.size(); ++i) 
                            if(rest[i] == ',' || rest[i] == '\t') 
                                rest[i] = ' ';
                        vector<string> toks = util::split_ws(rest);
                        for(size_t i = 0; i < toks.size(); ++i){ 
                            string tt = util::trim(toks[i]); 
                            if(tt.empty() || tt == "input") continue; 
                            pi_order.push_back(tt); 
                        }
                    }
                    else{
                        vector<string> toks = util::split_ws(line);
                        vector<int> row; 
                        row.reserve(toks.size());
                        for(size_t i = 0; i < toks.size(); ++i) 
                            if(toks[i] == "0" || toks[i] == "1") 
                                row.push_back(toks[i] == "1");
                        if(!row.empty()) 
                            patterns.push_back(row);
                    }
                }
                have_patterns = !patterns.empty();
            }
        }

        STA sta(lib, nl, libname, nl.module_name);
        sta.build_graph();
        sta.compute_loads();
        sta.step2();
        string outdir = string(".");
        sta.write_step1(outdir);
        sta.write_step2(outdir);
        sta.write_step3(outdir);
        if(have_patterns) sta.write_step4(outdir, patterns, pi_order);
        return 0;
    }
};

int main(int argc, char** argv){
    StaApp app; 
    return app.run(argc, argv);
}

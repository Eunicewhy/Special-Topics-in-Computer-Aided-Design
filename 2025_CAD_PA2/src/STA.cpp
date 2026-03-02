#include "../inc/STA.h"

void STA::build_graph(){
    for(size_t i = 0; i < nl.PIs.size(); ++i) 
        PIset.insert(nl.PIs[i]);
    for(size_t i = 0; i < nl.POs.size(); ++i) 
        POset.insert(nl.POs[i]);

    int G = (int)nl.gates.size();
    gate_in_nets.assign((size_t)G, vector<string>());   
    for(int i = 0; i < G; ++i){
        const Netlist::Gate &g = nl.gates[i];
        // get output pin name
        string op = string("ZN");   // default output pin name
        unordered_map<string, string>::const_iterator itopmap = nl.outpin.find(g.type);  // check output pin mapping
        if(itopmap != nl.outpin.end()) op = itopmap->second;
        // get output net name
        string onet = string();     // output net name
        unordered_map<string,string>::const_iterator ito = g.pins.find(op);
        if(ito != g.pins.end()) 
            onet = ito->second;
        gate_out_net[i] = onet;
        if(!onet.empty()) 
            net_driver[onet] = make_pair(i, op);

        // get input pin names
        vector<string> ins;
        unordered_map<string, vector<string> >::const_iterator itins = nl.inpins.find(g.type);
        if(itins != nl.inpins.end()) ins = itins->second; 
        else {
            for(unordered_map<string,string>::const_iterator kv = g.pins.begin(); kv != g.pins.end(); ++kv){ 
                if(kv->first != op) 
                    ins.push_back(kv->first); 
            }
        }
        sort(ins.begin(), ins.end());
        // get input net names
        for(size_t k = 0; k < ins.size(); ++k){
            unordered_map<string,string>::const_iterator itp = g.pins.find(ins[k]);
            if(itp == g.pins.end()) 
                continue;
            string net = itp->second;
            gate_in_nets[i].push_back(net);
            net_sinks[net].push_back(make_pair(i, ins[k]));
        }
    }
    // collect all nets
    for(size_t i = 0; i < nl.PIs.size(); ++i) 
        add_net(nl.PIs[i]);     // PI nets
    for(size_t i = 0; i < nl.POs.size(); ++i) 
        add_net(nl.POs[i]);     // PO nets
    for(unordered_map<int,string>::const_iterator kv = gate_out_net.begin(); kv != gate_out_net.end(); ++kv) 
        add_net(kv->second);    // gate output nets
    for(unordered_map<string, vector<pair<int,string>>>::const_iterator kv = net_sinks.begin(); kv != net_sinks.end(); ++kv) 
        add_net(kv->first);     // net sinks

    // initialize timing vectors
    net_arrival.assign(nets.size(), 0.0);
    net_tran.assign(nets.size(), 0.0);
    net_tran_r.assign(nets.size(), 0.0);
    net_tran_f.assign(nets.size(), 0.0);
    for(size_t i = 0; i < nl.PIs.size(); ++i){
        int id = net_index[nl.PIs[i]];
        net_arrival[id] = 0.0;
        net_tran[id] = 0.0;
    }
}

// Step 1
void STA::compute_loads(){
    int G = (int)nl.gates.size();
    gate_output_load.assign((size_t)G, 0.0);
    for(int i = 0; i < G; ++i){
        string onet = gate_out_net[i];   // output net name
        double L = 0.0;
        unordered_map<string, vector<pair<int, string>>>::const_iterator it = net_sinks.find(onet);  // find sinks
        if(it != net_sinks.end()){
            const vector<pair<int, string>> &vec = it->second;
            for(size_t t = 0; t < vec.size(); ++t){
                int gi = vec[t].first;      // sink gate index
                const string &ipin = vec[t].second;     // input pin name
                const CellLib::Cell *cell = lib.getCell(nl.gates[gi].type);
                double cap = 0.0;
                if(cell){
                    unordered_map<string,CellLib::Pin>::const_iterator ip = cell->pins.find(ipin);
                    if(ip != cell->pins.end()) 
                        cap = ip->second.cap;
                }
                L += cap;
            }
        }
        if(POset.find(onet) != POset.end()) L += 0.03; // PO extra load
        gate_output_load[i] = L;
    }
}

vector<int> STA::topo() const{
    int G = (int)nl.gates.size();
    vector<int> indeg(G, 0);
    unordered_map<string, int> out2g; 
    out2g.reserve(G*2);
    for(int i = 0; i < G; ++i) 
        out2g[gate_out_net.at(i)] = i;
    for(int i = 0; i < G; ++i){
        const vector<string> &ins = gate_in_nets.at(i);
        for(size_t k = 0; k < ins.size(); ++k){
            if(PIset.find(ins[k]) != PIset.end())   // skip PI nets
                continue;
            unordered_map<string,int>::const_iterator it = out2g.find(ins[k]);
            if(it != out2g.end()) 
                indeg[i]++;
        }
    }
    queue<int> q; 
    for(int i = 0; i < G; ++i) 
        if(indeg[i] == 0) 
            q.push(i);
    vector<int> ord; 
    ord.reserve((size_t)G);
    while(!q.empty()){
        int u = q.front(); 
        q.pop(); 
        ord.push_back(u);
        string on = gate_out_net.at(u);
        unordered_map<string, vector<pair<int,string> > >::const_iterator it = net_sinks.find(on);
        if(it != net_sinks.end()){
            const vector<pair<int,string>> &vec = it->second;
            for(size_t t = 0; t < vec.size(); ++t){ 
                if(--indeg[(size_t)vec[t].first] == 0) 
                    q.push(vec[t].first); 
            }
        }
    }
    if((int)ord.size() != G){
        for(int i = 0; i < G; ++i){
            if(find(ord.begin(), ord.end(), i) == ord.end()) 
                ord.push_back(i);
        }
    }
    return ord;
}

void STA::step2(){
    gt.assign(nl.gates.size(), GateTiming());
    vector<int> ord = topo();
    for(size_t idx = 0; idx < ord.size(); ++idx){
        int gi = ord[idx];    // gate index from topo order
        double bestA = -1e100; 
        int sel = -1; 
        double in_tr = 0.0;
        const vector<string> &ins = gate_in_nets[gi];
        for(int k=0; k < (int)ins.size(); ++k){
            unordered_map<string, int>::const_iterator itN = net_index.find(ins[(size_t)k]);
            if(itN == net_index.end()) 
                continue;
            int nid = itN->second;
            double wire = (PIset.find(ins[k]) != PIset.end() ? 0.0 : WIRE_DELAY);
            double arr  = net_arrival[nid] + wire;
            if(arr > bestA){ 
                bestA = arr; 
                sel = k; 
                in_tr = net_tran[nid]; 
            }
        }
        if(sel == -1){ 
            sel = 0; 
            in_tr = 0.0; 
            bestA = 0.0; 
        }

        double L = gate_output_load[gi];
        const CellLib::Cell *cell = lib.getCell(nl.gates[gi].type);
        double d0=0.0, t0=0.0, d1=0.0, t1=0.0;
        // calculate delays
        if (cell){
            if(!cell->cell_fall.v.empty() && !cell->fall_tran.v.empty()){
                d0 = cell->cell_fall.bilinear(L, in_tr);
                t0 = cell->fall_tran.bilinear(L, in_tr);
            }
            if(!cell->cell_rise.v.empty() && !cell->rise_tran.v.empty()){
                d1 = cell->cell_rise.bilinear(L, in_tr);
                t1 = cell->rise_tran.bilinear(L, in_tr);
            }
        }
        int worst = (d1 >= d0) ? 1 : 0;
        double d = (worst ? d1 : d0), tr=(worst ? t1 : t0);
        gt[gi].worst_logic = worst;
        gt[gi].prop_delay = d;
        gt[gi].out_tran   = tr;
        gt[gi].sel_in     = sel;

        unordered_map<int,string>::const_iterator itOut = gate_out_net.find(gi);
        if(itOut != gate_out_net.end()){
            int oid = net_index[itOut->second];
            net_arrival[oid] = bestA + d;
            net_tran[oid]    = tr;
            net_tran_f[oid]  = t0;
            net_tran_r[oid]  = t1;
        }
    }
}

pair<STA::Path, STA::Path> STA::step3_paths(){
    const double NEG = -1e100;
    vector<double> arr(nets.size(), NEG);
    vector<int>    prev(nets.size(), -1);

    // PIs = 0
    for(size_t i = 0; i < nl.PIs.size(); ++i){ 
        unordered_map<string, int>::const_iterator it = net_index.find(nl.PIs[i]); 
        if(it != net_index.end()) 
            arr[(size_t)it->second]=0.0; 
    }

    vector<int> ord = topo();
    for(size_t oi = 0; oi < ord.size(); ++oi){
        int gi = ord[oi];
        int sel = gt[gi].sel_in;
        if(sel < 0 || sel >= (int)gate_in_nets[gi].size()) 
            continue;
        const string &in_net  = gate_in_nets[gi][sel];
        const string &out_net = gate_out_net[gi];
        unordered_map<string, int>::const_iterator iti = net_index.find(in_net);
        unordered_map<string, int>::const_iterator ito = net_index.find(out_net);
        if(iti == net_index.end() || ito == net_index.end()) 
            continue;
        int in_id = iti->second, out_id = ito->second;
        if(arr[in_id] <= NEG/2) 
            continue;
        double wire = (PIset.find(in_net) != PIset.end() ? 0.0 : WIRE_DELAY);
        double cand = arr[in_id] + wire + gt[gi].prop_delay;
        if(cand > arr[out_id]){ 
            arr[out_id] = cand; 
            prev[out_id] = in_id; 
        }
    }

    // choose among POs
    int best_max = -1, best_min = -1; 
    double vmax = NEG, vmin = 1e100;
    for(size_t i=0;i<nl.POs.size();++i){ 
        unordered_map<string,int>::const_iterator it = net_index.find(nl.POs[i]); 
        if(it == net_index.end()) 
            continue; 
        int id = it->second; 
        if(arr[id] <= NEG/2) 
            continue; 
        if(arr[id] > vmax){ 
            vmax = arr[id]; 
            best_max = id; 
        } 
        if(arr[id] < vmin){ 
            vmin = arr[id]; 
            best_min = id; 
        } 
    }

    Path longest = build_path(arr, prev, best_max);
    Path shortest= build_path(arr, prev, best_min);
    return make_pair(longest, shortest);
}

vector< vector<STA::Row> > STA::step4(const vector< vector<int> >& patterns, const vector<string>& pi_order){
    vector<int> ord = topo();   // topological order
    vector<vector<STA::Row>> all; 
    all.reserve(patterns.size());
    for(size_t p = 0; p < patterns.size(); ++p){
        const vector<int> &pat = patterns[p];
        unordered_map<string, int> logic;
        for(size_t i = 0; i < pi_order.size() && i < pat.size(); ++i){
            logic[pi_order[i]] = pat[i];
        }
        vector<double> parr(nets.size(), -1e100);
        vector<double> pr_slew(nets.size(), 0.0), pf_slew(nets.size(), 0.0);
        for(size_t i = 0; i < nl.PIs.size(); ++i){ 
            int id = net_index[nl.PIs[i]]; 
            parr[id] = 0.0; 
            pr_slew[id] = 0.0; 
            pf_slew[id] = 0.0; 
        }

        vector<STA::Row> rows(nl.gates.size());
        for(size_t oi = 0; oi < ord.size(); ++oi){
            int gi = ord[oi];
            const Netlist::Gate &G = nl.gates[(size_t)gi];
            const vector<string> &ins = gate_in_nets[(size_t)gi];

            vector<int> vs; 
            vs.reserve(ins.size());
            for(size_t k = 0; k < ins.size(); ++k){ 
                unordered_map<string,int>::const_iterator it = logic.find(ins[k]); 
                vs.push_back(it == logic.end() ? 0 : it->second); 
            }
            bool INV = is_inv(G.type), NAND = is_nand(G.type), NOR = is_nor(G.type);

            int outv = 0;
            // determine output logic value
            if(INV) outv = (vs.empty()?0:(vs[0]^1));
            else if(NAND){ 
                int a = 1; 
                for(size_t q = 0; q < vs.size(); ++q) 
                    a&= vs[q]; 
                outv = a ^ 1; 
            }
            else if(NOR){ 
                int o=0; 
                for(size_t q = 0; q < vs.size(); ++q) 
                    o|= vs[q]; 
                outv = o ^ 1; 
            }
            else
                outv = (vs.empty() ? 0 : (vs[0] ^ 1));

            vector<int> ctrl_idx; 
            vector<int> non_idx;
            for(int k=0; k < (int)ins.size(); ++k){ 
                if(is_ctrl_val(NAND, NOR, vs[(size_t)k])) 
                    ctrl_idx.push_back(k); 
                else non_idx.push_back(k); 
            }

            bool want_min_ctrl = (NAND && outv==1) || (NOR && outv==0);
            bool want_min_non  = false;

            int chosen = -1;
            if(!ctrl_idx.empty())     
                chosen = pick_best(ins, ctrl_idx, want_min_ctrl, parr);
            else if(!non_idx.empty()) 
                chosen = pick_best(ins, non_idx,  want_min_non,  parr);
            if(chosen < 0 && !ins.empty()) 
                chosen = 0;

            double in_arr = 0.0;
            double in_tr_for_rise = 0.0;
            double in_tr_for_fall = 0.0;
            if(!ins.empty()){
                int nid = net_index.at(ins[chosen]);
                in_arr = parr[nid] + (PIset.find(ins[chosen]) != PIset.end() ? 0.0 : WIRE_DELAY);
                double in_rise_slew = pr_slew[nid];
                double in_fall_slew = pf_slew[nid];
                if(INV || NAND || NOR){ 
                    in_tr_for_rise = in_fall_slew; 
                    in_tr_for_fall = in_rise_slew; 
                }
                else{
                    in_tr_for_rise = in_rise_slew; 
                    in_tr_for_fall = in_fall_slew; 
                }
            }

            double L = gate_output_load[(size_t)gi];
            const CellLib::Cell *cell = lib.getCell(G.type);
            double cd = 0.0, otran = 0.0, out_r = 0.0, out_f = 0.0;
            if(cell){
                out_r = cell->rise_tran.bilinear(L, in_tr_for_rise);
                out_f = cell->fall_tran.bilinear(L, in_tr_for_fall);
                if(outv == 1){ 
                    cd = cell->cell_rise.bilinear(L, in_tr_for_rise); 
                    otran = out_r; 
                }
                else{ 
                    cd = cell->cell_fall.bilinear(L, in_tr_for_fall);  
                    otran = out_f; 
                }
            }

            int oid = net_index.at(gate_out_net[gi]);
            parr[oid]    = in_arr + cd;
            pr_slew[oid] = out_r;
            pf_slew[oid] = out_f;

            rows[gi].logic  = outv;
            rows[gi].cdelay = cd;
            rows[gi].otran  = otran;
            if(!gate_out_net[gi].empty()) 
                logic[gate_out_net[gi]] = outv;
        }
        all.push_back(rows);
    }
    return all;
}

vector<int> STA::sorted_gate_idx() const{
    vector<int> idx(nl.gates.size());
    for(size_t i = 0; i < idx.size(); ++i) 
        idx[i]=(int)i;
    sort(idx.begin(), idx.end(), GateComp(this));
    return idx;
}

void STA::write_step1(const string &dir){
    vector<int> ord = sorted_gate_idx();
    ofstream f((dir + "/" + libname + "_" + modulename + "_load.txt").c_str());
    f.setf(ios::fixed); 
    f << setprecision(6);
    for(size_t i = 0; i < ord.size(); ++i){ 
        int gi = ord[i]; 
        f << nl.gates[(size_t)gi].name << " "<< gate_output_load[(size_t)gi]<<"\n"; 
    }
}

void STA::write_step2(const string &dir){
    vector<int> ord = sorted_gate_idx();
    ofstream f((dir + "/" + libname + "_" + modulename + "_delay.txt").c_str());
    f.setf(ios::fixed); 
    f << setprecision(6);
    for(size_t i = 0; i < ord.size(); ++i){ 
        int gi = ord[i]; 
        const GateTiming &g = gt[(size_t)gi]; 
        f << nl.gates[(size_t)gi].name << " " << g.worst_logic << " " << g.prop_delay << " " << g.out_tran << "\n"; 
    }
}

void STA::write_step3(const string &dir){
    pair<Path,Path> pr = step3_paths();
    ofstream f((dir + "/" + libname + "_" + modulename + "_path.txt").c_str());
    f.setf(ios::fixed); 
    f << setprecision(6);
    f << "Longest delay = " << pr.first.delay << ", the path is: ";
    for(size_t i=0; i < pr.first.nets.size(); ++i){ 
        if(i)
            f << " -> "; 
        f << pr.first.nets[i]; 
    }
    f << "\n";
    f << "Shortest delay = " << pr.second.delay << ", the path is: ";
    for(size_t i=0; i<pr.second.nets.size(); ++i){ 
        if(i) 
            f << " -> "; 
        f << pr.second.nets[i]; 
    }
    f << "\n";
}

void STA::write_step4(const string &dir, const vector< vector<int> >& patterns, const vector<string>& pi_order){
    vector< vector<Row> > all = step4(patterns, pi_order);
    vector<int> ord = sorted_gate_idx();
    ofstream f((dir + "/" + libname + "_" + modulename + "_gate_info.txt").c_str());
    f.setf(ios::fixed); 
    f << setprecision(6);
    for(size_t p = 0; p < all.size(); ++p){ 
        if(p > 0) 
            f << "\n"; 
        for(size_t i = 0; i < ord.size(); ++i){ 
            int gi=ord[i]; 
            const Row &r = all[p][(size_t)gi]; 
            f << nl.gates[(size_t)gi].name << " " << r.logic << " " << r.cdelay << " " << r.otran<<"\n"; 
        } 
    }
    f << "\n"; 
}

// private
void STA::add_net(const string &n){ 
    if(n.empty()) return; 
    if(net_index.find(n) == net_index.end()){ 
        int id = (int)nets.size(); 
        net_index[n] = id; 
        nets.push_back(n); 
    } 
}

bool STA::is_inv (const string &t){ 
    return util::lower(t).find("inv") != string::npos; 
}

bool STA::is_nand(const string &t){ 
    return util::lower(t).find("nand")!= string::npos; 
}

bool STA::is_nor (const string &t){ 
    return util::lower(t).find("nor") != string::npos; 
}

bool STA::is_ctrl_val(bool NAND, bool NOR, int v){ 
    return (NAND && v==0) || (NOR && v==1); 
}

int STA::pick_best(const vector<string>& ins, const vector<int>& idxs, bool prefer_min, const vector<double>& parr) const{
    if(idxs.empty()) return -1;
    int best = idxs[0];
    for(size_t u = 1; u < idxs.size(); ++u){
        double ca = in_cost(ins, best, parr), cb = in_cost(ins, idxs[u], parr);
        if(prefer_min ? (cb<ca-1e-12) : (cb>ca+1e-12)) 
            best = idxs[u];
        else if(fabs(cb-ca)<=1e-12 && idxs[u] < best) 
            best = idxs[u];
    }
    return best;
}

double STA::in_cost(const vector<string>& ins, int k, const vector<double>& parr) const{
    int id = net_index.at(ins[(size_t)k]);
    double base = parr[(size_t)id];
    bool isPI = (PIset.find(ins[(size_t)k])!=PIset.end());
    return base + (isPI? 0.0 : WIRE_DELAY);
}

STA::Path STA::build_path(const vector<double>& arr, const vector<int>& prev, int end_id){
    STA::Path p; 
    if(end_id < 0) return p;
    p.delay = arr[end_id];
    vector<int> seq; 
    int cur = end_id; 
    while(cur != -1){ 
        seq.push_back(cur); 
        cur = prev[cur]; 
    }
    reverse(seq.begin(), seq.end()); 
    p.nets.reserve(seq.size());
    for(size_t i = 0; i < seq.size();++i) 
        p.nets.push_back(nets[seq[i]]);
    return p;
}
#include "Netlist.h"

// ===================== STA =====================
class STA{
public:
    struct GateTiming { 
        int worst_logic;
        double prop_delay;
        double out_tran;
        int sel_in;
        GateTiming(): worst_logic(-1), prop_delay(0.0), out_tran(0.0), sel_in(-1){} 
    };
    struct Path { 
        double delay;
        vector<string> nets;
        Path(): delay(-1e100){} 
    };
    struct Row { 
        int logic;
        double cdelay;
        double otran;
        Row(): logic(0), cdelay(0.0), otran(0.0){} 
    };

    STA(const CellLib &L, const Netlist &N, const string& ln, const string& mn)
    : lib(L), nl(N), libname(ln), modulename(mn), WIRE_DELAY(0.005){}

    void build_graph();     // build net_sinks, net_driver, gate_out_net, gate_in_nets
    void compute_loads();   // compute gate_output_load

    vector<int> topo() const;   // topological order of gate indices

    void step2();         // compute gate timing (gt)(delay)
    pair<Path,Path> step3_paths();  // compute critical paths (rising, falling)
    vector<vector<Row>> step4(const vector<vector<int>>& patterns, const vector<string>& pi_order);  // simulate patterns

    // ----- I/O helpers -----
    vector<int> sorted_gate_idx() const;  // gate indices in sorted order

    void write_step1(const string &dir);  // write gate output loads
    void write_step2(const string &dir);  // write gate timing (delay)
    void write_step3(const string &dir);  // write critical paths
    void write_step4(const string &dir, const vector<vector<int>>& patterns, const vector<string>& pi_order);  // write sim results

private:
    const CellLib &lib; 
    const Netlist &nl; 
    string libname, modulename; 
    const double WIRE_DELAY;

    unordered_map<string, vector<pair<int, string>>> net_sinks;  // net -> [(gate, ipin)]
    unordered_map<string, pair<int, string>>         net_driver; // net -> (gate, opin)
    unordered_map<int, string> gate_out_net;
    vector<vector<string>> gate_in_nets;    // gate input nets

    vector<double> gate_output_load; // Step1

    unordered_map<string,int> net_index; 
    vector<string> nets;
    vector<double> net_arrival, net_tran, net_tran_r, net_tran_f;
    unordered_set<string> PIset, POset;

    vector<GateTiming> gt; // Step2

    // for sorting gates
    struct GateComp{ 
        const STA* self; 
        explicit GateComp(const STA* s):self(s){} bool operator()(int a, int b) const{
            int na = util::inst_num(self->nl.gates[a].name), nb = util::inst_num(self->nl.gates[b].name);
            if(na != nb) return na < nb; 
            return self->nl.gates[a].name < self->nl.gates[b].name; 
        } 
    };

    void add_net(const string &n);  // add net to net_index and nets

    static bool is_inv (const string&t); // identify inverter
    static bool is_nand(const string&t); // identify NAND
    static bool is_nor (const string&t); // identify NOR
    static bool is_ctrl_val(bool NAND, bool NOR, int v);   // identify controlling value

    int pick_best(const vector<string>& ins, const vector<int>& idxs, bool prefer_min, const vector<double>& parr) const;   // pick best input net index
    double in_cost(const vector<string>& ins, int k, const vector<double>& parr) const;     // input cost function
    Path build_path(const vector<double>& arr, const vector<int>& prev, int end_id);        // build path from prev[]
};
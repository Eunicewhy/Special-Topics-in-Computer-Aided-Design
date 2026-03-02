#include "./include/espresso.h"

#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>

int main(int argc, char* argv[]) {
    auto start_time = chrono::steady_clock::now();
    if (argc != 3) {
        cerr << "Usage: sop <spec file> <out sop file>\n";
        return 1;
    }

    string spec = argv[1];
    string out = argv[2];

    Espresso E;
    if (!E.read_spec_file(spec)) {
        cerr << "Failed to read spec file: " << spec << endl;
        return 2;
    }

    cout << "[INFO] Spec loaded.\n";
    cout << "  n_bit = " << E.n_bit << "\n";
    cout << "  on_set size = " << E.on_set.size() << "\n";
    cout << "  dc_set size = " << E.dc_set.size() << "\n";
    cout << "  off_set size = " << E.off_set.size() << "\n";

    if (E.n_bit == 0) {
        if (!E.write_sop_file(out, vector<Cube>())) {
            cerr << "Failed to write out file.\n";
            return 3;
        }
        cout << "[INFO] n_bit=0, wrote empty SOP file.\n";
        return 0;
    }

    // 1) Expand all seeds (on U dc)
    vector<Cube> primes = E.expand_all();
    cout << "[INFO] Expand finished. Got " << primes.size() << " primes.\n";

    // initial irredundant cover
    vector<Cube> F = E.irredundant_cover(primes);
    cout << "[INFO] After initial irredundant_cover: F size = " << F.size() << "\n";
    vector<Cube> sop = F;

    // // extract essential primes
    // vector<Cube> E_ess = E.essential_primes(F);
    // cout << "[INFO] Extracted essentials: " << E_ess.size() << "\n";

    // // C = F - E
    // // build C by removing any cube equal to an essential
    // vector<Cube> C;
    // for (auto &c : F) {
    //     bool is_ess = false;
    //     for (auto &e : E_ess) if (c == e) { is_ess = true; break; }
    //     if (!is_ess) C.push_back(c);
    // }
    // cout << "[INFO] Initial C size (F - E) = " << C.size() << "\n";

    // // repeat reduce/expand/irredundant until stable
    // int iter = 0;
    // while (true) {
    //     ++iter;
    //     cout << "[LOOP] iteration " << iter << " start. C size = " << C.size() << "\n";
    //     vector<Cube> C_old = C;
    //     // reduce
    //     C = E.reduce_cover(C);
    //     // expand (note: expand_all expands seeds from on+dc; we currently don't have an expand(C) that expands cubes
    //     // so for now we can re-run expand_all to get new primes OR adapt expand to accept seeds; as quick approach
    //     // append the expanded primes of current C's covered minterms: here we approximate by re-running expand_all
    //     // (You can implement expand_from_cover if you want finer control.)
    //     vector<Cube> expanded = E.expand_all(); // conservative; or implement expand(C)
    //     // build new cover from expanded + C (avoid duplicates)
    //     vector<Cube> merged = expanded;
    //     merged.insert(merged.end(), C.begin(), C.end());
    //     // irredundant
    //     C = E.irredundant_cover(merged);
    //     cout << "[LOOP] iteration " << iter << " end. C size = " << C.size() << "\n";
    //     // check convergence
    //     if (C.size() == C_old.size()) {
    //         // to be stricter, check element-wise equality
    //         sort(C.begin(), C.end());
    //         sort(C_old.begin(), C_old.end());
    //         if (C == C_old) break;
    //     }
    //     if (iter > 1) { cout << "[LOOP] reached max iterations 50, breaking\n"; break; }
    // }

    // // final SOP = C + E_ess
    // vector<Cube> sop = C;
    // sop.insert(sop.end(), E_ess.begin(), E_ess.end());

    // List final SOP
    // for (size_t i = 0; i < sop.size(); ++i) {
    //     cout << "  Cube " << i << ": " << sop[i].to_string() << "\n";
    // }

    // Write output
    if (!E.write_sop_file(out, sop)) {
        cerr << "Failed to write output SOP file.\n";
        return 4;
    }

    cout << "[INFO] SOP written to " << out << "\n";
    auto current_time = chrono::steady_clock::now();
    cout << "Total Execution Time: " << (float)chrono::duration_cast<chrono::milliseconds>(current_time - start_time).count() / 1000 << "s" << endl;
    return 0;
}

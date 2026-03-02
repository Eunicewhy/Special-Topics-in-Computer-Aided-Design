#ifndef ESPRESSO_H
#define ESPRESSO_H

#include <vector>
#include <string>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <chrono>
using namespace std;

class Cube {
public:
    // bits: -1 => don't-care ('-'), 0 or 1 => literal
    vector<int> bits;
    Cube() {}
    Cube(int n);
    static Cube from_minterm(int m, int n);
    string to_string() const;
    bool covers_minterm(int m) const;
    bool operator<(Cube const& other) const;
    bool operator==(Cube const& other) const;
    int literal_count() const; // number of non-don't-care literals
};

class Espresso {
public:
    int n_bit;
    vector<int> on_set;      // decimal minterms of on-set
    vector<int> dc_set;      // decimal minterms of don't-care
    vector<int> off_set;     // computed

    // NEW: bit-maps for O(1) membership checks (size = 1<<n_bit)
    vector<char> off_mark;        // 1 if minterm is in off_set
    vector<char> covered_mark;    // 1 if minterm already covered by chosen primes

    Espresso(int n=0);
    bool read_spec_file(const string& path); // fills n_bit, on_set, dc_set
    bool write_sop_file(const string& path, const vector<Cube>& sop) const;

    // Core algorithm
    vector<Cube> expand_all(); // espresso expand -> returns prime implicants

    // NEW: reduction / irredundant / essential phases
    vector<Cube> irredundant_cover(const vector<Cube>& F); // remove redundant implicants from F
    vector<Cube> essential_primes(const vector<Cube>& F);  // extract essential primes (and mark covered)
    vector<Cube> reduce_cover(const vector<Cube>& C);      // reduce each implicant in C (shrink using other implicants)

private:
    void compute_off_set();

    // NEW helpers
    bool cube_has_off_minterm(const Cube& c) const; // fast check using off_mark (early stop)
    void mark_cube_covered(const Cube& c);          // mark all minterms covered by cube in covered_mark
    // when n_bit <= 24, 1<<n_bit fits in memory as vector<char>

};

#endif // ESPRESSO_H

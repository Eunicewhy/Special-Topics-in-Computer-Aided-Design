#include "../include/espresso.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_set>
#include <queue>
#include <numeric>
#include <cassert>
#include <iomanip>

using namespace std;
#define MAX_TIME 100 // seconds

// --- (existing Cube and Espresso implementations up to expand_all) ---
// (Keep your existing implementations; here I include them verbatim as in your original file)

Cube::Cube(int n) : bits(n, 0) {}

Cube Cube::from_minterm(int m, int n) {
    Cube c(n);
    for (int i = 0; i < n; ++i) {
        int bit = (m >> i) & 1;
        c.bits[n - 1 - i] = bit; // store MSB..LSB order in bits[0] = MSB
    }
    return c;
}

string Cube::to_string() const {
    string s;
    s.reserve(bits.size());
    for (int b : bits) {
        if (b == -1) s.push_back('-');
        else if (b == 0) s.push_back('0');
        else s.push_back('1');
    }
    return s;
}

bool Cube::covers_minterm(int m) const {
    int n = (int)bits.size();
    for (int i = 0; i < n; ++i) {
        int bit = (m >> (n - 1 - i)) & 1;
        if (bits[i] == -1) continue;
        if (bits[i] != bit) return false;
    }
    return true;
}

bool Cube::operator<(Cube const& other) const {
    if (bits.size() != other.bits.size()) return bits.size() < other.bits.size();
    return bits < other.bits;
}

bool Cube::operator==(Cube const& other) const {
    return bits == other.bits;
}

int Cube::literal_count() const {
    int c = 0;
    for (int b : bits) if (b != -1) ++c;
    return c;
}

// ---------------- Espresso implementation ----------------

Espresso::Espresso(int n) : n_bit(n) {}

bool Espresso::read_spec_file(const string& path) {
    ifstream ifs(path.c_str());
    if (!ifs.is_open()) return false;
    string line;

    // line 1: n_bit
    if (!getline(ifs, line)) return false;
    {
        istringstream iss(line);
        if (!(iss >> n_bit)) return false;
    }

    // line 2: on set
    if (!getline(ifs, line)) return false;
    {
        on_set.clear();
        istringstream iss(line);
        int v;
        while (iss >> v) on_set.push_back(v);
        sort(on_set.begin(), on_set.end());
        on_set.erase(unique(on_set.begin(), on_set.end()), on_set.end());
    }

    // line 3: dc set
    if (!getline(ifs, line)) return false;
    {
        dc_set.clear();
        istringstream iss(line);
        int v;
        while (iss >> v) dc_set.push_back(v);
        sort(dc_set.begin(), dc_set.end());
        dc_set.erase(unique(dc_set.begin(), dc_set.end()), dc_set.end());
    }

    compute_off_set();
    return true;
}

bool Espresso::write_sop_file(const string& path, const vector<Cube>& sop) const {
    ofstream ofs(path.c_str());
    if (!ofs.is_open()) return false;

    if (n_bit == 0) {
        ofs << endl;
        return true;
    }
    for (size_t i = 0; i < sop.size(); ++i) {
        ofs << sop[i].to_string();
        if (i + 1 < sop.size()) ofs << '\n';
    }
    return true;
}


// ---------------- compute_off_set (改寫以建立 off_mark) ---------------
void Espresso::compute_off_set() {
    // universe: 0 .. 2^n_bit - 1
    int maxv = 1 << n_bit;
    off_mark.assign(maxv, 0);
    for (int v : on_set) {
        if (v >= 0 && v < maxv) off_mark[v] = 0; // ensure not off
    }
    for (int v : dc_set) {
        if (v >= 0 && v < maxv) off_mark[v] = 0; // ensure not off
    }
    off_set.clear();

    // more direct: build a temporary mark vector then compute off_set
    vector<char> temp(1u << n_bit, 0);
    for (int v : on_set) if (v >= 0 && v < (1<<n_bit)) temp[v] = 1;
    for (int v : dc_set) if (v >= 0 && v < (1<<n_bit)) temp[v] = 1;

    off_set.clear();
    off_mark.assign(1u << n_bit, 0);
    for (int m = 0; m < (1<<n_bit); ++m) {
        if (!temp[m]) {
            off_set.push_back(m);
            off_mark[m] = 1;
        }
    }

    // init covered_mark to all zeros
    covered_mark.assign(1u << n_bit, 0);
}

// ---------------- helper: 產生 cube 的 minterm 並檢查是否有任何一個在 off_mark 中 -------------
// 這個函式會遞迴在 don't-care 位上展開所有組合，但會在第一個命中 off_mark 時立即回傳 true（早停）
bool Espresso::cube_has_off_minterm(const Cube& c) const {
    int n = (int)c.bits.size();
    // gather indices of don't-care bits to expand
    vector<int> dc_indices;
    int base = 0;
    for (int i = 0; i < n; ++i) {
        if (c.bits[i] == -1) {
            dc_indices.push_back(i);
        } else {
            // set bit in base accordingly
            int bit = c.bits[i];
            int pos = n - 1 - i; // bits[0] = MSB corresponds to (n-1)
            if (bit == 1) base |= (1 << pos);
        }
    }

    int dc_count = (int)dc_indices.size();
    // if no don't-cares, just test single minterm
    if (dc_count == 0) {
        return off_mark[base];
    }

    // iterate combinations, but early-exit if off_mark hit
    // number of combinations = 2^dc_count; when dc_count large this can be big,
    // but it's necessary: alternative (scanning off_set) is worse for many cubes.
    int combos = 1 << dc_count;
    for (int m = 0; m < combos; ++m) {
        int val = base;
        for (int j = 0; j < dc_count; ++j) {
            int idx = dc_indices[j];
            int pos = n - 1 - idx;
            if ((m >> j) & 1) val |= (1 << pos);
            else val &= ~(1 << pos);
        }
        if (off_mark[val]) return true; // early return
    }
    return false;
}

// ------------- helper: 將 cube 所 cover 的所有 minterm 都標記到 covered_mark -------------
void Espresso::mark_cube_covered(const Cube& c) {
    int n = (int)c.bits.size();
    vector<int> dc_indices;
    int base = 0;
    for (int i = 0; i < n; ++i) {
        if (c.bits[i] == -1) dc_indices.push_back(i);
        else {
            int bit = c.bits[i];
            int pos = n - 1 - i;
            if (bit == 1) base |= (1 << pos);
        }
    }
    int dc_count = (int)dc_indices.size();
    if (dc_count == 0) {
        covered_mark[base] = 1;
        return;
    }
    int combos = 1 << dc_count;
    for (int m = 0; m < combos; ++m) {
        int val = base;
        for (int j = 0; j < dc_count; ++j) {
            int idx = dc_indices[j];
            int pos = n - 1 - idx;
            if ((m >> j) & 1) val |= (1 << pos);
            else val &= ~(1 << pos);
        }
        covered_mark[val] = 1;
    }
}

// ---------------- expand_all (你原本的實作，保留不動) ----------------
vector<Cube> Espresso::expand_all() {
    auto start_time = chrono::steady_clock::now();

    // seeds = on_set ∪ dc_set
    vector<int> seeds = on_set;
    // seeds.insert(seeds.end(), dc_set.begin(), dc_set.end());
    sort(seeds.begin(), seeds.end());
    // seeds.erase(unique(seeds.begin(), seeds.end()), seeds.end());

    cout << "[DEBUG] Expand_all (weighted) : start with " << seeds.size() << " seeds.\n";

    // 計算 column count vector (same as before)
    vector<int> column_count(n_bit, 0);
    for (int m : seeds) {
        for (int b = 0; b < n_bit; ++b) {
            int bit = (m >> (n_bit - 1 - b)) & 1; // bits[0] = MSB
            if (bit == 1) column_count[b] += 1;
        }
    }

    // 計算 weight
    struct SeedInfo { int minterm; int weight; };
    vector<SeedInfo> sinfo;
    sinfo.reserve(seeds.size());
    for (int m : seeds) {
        int w = 0;
        for (int b = 0; b < n_bit; ++b) {
            int bit = (m >> (n_bit - 1 - b)) & 1;
            if (bit == 1) w += column_count[b];
        }
        sinfo.push_back({m, w});
    }

    sort(sinfo.begin(), sinfo.end(), [](const SeedInfo &a, const SeedInfo &b) {
        if (a.weight != b.weight) return a.weight > b.weight;
        return a.minterm > b.minterm;
    });

    // Phase 1: expand seeds → expanded_primes
    vector<Cube> expanded_primes;
    unordered_set<string> prime_set;

    for (size_t sidx = 0; sidx < sinfo.size(); ++sidx) {
        auto current_time = chrono::steady_clock::now();
        auto duration = chrono::duration_cast<chrono::seconds>(current_time - start_time).count();
        if (duration > MAX_TIME) {
            cout << "[DEBUG] Expand_all timeout after " << MAX_TIME << " seconds.\n";
            break;
        }

        int m = sinfo[sidx].minterm;

        // fast check: if this minterm is already covered by previous primes, skip
        if (covered_mark[m]) continue;

        Cube c = Cube::from_minterm(m, n_bit);

        // drop bits in order of column_count (same strategy)
        vector<int> bit_order(n_bit);
        iota(bit_order.begin(), bit_order.end(), 0);
        stable_sort(bit_order.begin(), bit_order.end(), [&](int a, int b) {
            if (column_count[a] != column_count[b]) return column_count[a] > column_count[b];
            return a > b;
        });

        bool changed = true;
        vector<int> origin_bits = c.bits;
        while (changed) {
            changed = false;
            for (int idx = 0; idx < n_bit; ++idx) {
                int i = bit_order[idx];
                if (c.bits[i] == -1) continue;
                int old = c.bits[i];
                c.bits[i] = -1;
                // use fast check: does this cube now cover any off minterm?
                if (cube_has_off_minterm(c)) {
                    c.bits[i] = old; // revert
                } else {
                    changed = true;
                }
            }
        }

        string cstr = c.to_string();
        if (prime_set.find(cstr) == prime_set.end()) {
            expanded_primes.push_back(c);
            prime_set.insert(cstr);
            // IMPORTANT: mark all minterms covered by this prime -> speed up future checks
            if(c.bits != origin_bits) mark_cube_covered(c);
            cout << "  [expand] seed_order " << sidx
                 << " (minterm=" << m << ", weight=" << sinfo[sidx].weight
                 << ") -> cube " << cstr << " (added)\n";
        } else {
            cout << "  [expand] seed_order " << sidx
                 << " (minterm=" << m << ", weight=" << sinfo[sidx].weight
                 << ") -> cube " << cstr << " (duplicate, skipped)\n";
        }
    }

    // Phase 2: merge expanded_primes + any on-set minterms not yet covered
    int remaining_on = 0;
    vector<Cube> result = expanded_primes;
    for (int on_m : on_set) {
        if (!covered_mark[on_m]) {
            ++remaining_on;
            Cube oc = Cube::from_minterm(on_m, n_bit);
            result.push_back(oc);
        }
    }
    cout << "[DEBUG] Remaining on-set minterms not covered: " << remaining_on << endl;

    // 排序 + 唯一化
    sort(result.begin(), result.end());
    result.erase(unique(result.begin(), result.end()), result.end());

    cout << "[DEBUG] Expand_all (weighted) done. primes size = " << result.size() << "\n";
    return result;
}


// ---------------- NEW FUNCTIONS: irredundant_cover, essential_primes, reduce_cover -------------

// Remove redundant implicants from F:
// Strategy:
//  - Build mapping from each on-set minterm (index into on_set) to the list of cubes that cover it
//  - If a cube covers no on-set minterm uniquely (i.e., every covered minterm is also covered by >=1 other cube),
//    then it is redundant and can be removed. Repeat until no change.
// Uses caching (per-cube coverage vector<char>) to speed repeated checks.
vector<Cube> Espresso::irredundant_cover(const vector<Cube>& F) {
    auto t0 = chrono::steady_clock::now();
    cout << "[IRRED] === Starting irredundant_cover (bitmap + xor optimization) ===\n";
    cout << "[IRRED] Input: F size = " << F.size()
         << ", ON-set size = " << on_set.size()
         << ", n_bit = " << n_bit << "\n";

    size_t K = on_set.size();
    size_t M = F.size();
    if (M == 0 || K == 0) {
        cout << "[IRRED] Empty input, return directly.\n";
        return F;
    }

    // on_index: minterm -> index in on_set (or -1)
    int universe = 1 << n_bit;
    vector<int> on_index(universe, -1);
    for (size_t k = 0; k < K; ++k) {
        int m = on_set[k];
        if (m >= 0 && m < universe) on_index[m] = (int)k;
    }

    size_t W = (K + 63) / 64;
    vector<vector<uint64_t>> coverage_bits(M, vector<uint64_t>(W, 0ull));
    vector<int> cover_count(K, 0);
    vector<int> xor_idx(K, 0);

    uint64_t total_enumerated = 0, total_set_bits = 0;
    size_t enum_path = 0, scan_path = 0;

    // --- build coverage bitmap ---
    for (size_t i = 0; i < M; ++i) {
        const Cube &c = F[i];
        int base = 0;
        vector<int> dc_indices;
        for (int b = 0; b < n_bit; ++b) {
            if (c.bits[b] == -1) dc_indices.push_back(b);
            else {
                int bit = c.bits[b];
                int pos = n_bit - 1 - b;
                if (bit == 1) base |= (1 << pos);
            }
        }
        int dc_count = (int)dc_indices.size();
        uint64_t combos = (dc_count >= 31) ? (1ull << 31) : (1ull << dc_count);
        if (combos <= (uint64_t)K * 2ull) {
            ++enum_path;
            total_enumerated += combos;
            for (uint64_t msk = 0; msk < combos; ++msk) {
                int val = base;
                for (int j = 0; j < dc_count; ++j) {
                    int idx = dc_indices[j];
                    int pos = n_bit - 1 - idx;
                    if ((msk >> j) & 1u) val |= (1 << pos);
                    else val &= ~(1 << pos);
                }
                int k = on_index[val];
                if (k != -1) {
                    size_t w = (size_t)k >> 6;
                    int off = k & 63;
                    uint64_t bit = 1ull << off;
                    if ((coverage_bits[i][w] & bit) == 0) {
                        coverage_bits[i][w] |= bit;
                        ++cover_count[k];
                        xor_idx[k] ^= (int)i;
                        ++total_set_bits;
                    }
                }
            }
        } else {
            ++scan_path;
            for (size_t k = 0; k < K; ++k) {
                int m = on_set[k];
                if (c.covers_minterm(m)) {
                    size_t w = (size_t)k >> 6;
                    int off = k & 63;
                    uint64_t bit = 1ull << off;
                    if ((coverage_bits[i][w] & bit) == 0) {
                        coverage_bits[i][w] |= bit;
                        ++cover_count[k];
                        xor_idx[k] ^= (int)i;
                        ++total_set_bits;
                    }
                }
            }
        }
    }

    auto t1 = chrono::steady_clock::now();
    cout << "[IRRED] Coverage built. enum_path=" << enum_path
         << ", scan_path=" << scan_path
         << ", total_set_bits=" << total_set_bits
         << ", avg_cover/cube=" << (double)total_set_bits / (double)M
         << ", build_time=" << chrono::duration_cast<chrono::milliseconds>(t1 - t0).count() << " ms\n";

    // --- compute unique_count (per cube) ---
    vector<int> unique_count(M, 0);
    for (size_t k = 0; k < K; ++k) {
        if (cover_count[k] == 1) {
            int only_i = xor_idx[k];
            if (only_i >= 0 && (size_t)only_i < M) unique_count[only_i]++;
        }
    }

    vector<char> alive(M, 1);
    deque<int> candidates;
    for (size_t i = 0; i < M; ++i)
        if (unique_count[i] == 0) candidates.push_back((int)i);

    cout << "[IRRED] Initial removable candidates = " << candidates.size() << "\n";

    // --- removal loop ---
    int removed = 0, attempts = 0, rejects = 0;
    while (!candidates.empty()) {
        int i = candidates.front();
        candidates.pop_front();
        if (!alive[i]) continue;
        ++attempts;
        if (unique_count[i] > 0) { ++rejects; continue; }

        bool can_remove = true;
        for (size_t w = 0; w < W && can_remove; ++w) {
            uint64_t word = coverage_bits[i][w];
            while (word) {
                int bit = __builtin_ctzll(word);
                size_t k = (w << 6) + bit;
                if (k < K && cover_count[k] <= 1) { can_remove = false; break; }
                word &= word - 1;
            }
        }
        if (!can_remove) { ++rejects; continue; }

        alive[i] = 0;
        ++removed;
        for (size_t w = 0; w < W; ++w) {
            uint64_t word = coverage_bits[i][w];
            while (word) {
                int bit = __builtin_ctzll(word);
                size_t k = (w << 6) + bit;
                if (k < K) {
                    cover_count[k]--;
                    xor_idx[k] ^= i;
                    if (cover_count[k] == 1) {
                        int remaining = xor_idx[k];
                        if (remaining >= 0 && (size_t)remaining < M)
                            unique_count[remaining]++;
                    }
                }
                word &= word - 1;
            }
        }
    }

    auto t2 = chrono::steady_clock::now();
    cout << "[IRRED] Removal: attempts=" << attempts
         << ", removed=" << removed
         << ", rejects=" << rejects
         << ", total_time=" << chrono::duration_cast<chrono::milliseconds>(t2 - t0).count() << " ms\n";

    vector<Cube> result;
    result.reserve(M - removed);
    for (size_t i = 0; i < M; ++i)
        if (alive[i]) result.push_back(F[i]);

    cout << "[IRRED] Done. Remaining cubes = " << result.size() << "\n";
    cout << "[IRRED] ==========================================\n";
    return result;
}




// Extract essential primes from F (primes that alone cover some on-set minterm).
// This also marks their covered minterms into covered_mark (so future operations can skip them).
vector<Cube> Espresso::essential_primes(const vector<Cube>& F) {
    auto t0 = chrono::steady_clock::now();
    cout << "[ESS] === Starting essential_primes (optimized) ===\n";
    cout << "[ESS] Input F size = " << F.size() << ", on_set size = " << on_set.size()
         << ", n_bit = " << n_bit << "\n";

    size_t M = F.size();
    size_t K = on_set.size();
    if (M == 0 || K == 0) {
        cout << "[ESS] Trivial case (empty). Return empty essentials.\n";
        return {};
    }

    // build on_index for O(1) minterm -> index in on_set
    int universe = 1 << n_bit;
    vector<int> on_index(universe, -1);
    for (size_t k = 0; k < K; ++k) {
        int m = on_set[k];
        if (m >= 0 && m < universe) on_index[m] = (int)k;
    }
    cout << "[ESS] on_index table built (size = " << universe << ").\n";

    // per-cube coverage vector<char> and reverse mapping minterm->cubes
    vector<vector<char>> coverage(M, vector<char>(K, 0));
    vector<vector<int>> minterm_to_cubes(K);
    uint64_t total_enum_combos = 0;
    size_t cubes_enum_path = 0, cubes_scan_path = 0;
    size_t coverage_count = 0;

    // Build coverage using fast path per-cube (enumerate combos if small, else scan on_set)
    for (size_t i = 0; i < M; ++i) {
        const Cube &c = F[i];
        // compute base and dc indices
        int base = 0;
        vector<int> dc_indices;
        for (int b = 0; b < n_bit; ++b) {
            if (c.bits[b] == -1) dc_indices.push_back(b);
            else {
                int bit = c.bits[b];
                int pos = n_bit - 1 - b;
                if (bit == 1) base |= (1 << pos);
            }
        }
        int dc_count = (int)dc_indices.size();
        uint64_t combos = (dc_count >= 31) ? (1ull << 31) : (1ull << dc_count); // cap for safety
        // Heuristic: enumerate if combos <= 2*K (like irredundant version)
        if (combos <= (uint64_t)K * 2ull) {
            ++cubes_enum_path;
            total_enum_combos += combos;
            for (uint64_t msk = 0; msk < combos; ++msk) {
                int val = base;
                for (int j = 0; j < dc_count; ++j) {
                    int idx = dc_indices[j];
                    int pos = n_bit - 1 - idx;
                    if ((msk >> j) & 1u) val |= (1 << pos);
                    else val &= ~(1 << pos);
                }
                int k = on_index[val];
                if (k != -1) {
                    if (!coverage[i][k]) {
                        coverage[i][k] = 1;
                        minterm_to_cubes[k].push_back((int)i);
                        ++coverage_count;
                    }
                }
            }
        } else {
            ++cubes_scan_path;
            // fall back: scan on_set
            for (size_t k = 0; k < K; ++k) {
                int m = on_set[k];
                if (c.covers_minterm(m)) {
                    if (!coverage[i][k]) {
                        coverage[i][k] = 1;
                        minterm_to_cubes[k].push_back((int)i);
                        ++coverage_count;
                    }
                }
            }
        }
    }

    auto t1 = chrono::steady_clock::now();
    auto build_ms = chrono::duration_cast<chrono::milliseconds>(t1 - t0).count();
    cout << "[ESS] Coverage built in " << build_ms << " ms. coverage_count=" << coverage_count
         << ", cubes_enum_path=" << cubes_enum_path << ", cubes_scan_path=" << cubes_scan_path
         << ", total_enum_combos=" << total_enum_combos << "\n";

    // Now find essential primes: minterms covered by exactly one cube
    vector<char> is_essential(M, 0);
    size_t essential_minterms = 0;
    for (size_t k = 0; k < K; ++k) {
        if (minterm_to_cubes[k].size() == 1) {
            int idx = minterm_to_cubes[k][0];
            if (!is_essential[idx]) {
                is_essential[idx] = 1;
            }
            ++essential_minterms;
        }
    }

    vector<Cube> essentials;
    essentials.reserve(M);
    for (size_t i = 0; i < M; ++i) {
        if (is_essential[i]) {
            essentials.push_back(F[i]);
            // mark all minterms covered by this essential prime as covered (maintain covered_mark)
            mark_cube_covered(F[i]);
            cout << "[ESS] Essential prime idx=" << i << " cube=" << F[i].to_string() << "\n";
        }
    }

    auto t2 = chrono::steady_clock::now();
    auto total_ms = chrono::duration_cast<chrono::milliseconds>(t2 - t0).count();
    cout << "[ESS] essential_primes done. essentials size = " << essentials.size()
         << ", essential_minterms = " << essential_minterms
         << ". total time = " << total_ms << " ms\n";
    cout << "[ESS] ==========================================\n";

    return essentials;
}



// Reduce each implicant in C (attempt to shrink it by assigning don't-cares -> literals)
// Strategy:
//  - We iterate cubes in descending weight (so heavy/overlapping cubes are reduced first)
//  - For each cube, we try to assign DC bits to 0/1 (one by one, greedy). When assigning, we ensure two constraints:
//      1) the reduced cube does not cover any off-set minterm (cube_has_off_minterm)
//      2) any on-set minterm removed from this cube must still be covered by some other cube in C (we maintain minterm_to_cubes counts)
//
// Notes: this is a heuristic (common in Espresso): it's local, greedy, but fast. We update mappings as reductions are accepted.
vector<Cube> Espresso::reduce_cover(const vector<Cube>& C) {
    auto t0 = chrono::steady_clock::now();
    cout << "[RED] Starting reduce_cover. input C size = " << C.size() << ", on_set size = " << on_set.size() << "\n";

    size_t M = C.size();
    size_t K = on_set.size();

    // Precompute column count (weight) to sort cubes descending as suggested by paper
    vector<int> column_count(n_bit, 0);
    for (const int m : on_set) {
        for (int b = 0; b < n_bit; ++b) {
            int bit = (m >> (n_bit - 1 - b)) & 1;
            if (bit == 1) column_count[b] += 1;
        }
    }
    // weight per cube (higher -> process earlier)
    vector<pair<int,int>> weight_idx; // (weight, idx)
    for (size_t i = 0; i < M; ++i) {
        int w = 0;
        for (int b = 0; b < n_bit; ++b) {
            if (C[i].bits[b] == 1) w += column_count[b];
        }
        weight_idx.push_back({w, (int)i});
    }
    sort(weight_idx.begin(), weight_idx.end(), [](const pair<int,int>& a, const pair<int,int>& b){
        if (a.first != b.first) return a.first > b.first;
        return a.second < b.second;
    });

    // build per-cube coverage vector and minterm->list of cubes
    vector<vector<char>> coverage(M, vector<char>(K,0));
    vector<vector<int>> minterm_to_cubes(K);
    for (size_t i = 0; i < M; ++i) {
        for (size_t k = 0; k < K; ++k) {
            if (C[i].covers_minterm(on_set[k])) {
                coverage[i][k] = 1;
                minterm_to_cubes[k].push_back((int)i);
            }
        }
    }

    vector<Cube> result = C; // mutable copy

    // For each cube (in order of descending weight) try to reduce it
    for (auto &p : weight_idx) {
        int idx = p.second;
        Cube &cube = result[idx];
        bool changed_any = false;

        // precompute original covered on_set indices
        vector<int> orig_covered;
        for (size_t k = 0; k < K; ++k) if (coverage[idx][k]) orig_covered.push_back((int)k);

        // gather dc positions to attempt assign; sort them by column_count descending to try influential positions first
        vector<int> dc_positions;
        for (int b = 0; b < n_bit; ++b) if (cube.bits[b] == -1) dc_positions.push_back(b);
        stable_sort(dc_positions.begin(), dc_positions.end(), [&](int a, int b){
            if (column_count[a] != column_count[b]) return column_count[a] > column_count[b];
            return a < b;
        });

        // Try to assign DC positions greedily
        for (int pos : dc_positions) {
            bool assigned = false;
            // try 0 then 1
            for (int val_try = 0; val_try <= 1; ++val_try) {
                Cube cand = cube;
                cand.bits[pos] = val_try;
                // must not include any off minterm
                if (cube_has_off_minterm(cand)) continue;

                // compute new coverage set for candidate (only check on_set minterms)
                vector<int> new_covered;
                new_covered.reserve(orig_covered.size());
                for (int k : orig_covered) {
                    if (cand.covers_minterm(on_set[k])) new_covered.push_back(k);
                }
                // removed = orig_covered - new_covered
                bool all_removed_covered_elsewhere = true;
                // we need to check for each removed minterm whether there's some other cube (not idx) covering it
                for (int k : orig_covered) {
                    // if k is present in new_covered, it's still covered by this cube
                    if (find(new_covered.begin(), new_covered.end(), k) != new_covered.end()) continue;
                    // otherwise check if any other cube covers it
                    bool covered_else = false;
                    for (int other : minterm_to_cubes[k]) {
                        if (other == idx) continue;
                        // other cube may have been reduced earlier, but coverage[] was not updated: however we keep coverage[] consistent when we accept a change (below)
                        if (coverage[other][k]) { covered_else = true; break; }
                    }
                    if (!covered_else) { all_removed_covered_elsewhere = false; break; }
                }

                if (all_removed_covered_elsewhere) {
                    // accept assignment
                    cout << "[RED] cube idx=" << idx << " assign pos=" << pos << " -> " << val_try << "\n";
                    // update cube and coverage & minterm_to_cubes
                    cube = cand;
                    // update coverage[idx]
                    vector<char> newcov(K,0);
                    for (size_t k = 0; k < K; ++k) {
                        if (cube.covers_minterm(on_set[k])) newcov[k] = 1;
                    }
                    // update minterm_to_cubes: for any k where coverage[idx][k] was 1 but newcov[k]==0, remove idx from list
                    for (size_t k = 0; k < K; ++k) {
                        if (coverage[idx][k] && !newcov[k]) {
                            auto &vec = minterm_to_cubes[k];
                            auto it = find(vec.begin(), vec.end(), idx);
                            if (it != vec.end()) vec.erase(it);
                        } else if (!coverage[idx][k] && newcov[k]) {
                            minterm_to_cubes[k].push_back(idx);
                        }
                    }
                    coverage[idx].swap(newcov);
                    // update orig_covered to new listing
                    orig_covered.clear();
                    for (size_t k = 0; k < K; ++k) if (coverage[idx][k]) orig_covered.push_back((int)k);
                    assigned = true;
                    changed_any = true;
                    break; // stop trying other val_try for this position; proceed to next DC position
                }
            } // end val_try loop
            // if assignment succeeded, continue to next dc pos; otherwise leave this pos as don't-care
            (void)assigned;
        } // end dc_positions loop

        if (changed_any) {
            cout << "[RED] cube idx=" << idx << " reduced -> " << cube.to_string() << "\n";
        }
    } // end per-cube

    // remove duplicates and return
    sort(result.begin(), result.end());
    result.erase(unique(result.begin(), result.end()), result.end());

    auto t1 = chrono::steady_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(t1 - t0).count();
    cout << "[RED] reduce_cover done. result size = " << result.size() << ". time = " << elapsed << " ms\n";
    return result;
}


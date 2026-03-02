#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <unordered_set>
#include <algorithm>
using namespace std;

// 隨機生成不重複的 minterm
vector<unsigned int> generateMinterms(int nbit, int count, const unordered_set<unsigned int>& exclude) {
    vector<unsigned int> result;
    unordered_set<unsigned int> used = exclude;

    while ((int)result.size() < count) {
        unsigned int m = rand() % (1u << nbit);
        if (used.find(m) == used.end()) {
            result.push_back(m);
            used.insert(m);
        }
    }
    sort(result.begin(), result.end()); // 排序
    return result;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <outputfile>\n";
        return 1;
    }

    srand(time(0));

    int nbit = 22;           // 變數數量
    int on_count = 2388408;       // ON set minterm 數量
    int dc_count = 434518;       // DC set minterm 數量

    unordered_set<unsigned int> used;
    vector<unsigned int> on_set = generateMinterms(nbit, on_count, used);
    for (unsigned int m : on_set) used.insert(m);

    vector<unsigned int> dc_set = generateMinterms(nbit, dc_count, used);

    ofstream fout(argv[1]);
    if (!fout.is_open()) {
        cerr << "Cannot open file: " << argv[1] << endl;
        return 1;
    }

    // 輸出格式
    fout << nbit << "\n";
    for (size_t i = 0; i < on_set.size(); ++i) {
        fout << on_set[i] << (i + 1 < on_set.size() ? " " : "\n");
    }
    for (size_t i = 0; i < dc_set.size(); ++i) {
        fout << dc_set[i] << (i + 1 < dc_set.size() ? " " : "\n");
    }

    fout.close();
    cout << "Test spec file generated (sorted): " << argv[1] << endl;

    return 0;
}
#include "../inc/util.h"

// ===================== Liberty =====================
class Table2D{
public:
    vector<double> idx1; // x: load
    vector<double> idx2; // y: input transition
    vector<vector<double>> v; // v[row(idx2)][col(idx1)]

    // find bounding indices for val in x
    static pair<int,int> boundIndex(const vector<double>& x, double val);

    // perform bilinear interpolation
    double bilinear(double load, double in_tr) const;
};
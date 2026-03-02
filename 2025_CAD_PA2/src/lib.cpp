#include "../inc/lib.h"

pair<int,int> Table2D::boundIndex(const vector<double>& x, double val){
    int n = (int)x.size();
    if(n == 0) return make_pair(-1, -1);    // empty
    if(n == 1) return make_pair(0, 0);  // single element
    if(val <= x.front()) return make_pair(0, 1);    // below range
    if(val >= x.back())  return make_pair(n-2, n-1);    // above range
    int lo = 0, hi = n - 1;
    while(lo + 1 < hi){
        int m = (lo + hi) / 2;
        if(x[m] <= val) lo = m;  // go right 
        else hi = m;  // go left
    }
    return make_pair(lo, lo + 1);   // find indices
}

double Table2D::bilinear(double load, double in_tr) const{
    if(idx1.empty() || idx2.empty() || v.empty()) 
        return 0.0;
    pair<int, int> bx = boundIndex(idx1, load);
    pair<int, int> by = boundIndex(idx2, in_tr);
    int x0 = bx.first, x1 = bx.second;
    int y0 = by.first, y1 = by.second;
    // if corners are the same, no interpolation needed
    if(x0 == x1 && y0 == y1)
        return v[y0][x0];
    // handle edge cases where only one dimension needs interpolation
    if(y0 == y1){
        double X0 = idx1[x0], X1 = idx1[x1];
        double f0 = v[y0][x0], f1 = v[y0][x1];
        if(X1 == X0) return f0;
        return f0 + (f1 - f0) * ((load - X0) / (X1 - X0));
    }
    if(x0 == x1){
        double Y0 = idx2[y0], Y1 = idx2[y1];
        double f0 = v[y0][x0], f1 = v[y1][x1];
        if(Y1 == Y0) return f0;
        return f0 + (f1-f0) * ((in_tr - Y0) / (Y1 - Y0));
    }
    // bilinear interpolation
    double X0 = idx1[x0], X1 = idx1[x1]; 
    double Y0 = idx2[y0], Y1 = idx2[y1];
    double Q11 = v[y0][x0], Q21 = v[y0][x1];
    double Q12 = v[y1][x0], Q22 = v[y1][x1];
    double tx = (X1 == X0) ? 0.0 : (load - X0) / (X1 - X0);
    double ty = (Y1 == Y0) ? 0.0 : (in_tr - Y0) / (Y1 - Y0);
    double R1 = Q11 * (1.0 - tx) + Q21 * tx;
    double R2 = Q12 * (1.0 - tx) + Q22 * tx;
    return R1 * (1.0 - ty) + R2 * ty;
}
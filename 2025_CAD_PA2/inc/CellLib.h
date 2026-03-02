#include "../inc/lib.h"

class CellLib{
public:
    struct Pin{ 
        string name, dir;
        double cap;
        Pin() : cap(0.0){}
    };
    struct Cell{
        string name;
        unordered_map<string,Pin> pins;
        string output_pin;
        Table2D cell_rise, cell_fall;
        Table2D rise_tran, fall_tran;
    };
    // ===== added: templates & library name =====
    struct TableTemplate{
        vector<double> idx1, idx2;
    };
    string library_name;                 // library(<name>) captured
    vector<double> idx1, idx2;           // keep as fallback indices
    unordered_map<string, TableTemplate> templates;  // lu_table_template name -> indices

    unordered_map<string, Cell> cells;

    const Cell* getCell(const string &t) const;     // find cell by name
    static string strip_comments(const string &s);  // remove comments(// or /* ... */) from a string
    static vector< vector<double> > parse_value_matrix(const string &blk, int cols, int rows);  // parse a matrix of values from values(...)
    void fill_table_from_block(const string &tblk, const string &name, Table2D &out);  // fill a Table2D from timing(){...}
// fill a Table2D from timing(){...}
    bool parse(const string &path);  // parse a .lib file
};
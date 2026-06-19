// ============================================================================
// M1 - filehandler.cpp
// ----------------------------------------------------------------------------
// Implementation of the generic text-file engine. Only <fstream>, <sstream>,
// <string> and <vector> are used here - no STL algorithms, no <map>, no class.
// ============================================================================

#include "filehandler.h"
#include <fstream>
#include <sstream>

using namespace std;

// ----------------------------------------------------------------------------
// Internal helper: should this field be wrapped in quotes when we write it?
// A field needs quoting if it contains a comma (otherwise the comma would look
// like a column separator when the file is read back).
// ----------------------------------------------------------------------------
static string quoteIfNeeded(const string& field) {
    bool needsQuotes = false;
    for (size_t i = 0; i < field.size(); i++) {
        if (field[i] == ',') {
            needsQuotes = true;
            break;
        }
    }
    if (needsQuotes) {
        return "\"" + field + "\"";   // e.g. Khan, Ahmed  ->  "Khan, Ahmed"
    }
    return field;
}

// ----------------------------------------------------------------------------
// readTXT - the heart of the module.
// We read the file ONE CHARACTER AT A TIME (the PDF forbids a getline-and-split
// approach) and build up fields and rows by hand. A boolean flag remembers
// whether we are currently inside a quoted field so that commas inside quotes
// are treated as ordinary text, not separators.
// ----------------------------------------------------------------------------
Table readTXT(const string& path) {
    Table rows;
    ifstream fin(path.c_str());
    if (!fin.is_open()) {
        return rows;   // file missing -> return an empty table, never crash
    }

    string field = "";     // characters of the field we are currently building
    Row current;           // fields of the row we are currently building
    bool inQuotes = false; // are we inside a "..." section?
    bool headerSkipped = false;
    bool sawAnything = false; // did the current line contain any character?
    char ch;

    while (fin.get(ch)) {
        if (ch == '\r') {
            continue;              // ignore Windows carriage returns entirely
        }

        if (ch == '"') {
            inQuotes = !inQuotes;  // flip in/out of quote mode, drop the quote
            sawAnything = true;
            continue;
        }

        if (ch == ',' && !inQuotes) {
            current.push_back(field);  // comma ends a field
            field = "";
            sawAnything = true;
        }
        else if (ch == '\n' && !inQuotes) {
            current.push_back(field);  // newline ends the last field + the row
            field = "";
            if (!headerSkipped) {
                headerSkipped = true;  // first line is the header -> discard it
            } else {
                rows.push_back(current);
            }
            current.clear();
            sawAnything = false;
        }
        else {
            field += ch;               // an ordinary character of the field
            sawAnything = true;
        }
    }

    // If the file did not end with a newline, the last row is still pending.
    if (sawAnything || !current.empty() || !field.empty()) {
        current.push_back(field);
        if (headerSkipped) {
            rows.push_back(current);
        }
    }

    fin.close();
    return rows;
}

// ----------------------------------------------------------------------------
// readHeader - grab just the first physical line (used before a rewrite).
// ----------------------------------------------------------------------------
string readHeader(const string& path) {
    ifstream fin(path.c_str());
    string header = "";
    if (!fin.is_open()) {
        return header;
    }
    char ch;
    while (fin.get(ch)) {
        if (ch == '\n') break;
        if (ch == '\r') continue;
        header += ch;
    }
    fin.close();
    return header;
}

// ----------------------------------------------------------------------------
// writeTXT - overwrite the whole file (header first, then every row).
// ----------------------------------------------------------------------------
void writeTXT(const string& path, const string& header, const Table& rows) {
    ofstream fout(path.c_str());   // opening in default mode truncates the file
    fout << header << "\n";
    for (size_t i = 0; i < rows.size(); i++) {
        for (size_t j = 0; j < rows[i].size(); j++) {
            if (j > 0) fout << ",";
            fout << quoteIfNeeded(rows[i][j]);
        }
        fout << "\n";
    }
    fout.close();
}

// ----------------------------------------------------------------------------
// appendTXT - add a single row to the end (used when enrolling, marking
// attendance, etc.). ios::app means "append" so the file is not reloaded.
// ----------------------------------------------------------------------------
void appendTXT(const string& path, const Row& row) {
    ofstream fout(path.c_str(), ios::app);
    for (size_t j = 0; j < row.size(); j++) {
        if (j > 0) fout << ",";
        fout << quoteIfNeeded(row[j]);
    }
    fout << "\n";
    fout.close();
}

// ----------------------------------------------------------------------------
// findRow - plain linear search down the column colIndex.
// ----------------------------------------------------------------------------
Row findRow(const string& path, int colIndex, const string& value) {
    Table rows = readTXT(path);
    for (size_t i = 0; i < rows.size(); i++) {
        if (colIndex < (int)rows[i].size() && rows[i][colIndex] == value) {
            return rows[i];
        }
    }
    return Row();   // empty -> "not found"
}

// ----------------------------------------------------------------------------
// rowExists - thin wrapper around findRow.
// ----------------------------------------------------------------------------
bool rowExists(const string& path, int colIndex, const string& value) {
    Row found = findRow(path, colIndex, value);
    return !found.empty();
}

// ----------------------------------------------------------------------------
// Numeric helpers. stringstream does the heavy lifting; if the text is not a
// valid number we simply return 0 so the caller never has to handle exceptions.
// ----------------------------------------------------------------------------
int toInt(const string& s) {
    stringstream ss(s);
    int value = 0;
    ss >> value;
    return value;
}

double toDouble(const string& s) {
    stringstream ss(s);
    double value = 0.0;
    ss >> value;
    return value;
}

string intToStr(int value) {
    stringstream ss;
    ss << value;
    return ss.str();
}

string doubleToStr(double value, int decimals) {
    stringstream ss;
    ss.setf(ios::fixed);
    ss.precision(decimals);
    ss << value;
    return ss.str();
}

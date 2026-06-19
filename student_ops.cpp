// ============================================================================
// M2 - student_ops.cpp
// ----------------------------------------------------------------------------
// Student record management built on top of the M1 file engine.
// ============================================================================

#include "student_ops.h"
#include <iostream>
#include <iomanip>

using namespace std;

// ----------------------------------------------------------------------------
// Small private string helpers (kept here because only this module needs them).
// ----------------------------------------------------------------------------
static char lowerChar(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

static string toLowerStr(const string& s) {
    string out = "";
    for (size_t i = 0; i < s.size(); i++) {
        out += lowerChar(s[i]);
    }
    return out;
}

// Does 'haystack' contain 'needle'? Written by hand (no std::find / no
// string::find) using a sliding window of substr comparisons.
static bool containsSubstring(const string& haystack, const string& needle) {
    if (needle.empty()) return true;
    if (needle.size() > haystack.size()) return false;
    for (size_t start = 0; start + needle.size() <= haystack.size(); start++) {
        bool match = true;
        for (size_t k = 0; k < needle.size(); k++) {
            if (haystack[start + k] != needle[k]) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

static bool isDigitChar(char c) {
    return c >= '0' && c <= '9';
}

// ----------------------------------------------------------------------------
// Row <-> Student conversions.
// ----------------------------------------------------------------------------
Student rowToStudent(const Row& r) {
    Student s;
    s.roll       = r[STU_ROLL];
    s.name       = r[STU_NAME];
    s.department = r[STU_DEPT];
    s.semester   = toInt(r[STU_SEM]);
    s.cgpa       = toDouble(r[STU_CGPA]);
    s.status     = r[STU_STATUS];
    return s;
}

Row studentToRow(const Student& s) {
    Row r;
    r.push_back(s.roll);
    r.push_back(s.name);
    r.push_back(s.department);
    r.push_back(intToStr(s.semester));
    r.push_back(doubleToStr(s.cgpa, 2));
    r.push_back(s.status);
    return r;
}

// ----------------------------------------------------------------------------
// Validation: roll must look exactly like  BSAI-YY-XXX
//   positions: 0123456789 10
//              B S A I - d d - d d d
// We check the length, the fixed letters/dashes, and that the YY and XXX parts
// are digits. substr is used to pull out each piece, as the PDF requires.
// ----------------------------------------------------------------------------
bool isValidRollFormat(const string& roll) {
    if (roll.size() != 11) return false;          // BSAI-YY-XXX is 11 chars
    if (roll.substr(0, 5) != "BSAI-") return false;
    if (roll[7] != '-') return false;

    string yy  = roll.substr(5, 2);   // the two year digits
    string xxx = roll.substr(8, 3);   // the three serial digits

    for (size_t i = 0; i < yy.size(); i++) {
        if (!isDigitChar(yy[i])) return false;
    }
    for (size_t i = 0; i < xxx.size(); i++) {
        if (!isDigitChar(xxx[i])) return false;
    }
    return true;
}

bool nameHasNoDigits(const string& name) {
    for (size_t i = 0; i < name.size(); i++) {
        if (isDigitChar(name[i])) return false;
    }
    return true;
}

bool isValidCgpa(double cgpa) {
    return cgpa >= 0.0 && cgpa <= 4.0;
}

// ----------------------------------------------------------------------------
// addStudent: validate, reject duplicates, then append a single row.
// ----------------------------------------------------------------------------
bool addStudent(const Student& s) {
    if (!isValidRollFormat(s.roll)) {
        cout << "  [X] Roll number must be in the format BSAI-YY-XXX.\n";
        return false;
    }
    if (rowExists(STUDENTS_FILE, STU_ROLL, s.roll)) {
        cout << "  [X] A student with roll " << s.roll << " already exists.\n";
        return false;
    }
    if (s.name.empty() || !nameHasNoDigits(s.name)) {
        cout << "  [X] Name must be non-empty and contain no digits.\n";
        return false;
    }
    if (!isValidCgpa(s.cgpa)) {
        cout << "  [X] CGPA must be between 0.0 and 4.0.\n";
        return false;
    }

    appendTXT(STUDENTS_FILE, studentToRow(s));
    cout << "  [OK] Student " << s.roll << " added successfully.\n";
    return true;
}

// ----------------------------------------------------------------------------
// searchByRoll: single linear lookup via M1.
// ----------------------------------------------------------------------------
Student searchByRoll(const string& roll) {
    Row r = findRow(STUDENTS_FILE, STU_ROLL, roll);
    if (r.empty()) {
        Student empty;            // empty.roll == "" signals "not found"
        empty.roll = "";
        return empty;
    }
    return rowToStudent(r);
}

// ----------------------------------------------------------------------------
// searchByName: collect every row whose name contains the text.
// ----------------------------------------------------------------------------
vector<Student> searchByName(const string& part) {
    vector<Student> results;
    Table rows = readTXT(STUDENTS_FILE);
    string needle = toLowerStr(part);
    for (size_t i = 0; i < rows.size(); i++) {
        string hay = toLowerStr(rows[i][STU_NAME]);
        if (containsSubstring(hay, needle)) {
            results.push_back(rowToStudent(rows[i]));
        }
    }
    return results;
}

// ----------------------------------------------------------------------------
// updateStudent: load -> find -> change one field -> write back.
// The roll column (index 0) is protected from editing.
// ----------------------------------------------------------------------------
bool updateStudent(const string& roll, int colIndex, const string& newValue) {
    if (colIndex == STU_ROLL) {
        cout << "  [X] The roll number cannot be changed.\n";
        return false;
    }
    Table rows = readTXT(STUDENTS_FILE);
    bool found = false;
    for (size_t i = 0; i < rows.size(); i++) {
        if (rows[i][STU_ROLL] == roll) {
            rows[i][colIndex] = newValue;
            found = true;
            break;
        }
    }
    if (!found) {
        cout << "  [X] Roll " << roll << " not found.\n";
        return false;
    }
    writeTXT(STUDENTS_FILE, readHeader(STUDENTS_FILE), rows);
    return true;
}

// ----------------------------------------------------------------------------
// softDelete: reuse updateStudent to set the status column.
// ----------------------------------------------------------------------------
bool softDelete(const string& roll) {
    return updateStudent(roll, STU_STATUS, "inactive");
}

// ----------------------------------------------------------------------------
// listActiveStudents: gather active students then SELECTION SORT by roll.
// Selection sort: repeatedly find the smallest remaining roll and swap it to
// the front of the unsorted part.
// ----------------------------------------------------------------------------
vector<Student> listActiveStudents() {
    vector<Student> active;
    Table rows = readTXT(STUDENTS_FILE);
    for (size_t i = 0; i < rows.size(); i++) {
        if (rows[i][STU_STATUS] == "active") {
            active.push_back(rowToStudent(rows[i]));
        }
    }

    for (size_t i = 0; i < active.size(); i++) {
        size_t minIndex = i;
        for (size_t j = i + 1; j < active.size(); j++) {
            if (active[j].roll < active[minIndex].roll) {
                minIndex = j;
            }
        }
        if (minIndex != i) {
            Student temp     = active[i];
            active[i]        = active[minIndex];
            active[minIndex] = temp;
        }
    }
    return active;
}

// ----------------------------------------------------------------------------
// Bonus: search-as-you-type.
// The user types characters; after EACH keystroke we clear the screen-ish and
// reprint the active students whose roll or (lower-cased) name contains the
// running query. Typing '#' finishes. Backspace handling keeps it usable.
// ----------------------------------------------------------------------------
void searchAsYouType() {
    vector<Student> active = listActiveStudents();
    string query = "";

    cout << "\n  -- Live search (type letters; '#' to finish) --\n";
    cout << "  Query: \n";

    char ch;
    while (cin.get(ch)) {
        if (ch == '#') break;
        if (ch == '\n') continue;          // ignore the Enter key itself

        if (ch == 8 || ch == 127) {        // backspace / delete
            if (!query.empty()) query = query.substr(0, query.size() - 1);
        } else {
            query += lowerChar(ch);
        }

        cout << "\n  Query: " << query << "\n";
        cout << "  ----------------------------------------\n";
        int shown = 0;
        for (size_t i = 0; i < active.size(); i++) {
            string roll = toLowerStr(active[i].roll);
            string name = toLowerStr(active[i].name);
            if (containsSubstring(roll, query) || containsSubstring(name, query)) {
                cout << "   " << left << setw(14) << active[i].roll
                     << active[i].name << "\n";
                shown++;
            }
        }
        if (shown == 0) cout << "   (no matches)\n";
    }
    cout << "  -- Live search ended --\n";
}

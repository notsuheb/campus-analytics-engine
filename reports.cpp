// ============================================================================
// M7 - reports.cpp
// ----------------------------------------------------------------------------
// Formatted reports. These functions only READ data through the other modules
// and print tidy tables; they never modify any file (except exportReportToFile,
// which writes a copy of a report to disk).
// ============================================================================

#include "reports.h"
#include "filehandler.h"
#include "student_ops.h"
#include "course_ops.h"
#include "attendance.h"
#include "grades.h"
#include "fee_tracker.h"
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace std;

// ----------------------------------------------------------------------------
// printMeritList: copy the active students, bubble sort by CGPA descending,
// then print with a rank column.
// ----------------------------------------------------------------------------
void printMeritList() {
    vector<Student> students = listActiveStudents();   // already roll-sorted

    // bubble sort by CGPA, highest first
    for (size_t i = 0; i + 1 < students.size(); i++) {
        for (size_t j = 0; j + 1 < students.size() - i; j++) {
            if (students[j].cgpa < students[j + 1].cgpa) {
                Student t       = students[j];
                students[j]     = students[j + 1];
                students[j + 1] = t;
            }
        }
    }

    cout << "\n  MERIT LIST (active students by CGPA)\n";
    cout << "  " << string(48, '=') << "\n";
    cout << "  " << left << setw(6) << "Rank" << setw(14) << "Roll"
         << setw(22) << "Name" << "CGPA\n";
    cout << "  " << string(48, '-') << "\n";

    for (size_t i = 0; i < students.size(); i++) {
        cout << "  " << left << setw(6) << (intToStr((int)i + 1))
             << setw(14) << students[i].roll
             << setw(22) << students[i].name
             << fixed << setprecision(2) << students[i].cgpa << "\n";
    }
    cout << "  " << string(48, '=') << "\n";
}

// ----------------------------------------------------------------------------
// printAttendanceDefaulters: reuse M4's getShortageList.
// ----------------------------------------------------------------------------
void printAttendanceDefaulters() {
    vector<ShortageRecord> list = getShortageList();

    cout << "\n  ATTENDANCE DEFAULTERS (below 75%)\n";
    cout << "  " << string(54, '=') << "\n";
    cout << "  " << left << setw(14) << "Roll" << setw(22) << "Name"
         << setw(8) << "Course" << "Pct\n";
    cout << "  " << string(54, '-') << "\n";

    if (list.empty()) {
        cout << "   (none)\n";
    }
    for (size_t i = 0; i < list.size(); i++) {
        cout << "  " << left << setw(14) << list[i].roll
             << setw(22) << list[i].name
             << setw(8) << list[i].course
             << fixed << setprecision(1) << list[i].percentage << "%\n";
    }
    cout << "  " << string(54, '=') << "\n";
}

// ----------------------------------------------------------------------------
// printFeeDefaulters: reuse M6's getDefaulters (already bubble-sorted).
// ----------------------------------------------------------------------------
void printFeeDefaulters() {
    vector<Defaulter> list = getDefaulters();

    cout << "\n  FEE DEFAULTERS (outstanding balance)\n";
    cout << "  " << string(56, '=') << "\n";
    cout << "  " << left << setw(14) << "Roll" << setw(22) << "Name"
         << setw(14) << "Outstanding" << "Weeks\n";
    cout << "  " << string(56, '-') << "\n";

    if (list.empty()) {
        cout << "   (none)\n";
    }
    for (size_t i = 0; i < list.size(); i++) {
        cout << "  " << left << setw(14) << list[i].roll
             << setw(22) << list[i].name
             << setw(14) << ("Rs " + doubleToStr(list[i].outstanding, 0))
             << list[i].weeksOverdue << "\n";
    }
    cout << "  " << string(56, '=') << "\n";
}

// ----------------------------------------------------------------------------
// printSemesterResult: one line per active student with GPA + attendance flag.
// ----------------------------------------------------------------------------
void printSemesterResult(int semester) {
    vector<Student> students = listActiveStudents();

    cout << "\n  SEMESTER " << semester << " RESULT SHEET\n";
    cout << "  +" << string(58, '-') << "+\n";
    cout << "  | " << left << setw(13) << "Roll" << setw(22) << "Name"
         << setw(8) << "GPA" << setw(15) << "Attendance" << "|\n";
    cout << "  +" << string(58, '-') << "+\n";

    for (size_t i = 0; i < students.size(); i++) {
        double gpa = computeGPA(students[i].roll, semester);

        // attendance status = "Short" if the student is below 75% anywhere
        string attStatus = "OK";
        Table enrolls = readTXT(ENROLLMENTS_FILE);
        for (size_t e = 0; e < enrolls.size(); e++) {
            if (enrolls[e][ENR_ROLL] == students[i].roll &&
                enrolls[e][ENR_STATUS] == "active") {
                double pct = getAttendancePct(students[i].roll, enrolls[e][ENR_CODE]);
                // only count courses that actually have sessions
                if (pct > 0.0 && pct < 75.0) { attStatus = "Short"; break; }
            }
        }

        cout << "  | " << left << setw(13) << students[i].roll
             << setw(22) << students[i].name
             << setw(8) << doubleToStr(gpa, 2)
             << setw(15) << attStatus << "|\n";
    }
    cout << "  +" << string(58, '-') << "+\n";
}

// ----------------------------------------------------------------------------
// printDepartmentSummary: group students by department using PARALLEL ARRAYS
// (the project forbids std::map, so we manage the grouping by hand).
// ----------------------------------------------------------------------------
void printDepartmentSummary() {
    Table rows = readTXT(STUDENTS_FILE);

    // parallel arrays: index i describes one department
    vector<string> deptNames;
    vector<int>    counts;
    vector<double> cgpaSums;
    vector<int>    passCounts;

    for (size_t i = 0; i < rows.size(); i++) {
        if (rows[i][STU_STATUS] != "active") continue;

        string dept = rows[i][STU_DEPT];
        double cgpa = toDouble(rows[i][STU_CGPA]);

        // find this department in our parallel arrays (linear search)
        int idx = -1;
        for (size_t d = 0; d < deptNames.size(); d++) {
            if (deptNames[d] == dept) { idx = (int)d; break; }
        }
        if (idx == -1) {                 // new department -> add a slot
            deptNames.push_back(dept);
            counts.push_back(0);
            cgpaSums.push_back(0.0);
            passCounts.push_back(0);
            idx = (int)deptNames.size() - 1;
        }

        counts[idx]   += 1;
        cgpaSums[idx] += cgpa;
        if (cgpa >= PASS_CGPA) passCounts[idx] += 1;
    }

    cout << "\n  DEPARTMENT SUMMARY\n";
    cout << "  " << string(58, '=') << "\n";
    cout << "  " << left << setw(28) << "Department" << setw(8) << "Count"
         << setw(12) << "Avg CGPA" << "Pass %\n";
    cout << "  " << string(58, '-') << "\n";

    for (size_t d = 0; d < deptNames.size(); d++) {
        double avg  = counts[d] ? cgpaSums[d] / counts[d] : 0.0;
        double pass = counts[d] ? (100.0 * passCounts[d] / counts[d]) : 0.0;
        cout << "  " << left << setw(28) << deptNames[d]
             << setw(8)  << counts[d]
             << setw(12) << doubleToStr(avg, 2)
             << doubleToStr(pass, 1) << "%\n";
    }
    cout << "  " << string(58, '=') << "\n";
}

// ----------------------------------------------------------------------------
// exportReportToFile: temporarily point cout at a file, run a report, then
// restore cout. This is the classic rdbuf() redirect trick.
// ----------------------------------------------------------------------------
void exportReportToFile(const string& filename, int choice, int semester) {
    ofstream fout(filename.c_str());
    if (!fout.is_open()) {
        cout << "  [X] Could not open " << filename << " for writing.\n";
        return;
    }

    streambuf* originalCout = cout.rdbuf();   // remember the real screen buffer
    cout.rdbuf(fout.rdbuf());                 // send cout to the file instead

    switch (choice) {
        case 1: printMeritList();            break;
        case 2: printAttendanceDefaulters(); break;
        case 3: printFeeDefaulters();        break;
        case 4: printSemesterResult(semester); break;
        case 5: printDepartmentSummary();    break;
        default: cout << "Unknown report choice.\n"; break;
    }

    cout.rdbuf(originalCout);                  // put cout back on the screen
    fout.close();
    cout << "  [OK] Report written to " << filename << "\n";
}

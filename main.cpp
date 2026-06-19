// ============================================================================
// M0 - main.cpp
// ----------------------------------------------------------------------------
// Entry point and the 3-level nested menu:
//   Level 1: the main menu (pick a module)
//   Level 2: that module's menu (pick an action)
//   Level 3: some actions ask a further question (which field / which report)
//
// main.cpp contains NO business logic of its own - every option simply calls a
// function from one of the seven modules. That is the cross-file call structure
// the project requires.
// ============================================================================

#include <iostream>
#include <string>
#include <limits>

#include "filehandler.h"
#include "student_ops.h"
#include "course_ops.h"
#include "attendance.h"
#include "grades.h"
#include "fee_tracker.h"
#include "reports.h"

using namespace std;

// ----------------------------------------------------------------------------
// Small input helpers so the menu code stays readable.
// ----------------------------------------------------------------------------
static int readInt(const string& prompt) {
    int value;
    while (true) {
        cout << prompt;
        if (cin >> value) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
        cout << "  Please enter a whole number.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

static string readLine(const string& prompt) {
    cout << prompt;
    string line;
    getline(cin, line);
    return line;
}

static double readDouble(const string& prompt) {
    double value;
    while (true) {
        cout << prompt;
        if (cin >> value) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
        cout << "  Please enter a number.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

// ============================================================================
// LEVEL 2 MENUS - one function per module
// ============================================================================

// ---- M2: Student Management ------------------------------------------------
static void studentMenu() {
    while (true) {
        cout << "\n  -- Student Management --\n"
             << "   1. Add student\n"
             << "   2. Search by roll\n"
             << "   3. Search by name\n"
             << "   4. Update student\n"
             << "   5. Soft-delete student\n"
             << "   6. List active students\n"
             << "   7. Live search (search-as-you-type)\n"
             << "   0. Back\n";
        int choice = readInt("  Choice: ");

        if (choice == 0) return;
        else if (choice == 1) {
            Student s;
            s.roll       = readLine("  Roll (BSAI-YY-XXX): ");
            s.name       = readLine("  Name: ");
            s.department = readLine("  Department: ");
            s.semester   = readInt("  Semester: ");
            s.cgpa       = readDouble("  CGPA (0.0-4.0): ");
            s.status     = "active";
            addStudent(s);
        }
        else if (choice == 2) {
            Student s = searchByRoll(readLine("  Roll: "));
            if (s.roll.empty()) cout << "  Not found.\n";
            else cout << "  " << s.roll << " | " << s.name << " | " << s.department
                      << " | sem " << s.semester << " | CGPA " << s.cgpa
                      << " | " << s.status << "\n";
        }
        else if (choice == 3) {
            vector<Student> r = searchByName(readLine("  Name contains: "));
            cout << "  " << r.size() << " match(es):\n";
            for (size_t i = 0; i < r.size(); i++)
                cout << "   " << r[i].roll << "  " << r[i].name << "\n";
        }
        else if (choice == 4) {
            string roll = readLine("  Roll to update: ");
            // LEVEL 3: choose which field
            cout << "   1. Name  2. Department  3. Semester  4. CGPA  5. Status\n";
            int f = readInt("  Field: ");
            int col = -1;
            if (f == 1) col = STU_NAME;
            else if (f == 2) col = STU_DEPT;
            else if (f == 3) col = STU_SEM;
            else if (f == 4) col = STU_CGPA;
            else if (f == 5) col = STU_STATUS;
            if (col == -1) { cout << "  Invalid field.\n"; continue; }
            string val = readLine("  New value: ");
            if (updateStudent(roll, col, val)) cout << "  [OK] Updated.\n";
        }
        else if (choice == 5) {
            if (softDelete(readLine("  Roll to soft-delete: ")))
                cout << "  [OK] Marked inactive.\n";
        }
        else if (choice == 6) {
            vector<Student> a = listActiveStudents();
            cout << "  " << a.size() << " active students:\n";
            for (size_t i = 0; i < a.size(); i++)
                cout << "   " << a[i].roll << "  " << a[i].name << "\n";
        }
        else if (choice == 7) {
            searchAsYouType();
        }
        else cout << "  Invalid choice.\n";
    }
}

// ---- M3: Course Management -------------------------------------------------
static void courseMenu() {
    while (true) {
        cout << "\n  -- Course Management --\n"
             << "   1. Enroll student in a course\n"
             << "   2. Drop a course\n"
             << "   3. Show credit load\n"
             << "   4. Check prerequisite\n"
             << "   5. List enrolled students\n"
             << "   0. Back\n";
        int choice = readInt("  Choice: ");

        if (choice == 0) return;
        else if (choice == 1) {
            string roll = readLine("  Roll: ");
            string code = readLine("  Course code: ");
            int sem     = readInt("  Semester: ");
            EnrollResult r = enrollStudent(roll, code, sem);
            cout << "  " << enrollResultMessage(r) << "\n";
        }
        else if (choice == 2) {
            string roll = readLine("  Roll: ");
            string code = readLine("  Course code: ");
            if (dropCourse(roll, code)) cout << "  [OK] Course dropped.\n";
        }
        else if (choice == 3) {
            string roll = readLine("  Roll: ");
            int sem     = readInt("  Semester: ");
            cout << "  Credit load: " << getCreditLoad(roll, sem) << " hours\n";
        }
        else if (choice == 4) {
            string roll = readLine("  Roll: ");
            string code = readLine("  Course code: ");
            cout << "  Prerequisite "
                 << (checkPrerequisite(roll, code) ? "satisfied." : "NOT satisfied.")
                 << "\n";
        }
        else if (choice == 5) {
            vector<Student> e = listEnrolledStudents(readLine("  Course code: "));
            cout << "  " << e.size() << " enrolled:\n";
            for (size_t i = 0; i < e.size(); i++)
                cout << "   " << e[i].roll << "  " << e[i].name << "\n";
        }
        else cout << "  Invalid choice.\n";
    }
}

// ---- M4: Attendance --------------------------------------------------------
static void attendanceMenu() {
    while (true) {
        cout << "\n  -- Attendance --\n"
             << "   1. Mark attendance for a session\n"
             << "   2. View a student's attendance %\n"
             << "   3. Shortage list (below 75%)\n"
             << "   4. Undo last marked session\n"
             << "   5. Daily sheet for a date\n"
             << "   0. Back\n";
        int choice = readInt("  Choice: ");

        if (choice == 0) return;
        else if (choice == 1) {
            string code = readLine("  Course code: ");
            string date = readLine("  Date (DD-MM-YYYY): ");
            markAttendance(code, date);
        }
        else if (choice == 2) {
            string roll = readLine("  Roll: ");
            string code = readLine("  Course code: ");
            cout << "  Attendance: " << getAttendancePct(roll, code) << "%\n";
        }
        else if (choice == 3) {
            vector<ShortageRecord> s = getShortageList();
            cout << "  " << s.size() << " shortage record(s):\n";
            for (size_t i = 0; i < s.size(); i++)
                cout << "   " << s[i].roll << "  " << s[i].course
                     << "  " << s[i].percentage << "%\n";
        }
        else if (choice == 4) {
            undoLastSession();
        }
        else if (choice == 5) {
            string code = readLine("  Course code: ");
            string date = readLine("  Date (DD-MM-YYYY): ");
            printDailySheet(code, date);
        }
        else cout << "  Invalid choice.\n";
    }
}

// ---- M5: Grades ------------------------------------------------------------
static void gradesMenu() {
    while (true) {
        cout << "\n  -- Grades --\n"
             << "   1. Enter marks for a student\n"
             << "   2. View semester GPA\n"
             << "   3. Class statistics for a course\n"
             << "   0. Back\n";
        int choice = readInt("  Choice: ");

        if (choice == 0) return;
        else if (choice == 1) {
            string roll = readLine("  Roll: ");
            string code = readLine("  Course code: ");
            int sem     = readInt("  Semester: ");
            enterMarks(roll, code, sem);
        }
        else if (choice == 2) {
            string roll = readLine("  Roll: ");
            int sem     = readInt("  Semester: ");
            cout << "  GPA: " << computeGPA(roll, sem) << "\n";
        }
        else if (choice == 3) {
            Stats st = computeClassState(readLine("  Course code: "));
            cout << "  Highest: " << st.highest << "  Lowest: " << st.lowest
                 << "  Mean: " << st.mean << "  Median: " << st.median << "\n";
        }
        else cout << "  Invalid choice.\n";
    }
}

// ---- M6: Fees --------------------------------------------------------------
static void feesMenu() {
    while (true) {
        cout << "\n  -- Fees --\n"
             << "   1. Record a payment\n"
             << "   2. Compute late fine\n"
             << "   3. Generate receipt\n"
             << "   4. Fee defaulters\n"
             << "   0. Back\n";
        int choice = readInt("  Choice: ");

        if (choice == 0) return;
        else if (choice == 1) {
            string roll  = readLine("  Roll: ");
            double amt   = readDouble("  Amount: ");
            string date  = readLine("  Payment date (DD-MM-YYYY): ");
            string method= readLine("  Method (Cash/Online/Bank): ");
            recordPayment(roll, amt, date, method);
        }
        else if (choice == 2) {
            cout << "  Late fine: Rs "
                 << computeLateFine(readLine("  Roll: ")) << "\n";
        }
        else if (choice == 3) {
            generateReceipt(readLine("  Roll: "));
        }
        else if (choice == 4) {
            printFeeDefaulters();
        }
        else cout << "  Invalid choice.\n";
    }
}

// ---- M7: Reports -----------------------------------------------------------
static void reportsMenu() {
    while (true) {
        cout << "\n  -- Reports --\n"
             << "   1. Merit list\n"
             << "   2. Attendance defaulters\n"
             << "   3. Fee defaulters\n"
             << "   4. Semester result sheet\n"
             << "   5. Department summary\n"
             << "   6. Export a report to a file\n"
             << "   0. Back\n";
        int choice = readInt("  Choice: ");

        if (choice == 0) return;
        else if (choice == 1) printMeritList();
        else if (choice == 2) printAttendanceDefaulters();
        else if (choice == 3) printFeeDefaulters();
        else if (choice == 4) printSemesterResult(readInt("  Semester: "));
        else if (choice == 5) printDepartmentSummary();
        else if (choice == 6) {
            // LEVEL 3: which report, and to which file
            cout << "   1. Merit  2. Attendance  3. Fees  4. Result  5. Dept\n";
            int which = readInt("  Report to export: ");
            int sem = 2;
            if (which == 4) sem = readInt("  Semester: ");
            string file = readLine("  Output filename (e.g. merit.txt): ");
            exportReportToFile(file, which, sem);
        }
        else cout << "  Invalid choice.\n";
    }
}

// ============================================================================
// LEVEL 1 - the main menu loop
// ============================================================================
int main() {
    cout << "\n====================================================\n";
    cout << "        CAMPUS ANALYTICS ENGINE  (BS AI)\n";
    cout << "====================================================\n";

    while (true) {
        cout << "\n  ===== MAIN MENU =====\n"
             << "   1. Student Management\n"
             << "   2. Course Management\n"
             << "   3. Attendance\n"
             << "   4. Grades\n"
             << "   5. Fees\n"
             << "   6. Reports\n"
             << "   0. Exit\n";
        int choice = readInt("  Choice: ");

        if (choice == 0) {
            cout << "  Goodbye!\n";
            break;
        }
        else if (choice == 1) studentMenu();
        else if (choice == 2) courseMenu();
        else if (choice == 3) attendanceMenu();
        else if (choice == 4) gradesMenu();
        else if (choice == 5) feesMenu();
        else if (choice == 6) reportsMenu();
        else cout << "  Invalid choice.\n";
    }
    return 0;
}

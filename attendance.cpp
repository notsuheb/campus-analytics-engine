// ============================================================================
// M4 - attendance.cpp
// ----------------------------------------------------------------------------
// Attendance marking + analytics. The "undo" feature keeps an in-memory
// snapshot of the file taken just before the last marking session.
// ============================================================================

#include "attendance.h"
#include "course_ops.h"
#include "student_ops.h"
#include <iostream>
#include <iomanip>

using namespace std;

// The pre-session snapshot used by undoLastSession. It lives for the whole run
// of the program (a "static" file-scope variable), which is exactly the
// behaviour the spec wants: mark a session, then optionally undo it.
static Table g_attendanceBackup;
static bool  g_hasBackup = false;

// The 75% threshold appears in several places, so name it once.
static const double SHORTAGE_THRESHOLD = 75.0;

// ----------------------------------------------------------------------------
// Private helper: next attendance log id (L00001, L00002, ...).
// ----------------------------------------------------------------------------
static string nextLogId() {
    Table rows = readTXT(ATTENDANCE_FILE);
    int maxNum = 0;
    for (size_t i = 0; i < rows.size(); i++) {
        int n = toInt(rows[i][ATT_LOGID].substr(1));   // drop the leading 'L'
        if (n > maxNum) maxNum = n;
    }
    int next = maxNum + 1;
    string digits = intToStr(next);
    while (digits.size() < 5) digits = "0" + digits;
    return "L" + digits;
}

// Private helper: how many sessions are recorded for this student+course.
static int sessionCount(const string& roll, const string& courseCode) {
    Table rows = readTXT(ATTENDANCE_FILE);
    int count = 0;
    for (size_t i = 0; i < rows.size(); i++) {
        if (rows[i][ATT_ROLL] == roll && rows[i][ATT_CODE] == courseCode) {
            count++;
        }
    }
    return count;
}

// ----------------------------------------------------------------------------
// getAttendancePct: classic accumulator loop.
//   P -> +1.0,  L -> +0.5,  A -> +0.0
// then divide by the number of sessions and scale to a percentage.
// ----------------------------------------------------------------------------
double getAttendancePct(const string& roll, const string& courseCode) {
    Table rows = readTXT(ATTENDANCE_FILE);
    double credit = 0.0;
    int    total  = 0;

    for (size_t i = 0; i < rows.size(); i++) {
        if (rows[i][ATT_ROLL] == roll && rows[i][ATT_CODE] == courseCode) {
            string st = rows[i][ATT_STATUS];
            if (st == "P")      credit += 1.0;
            else if (st == "L") credit += 0.5;
            // "A" adds nothing
            total++;
        }
    }

    if (total == 0) return 0.0;
    return (credit / total) * 100.0;
}

// ----------------------------------------------------------------------------
// getShortageList: for every ACTIVE enrollment, compute the percentage and
// keep the ones below 75% (only when the student actually has sessions).
// ----------------------------------------------------------------------------
vector<ShortageRecord> getShortageList() {
    vector<ShortageRecord> shortages;
    Table enrolls = readTXT(ENROLLMENTS_FILE);

    for (size_t i = 0; i < enrolls.size(); i++) {
        if (enrolls[i][ENR_STATUS] != "active") continue;

        string roll = enrolls[i][ENR_ROLL];
        string code = enrolls[i][ENR_CODE];

        if (sessionCount(roll, code) == 0) continue;   // no data -> skip

        double pct = getAttendancePct(roll, code);
        if (pct < SHORTAGE_THRESHOLD) {
            ShortageRecord rec;
            rec.roll       = roll;
            rec.course     = code;
            rec.percentage = pct;
            Student s      = searchByRoll(roll);
            rec.name       = s.roll.empty() ? "(unknown)" : s.name;
            shortages.push_back(rec);
        }
    }
    return shortages;
}

// ----------------------------------------------------------------------------
// markAttendance: snapshot first, then prompt P/A/L for each enrolled student
// and append one row per student.
// ----------------------------------------------------------------------------
void markAttendance(const string& courseCode, const string& date) {
    vector<Student> enrolled = listEnrolledStudents(courseCode);
    if (enrolled.empty()) {
        cout << "  [!] No active students are enrolled in " << courseCode << ".\n";
        return;
    }

    // Take the pre-session snapshot so the whole session can be undone.
    g_attendanceBackup = readTXT(ATTENDANCE_FILE);
    g_hasBackup = true;

    cout << "\n  Marking " << courseCode << " for " << date
         << "  (enter P = present, A = absent, L = late)\n";

    for (size_t i = 0; i < enrolled.size(); i++) {
        string status = "";
        // keep asking until a valid code is given
        while (status != "P" && status != "A" && status != "L") {
            cout << "   " << left << setw(14) << enrolled[i].roll
                 << setw(20) << enrolled[i].name << " : ";
            cin >> status;
            if (status == "p") status = "P";
            if (status == "a") status = "A";
            if (status == "l") status = "L";
            if (status != "P" && status != "A" && status != "L") {
                cout << "   Please type P, A or L.\n";
            }
        }

        Row r;
        r.push_back(nextLogId());
        r.push_back(enrolled[i].roll);
        r.push_back(courseCode);
        r.push_back(date);
        r.push_back(status);
        appendTXT(ATTENDANCE_FILE, r);
    }
    cout << "  [OK] Attendance saved for " << enrolled.size() << " students.\n";
}

// ----------------------------------------------------------------------------
// undoLastSession: rewrite the file from the snapshot, then clear it so the
// same session cannot be undone twice.
// ----------------------------------------------------------------------------
bool undoLastSession() {
    if (!g_hasBackup) {
        cout << "  [X] Nothing to undo (no session marked in this run).\n";
        return false;
    }
    writeTXT(ATTENDANCE_FILE, readHeader(ATTENDANCE_FILE), g_attendanceBackup);
    g_hasBackup = false;
    g_attendanceBackup.clear();
    cout << "  [OK] Last attendance session was undone.\n";
    return true;
}

// ----------------------------------------------------------------------------
// printDailySheet: show each enrolled student's mark on a specific date.
// ----------------------------------------------------------------------------
void printDailySheet(const string& courseCode, const string& date) {
    vector<Student> enrolled = listEnrolledStudents(courseCode);
    Table att = readTXT(ATTENDANCE_FILE);

    cout << "\n  Daily attendance sheet - " << courseCode
         << "  (" << date << ")\n";
    cout << "  " << string(46, '-') << "\n";
    cout << "  " << left << setw(14) << "Roll" << setw(24) << "Name"
         << "Status\n";
    cout << "  " << string(46, '-') << "\n";

    for (size_t i = 0; i < enrolled.size(); i++) {
        string mark = "-";   // default if no record for that date
        for (size_t j = 0; j < att.size(); j++) {
            if (att[j][ATT_ROLL] == enrolled[i].roll &&
                att[j][ATT_CODE] == courseCode &&
                att[j][ATT_DATE] == date) {
                mark = att[j][ATT_STATUS];
                break;
            }
        }
        cout << "  " << left << setw(14) << enrolled[i].roll
             << setw(24) << enrolled[i].name << mark << "\n";
    }
    cout << "  " << string(46, '-') << "\n";
}

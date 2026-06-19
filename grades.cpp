// ============================================================================
// M5 - grades.cpp
// ----------------------------------------------------------------------------
// Grade calculations. No sorting library is used: the median is found with a
// hand-written selection sort, and best-three-of-five with a min-finding loop.
// ============================================================================

#include "grades.h"
#include "filehandler.h"
#include "student_ops.h"
#include "course_ops.h"
#include "attendance.h"
#include <iostream>
#include <iomanip>

using namespace std;

static const string GRADES_HEADER =
    "roll_no,course_code,semester,quiz,assignment,mid,final,total,grade";

// ----------------------------------------------------------------------------
// Private helper: read a number from the user, re-prompting until it is within
// [lo, hi]. Keeps enterMarks free of repetitive validation code.
// ----------------------------------------------------------------------------
static double readInRange(const string& label, double lo, double hi) {
    double value;
    while (true) {
        cout << "    " << label << " (" << lo << "-" << hi << "): ";
        if (cin >> value && value >= lo && value <= hi) {
            return value;
        }
        cout << "    Invalid - please enter a number between "
             << lo << " and " << hi << ".\n";
        cin.clear();
        cin.ignore(10000, '\n');
    }
}

// Private helper: read how many items (1..maxItems) to enter.
static int readCount(const string& label, int maxItems) {
    int n;
    while (true) {
        cout << "  Number of " << label << " (1-" << maxItems << "): ";
        if (cin >> n && n >= 1 && n <= maxItems) {
            return n;
        }
        cout << "  Invalid count.\n";
        cin.clear();
        cin.ignore(10000, '\n');
    }
}

// Private helper: insert-or-update one student's grade row in grades.txt.
static void saveGrade(const Row& newRow) {
    Table rows = readTXT(GRADES_FILE);          // empty if file does not exist
    bool replaced = false;
    for (size_t i = 0; i < rows.size(); i++) {
        if (rows[i][GRD_ROLL] == newRow[GRD_ROLL] &&
            rows[i][GRD_CODE] == newRow[GRD_CODE]) {
            rows[i] = newRow;                   // overwrite existing entry
            replaced = true;
            break;
        }
    }
    if (!replaced) rows.push_back(newRow);
    writeTXT(GRADES_FILE, GRADES_HEADER, rows);
}

// ----------------------------------------------------------------------------
// bestThreeOfFive: exclude the two lowest values using a min-finding loop.
//   - 0 values  -> 0
//   - 1-2 values-> plain average (cannot drop two)
//   - 3+ values -> (sum - lowest - secondLowest) / (count - 2)
// ----------------------------------------------------------------------------
double bestThreeOfFive(const vector<double>& marks) {
    int n = (int)marks.size();
    if (n == 0) return 0.0;

    double sum = 0.0;
    for (int i = 0; i < n; i++) sum += marks[i];

    if (n <= 2) return sum / n;                 // edge case

    // Find the lowest and the second lowest in a single pass.
    double low1 = marks[0];   // smallest
    double low2 = marks[1];   // second smallest
    if (low2 < low1) { double t = low1; low1 = low2; low2 = t; }
    for (int i = 2; i < n; i++) {
        if (marks[i] < low1) {
            low2 = low1;
            low1 = marks[i];
        } else if (marks[i] < low2) {
            low2 = marks[i];
        }
    }
    return (sum - low1 - low2) / (n - 2);
}

// ----------------------------------------------------------------------------
// computeWeightedTotal: the exact weighting from the spec.
// ----------------------------------------------------------------------------
double computeWeightedTotal(double quiz, double asgn, double mid, double final_) {
    return quiz * 0.10 + asgn * 0.10 + mid * 0.30 + final_ * 0.50;
}

// ----------------------------------------------------------------------------
// getLetterGrade: threshold ladder (checked high to low).
// ----------------------------------------------------------------------------
string getLetterGrade(double total) {
    if (total >= 85) return "A";
    if (total >= 80) return "B+";
    if (total >= 70) return "B";
    if (total >= 65) return "C+";
    if (total >= 60) return "C";
    if (total >= 50) return "D";
    return "F";
}

// ----------------------------------------------------------------------------
// gradePoint: letter -> 4.0-scale points.
// ----------------------------------------------------------------------------
double gradePoint(const string& letter) {
    if (letter == "A")  return 4.0;
    if (letter == "B+") return 3.5;
    if (letter == "B")  return 3.0;
    if (letter == "C+") return 2.5;
    if (letter == "C")  return 2.0;
    if (letter == "D")  return 1.0;
    return 0.0;   // F
}

// ----------------------------------------------------------------------------
// applyAttendancePenalty: shortage overrides everything.
// ----------------------------------------------------------------------------
string applyAttendancePenalty(const string& roll, const string& courseCode,
                              const string& grade) {
    if (getAttendancePct(roll, courseCode) < 75.0) {
        return "F";
    }
    return grade;
}

// ----------------------------------------------------------------------------
// enterMarks: collect, validate, compute, store.
// ----------------------------------------------------------------------------
void enterMarks(const string& roll, const string& courseCode, int semester) {
    Student s = searchByRoll(roll);
    if (s.roll.empty()) {
        cout << "  [X] No such student.\n";
        return;
    }
    Course c = getCourse(courseCode);
    if (c.code.empty()) {
        cout << "  [X] No such course.\n";
        return;
    }

    cout << "\n  Entering marks for " << s.name << " in " << c.name << "\n";

    // --- quizzes (best 3 of 5) ---
    int qn = readCount("quizzes", 5);
    vector<double> quizzes;
    for (int i = 0; i < qn; i++) {
        quizzes.push_back(readInRange("Quiz " + intToStr(i + 1), 0, QUIZ_MAX));
    }
    double quizAvg = bestThreeOfFive(quizzes);            // out of QUIZ_MAX
    double quizPct = quizAvg / QUIZ_MAX * 100.0;          // -> percentage

    // --- assignments (simple average) ---
    int an = readCount("assignments", 5);
    vector<double> asgns;
    for (int i = 0; i < an; i++) {
        asgns.push_back(readInRange("Assignment " + intToStr(i + 1), 0, ASGN_MAX));
    }
    double asgnSum = 0.0;
    for (size_t i = 0; i < asgns.size(); i++) asgnSum += asgns[i];
    double asgnPct = (asgnSum / asgns.size()) / ASGN_MAX * 100.0;

    // --- mid and final ---
    double mid     = readInRange("Mid term", 0, MID_MAX);
    double final_  = readInRange("Final term", 0, FINAL_MAX);
    double midPct  = mid    / MID_MAX   * 100.0;
    double finalPct= final_ / FINAL_MAX * 100.0;

    // --- weighted total + grade (with attendance penalty) ---
    double total = computeWeightedTotal(quizPct, asgnPct, midPct, finalPct);
    string grade = getLetterGrade(total);
    string finalGrade = applyAttendancePenalty(roll, courseCode, grade);

    // --- persist ---
    Row row;
    row.push_back(roll);
    row.push_back(courseCode);
    row.push_back(intToStr(semester));
    row.push_back(doubleToStr(quizPct, 2));
    row.push_back(doubleToStr(asgnPct, 2));
    row.push_back(doubleToStr(midPct, 2));
    row.push_back(doubleToStr(finalPct, 2));
    row.push_back(doubleToStr(total, 2));
    row.push_back(finalGrade);
    saveGrade(row);

    cout << fixed << setprecision(2);
    cout << "  Weighted total: " << total << " / 100\n";
    cout << "  Grade: " << finalGrade;
    if (finalGrade == "F" && grade != "F") {
        cout << "  (forced to F by attendance shortage)";
    }
    cout << "\n";
}

// ----------------------------------------------------------------------------
// computeGPA: credit-weighted average of grade points in a semester.
//   GPA = sum(points * credits) / sum(credits)
// ----------------------------------------------------------------------------
double computeGPA(const string& roll, int semester) {
    Table grades  = readTXT(GRADES_FILE);
    Table courses = readTXT(COURSES_FILE);

    double weightedPoints = 0.0;
    int    totalCredits   = 0;

    for (size_t i = 0; i < grades.size(); i++) {
        if (grades[i][GRD_ROLL] == roll &&
            toInt(grades[i][GRD_SEM]) == semester) {

            // find the credit hours for this course (nested loop)
            int credits = 0;
            for (size_t j = 0; j < courses.size(); j++) {
                if (courses[j][CRS_CODE] == grades[i][GRD_CODE]) {
                    credits = toInt(courses[j][CRS_CREDITS]);
                    break;
                }
            }
            weightedPoints += gradePoint(grades[i][GRD_GRADE]) * credits;
            totalCredits   += credits;
        }
    }

    if (totalCredits == 0) return 0.0;
    return weightedPoints / totalCredits;
}

// ----------------------------------------------------------------------------
// computeClassState: gather all totals for a course, then derive the stats.
// The median needs the values in order, so we selection-sort a local copy.
// ----------------------------------------------------------------------------
Stats computeClassState(const string& courseCode) {
    Stats st;
    st.highest = 0.0;
    st.lowest  = 0.0;
    st.mean    = 0.0;
    st.median  = 0.0;

    Table grades = readTXT(GRADES_FILE);
    vector<double> totals;
    for (size_t i = 0; i < grades.size(); i++) {
        if (grades[i][GRD_CODE] == courseCode) {
            totals.push_back(toDouble(grades[i][GRD_TOTAL]));
        }
    }
    if (totals.empty()) return st;

    // selection sort (ascending)
    for (size_t i = 0; i < totals.size(); i++) {
        size_t minIndex = i;
        for (size_t j = i + 1; j < totals.size(); j++) {
            if (totals[j] < totals[minIndex]) minIndex = j;
        }
        if (minIndex != i) {
            double t          = totals[i];
            totals[i]         = totals[minIndex];
            totals[minIndex]  = t;
        }
    }

    st.lowest  = totals[0];
    st.highest = totals[totals.size() - 1];

    double sum = 0.0;
    for (size_t i = 0; i < totals.size(); i++) sum += totals[i];
    st.mean = sum / totals.size();

    size_t n = totals.size();
    if (n % 2 == 1) {
        st.median = totals[n / 2];
    } else {
        st.median = (totals[n / 2 - 1] + totals[n / 2]) / 2.0;
    }
    return st;
}

// ============================================================================
// M3 - course_ops.cpp
// ----------------------------------------------------------------------------
// Enrollment business rules. All checks use plain loops over the data files
// read through the M1 engine.
// ============================================================================

#include "course_ops.h"
#include <iostream>

using namespace std;

// Columns we need from grades.txt for the prerequisite check. They are declared
// locally so this module does not have to depend on M5 (grades) just to read
// three fields. (grades.txt layout is documented in grades.h.)
static const int GRD_ROLL_COL  = 0;
static const int GRD_CODE_COL  = 1;
static const int GRD_GRADE_COL = 8;

// Maximum credit hours a student may carry in one semester.
static const int MAX_CREDIT_LOAD = 21;

// ----------------------------------------------------------------------------
// Conversions / lookups.
// ----------------------------------------------------------------------------
Course rowToCourse(const Row& r) {
    Course c;
    c.code         = r[CRS_CODE];
    c.name         = r[CRS_NAME];
    c.credits      = toInt(r[CRS_CREDITS]);
    c.instructor   = r[CRS_INSTRUCTOR];
    c.capacity     = toInt(r[CRS_CAPACITY]);
    c.enrolled     = toInt(r[CRS_ENROLLED]);
    c.prerequisite = r[CRS_PREREQ];
    return c;
}

Course getCourse(const string& code) {
    Row r = findRow(COURSES_FILE, CRS_CODE, code);
    if (r.empty()) {
        Course empty;
        empty.code = "";
        return empty;
    }
    return rowToCourse(r);
}

string enrollResultMessage(EnrollResult r) {
    switch (r) {
        case ENROLL_SUCCESS:        return "Enrollment successful.";
        case ERR_STUDENT_NOT_FOUND: return "No student with that roll number.";
        case ERR_STUDENT_INACTIVE:  return "Student is inactive (soft-deleted).";
        case ERR_COURSE_NOT_FOUND:  return "No course with that code.";
        case ERR_NO_SEATS:          return "Course is full (no seats left).";
        case ERR_ALREADY_ENROLLED:  return "Student is already enrolled in this course.";
        case ERR_CREDIT_OVERLOAD:   return "Enrolling would exceed the 21 credit-hour limit.";
        case ERR_PREREQ_NOT_MET:    return "Prerequisite not satisfied.";
    }
    return "Unknown result.";
}

// ----------------------------------------------------------------------------
// Private helper: count how many students are ACTIVELY enrolled in a course.
// ----------------------------------------------------------------------------
static int countActiveEnrollments(const string& code) {
    Table rows = readTXT(ENROLLMENTS_FILE);
    int count = 0;
    for (size_t i = 0; i < rows.size(); i++) {
        if (rows[i][ENR_CODE] == code && rows[i][ENR_STATUS] == "active") {
            count++;
        }
    }
    return count;
}

// Private helper: is this student already actively enrolled in this course?
static bool alreadyEnrolled(const string& roll, const string& code) {
    Table rows = readTXT(ENROLLMENTS_FILE);
    for (size_t i = 0; i < rows.size(); i++) {
        if (rows[i][ENR_ROLL] == roll &&
            rows[i][ENR_CODE] == code &&
            rows[i][ENR_STATUS] == "active") {
            return true;
        }
    }
    return false;
}

// Private helper: build the next enrollment id (E0001, E0002, ...).
static string nextEnrollmentId() {
    Table rows = readTXT(ENROLLMENTS_FILE);
    int maxNum = 0;
    for (size_t i = 0; i < rows.size(); i++) {
        // ids look like "E0123" -> drop the 'E', read the number
        string num = rows[i][ENR_ID].substr(1);
        int n = toInt(num);
        if (n > maxNum) maxNum = n;
    }
    int next = maxNum + 1;

    // zero-pad to 4 digits: E0007
    string digits = intToStr(next);
    while (digits.size() < 4) digits = "0" + digits;
    return "E" + digits;
}

// Private helper: change a course's stored enrolled counter by delta (+1/-1).
static void adjustCourseEnrolled(const string& code, int delta) {
    Table rows = readTXT(COURSES_FILE);
    for (size_t i = 0; i < rows.size(); i++) {
        if (rows[i][CRS_CODE] == code) {
            int current = toInt(rows[i][CRS_ENROLLED]) + delta;
            if (current < 0) current = 0;
            rows[i][CRS_ENROLLED] = intToStr(current);
            break;
        }
    }
    writeTXT(COURSES_FILE, readHeader(COURSES_FILE), rows);
}

// ----------------------------------------------------------------------------
// getCreditLoad: nested loop. Outer loop walks the student's active
// enrollments; inner loop finds that course to read its credit hours.
// ----------------------------------------------------------------------------
int getCreditLoad(const string& roll, int semester) {
    Table enrolls = readTXT(ENROLLMENTS_FILE);
    Table courses = readTXT(COURSES_FILE);
    int totalCredits = 0;

    for (size_t i = 0; i < enrolls.size(); i++) {
        if (enrolls[i][ENR_ROLL] == roll &&
            enrolls[i][ENR_STATUS] == "active" &&
            toInt(enrolls[i][ENR_SEM]) == semester) {

            for (size_t j = 0; j < courses.size(); j++) {
                if (courses[j][CRS_CODE] == enrolls[i][ENR_CODE]) {
                    totalCredits += toInt(courses[j][CRS_CREDITS]);
                    break;
                }
            }
        }
    }
    return totalCredits;
}

// ----------------------------------------------------------------------------
// checkPrerequisite: look up the course's prereq; if "NONE" we are done,
// otherwise search grades.txt for a non-F grade in that prereq course.
// ----------------------------------------------------------------------------
bool checkPrerequisite(const string& roll, const string& code) {
    Course c = getCourse(code);
    if (c.code.empty()) return false;            // course itself missing
    if (c.prerequisite == "NONE" || c.prerequisite.empty()) {
        return true;                             // nothing required
    }

    Table grades = readTXT(GRADES_FILE);
    for (size_t i = 0; i < grades.size(); i++) {
        if (grades[i][GRD_ROLL_COL] == roll &&
            grades[i][GRD_CODE_COL] == c.prerequisite) {
            string g = grades[i][GRD_GRADE_COL];
            if (g != "F" && !g.empty()) {
                return true;                     // passed the prerequisite
            }
        }
    }
    return false;
}

// ----------------------------------------------------------------------------
// enrollStudent: run each rule in a clear order and stop at the first failure.
// ----------------------------------------------------------------------------
EnrollResult enrollStudent(const string& roll, const string& code, int semester) {
    Student s = searchByRoll(roll);
    if (s.roll.empty())             return ERR_STUDENT_NOT_FOUND;
    if (s.status != "active")       return ERR_STUDENT_INACTIVE;

    Course c = getCourse(code);
    if (c.code.empty())             return ERR_COURSE_NOT_FOUND;

    if (alreadyEnrolled(roll, code)) return ERR_ALREADY_ENROLLED;

    if (countActiveEnrollments(code) >= c.capacity) return ERR_NO_SEATS;

    if (getCreditLoad(roll, semester) + c.credits > MAX_CREDIT_LOAD) {
        return ERR_CREDIT_OVERLOAD;
    }

    if (!checkPrerequisite(roll, code)) return ERR_PREREQ_NOT_MET;

    // All rules passed -> write the new enrollment row and bump the counter.
    Row newRow;
    newRow.push_back(nextEnrollmentId());
    newRow.push_back(roll);
    newRow.push_back(code);
    newRow.push_back(intToStr(semester));
    newRow.push_back("15-01-2024");   // enrollment date (demo default)
    newRow.push_back("active");
    appendTXT(ENROLLMENTS_FILE, newRow);

    adjustCourseEnrolled(code, +1);
    return ENROLL_SUCCESS;
}

// ----------------------------------------------------------------------------
// dropCourse: refuse if ANY attendance row exists for this student+course,
// otherwise mark the enrollment "dropped" and decrement the counter.
// ----------------------------------------------------------------------------
bool dropCourse(const string& roll, const string& code) {
    // Rule: cannot drop once attendance has started.
    Table att = readTXT(ATTENDANCE_FILE);
    for (size_t i = 0; i < att.size(); i++) {
        if (att[i][1] == roll && att[i][2] == code) {   // ATT_ROLL=1, ATT_CODE=2
            cout << "  [X] Cannot drop: attendance already exists for this course.\n";
            return false;
        }
    }

    Table enrolls = readTXT(ENROLLMENTS_FILE);
    bool found = false;
    for (size_t i = 0; i < enrolls.size(); i++) {
        if (enrolls[i][ENR_ROLL] == roll &&
            enrolls[i][ENR_CODE] == code &&
            enrolls[i][ENR_STATUS] == "active") {
            enrolls[i][ENR_STATUS] = "dropped";
            found = true;
            break;
        }
    }
    if (!found) {
        cout << "  [X] No active enrollment found to drop.\n";
        return false;
    }

    writeTXT(ENROLLMENTS_FILE, readHeader(ENROLLMENTS_FILE), enrolls);
    adjustCourseEnrolled(code, -1);
    return true;
}

// ----------------------------------------------------------------------------
// listEnrolledStudents: join active enrollments of a course back to the
// student records.
// ----------------------------------------------------------------------------
vector<Student> listEnrolledStudents(const string& code) {
    vector<Student> result;
    Table enrolls = readTXT(ENROLLMENTS_FILE);
    for (size_t i = 0; i < enrolls.size(); i++) {
        if (enrolls[i][ENR_CODE] == code && enrolls[i][ENR_STATUS] == "active") {
            Student s = searchByRoll(enrolls[i][ENR_ROLL]);
            if (!s.roll.empty()) {
                result.push_back(s);
            }
        }
    }
    return result;
}

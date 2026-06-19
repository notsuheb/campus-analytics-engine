#ifndef COURSE_OPS_H
#define COURSE_OPS_H

// ============================================================================
// M3 - course_ops.h
// ----------------------------------------------------------------------------
// Course enrollment logic: enrol, drop, credit-load limits and prerequisites.
// Depends on M1 (filehandler) and M2 (student_ops).
// ============================================================================

#include <string>
#include <vector>
#include "filehandler.h"
#include "student_ops.h"

// ---- Column positions inside courses.txt -----------------------------------
const int CRS_CODE       = 0;
const int CRS_NAME       = 1;
const int CRS_CREDITS    = 2;
const int CRS_INSTRUCTOR = 3;
const int CRS_CAPACITY   = 4;
const int CRS_ENROLLED   = 5;
const int CRS_PREREQ     = 6;

// ---- Column positions inside enrollments.txt -------------------------------
const int ENR_ID     = 0;
const int ENR_ROLL   = 1;
const int ENR_CODE   = 2;
const int ENR_SEM    = 3;
const int ENR_DATE   = 4;
const int ENR_STATUS = 5;

// Typed view of one course row.
struct Course {
    std::string code;
    std::string name;
    int         credits;
    std::string instructor;
    int         capacity;
    int         enrolled;
    std::string prerequisite;   // a course code, or "NONE"
};

// Every possible outcome of an enrolment attempt. Returning an enum (instead of
// a bool) lets the menu print exactly WHY an enrolment failed.
enum EnrollResult {
    ENROLL_SUCCESS,
    ERR_STUDENT_NOT_FOUND,
    ERR_STUDENT_INACTIVE,
    ERR_COURSE_NOT_FOUND,
    ERR_NO_SEATS,
    ERR_ALREADY_ENROLLED,
    ERR_CREDIT_OVERLOAD,
    ERR_PREREQ_NOT_MET
};

// Turns an EnrollResult code into a human-readable sentence.
std::string enrollResultMessage(EnrollResult r);

// ---- Conversion + lookup ----------------------------------------------------
Course rowToCourse(const Row& r);
Course getCourse(const std::string& code);   // empty .code if not found

// ---- Required operations ---------------------------------------------------

// Runs every business rule in order and, if all pass, appends an enrolment row
// and bumps the course's enrolled counter. Returns the outcome code.
EnrollResult enrollStudent(const std::string& roll, const std::string& code, int semester);

// Drops a course only when the student has NO attendance recorded for it.
// Sets the enrolment status to "dropped". Returns false if not allowed/found.
bool dropCourse(const std::string& roll, const std::string& code);

// Total credit hours a student is actively carrying in a semester.
// Implemented with a nested loop over enrollments x courses.
int getCreditLoad(const std::string& roll, int semester);

// True if the course has no prerequisite, or the student has a passing
// (non-F) grade in the prerequisite course.
bool checkPrerequisite(const std::string& roll, const std::string& code);

// All ACTIVE students currently enrolled (status "active") in a course.
std::vector<Student> listEnrolledStudents(const std::string& code);

#endif // COURSE_OPS_H

#ifndef GRADES_H
#define GRADES_H

// ============================================================================
// M5 - grades.h
// ----------------------------------------------------------------------------
// Marks entry, weighted totals, letter grades, semester GPA and class stats.
// Depends on M1 (filehandler), M2 (student_ops) and M4 (attendance, for the
// "fail on shortage" penalty).
//
// This module OWNS grades.txt and creates it on first use. Its layout is:
//   roll_no,course_code,semester,quiz,assignment,mid,final,total,grade
// where quiz/assignment/mid/final are stored as PERCENTAGES (0-100) so the
// weighted formula stays a clean out-of-100 calculation.
// ============================================================================

#include <string>
#include <vector>

// ---- Column positions inside grades.txt ------------------------------------
const int GRD_ROLL  = 0;
const int GRD_CODE  = 1;
const int GRD_SEM   = 2;
const int GRD_QUIZ  = 3;
const int GRD_ASGN  = 4;
const int GRD_MID   = 5;
const int GRD_FINAL = 6;
const int GRD_TOTAL = 7;
const int GRD_GRADE = 8;

// Raw maximum marks for each assessment (used to convert to a percentage).
const double QUIZ_MAX  = 10.0;
const double ASGN_MAX  = 10.0;
const double MID_MAX   = 40.0;
const double FINAL_MAX = 60.0;

// Result of computeClassState - the four summary numbers for one course.
struct Stats {
    double highest;
    double lowest;
    double mean;
    double median;
};

// ---- Required operations ---------------------------------------------------

// Interactive marks entry for one student in one course. Validates each input
// range, computes the weighted total + letter grade (with attendance penalty)
// and saves the result to grades.txt.
void enterMarks(const std::string& roll, const std::string& courseCode, int semester);

// Average of the best three values: the two lowest are excluded with a loop
// (no sorting). Handles the edge case of fewer than three values.
double bestThreeOfFive(const std::vector<double>& marks);

// quiz*0.10 + asgn*0.10 + mid*0.30 + final_*0.50  (all inputs are 0-100).
double computeWeightedTotal(double quiz, double asgn, double mid, double final_);

// Maps a 0-100 total to a letter grade.
std::string getLetterGrade(double total);

// Numeric grade point (4.0 scale) for a letter grade.
double gradePoint(const std::string& letter);

// Credit-weighted GPA across every graded course of a student in a semester.
double computeGPA(const std::string& roll, int semester);

// highest / lowest / mean / median of the totals recorded for a course.
Stats computeClassState(const std::string& courseCode);

// Returns "F" if the student's attendance in the course is below 75%,
// otherwise returns the grade unchanged.
std::string applyAttendancePenalty(const std::string& roll,
                                   const std::string& courseCode,
                                   const std::string& grade);

#endif // GRADES_H

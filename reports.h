#ifndef REPORTS_H
#define REPORTS_H

// ============================================================================
// M7 - reports.h
// ----------------------------------------------------------------------------
// Read-only, formatted console reports built by combining the lower modules.
// Depends on M5 (grades), M4 (attendance) and M6 (fee_tracker).
// ============================================================================

#include <string>

// A student "passes" overall at a CGPA of 2.0 or above.
const double PASS_CGPA = 2.0;

// ---- Required reports ------------------------------------------------------

// Active students ranked by CGPA (highest first) with a rank column.
void printMeritList();

// Every (student, course) pair that is below 75% attendance.
void printAttendanceDefaulters();

// Students who still owe fees, with outstanding amount and weeks overdue.
void printFeeDefaulters();

// Per-student result sheet for a semester: GPA and attendance status.
void printSemesterResult(int semester);

// Department-wise headcount, average CGPA and pass rate (parallel arrays).
void printDepartmentSummary();

// Runs one of the reports above but sends its output to a .txt file instead of
// the screen, by temporarily swapping the destination of cout.
//   choice: 1=merit, 2=attendance, 3=fees, 4=semester result, 5=dept summary
void exportReportToFile(const std::string& filename, int choice, int semester);

#endif // REPORTS_H

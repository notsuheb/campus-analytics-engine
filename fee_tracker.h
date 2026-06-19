#ifndef FEE_TRACKER_H
#define FEE_TRACKER_H

// ============================================================================
// M6 - fee_tracker.h
// ----------------------------------------------------------------------------
// Fee payments, late fines and receipts. All date maths is done BY HAND
// (the project forbids <ctime>): dates are DD-MM-YYYY strings that we parse and
// convert to a day count using a month-length array.
// Depends on M1 (filehandler) and M2 (student_ops).
// ============================================================================

#include <string>
#include <vector>

// ---- Column positions inside fees.txt --------------------------------------
const int FEE_ID      = 0;
const int FEE_ROLL    = 1;
const int FEE_SEM     = 2;
const int FEE_TOTAL   = 3;
const int FEE_PAID    = 4;
const int FEE_DUE     = 5;
const int FEE_PAYDATE = 6;
const int FEE_METHOD  = 7;
const int FEE_STATUS  = 8;

// Because <ctime> is forbidden, the program has no idea what "today" is, so we
// fix a reference date that represents "now" for overdue calculations. It sits
// just after the latest payment in the dataset (11-04-2024).
const std::string SYSTEM_DATE = "30-04-2024";

// Late-fine policy: 2% of the tuition for every COMPLETE week the payment is
// late.
const double WEEKLY_FINE_RATE = 0.02;

// One row of the fee-defaulter report.
struct Defaulter {
    std::string roll;
    std::string name;
    double      outstanding;   // total_fee - amount_paid
    int         weeksOverdue;
};

// ---- Date utilities (manual, no <ctime>) -----------------------------------
bool isValidDate(const std::string& date);        // strict DD-MM-YYYY check
int  daysBetween(const std::string& d1, const std::string& d2); // d2 - d1

// ---- Required operations ---------------------------------------------------

// Adds a payment of 'amount' to a student's fee record, stamps the payment
// date/method and recomputes the status. Returns false on a bad date or roll.
bool recordPayment(const std::string& roll, double amount,
                   const std::string& date, const std::string& method);

// 2% of tuition per complete late week, based on due_date vs paid_date.
double computeLateFine(const std::string& roll);

// Prints a formatted receipt (tuition, fine, paid, balance) using setw/setfill.
void generateReceipt(const std::string& roll);

// Every student who still owes money, sorted by outstanding amount (largest
// first) using BUBBLE SORT.
std::vector<Defaulter> getDefaulters();

#endif // FEE_TRACKER_H

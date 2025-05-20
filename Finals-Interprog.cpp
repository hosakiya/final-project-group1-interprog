#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <limits>
#include <sstream>
#include <regex>
#include <fstream>
#include <climits>
using namespace std;


const string CURRENT_DATE = "2025-05-19";
const int CURRENT_HOUR = 22;
const int CURRENT_MINUTE = 19;

// -------- Exception Handling --------
class ReservationException : public exception {
    string message;
public:
    ReservationException(const string& msg) : message(msg) {}
    const char* what() const noexcept override { return message.c_str(); }
};

// -------- Reservation Struct --------
struct Reservation {
    string id;
    string customerName;
    string phoneNumber;
    int partySize;
    string date;
    string time;
    int tableNumber;

    Reservation(const string& id, const string& name, const string& phone, int size, const string& date, const string& time, int table)
        : id(id), customerName(name), phoneNumber(phone), partySize(size), date(date), time(time), tableNumber(table) {}
};

// -------- Validation Functions --------
bool validatePhoneNumber(const string& phone) {
    regex phoneRegex("\\d{3}-\\d{3}-\\d{4}");
    return regex_match(phone, phoneRegex);
}

bool validateDate(const string& date) {
    regex dateRegex("\\d{4}-\\d{2}-\\d{2}"); 
    if (!regex_match(date, dateRegex)) {
        return false;
    }
    int year, month, day;
    sscanf(date.c_str(), "%d-%d-%d", &year, &month, &day);
    if (month < 1 || month > 12 || day < 1 || day > 31) {
        return false; 
    }
    string currentDate = CURRENT_DATE;
    if (date < currentDate) {
        return false; 
    }
    return true;
}

bool validateTime(const string& time, const string& date) {
    regex timeRegex("\\d{2}:\\d{2}"); 
    if (!regex_match(time, timeRegex)) {
        return false;
    }
    int hour, minute;
    sscanf(time.c_str(), "%d:%d", &hour, &minute);
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        return false;
    }
    if (date == CURRENT_DATE) {
        if (hour < CURRENT_HOUR || (hour == CURRENT_HOUR && minute <= CURRENT_MINUTE)) {
            return false; 
        }
    }
    return true;
}

bool validatePartySize(int size) {
    return size >= 1;
}

bool validateReservationId(const string& id) {
    regex idRegex("ID \\d+A"); // e.g., "ID 1A"
    return regex_match(id, idRegex);
}

// -------- Input Validation Function --------
bool validateNumericInput(const string& input, int& result, int minVal, int maxVal) {
    if (input.empty() || !all_of(input.begin(), input.end(), ::isdigit)) {
        return false;
    }
    try {
        size_t pos;
        result = stoi(input, &pos);
        if (pos != input.length()) {
            return false;
        }
        if (result < minVal || result > maxVal) {
            return false;
        }
        return true;
    } catch (...) {
        return false;
    }
}

// -------- Singleton Pattern --------
class ReservationManager {
private:
    vector<bool> tables;
    vector<Reservation> reservations;
    static unique_ptr<ReservationManager> instance;
    int nextReservationId;
    ReservationManager() : tables(10, true), nextReservationId(1) {}

    string getCurrentTimestamp() {
        ostringstream oss;
        oss << "[" << CURRENT_DATE << " " << (CURRENT_HOUR < 10 ? "0" : "") << CURRENT_HOUR << ":"
            << (CURRENT_MINUTE < 10 ? "0" : "") << CURRENT_MINUTE << ":00]";
        return oss.str();
    }

    void writeLogToFile(const string& logEntry) {
        ofstream logFile("logs.txt", ios::app); // Append mode
        if (logFile.is_open()) {
            logFile << logEntry << "\n";
            logFile.close();
        } else {
            throw ReservationException("Unable to open log file.");
        }
    }

public:
    bool reservationIdExists(const string& id, const string& excludeId = "") {
        for (const auto& res : reservations) {
            if (res.id == id && res.id != excludeId) {
                return true;
            }
        }
        return false;
    }

public:
    static ReservationManager& getInstance() {
        if (!instance)
            instance.reset(new ReservationManager());
        return *instance;
    }

    void logLogin(const string& role, const string& username) {
        string timestamp = getCurrentTimestamp();
        string logEntry = timestamp + " [" + role + ": " + username + "] Logged in";
        writeLogToFile(logEntry);
    }

    void logReservationAction(const string& role, const string& username, const string& action, const string& details) {
        string timestamp = getCurrentTimestamp();
        string logEntry = timestamp + " [" + role + ": " + username + "] " + action + " " + details;
        writeLogToFile(logEntry);
    }

    void logError(const string& role, const string& username, const string& action, const string& errorMsg) {
        string timestamp = getCurrentTimestamp();
        string logEntry = timestamp + " [" + role + ": " + username + "] " + action + " Error: " + errorMsg;
        writeLogToFile(logEntry);
    }

    void viewTableAvailability() {
        for (int i = 0; i < tables.size(); ++i) {
            cout << "Table " << i + 1 << " is " << (tables[i] ? "AVAILABLE" : "BOOKED") << endl;
        }
    }

    bool hasReservations(const string& customerName) {
        for (const auto& res : reservations) {
            if (res.customerName == customerName) {
                return true;
            }
        }
        return false;
    }

    int reserveTable(const string& customerName, const string& phoneNumber, 
                    int partySize, const string& date, const string& time, int tableNumber) {
        if (!validatePhoneNumber(phoneNumber)) {
            throw ReservationException("Invalid phone number format. Use XXX-XXX-XXXX.");
        }
        if (!validatePartySize(partySize)) {
            throw ReservationException("Party size must be at least 1.");
        }
        if (!validateDate(date)) {
            throw ReservationException("Invalid date format (use YYYY-MM-DD) or date is in the past.");
        }
        if (!validateTime(time, date)) {
            throw ReservationException("Invalid time format (use HH:MM) or time is in the past for today.");
        }
        if (tableNumber < 0 || tableNumber >= tables.size()) {
            throw ReservationException("Invalid table number. Must be between 1 and 10.");
        }
        if (!tables[tableNumber]) {
            throw ReservationException("Selected table is already booked.");
        }
        tables[tableNumber] = false;

        string reservationId;
        do {
            reservationId = "ID " + to_string(nextReservationId++) + "A";
        } while (reservationIdExists(reservationId));

        reservations.emplace_back(reservationId, customerName, phoneNumber, partySize, date, time, tableNumber);
        logReservationAction("Customer", customerName, "Reserved table", "#" + to_string(tableNumber + 1) + " for " + 
                            to_string(partySize) + " on " + date + " at " + time);
        return tableNumber;
    }

    void cancelReservation(const string& reservationId, const string& customerName) {
        if (!validateReservationId(reservationId)) {
            throw ReservationException("Invalid reservation ID format. Use 'ID <number>A', e.g., ID 1A.");
        }
        bool hasReservation = false;
        int tableIndex = -1;
        for (const auto& res : reservations) {
            if (res.id == reservationId && res.customerName == customerName) {
                hasReservation = true;
                tableIndex = res.tableNumber;
                break;
            }
        }
        if (!hasReservation) {
            throw ReservationException("No reservation to cancel.");
        }
        tables[tableIndex] = true;
        auto it = reservations.begin();
        while (it != reservations.end()) {
            if (it->id == reservationId && it->customerName == customerName) {
                it = reservations.erase(it);
            } else {
                ++it;
            }
        }
        logReservationAction("Customer", customerName, "Cancelled reservation", "ID " + reservationId);
    }

    void viewCustomerReservations(const string& customerName) {
        cout << "\n--- Your Reservations ---\n";
        bool hasReservations = false;
        for (const auto& res : reservations) {
            if (res.customerName == customerName) {
                cout << "ID: " << res.id << ", Name: " << res.customerName 
                     << ", Contact: " << res.phoneNumber << ", Party Size: " << res.partySize 
                     << ", Date: " << res.date << ", Time: " << res.time 
                     << ", Table: " << res.tableNumber + 1 << endl;
                hasReservations = true;
            }
        }
        if (!hasReservations) {
            cout << "No reservation to view.\n";
        }
    }

    void updateReservation(const string& reservationId, const string& customerName, 
                           const string& newId, const string& newName, const string& newPhone, int newPartySize, 
                           const string& newDate, const string& newTime, int newTableIndex) {
        if (!validateReservationId(reservationId)) {
            throw ReservationException("Invalid reservation ID format. Use 'ID <number>A', e.g., ID 1A.");
        }
        bool hasReservation = false;
        for (const auto& res : reservations) {
            if (res.id == reservationId && res.customerName == customerName) {
                hasReservation = true;
                break;
            }
        }
        if (!hasReservation) {
            throw ReservationException("No reservation to update.");
        }

        if (newId != "0") {
            if (!validateReservationId(newId)) {
                throw ReservationException("Invalid new reservation ID format. Use 'ID <number>A', e.g., ID 1A.");
            }
            if (reservationIdExists(newId, reservationId)) {
                throw ReservationException("New reservation ID already exists. Choose a different ID.");
            }
        }
        if (newPhone != "0" && !validatePhoneNumber(newPhone)) {
            throw ReservationException("Invalid phone number format. Use XXX-XXX-XXXX.");
        }
        if (newPartySize != 0 && !validatePartySize(newPartySize)) {
            throw ReservationException("Party size must be at least 1.");
        }
        if (newDate != "0" && !validateDate(newDate)) {
            throw ReservationException("Invalid date format (use YYYY-MM-DD) or date is in the past.");
        }
        if (newTime != "0" && !validateTime(newTime, newDate != "0" ? newDate : CURRENT_DATE)) {
            throw ReservationException("Invalid time format (use HH:MM) or time is in the past for today.");
        }

        int oldTableIndex = -1;
        for (auto& res : reservations) {
            if (res.id == reservationId && res.customerName == customerName) {
                oldTableIndex = res.tableNumber;
                break;
            }
        }
        if (newTableIndex != -1) {
            if (newTableIndex < 0 || newTableIndex >= tables.size()) {
                throw ReservationException("Invalid new table index.");
            }
            tables[oldTableIndex] = true;
            if (!tables[newTableIndex]) {
                tables[oldTableIndex] = false; // Revert the change
                throw ReservationException("Selected table is already booked.");
            }
            tables[newTableIndex] = false;
        } else {
            newTableIndex = oldTableIndex;
        }

        for (auto& res : reservations) {
            if (res.id == reservationId && res.customerName == customerName) {
                if (newId != "0") res.id = newId;
                if (newName != "0") res.customerName = newName;
                if (newPhone != "0") res.phoneNumber = newPhone;
                if (newPartySize != 0) res.partySize = newPartySize;
                if (newDate != "0") res.date = newDate;
                if (newTime != "0") res.time = newTime;
                res.tableNumber = newTableIndex;
                break;
            }
        }
        logReservationAction("Customer", customerName, "Updated reservation", "ID " + reservationId);
    }

    void viewLogs() {
        cout << "--- System Logs ---\n\n";
        ifstream logFile("logs.txt");
        if (logFile.is_open()) {
            string line;
            while (getline(logFile, line)) {
                cout << line << "\n";
            }
            logFile.close();
        } else {
            cout << "Unable to open log file.\n";
        }
    }
};

unique_ptr<ReservationManager> ReservationManager::instance = nullptr;

// -------- Abstraction + Polymorphism --------
class User {
protected:
    string username;
    string role;
public:
    User(const string& name, const string& r) : username(name), role(r) {
        ReservationManager::getInstance().logLogin(role, name);
    }
    virtual bool showMenu() = 0; // Return true to logout, false to continue
    virtual ~User() = default;
};

// Account database
map<string, string> receptionistAccounts;
map<string, string> customerAccounts;

// -------- Inheritance for Roles --------
class Customer : public User {
public:
    Customer(const string& name) : User(name, "Customer") {}
    bool showMenu() override {
        bool isRunning = true;
        while (isRunning) {
            string input;
            int choice;
            cout << "\n[Customer Menu - " << username << "]\n";
            cout << "1. View My Reservations\n";
            cout << "2. Reserve Table\n";
            cout << "3. View Availability\n";
            cout << "4. Update Reservation\n";
            cout << "5. Cancel Reservation\n";
            cout << "6. Exit\nChoice: ";
            getline(cin, input);

            if (!validateNumericInput(input, choice, 1, 6)) {
                cout << "Invalid choice. Please enter a single number between 1 and 6 (e.g., 1, not 1a, 1.1, or 1 1).\n";
                continue;
            }

            switch (choice) {
                case 1:
                    ReservationManager::getInstance().viewCustomerReservations(username);
                    break;
                case 2: {
                    string phoneNumber, date, time, partySizeInput, tableInput;
                    int partySize, tableNumber;

                    while (true) {
                        cout << "Enter your phone number (e.g., 123-456-7890): ";
                        getline(cin, phoneNumber);
                        if (validatePhoneNumber(phoneNumber)) {
                            break;
                        }
                        cout << "Error: Invalid phone number format. Use XXX-XXX-XXXX.\n";
                        ReservationManager::getInstance().logError("Customer", username, "Failed to reserve table", "Invalid phone number format.");
                    }

                    while (true) {
                        cout << "Enter party size (must be at least 1): ";
                        getline(cin, partySizeInput);
                        if (!validateNumericInput(partySizeInput, partySize, 1, INT_MAX)) {
                            cout << "Error: Invalid party size. Must be a single number >= 1 (e.g., 2, not 2a, 2.1, or 2 2).\n";
                            ReservationManager::getInstance().logError("Customer", username, "Failed to reserve table", "Invalid party size.");
                            continue;
                        }
                        if (!validatePartySize(partySize)) {
                            cout << "Error: Party size must be at least 1.\n";
                            ReservationManager::getInstance().logError("Customer", username, "Failed to reserve table", "Party size must be at least 1.");
                            continue;
                        }
                        break;
                    }

                    while (true) {
                        cout << "Enter reservation date (e.g., YYYY-MM-DD, must be on or after 2025-05-19): ";
                        getline(cin, date);
                        if (validateDate(date)) {
                            break;
                        }
                        cout << "Error: Invalid date format (use YYYY-MM-DD) or date is in the past.\n";
                        ReservationManager::getInstance().logError("Customer", username, "Failed to reserve table", "Invalid date format or date is in the past.");
                    }

                    while (true) {
                        cout << "Enter reservation time (e.g., HH:MM in 24-hour format, must be after 22:19 if today): ";
                        getline(cin, time);
                        if (validateTime(time, date)) {
                            break;
                        }
                        cout << "Error: Invalid time format (use HH:MM) or time is in the past for today.\n";
                        ReservationManager::getInstance().logError("Customer", username, "Failed to reserve table", "Invalid time format or time is in the past.");
                    }

                    while (true) {
                        cout << "Available tables:\n";
                        ReservationManager::getInstance().viewTableAvailability();
                        cout << "Enter table number to reserve (1-10): ";
                        getline(cin, tableInput);
                        if (!validateNumericInput(tableInput, tableNumber, 1, 10)) {
                            cout << "Error: Invalid table number. Must be a single number between 1 and 10 (e.g., 1, not 1a, 1.1, or 1 1).\n";
                            ReservationManager::getInstance().logError("Customer", username, "Failed to reserve table", "Invalid table number.");
                            continue;
                        }
                        tableNumber--; // Convert to 0-based index
                        break;
                    }

                    try {
                        int table = ReservationManager::getInstance().reserveTable(username, phoneNumber, partySize, date, time, tableNumber);
                        cout << "Reserved Table #" << table + 1 << " successfully!\n";
                    } catch (const ReservationException& ex) {
                        cout << "Error: " << ex.what() << endl;
                        ReservationManager::getInstance().logError("Customer", username, "Failed to reserve table", ex.what());
                        cout << "Reservation failed. Returning to menu.\n";
                    }
                    break;
                }
                case 3:
                    ReservationManager::getInstance().viewTableAvailability();
                    break;
                case 4: {
                    if (!ReservationManager::getInstance().hasReservations(username)) {
                        cout << "No reservations.\n";
                        break;
                    }

                    string reservationId, newId, newName, newPhone, newDate, newTime, newPartySizeInput, newTableChoiceInput;
                    int newPartySize, newTableChoice;

                    while (true) {
                        cout << "Enter reservation ID to update (e.g., ID 1A): ";
                        getline(cin, reservationId);
                        try {
                            if (!validateReservationId(reservationId)) {
                                throw ReservationException("Invalid reservation ID format. Use 'ID <number>A', e.g., ID 1A.");
                            }
                            if (!ReservationManager::getInstance().hasReservations(username)) {
                                throw ReservationException("No reservation to update.");
                            }
                            bool hasReservation = false;
                            ReservationManager::getInstance().viewCustomerReservations(username); // Show reservations to check
                            hasReservation = ReservationManager::getInstance().hasReservations(username);
                            if (!hasReservation) {
                                throw ReservationException("No reservation to update.");
                            }
                            break;
                        } catch (const ReservationException& ex) {
                            cout << "Error: " << ex.what() << endl;
                            ReservationManager::getInstance().logError("Customer", username, "Failed to update reservation", ex.what());
                        }
                    }

                    while (true) {
                        cout << "Enter new ID (e.g., ID 2A, or 0 to keep current): ";
                        getline(cin, newId);
                        if (newId == "0") break;
                        try {
                            if (!validateReservationId(newId)) {
                                throw ReservationException("Invalid new reservation ID format. Use 'ID <number>A', e.g., ID 1A.");
                            }
                            if (ReservationManager::getInstance().reservationIdExists(newId, reservationId)) {
                                throw ReservationException("New reservation ID already exists. Choose a different ID.");
                            }
                            break;
                        } catch (const ReservationException& ex) {
                            cout << "Error: " << ex.what() << endl;
                            ReservationManager::getInstance().logError("Customer", username, "Failed to update reservation", ex.what());
                        }
                    }

                    while (true) {
                        cout << "Enter new name (or 0 to keep current): ";
                        getline(cin, newName);
                        break;
                    }

                    while (true) {
                        cout << "Enter new phone number (e.g., 123-456-7890, or 0 to keep current): ";
                        getline(cin, newPhone);
                        if (newPhone == "0") break;
                        if (validatePhoneNumber(newPhone)) break;
                        cout << "Error: Invalid phone number format. Use XXX-XXX-XXXX.\n";
                        ReservationManager::getInstance().logError("Customer", username, "Failed to update reservation", "Invalid phone number format.");
                    }

                    while (true) {
                        cout << "Enter new party size (must be at least 1, or 0 to keep current): ";
                        getline(cin, newPartySizeInput);
                        if (newPartySizeInput == "0") {
                            newPartySize = 0;
                            break;
                        }
                        if (!validateNumericInput(newPartySizeInput, newPartySize, 1, INT_MAX)) {
                            cout << "Error: Invalid party size. Must be a single number >= 1 (e.g., 2, not 2a, 2.1, or 2 2).\n";
                            ReservationManager::getInstance().logError("Customer", username, "Failed to update reservation", "Invalid party size.");
                            continue;
                        }
                        if (!validatePartySize(newPartySize)) {
                            cout << "Error: Party size must be at least 1.\n";
                            ReservationManager::getInstance().logError("Customer", username, "Failed to update reservation", "Party size must be at least 1.");
                            continue;
                        }
                        break;
                    }

                    while (true) {
                        cout << "Enter new date (e.g., YYYY-MM-DD, must be on or after 2025-05-19, or 0 to keep current): ";
                        getline(cin, newDate);
                        if (newDate == "0") break;
                        if (validateDate(newDate)) break;
                        cout << "Error: Invalid date format (use YYYY-MM-DD) or date is in the past.\n";
                        ReservationManager::getInstance().logError("Customer", username, "Failed to update reservation", "Invalid date format or date is in the past.");
                    }

                    while (true) {
                        cout << "Enter new time (e.g., HH:MM in 24-hour format, must be after 22:19 if today, or 0 to keep current): ";
                        getline(cin, newTime);
                        if (newTime == "0") break;
                        if (validateTime(newTime, newDate != "0" ? newDate : CURRENT_DATE)) break;
                        cout << "Error: Invalid time format (use HH:MM) or time is in the past for today.\n";
                        ReservationManager::getInstance().logError("Customer", username, "Failed to update reservation", "Invalid time format or time is in the past.");
                    }

                    while (true) {
                        cout << "Table options: 0 to keep current, or enter a specific table number (1-10):\n";
                        ReservationManager::getInstance().viewTableAvailability();
                        cout << "Choice: ";
                        getline(cin, newTableChoiceInput);
                        if (!validateNumericInput(newTableChoiceInput, newTableChoice, 0, 10)) {
                            cout << "Error: Invalid table choice. Must be a single number between 0 and 10 (e.g., 1, not 1a, 1.1, or 1 1).\n";
                            ReservationManager::getInstance().logError("Customer", username, "Failed to update reservation", "Invalid table choice.");
                            continue;
                        }
                        break;
                    }

                    // Confirmation prompt
                    string confirm;
                    cout << "Confirm update? Yes or No: ";
                    getline(cin, confirm);
                    if (confirm != "Yes" && confirm != "yes") {
                        cout << "Update cancelled.\n";
                        break;
                    }

                    try {
                        int newTableIndex = -1;
                        if (newTableChoice != 0) {
                            newTableIndex = newTableChoice - 1;
                        }
                        ReservationManager::getInstance().updateReservation(reservationId, username, 
                                                                            newId, newName, newPhone, newPartySize, 
                                                                            newDate, newTime, newTableIndex);
                        cout << "Reservation updated successfully.\n";
                    } catch (const ReservationException& ex) {
                        cout << "Error: " << ex.what() << endl;
                        ReservationManager::getInstance().logError("Customer", username, "Failed to update reservation", ex.what());
                        cout << "Update failed. Returning to menu.\n";
                    }
                    break;
                }
                case 5: {
                    if (!ReservationManager::getInstance().hasReservations(username)) {
                        cout << "No reservations.\n";
                        break;
                    }

                    bool processComplete = false;
                    while (!processComplete) {
                        try {
                            string reservationId;
                            cout << "Enter reservation ID to cancel (e.g., ID 1A): ";
                            getline(cin, reservationId);

                            // Show the reservation to confirm
                            ReservationManager::getInstance().viewCustomerReservations(username);

                            // Confirmation prompt
                            string confirm;
                            cout << "Confirm cancellation? Yes or No: ";
                            getline(cin, confirm);
                            if (confirm != "Yes" && confirm != "yes") {
                                cout << "Cancellation aborted.\n";
                                processComplete = true;
                                break;
                            }

                            ReservationManager::getInstance().cancelReservation(reservationId, username);
                            cout << "Reservation cancelled.\n";
                            processComplete = true;
                        } catch (const ReservationException& ex) {
                            cout << "Error: " << ex.what() << endl;
                            ReservationManager::getInstance().logError("Customer", username, "Failed to cancel reservation", ex.what());
                            cout << "Please try again.\n";
                        }
                    }
                    break;
                }
                case 6: {
                    string logout;
                    cout << "Logout? Yes or No: ";
                    getline(cin, logout);
                    if (logout == "Yes" || logout == "yes") {
                        return true; // Logout
                    }
                    break; // Continue in menu
                }
            }
        }
        return false; // Default: continue in menu
    }
};

class Receptionist : public User {
public:
    Receptionist(const string& name) : User(name, "Receptionist") {}
    bool showMenu() override {
        bool isRunning = true;
        while (isRunning) {
            string input;
            int choice;
            cout << "\n[Receptionist Menu - " << username << "]\n";
            cout << "1. View Reservations\n2. View Table Availability\n3. Exit\nChoice: ";
            getline(cin, input);

            if (!validateNumericInput(input, choice, 1, 3)) {
                cout << "Invalid choice. Please enter a single number between 1 and 3 (e.g., 1, not 1a, 1.1, or 1 1).\n";
                continue;
            }

            switch (choice) {
                case 1:
                    ReservationManager::getInstance().viewLogs();
                    break;
                case 2:
                    ReservationManager::getInstance().viewTableAvailability();
                    break;
                case 3: {
                    string logout;
                    cout << "Logout? Yes or No: ";
                    getline(cin, logout);
                    if (logout == "Yes" || logout == "yes") {
                        return true; // Logout
                    }
                    break; // Continue in menu
                }
            }
        }
        return false; // Default: continue in menu
    }
};

class Admin : public User {
public:
    Admin(const string& name) : User(name, "Admin") {}
    bool showMenu() override {
        bool isRunning = true;
        while (isRunning) {
            string input;
            int choice;
            cout << "\n[Admin Menu - " << username << "]\n";
            cout << "1. View Logs\n";
            cout << "2. View Table Availability\n";
            cout << "3. Update Reservation\n";
            cout << "4. Cancel Reservation\n";
            cout << "5. Create Receptionist Account\n";
            cout << "6. Exit\nChoice: ";
            getline(cin, input);

            if (!validateNumericInput(input, choice, 1, 6)) {
                cout << "Invalid choice. Please enter a single number between 1 and 6 (e.g., 1, not 1a, 1.1, or 1 1).\n";
                continue;
            }

            switch (choice) {
                case 1:
                    ReservationManager::getInstance().viewLogs();
                    break;
                case 2:
                    ReservationManager::getInstance().viewTableAvailability();
                    break;
                case 3: {
                    // Implementation for updating a reservation
                    string customerName, reservationId, newId, newName, newPhone, newDate, newTime, newPartySizeInput, newTableChoiceInput;
                    int newPartySize, newTableChoice;

                    cout << "Enter customer name: ";
                    getline(cin, customerName);
                    
                    if (!ReservationManager::getInstance().hasReservations(customerName)) {
                        cout << "No reservations found for this customer.\n";
                        break;
                    }

                    while (true) {
                        cout << "Enter reservation ID to update (e.g., ID 1A): ";
                        getline(cin, reservationId);
                        try {
                            if (!validateReservationId(reservationId)) {
                                throw ReservationException("Invalid reservation ID format. Use 'ID <number>A', e.g., ID 1A.");
                            }
                            if (!ReservationManager::getInstance().hasReservations(customerName)) {
                                throw ReservationException("No reservation to update.");
                            }
                            break;
                        } catch (const ReservationException& ex) {
                            cout << "Error: " << ex.what() << endl;
                        }
                    }

                    // Show current reservation
                    ReservationManager::getInstance().viewCustomerReservations(customerName);

                    // Get update information
                    cout << "Enter new ID (or 0 to keep current): ";
                    getline(cin, newId);
                    cout << "Enter new name (or 0 to keep current): ";
                    getline(cin, newName);
                    
                    while (true) {
                        cout << "Enter new phone number (e.g., 123-456-7890, or 0 to keep current): ";
                        getline(cin, newPhone);
                        if (newPhone == "0" || validatePhoneNumber(newPhone)) break;
                        cout << "Error: Invalid phone number format. Use XXX-XXX-XXXX.\n";
                    }

                    while (true) {
                        cout << "Enter new party size (or 0 to keep current): ";
                        getline(cin, newPartySizeInput);
                        if (newPartySizeInput == "0") {
                            newPartySize = 0;
                            break;
                        }
                        if (validateNumericInput(newPartySizeInput, newPartySize, 1, INT_MAX)) break;
                        cout << "Error: Invalid party size.\n";
                    }

                    while (true) {
                        cout << "Enter new date (YYYY-MM-DD, or 0 to keep current): ";
                        getline(cin, newDate);
                        if (newDate == "0" || validateDate(newDate)) break;
                        cout << "Error: Invalid date format.\n";
                    }

                    while (true) {
                        cout << "Enter new time (HH:MM, or 0 to keep current): ";
                        getline(cin, newTime);
                        if (newTime == "0" || validateTime(newTime, newDate != "0" ? newDate : CURRENT_DATE)) break;
                        cout << "Error: Invalid time format.\n";
                    }

                    while (true) {
                        cout << "Table options: 0 to keep current, or enter table number (1-10):\n";
                        ReservationManager::getInstance().viewTableAvailability();
                        cout << "Choice: ";
                        getline(cin, newTableChoiceInput);
                        if (validateNumericInput(newTableChoiceInput, newTableChoice, 0, 10)) break;
                        cout << "Error: Invalid table choice.\n";
                    }

                    try {
                        int newTableIndex = (newTableChoice != 0) ? newTableChoice - 1 : -1;
                        ReservationManager::getInstance().updateReservation(reservationId, customerName, 
                                                                          newId, newName, newPhone, newPartySize, 
                                                                          newDate, newTime, newTableIndex);
                        cout << "Reservation updated successfully.\n";
                    } catch (const ReservationException& ex) {
                        cout << "Error: " << ex.what() << endl;
                    }
                    break;
                }
                case 4: {
                    // Implementation for canceling a reservation
                    string customerName, reservationId;
                    
                    cout << "Enter customer name: ";
                    getline(cin, customerName);
                    
                    if (!ReservationManager::getInstance().hasReservations(customerName)) {
                        cout << "No reservations found for this customer.\n";
                        break;
                    }

                    cout << "Enter reservation ID to cancel (e.g., ID 1A): ";
                    getline(cin, reservationId);

                    try {
                        ReservationManager::getInstance().cancelReservation(reservationId, customerName);
                        cout << "Reservation cancelled successfully.\n";
                    } catch (const ReservationException& ex) {
                        cout << "Error: " << ex.what() << endl;
                    }
                    break;
                }
                case 5: {
                    string recUsername, recPassword;
                    while (true) {
                        cout << "Enter new receptionist username: ";
                        getline(cin, recUsername);
                        if (receptionistAccounts.count(recUsername)) {
                            cout << "Username already exists. Please choose a different username.\n";
                            continue;
                        }
                        break;
                    }
                    cout << "Enter password: ";
                    getline(cin, recPassword);
                    receptionistAccounts[recUsername] = recPassword;
                    cout << "Receptionist account created.\n";
                    break;
                }
                case 6: {
    string logout;
    cout << "Logout? (Y/N or Yes/No): ";
    getline(cin, logout);
    // Convert to lowercase for case-insensitive comparison
    transform(logout.begin(), logout.end(), logout.begin(), ::tolower);
    if (logout == "yes" || logout == "y") {
        return true; // Logout
    }
    break;
}
            }
        }
        return false;
    }
};

// -------- Main Driver --------
int main() {
    const string adminUsername = "admin";
    const string adminPassword = "admin123";

    bool isRunning = true;
    while (isRunning) {
        string input;
        int roleChoice;
        cout << "\n[Role Selection]\n1. Admin\n2. Receptionist\n3. Customer\n4. Exit\nChoose role: ";
        getline(cin, input);

        if (!validateNumericInput(input, roleChoice, 1, 4)) {
            cout << "Invalid choice. Please enter a single number between 1 and 4 (e.g., 1, not 1a, 1.1, or 1 1).\n";
            continue;
        }

        string username, password;
        unique_ptr<User> user;

        switch (roleChoice) {
            case 1: {
                bool credentialsValid = false;
                while (!credentialsValid) {
                    cout << "Enter Admin username: ";
                    getline(cin, username);
                    cout << "Enter Admin password: ";
                    getline(cin, password);
                    if (username == adminUsername && password == adminPassword) {
                        user = unique_ptr<Admin>(new Admin(username));
                        credentialsValid = true;
                    } else {
                        cout << "Invalid admin credentials. Please try again.\n";
                    }
                }
                break;
            }

            case 2: {
                bool credentialsValid = false;
                while (!credentialsValid) {
                    cout << "Enter Receptionist username: ";
                    getline(cin, username);
                    cout << "Enter password: ";
                    getline(cin, password);
                    if (receptionistAccounts.count(username) && receptionistAccounts[username] == password) {
                        user = unique_ptr<Receptionist>(new Receptionist(username));
                        credentialsValid = true;
                    } else {
                        cout << "Invalid receptionist credentials. Please try again.\n";
                    }
                }
                break;
            }

            case 3: {
                int custOption;
                string custInput;
                while (true) {
                    cout << "\n1. Create Customer Account\n2. Login to Customer Account\nChoice: ";
                    getline(cin, custInput);
                    if (validateNumericInput(custInput, custOption, 1, 2)) {
                        break;
                    }
                    cout << "Invalid choice. Please enter a single number between 1 and 2 (e.g., 1, not 1a, 1.1, or 1 1).\n";
                }

                if (custOption == 1) {
                    bool usernameValid = false;
                    while (!usernameValid) {
                        cout << "Enter username: ";
                        getline(cin, username);
                        if (customerAccounts.count(username)) {
                            cout << "Account already exists. Please choose a different username.\n";
                            continue;
                        }
                        usernameValid = true;
                    }
                    cout << "Enter password: ";
                    getline(cin, password);
                    customerAccounts[username] = password;
                    cout << "Customer account created.\n";
                    user = unique_ptr<Customer>(new Customer(username));
                } else if (custOption == 2) {
                    bool credentialsValid = false;
                    while (!credentialsValid) {
                        cout << "Enter username: ";
                        getline(cin, username);
                        cout << "Enter password: ";
                        getline(cin, password);
                        if (customerAccounts.count(username) && customerAccounts[username] == password) {
                            user = unique_ptr<Customer>(new Customer(username));
                            credentialsValid = true;
                        } else {
                            cout << "Invalid credentials. Please try again.\n";
                        }
                    }
                }
                break;
            }

            case 4:
                isRunning = false;
                continue;
        }

        if (user) {
            bool logout = user->showMenu();
            if (logout) {
                user.reset(); // Clear the current user
                continue; // Loop back to role selection
            }
        }
    }

    return 0;
}

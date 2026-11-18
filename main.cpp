#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;

struct LostFoundItem {
    string id;
    string status;
    string category;
    string name;
    string color;
    string brand;
    string location;
    string description;
    string contact;
    string date;
    bool resolved = false;
};

string csvEscape(const string& value) {
    string escaped;
    for (char c : value) {
        if (c == '"') escaped += "\"\"";
        else escaped += c;
    }
    if (escaped.find_first_of(",\"\r\n") != string::npos)
        escaped = "\"" + escaped + "\"";
    return escaped;
}

vector<string> parseCsvLine(const string& line) {
    vector<string> fields;
    string field;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"') {
                field += '"';
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == ',' && !inQuotes) {
            fields.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    fields.push_back(field);
    return fields;
}

class CsvDatabase {
private:
    string filename_;
    const string header_ =
        "id,status,category,name,color,brand,location,description,contact,date,resolved";

public:
    explicit CsvDatabase(const string& filename) : filename_(filename) {
        ifstream check(filename_);
        if (!check.good()) {
            ofstream out(filename_);
            if (!out) throw runtime_error("Could not create data file: " + filename_);
            out << header_ << '\n';
        }
    }

    vector<LostFoundItem> readAll() const {
        ifstream in(filename_);
        if (!in) throw runtime_error("Could not open data file: " + filename_);

        vector<LostFoundItem> items;
        string line;
        getline(in, line);
        size_t lineNumber = 1;
        while (getline(in, line)) {
            ++lineNumber;
            if (line.empty()) continue;

            vector<string> fields = parseCsvLine(line);
            if (fields.size() != 11) {
                cerr << "Skipping malformed CSV row " << lineNumber << ".\n";
                continue;
            }

            LostFoundItem item;
            item.id = fields[0];
            item.status = fields[1];
            item.category = fields[2];
            item.name = fields[3];
            item.color = fields[4];
            item.brand = fields[5];
            item.location = fields[6];
            item.description = fields[7];
            item.contact = fields[8];
            item.date = fields[9];
            item.resolved = fields[10] == "1" || fields[10] == "true";
            items.push_back(item);
        }
        return items;
    }

    void writeAll(const vector<LostFoundItem>& items) const {
        ofstream out(filename_, ios::trunc);
        if (!out) throw runtime_error("Could not write data file: " + filename_);
        out << header_ << '\n';
        for (const LostFoundItem& item : items) {
            out << csvEscape(item.id) << ','
                << csvEscape(item.status) << ','
                << csvEscape(item.category) << ','
                << csvEscape(item.name) << ','
                << csvEscape(item.color) << ','
                << csvEscape(item.brand) << ','
                << csvEscape(item.location) << ','
                << csvEscape(item.description) << ','
                << csvEscape(item.contact) << ','
                << csvEscape(item.date) << ','
                << (item.resolved ? "1" : "0") << '\n';
        }
        if (!out) throw runtime_error("Failed while saving data file: " + filename_);
    }
};

string lower(string value) {
    transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(tolower(c)); });
    return value;
}

string trim(const string& value) {
    const size_t first = value.find_first_not_of(" \t\r\n");
    if (first == string::npos) return "";
    const size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

string readLine(const string& prompt) {
    cout << prompt;
    string value;
    if (!getline(cin, value)) throw runtime_error("Input ended.");
    return trim(value);
}

string readRequired(const string& prompt) {
    while (true) {
        string value = readLine(prompt);
        if (!value.empty()) return value;
        cout << "This field is required. Please try again.\n";
    }
}

int readChoice() {
    while (true) {
        string value = readLine("Choose an option: ");
        try {
            size_t consumed = 0;
            int choice = stoi(value, &consumed);
            if (consumed == value.size()) return choice;
        } catch (const exception&) {
        }
        cout << "Enter a number from the menu.\n";
    }
}

string createId(const vector<LostFoundItem>& items) {
    unsigned long long number = 1;
    for (const LostFoundItem& item : items) {
        if (item.id.rfind("ITEM", 0) != 0) continue;
        try {
            number = max(number, stoull(item.id.substr(4)) + 1);
        } catch (const exception&) {
        }
    }
    return "ITEM" + to_string(number);
}

void printItem(const LostFoundItem& item, int index, int score = -1) {
    cout << "\n" << index << ". [" << item.id << "] "
         << (item.status == "found" ? "FOUND" : "LOST") << ": " << item.name;
    if (score >= 0) cout << "  (" << score << "% match)";
    cout << "\n   Category: " << item.category;
    if (!item.color.empty()) cout << " | Color: " << item.color;
    if (!item.brand.empty()) cout << " | Brand: " << item.brand;
    cout << "\n   Location: " << item.location << " | Date: " << item.date
         << "\n   Details: " << (item.description.empty() ? "None" : item.description)
         << "\n   Contact: " << item.contact
         << "\n   Status: " << (item.resolved ? "Returned/resolved" : "Open") << '\n';
}

void listItems(const vector<LostFoundItem>& items, bool includeResolved = false) {
    int displayed = 0;
    for (const LostFoundItem& item : items) {
        if (!includeResolved && item.resolved) continue;
        printItem(item, ++displayed);
    }
    if (displayed == 0) cout << "\nNo " << (includeResolved ? "" : "open ") << "reports found.\n";
}

bool matchesText(const string& value, const string& query) {
    return !query.empty() && lower(value).find(lower(query)) != string::npos;
}

int matchScore(const LostFoundItem& item, const LostFoundItem& query) {
    int score = 0;
    const string itemName = lower(item.name);
    const string queryName = lower(query.name);
    if (!queryName.empty()) {
        if (itemName == queryName) score += 55;
        else if (itemName.find(queryName) != string::npos ||
                 queryName.find(itemName) != string::npos) score += 40;
        else {
            istringstream words(queryName);
            string word;
            int matched = 0;
            while (words >> word) {
                if (word.size() > 2 && itemName.find(word) != string::npos) ++matched;
            }
            if (matched > 0) score += min(35, matched * 15);
        }
    }
    if (!query.category.empty() && lower(item.category) == lower(query.category))
        score += 15;
    if (!query.color.empty() && matchesText(item.color, query.color))
        score += 15;
    if (!query.brand.empty() && matchesText(item.brand, query.brand))
        score += 10;
    if (!query.description.empty() &&
        (matchesText(item.description, query.description) ||
         matchesText(item.location, query.description))) score += 10;
    return min(score, 100);
}

void showMatches(const vector<LostFoundItem>& items, const LostFoundItem& query) {
    const string targetStatus = query.status == "lost" ? "found" : "lost";
    vector<pair<int, const LostFoundItem*>> matches;
    for (const LostFoundItem& item : items) {
        if (item.resolved || item.status != targetStatus) continue;
        int score = matchScore(item, query);
        if (score >= 20) matches.push_back({score, &item});
    }
    sort(matches.begin(), matches.end(),
        [](const auto& a, const auto& b) { return a.first > b.first; });

    cout << "\nPossible " << targetStatus << " matches:\n";
    if (matches.empty()) {
        cout << "No possible matches found yet.\n";
        return;
    }
    for (size_t i = 0; i < matches.size(); ++i)
        printItem(*matches[i].second, static_cast<int>(i + 1), matches[i].first);
}

LostFoundItem promptItem(const string& status) {
    LostFoundItem item;
    item.status = status;
    cout << "\nEnter the report details (" << status << " item):\n";
    item.name = readRequired("Item name: ");
    item.category = readRequired("Category: ");
    item.color = readLine("Color (optional): ");
    item.brand = readLine("Brand (optional): ");
    item.location = readRequired(status == "found"
        ? "Where was it found? " : "Where was it last seen? ");
    item.description = readLine("Other details (optional): ");
    item.contact = readRequired("Your phone or email: ");
    item.date = readRequired("Date (e.g. 2026-11-13): ");
    return item;
}

void addReport(CsvDatabase& database, const string& status) {
    vector<LostFoundItem> items = database.readAll();
    LostFoundItem item = promptItem(status);
    item.id = createId(items);
    items.push_back(item);
    database.writeAll(items);
    cout << "\nReport saved with ID " << item.id << ".\n";
    showMatches(items, item);
}

void searchFoundItems(const vector<LostFoundItem>& items) {
    LostFoundItem query;
    query.status = "lost";
    cout << "\nDescribe the item you lost:\n";
    query.name = readRequired("Item name: ");
    query.category = readLine("Category (optional): ");
    query.color = readLine("Color (optional): ");
    query.brand = readLine("Brand (optional): ");
    query.description = readLine("Other details (optional): ");
    showMatches(items, query);
}

void markReturned(CsvDatabase& database) {
    vector<LostFoundItem> items = database.readAll();
    string id = readRequired("Enter the report ID to mark returned: ");
    for (LostFoundItem& item : items) {
        if (item.id == id) {
            if (item.resolved) {
                cout << "That report is already marked returned/resolved.\n";
                return;
            }
            item.resolved = true;
            database.writeAll(items);
            cout << "Report " << id << " marked returned/resolved.\n";
            return;
        }
    }
    cout << "No report found with ID " << id << ".\n";
}

void showStats(const vector<LostFoundItem>& items) {
    int found = 0, lost = 0, returned = 0;
    for (const LostFoundItem& item : items) {
        if (item.resolved) ++returned;
        else if (item.status == "found") ++found;
        else if (item.status == "lost") ++lost;
    }
    cout << "\nOpen found reports: " << found
         << "\nOpen lost reports: " << lost
         << "\nReturned/resolved: " << returned << "\n";
}

void printMenu() {
    cout << "\n=== Campus Connect: Lost & Found ===\n"
         << "1. View open reports\n"
         << "2. Search found items for a lost item\n"
         << "3. Report a found item\n"
         << "4. Report a lost item\n"
         << "5. Mark a report returned/resolved\n"
         << "6. View statistics\n"
         << "0. Exit\n";
}

int main(int argc, char* argv[]) {
    try {
        const string filename = argc > 1 ? argv[1] : "items.csv";
        CsvDatabase database(filename);
        cout << "Campus Connect terminal app\n"
             << "Data file: " << filename << '\n';

        while (true) {
            printMenu();
            int choice = readChoice();
            switch (choice) {
                case 1:
                    listItems(database.readAll());
                    break;
                case 2:
                    searchFoundItems(database.readAll());
                    break;
                case 3:
                    addReport(database, "found");
                    break;
                case 4:
                    addReport(database, "lost");
                    break;
                case 5:
                    markReturned(database);
                    break;
                case 6:
                    showStats(database.readAll());
                    break;
                case 0:
                    cout << "Goodbye.\n";
                    return 0;
                default:
                    cout << "Choose one of the listed options.\n";
            }
        }
    } catch (const exception& error) {
        if (string(error.what()) != "Input ended.")
            cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}

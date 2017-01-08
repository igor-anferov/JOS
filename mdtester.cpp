#include <iostream>
#include <sstream>
#include <string>

using namespace std;

enum Colours {
    Red = 31,
    Green
};

void colored_output(const string s, Colours c) {
    cout << "\x1b["+to_string(c)+"m"+s+"\x1b[0m" << endl;
}

void wait_for(const string need) {
    string s;
    do {
        getline(cin, s);
    } while (s != need);
}

int wait_for_prefix(const string need) {
    string s;
    do {
        getline(cin, s);
    } while (s.find(need) != 0 );
    
    stringstream ss(s.substr(need.length()));
    int ret;
    ss << hex;
    ss >> ret;
    return ret;
}

void assert(bool s) {
    if (!s) {
        colored_output("Testing failed!", Red);
        exit(-1);
    }
}

int main() {
    wait_for("test1: pages filled");
    wait_for("+ Environment 00001001:");
    int first_page = wait_for_prefix("        PTX: 0000 | va: 0xe0000*** --> pa: ");
    int second_page = wait_for_prefix("        PTX: 0001 | va: 0xe0001*** --> pa: ");
    assert(first_page != second_page);
    wait_for("--------- END OF MEMORY DEDUPLICATION ---------");
    wait_for("+ Environment 00001001:");
    first_page = wait_for_prefix("        PTX: 0000 | va: 0xe0000*** --> pa: ");
    second_page = wait_for_prefix("        PTX: 0001 | va: 0xe0001*** --> pa: ");
    assert(first_page == second_page);
    colored_output("Test 1 pass!", Green);
    
    wait_for("test2: page filled");
    wait_for("--------- END OF MEMORY DEDUPLICATION ---------");
    wait_for("+ Environment 00001001:");
    first_page = wait_for_prefix("        PTX: 0000 | va: 0xe0000*** --> pa: ");
    second_page = wait_for_prefix("        PTX: 0001 | va: 0xe0001*** --> pa: ");
    assert(first_page == second_page);
    wait_for("+ Environment 00001002:");
    int fourth_page = wait_for_prefix("        PTX: 0003 | va: 0xe0003*** --> pa: ");
    assert(fourth_page == second_page);
    colored_output("Test 2 pass!", Green);
    
    wait_for("test1: page 2 changed");
    wait_for("+ Environment 00001001:");
    first_page = wait_for_prefix("        PTX: 0000 | va: 0xe0000*** --> pa: ");
    second_page = wait_for_prefix("        PTX: 0001 | va: 0xe0001*** --> pa: ");
    assert(first_page != second_page);
    wait_for("+ Environment 00001002:");
    fourth_page = wait_for_prefix("        PTX: 0003 | va: 0xe0003*** --> pa: ");
    assert(fourth_page == first_page);
    colored_output("Test 3 pass!", Green);
    
    wait_for("test1: page 3 filled");
    wait_for("--------- END OF MEMORY DEDUPLICATION ---------");
    wait_for("+ Environment 00001001:");
    first_page = wait_for_prefix("        PTX: 0000 | va: 0xe0000*** --> pa: ");
    second_page = wait_for_prefix("        PTX: 0001 | va: 0xe0001*** --> pa: ");
    int third_page = wait_for_prefix("        PTX: 0002 | va: 0xe0002*** --> pa: ");
    assert(third_page == first_page);
    wait_for("+ Environment 00001002:");
    fourth_page = wait_for_prefix("        PTX: 0003 | va: 0xe0003*** --> pa: ");
    assert(fourth_page == first_page);
    colored_output("Test 4 pass!", Green);
    
    return 0;
}

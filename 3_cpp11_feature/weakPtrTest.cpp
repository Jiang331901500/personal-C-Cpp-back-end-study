#include <iostream>
#include <memory>
using namespace std;

int main()
{
    shared_ptr<int> sp(new int(5));
    weak_ptr<int> wp = sp;      // 1

    cout << "wp use_count = " << wp.use_count() << endl;    // 2

    if(!wp.expired())   // 3
        cout << "*wp value = " << *(wp.lock()) << endl; // 4
}
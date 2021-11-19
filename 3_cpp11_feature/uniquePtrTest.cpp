#include <iostream>
#include <memory>
using namespace std;
int main()
{
    unique_ptr<int> up1(new int(10));
    unique_ptr<int> up2 = move(up1);    // move
    auto up3 = make_unique<int>(5);

    // cout << "*up1 = " << *up1 << endl;   // up1 has been released
    cout << "*up2 = " << *up2 << endl;
    cout << "*up3 = " << *up3 << endl;

    unique_ptr<int []> upArray(new int[100]);
    unique_ptr<int, void(*)(int *)> up(new int(1), [](int* p){delete p;});
}
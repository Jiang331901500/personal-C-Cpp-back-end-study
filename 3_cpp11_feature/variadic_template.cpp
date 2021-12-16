#include <iostream>
#include <functional>
using namespace std;

template<typename F, typename...Args>
void expand(const F& f, Args&&...args)
{
    initializer_list<int>{(f( forward<Args>(args) ), 0)...};
}

int main()
{
    expand([](int i){ cout << i << endl; }, 1,2,3,4);
}
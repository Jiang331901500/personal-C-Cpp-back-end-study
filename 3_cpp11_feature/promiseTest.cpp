#include <iostream>
#include <string>
#include <thread>
#include <future>
using namespace std;
using sec = chrono::seconds;
using this_thread::sleep_for;

void fullfill_the_promise(promise<string>& promis)
{
    sleep_for(sec(2));
    promis.set_value("Promise has been fullfilled!");   // after set_value, future is ready
}

int main()
{
    promise<string> myPromise;          // I promise that a string will be set!

    future<string> result = myPromise.get_future();      // this is where it will be put, let's wait and see.

    thread deliver(fullfill_the_promise, ref(myPromise));    // Hey, bro. Please help me fullfill it.
    cout << "Ok. let's wait...\n";
    cout << "promise result: " << result.get() << endl;     // promise is done

    deliver.join();
}
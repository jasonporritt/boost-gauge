#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

class Timer
{
    thread th;
    bool running = false;

public:
    typedef std::chrono::milliseconds Interval;
    typedef std::function<void(void)> Timeout;

    void start(const Interval &interval,
               const Timeout &timeout)
    {
        running = true;

        th = thread([=]()
        {
            while (running == true) {
                this_thread::sleep_for(interval);
                timeout();
            }
        });

    }

    void stop()
    {
        running = false;
        th.join();
    }
};

int main(void)
{
    Timer tHello;
    tHello.start(chrono::milliseconds(1000), []()
    {
        cout << "Hello!" << endl;
    });

    thread th([&]()
    {
        this_thread::sleep_for(chrono::seconds(2));
        tHello.stop();
    });

    th.join();

    return 0;
}

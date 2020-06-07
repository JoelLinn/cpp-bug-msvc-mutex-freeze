#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include <Windows.h>

// spend time without calling kernel sleep
// likely wouldn't matter but don't take chances
void SpinWait(std::chrono::milliseconds dur) {
    auto start = std::chrono::system_clock::now();
    while (dur > std::chrono::system_clock::now() - start) {}
}

std::mutex mutex_;

std::thread t_spin_a;
std::thread t_spin_b;

// thread main
void main_spin(const std::wstring& ident) {
    const auto name = L"main_spin_" + ident;
    SetThreadDescription(GetCurrentThread(), name.c_str());

    int i = 0;
    while (i < 10) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::wcout << ident << L" spinning..  " << ++i << std::endl;
        }
        SpinWait(std::chrono::milliseconds(100));
    }
}

// This _sometimes_ results in an application freeze after printing
// "I'm in danger..."

int main()
{
    SetThreadDescription(GetCurrentThread(), L"main");

    t_spin_a = std::thread([]() {main_spin(L"A"); });
    t_spin_b = std::thread([]() {main_spin(L"B"); });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "I'm in danger..." << std::endl;
        // Make crt give up spinning on the mutex by calling into 
        // NtWaitForAlertByThreadId() syscall.
        SpinWait(std::chrono::milliseconds(200));
        // Suspend thread 'a' which is trying to lock the mutex_
        SuspendThread(t_spin_a.native_handle());
    }

    t_spin_a.detach();
    t_spin_b.join();

    return 0;
}

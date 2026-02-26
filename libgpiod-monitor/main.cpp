#include <iostream>
#include <atomic> // for threadsafe variables (variables shared between threads must be atomic)
#include <csignal> // to safely handle CTRL+C signal interrupt

#include <unistd.h>
#include <gpiod.hpp>

/*
This code is an attempting to explain how to monitor a gpioline for edge-events (voltage changes) and keep track of the time it took.
Typically you don't want to block your entire program to simply increment a number to keep track of time, so you ideally delegate that task to a
background thread while your main logic remains running. I'll show how to do that here.

In this code I'm going to have a loop that watches for an edge-event in the main thread and increments a timer in another thread.
I'll also show how to safely stop the background thread via CTRL+C without terminating the entire program.
*/

// Start off by making your interrupt variable and your threadsafe counter:
std::atomic<bool> continue_running(true);
std::atomic<int> seconds(0);

// This will be the function the second thread runs while the main thread will run main() as usual.
int count_forever(){
    std::cout << "Thread 2: Starting monitor timer..." << '\n';
    fsleep(0.1);

    // using continue_running variable as start/stop condition (we're going to link this to CTRL+C)
    while(continue_running){
        std::cout << "Thread 2: " << "Seconds Elapsed = " << seconds << '\n';
        seconds++;

        fsleep(1);
    }
}

// This function will be registered to the CTRL+C interrupt (SIGINT) via <csignal>
// That means instead of terminating the program CTRL+C will run this function to stop the second thread instead.
void stop_counting(int signum){
    std::cout << "Interrupt signal: " << signum << " recieved. Stopping timer..." << '\n';
    continue_running = false;
}

// sleep for X seconds
double fsleep(double x){
    return usleep(x * 1000000);
}


int main(){

    // Register stop_counting() as your CTRL+C function via <csignal>
    std::signal(SIGINT, stop_counting);

    // Begin program
    std::cout << "Thread 1: This is the main thread. In here we'll be monitoring gpioline " << "for edge-events indicating a change in voltage." << '\n';

    gpiod::chip main_header("/dev/gpiochip0"); // 40pin header on the rpi4.
    std::cout << "Thread 1: Successfully instantitated chip object." << '\n';

    

    return 0;
}



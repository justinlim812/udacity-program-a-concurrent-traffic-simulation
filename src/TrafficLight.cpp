#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */
template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function.
    std::unique_lock<std::mutex> uLock(_mutex);
    _cond.wait(uLock, [this] {return !_queue.empty();});

    T msg = std::move(_queue.back());
    _queue.pop_back();
    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::lock_guard<std::mutex> lock_guard(_mutex);
    
    // std::cout << "Message: " << msg << "sent to queue" << std::endl;
    _queue.clear();
    _queue.push_back(std::move(msg));
    _cond.notify_one();
}


/* Implementation of class "TrafficLight" */
TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
    while(true){
       auto currentLight = _trafficLightPhases.receive();
       if (currentLight == TrafficLightPhase::green){
        return;
       }
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

    // Initialize stop watch
    std::chrono::high_resolution_clock::time_point startWatch = std::chrono::high_resolution_clock::now();
    int64_t duration{0};

    while (true){
        // Generate random values in range 4 to 6
        std::default_random_engine generator;
        std::uniform_int_distribution<int> distribution(4,6);
        int randomDuration = distribution(generator);

        // Update duration
        while(duration < randomDuration){
            std::chrono::high_resolution_clock::time_point stopWatch = std::chrono::high_resolution_clock::now();
            duration = std::chrono::duration_cast<std::chrono::seconds>( stopWatch - startWatch ).count();
            // std::cout << "Duration: " << duration <<" seconds" << std::endl;

            // Wait for 1ms to reduce load on processor
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        
        // Toggle between red and green lights
        if(_currentPhase == TrafficLightPhase::red){
            _currentPhase = TrafficLightPhase::green;
            std::unique_lock<std::mutex> lightLock(_mutex);
            std::cout << "Traffic light #" << std::this_thread::get_id() << " is now green" << std::endl;
            lightLock.unlock();
        }
        // Light is green
        else{
            _currentPhase = TrafficLightPhase::red;
            std::unique_lock<std::mutex> lightLock(_mutex);
            std::cout << "Traffic light #" << std::this_thread::get_id() << " is now red" << std::endl;
            lightLock.unlock();
        }

        // Push each new traffic light phase into message queue
        _trafficLightPhases.send(std::move(_currentPhase));

        // Reset stopwatch
        startWatch = std::chrono::high_resolution_clock::now();
        duration = 0;
    }
}
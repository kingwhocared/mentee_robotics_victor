#include <pybind11/pybind11.h>

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

#include <iostream>
#include <thread>
#include <iostream>
#include <chrono>
#include <functional>
#include <vector>
#include <mutex>
#include <map>
#include <string>

using std::string;
using namespace std;

int add(int i, int j) {
    return i + j;
}

string mypybind_hello_world() {
    std::cout << "I can also print to stdout from within the c++ code.";
    return "hello from c++!";
}

void init_brain_dummy(int dummy_n, string dummy_func) {
    std::cout << "I can work with:";
    std::cout << dummy_n;
    std::cout << dummy_func;
}

int MILLISECONDS_IN_100HZ;
int MILLISECONDS_IN_10HZ;
int MILLISECONDS_IN_250HZ;


class Motor {
    public:
        float motor_pos;
        int motor_id;
        Motor(int motor_id) {
            this->motor_id = motor_id;
            this->motor_pos = 1.0;
        }
};

vector<Motor> motors = {};
//vector<float> motors_positions = {};
mutex motors_mutex;
//mutex motors_positions_mutex;

int N_MOTORS;
void(*selected_net_function)();

float random_number_generator() {
    float random_number = ((float)rand()) / RAND_MAX;
    random_number -= 0.5; // from 0:1 to -0.5:0.5
    random_number *= 2; // from -0.5:0.5 to -1:1

    //std::cout << "random_number: ";
    //std::cout << random_number;
    //std::cout << "\n";

    return random_number;
}

void net_function_set_all_motors_to_1() {
    motors_mutex.lock();
    std::cout << "[Net Actions] - Setting all motors to 1\n";
    for (int i = 0; i < N_MOTORS; i++) {
        motors[i].motor_pos = 1.0;
    }
    motors_mutex.unlock();
}

void net_function_set_all_motors_to_2() {
    motors_mutex.lock();
    std::cout << "[Net Actions] - Setting all motors to 2\n";
    for (int i = 0; i < N_MOTORS; i++) {
        motors[i].motor_pos = 2.0;
    }
    motors_mutex.unlock();
}

std::map<std::string, void (*)()> funcname_to_func_mapping;

void LoopForeverAndExecuteWithTimePadding(void (*func_to_execute)(), int miliseconds_to_pad) {
    while (true) {
        auto start = chrono::steady_clock::now();
        func_to_execute();
        auto end = chrono::steady_clock::now();
        auto elapsed_time_in_miliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        //std::cout << "elapsed_time_in_miliseconds:\n";
        //std::cout << elapsed_time_in_miliseconds.count();
        auto remaining_miliseconds_to_sleep = std::chrono::milliseconds(miliseconds_to_pad) - elapsed_time_in_miliseconds;
        std::this_thread::sleep_for(remaining_miliseconds_to_sleep);
    }
}

void MainLoopLogic() {
    //std::cout << "MainLoopLogic\n";
    selected_net_function();


    //motors_mutex.lock();
    //std::cout << "[my looping] - ";
    //for (int i = 0; i < N_MOTORS; i++) {
    //    std::cout << motors[i].motor_pos;
    //    if (i != N_MOTORS - 1) {
    //        std::cout << ", ";
    //    }
    //}
    //std::cout << "\n";
    //motors_mutex.unlock();
}

void MotorsLoopLogic() {
    //std::cout << "MotorsLoopLogic\n";

    motors_mutex.lock();
    std::cout << "[Motor positions] - ";
    for (int i = 0; i < N_MOTORS; i++) {
        motors[i].motor_pos *= 1.1;
        std::cout << motors[i].motor_pos;
        if (i != N_MOTORS - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "\n";
    motors_mutex.unlock();
}

void PositionLoopLogic() {
    //std::cout << "PositionLoopLogic\n";

    motors_mutex.lock();
    std::cout << "[Position] - ";

    for (int i = 0; i < N_MOTORS; i++) {
        motors[i].motor_pos *= (1 + 0.001 * random_number_generator());
        std::cout << motors[i].motor_pos;
        if (i != N_MOTORS - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "\n";
    motors_mutex.unlock();
}

void MainLoopThreadLogic() {
    //std::cout << "MainLoopThread Initiated.\n";
    LoopForeverAndExecuteWithTimePadding(&MainLoopLogic, MILLISECONDS_IN_10HZ);
}

void MotorsLoopThreadLogic() {
    //std::cout << "MotorsLoopThread Initiated.\n";
    LoopForeverAndExecuteWithTimePadding(&MotorsLoopLogic, MILLISECONDS_IN_100HZ);
}

void PositionLoopThreadLogic() {
    //std::cout << "PositionLoopThread Initiated.\n";
    LoopForeverAndExecuteWithTimePadding(&PositionLoopLogic, MILLISECONDS_IN_250HZ);
}


void init_brain(int n_motors, string selected_net_function_name, int DEBUG_SLOW_TIME_FACTOR)
{
    MILLISECONDS_IN_100HZ = DEBUG_SLOW_TIME_FACTOR * 1.0 * 1000 / 100;
    MILLISECONDS_IN_10HZ = DEBUG_SLOW_TIME_FACTOR * 1.0 * 1000 / 10;
    MILLISECONDS_IN_250HZ = DEBUG_SLOW_TIME_FACTOR * 1.0 * 1000 / 250;

    funcname_to_func_mapping["net_function_set_all_motors_to_1"] = net_function_set_all_motors_to_1;
    funcname_to_func_mapping["net_function_set_all_motors_to_2"] = net_function_set_all_motors_to_2;

    selected_net_function = funcname_to_func_mapping[selected_net_function_name];

    N_MOTORS = n_motors;
    // init motors and positions
    for (int motor_i = 1; motor_i <= N_MOTORS; motor_i++) {
        motors.push_back(Motor(motor_i));
        //motors_positions.push_back(1.0);
    }

    thread MainLoopThread(MainLoopThreadLogic);
    thread MotorsLoopThread(MotorsLoopThreadLogic);
    thread PositionLoopThread(PositionLoopThreadLogic);

    MainLoopThread.join();
    //MotorsLoopThread.join();
    std::cout << "main() is done.\n";
}


namespace py = pybind11;

PYBIND11_MODULE(my_robot_brain_cpp_logic, m) {
    m.doc() = R"pbdoc(
        Pybind11 example plugin
        -----------------------

        .. currentmodule:: my_robot_brain_cpp_logic

        .. autosummary::
           :toctree: _generate

           add
           subtract
    )pbdoc";

    m.def("add", &add, R"pbdoc(
        Add two numbers

        Some other explanation about the add function.
    )pbdoc");

    m.def("mypybind_hello_world", &mypybind_hello_world, R"pbdoc(
        docs of mypybind_hello_world
    )pbdoc");

    m.def("init_brain_dummy", &init_brain_dummy, R"pbdoc(
        docs of init_brain_dummy
    )pbdoc");

    m.def("init_brain", &init_brain, R"pbdoc(
        docs of init_brain
    )pbdoc");

    m.def("subtract", [](int i, int j) { return i - j; }, R"pbdoc(
        Subtract two numbers

        Some other explanation about the subtract function.
    )pbdoc");

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}

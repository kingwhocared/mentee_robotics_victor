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
#include <math.h>


// i should have imported pi constant properly, instead of defining it here.
# define M_PI           3.14159265358979323846

using std::string;
using namespace std;

int MILLISECONDS_IN_100HZ;
int MILLISECONDS_IN_10HZ;
int MILLISECONDS_IN_250HZ;

float MAX_STABLE_FOOT_DISTANCE_FROM_KNEE;
int N_LEGS;
int N_LEGS_NEEDED_TO_STAY_STANDING;
float POSITION_NOISE_FACTOR;
void(*selected_net_function)();
int MILISECONDS_TO_PLACE_FOOT_BACK_BELOW_KNEE;
bool the_spider_is_standing = true; // no mutex required here, even though multiple threads use this.
float TAU_P;
float TAU_D;
int tens_of_miliseconds_survived = 0;

float random_number_generator() {
    float random_number = ((float)rand()) / RAND_MAX;
    random_number -= 0.5; // from 0:1 to -0.5:0.5
    random_number *= 2; // from -0.5:0.5 to -1:1

    //std::cout << "random_number: ";
    //std::cout << random_number;
    //std::cout << "\n";

    return random_number;
}

class SpiderLeg {
    public:
        const float neutral_knee_position = 100.0;
        float degrees_leg_should_be;
        const float epsilon_min_leg_angle_with_neighboring_leg = (2 * M_PI * 5 / 360); // legs cannot be exactly perpendicular
        // for lateral position accel and velocity
        float foot_distance_from_neutral_knee_position;
        float lateral_muscle_state;
        float lateral_leg_velocity;
        // for angular position accel and velocity
        float leg_horizontal_angle_delta;
        float rotational_muscle_state;
        float angular_velocity;

        int miliseconds_since_started_to_moveback_knee;
        bool is_in_contact_with_the_ground;
        int id;
        SpiderLeg* leg_to_the_left;
        SpiderLeg* leg_to_the_right;
        SpiderLeg(int id, int total_n_legs) {
            this->id = id;
            this->lateral_muscle_state = 0.0;
            this->lateral_leg_velocity = 0.0;
            this->rotational_muscle_state = 0.0;
            this->angular_velocity = 0.0;
            this->foot_distance_from_neutral_knee_position = 0.0;
            this->degrees_leg_should_be = id * (2 * M_PI / total_n_legs);
            std::cout << "Leg: ";
            std::cout << id;
            std::cout << " should be at angle: ";
            std::cout << this->degrees_leg_should_be;
            std::cout << "\n";
            this->leg_horizontal_angle_delta = 0.0;
            this->is_in_contact_with_the_ground = true;
            this->leg_to_the_left = NULL; // is expected to be filled later.
        }
        std::tuple<float, float> get_leg_position_in_cartesian_coords() {
            float dist_from_center_of_mass = this->neutral_knee_position + this->foot_distance_from_neutral_knee_position;
            float angle_from_center = this->leg_horizontal_angle_delta + this->degrees_leg_should_be;
            float x = dist_from_center_of_mass * cos(angle_from_center);
            float y = dist_from_center_of_mass * sin(angle_from_center);
            return std::make_tuple(x, y);
        }
        void set_leg_position_from_cartesian_coords(float x, float y) {
            float r = sqrt(x * x + y * y);
            float theta = atan2(y, x);
            float _theta_raw = theta;
            if (theta < 0) {
                theta += 2 * M_PI;
            }
            else if (theta >= 2 * M_PI) {
                theta -= 2 * M_PI;
            }
            this->foot_distance_from_neutral_knee_position = r - this->neutral_knee_position;
            float new_angle = theta - this->degrees_leg_should_be;
            //if (new_angle < 0) {
            //    new_angle += 2 * M_PI;
            //}
            //else if (new_angle >= 2 * M_PI) {
            //    new_angle -= 2 * M_PI;
            //}
            this->leg_horizontal_angle_delta = new_angle;
        }
        void add_random_noise_to_leg_position() {
            float x;
            float y;
            tie(x, y) = this->get_leg_position_in_cartesian_coords();
            x += POSITION_NOISE_FACTOR * random_number_generator();
            y += POSITION_NOISE_FACTOR * random_number_generator();
            this->set_leg_position_from_cartesian_coords(x, y);
        }
        void apply_muscle_forces_and_move_the_leg() {
            if (this->is_in_contact_with_the_ground) {
                // update velocities
                this->angular_velocity += this->rotational_muscle_state;
                this->lateral_leg_velocity += this->lateral_muscle_state;
                // move the leg
                this->leg_horizontal_angle_delta += this->angular_velocity;
                this->foot_distance_from_neutral_knee_position += this->lateral_leg_velocity;
            }
        }
        // this function is wrong. keeping it for reference.
        void _make_sure_neighboring_legs_dont_cross() {
            if ((not this->is_in_contact_with_the_ground) or (not this->leg_to_the_left->is_in_contact_with_the_ground)) {
                return; // constraint isnt enforced when a leg is airborne.
            }
            // actually, each leg just needs to check with on of its neighbors
            float left_leg_angle = this->leg_to_the_left->degrees_leg_should_be + this->leg_to_the_left->leg_horizontal_angle_delta;
            float this_leg_angle = this->degrees_leg_should_be + this->leg_horizontal_angle_delta;
            if (this->leg_to_the_left->id == 0) {
                left_leg_angle += 2 * M_PI;
            }
            if (left_leg_angle < (this_leg_angle + this->epsilon_min_leg_angle_with_neighboring_leg)) {
                // there is a leg cross-over, undo it.
                std::cout << "leg crossover detected between: ";
                std::cout << this->id;
                std::cout << ", ";
                std::cout << this->leg_to_the_left->id;
                std::cout << "\n";
                float middle = (left_leg_angle + this_leg_angle) / 2;
                this->leg_horizontal_angle_delta = (middle - (this->epsilon_min_leg_angle_with_neighboring_leg) / 2) - this->degrees_leg_should_be;
                this->leg_to_the_left->leg_horizontal_angle_delta = (middle - (this->epsilon_min_leg_angle_with_neighboring_leg) / 2) - this->leg_to_the_left->degrees_leg_should_be;
                // ensure angle between -2pi and 2pi
                if (this->leg_horizontal_angle_delta < 2 * M_PI) {
                    this->leg_horizontal_angle_delta += 2 * M_PI;
                }
                if (this->leg_to_the_left->leg_horizontal_angle_delta < 2 * M_PI) {
                    this->leg_to_the_left->leg_horizontal_angle_delta += 2 * M_PI;
                }
                if (this->leg_horizontal_angle_delta > 2 * M_PI) {
                    this->leg_horizontal_angle_delta -= 2 * M_PI;
                }
                if (this->leg_to_the_left->leg_horizontal_angle_delta > 2 * M_PI) {
                    this->leg_to_the_left->leg_horizontal_angle_delta -= 2 * M_PI;
                }
                // zero the angular velocity due to the collision
                this->angular_velocity = 0.0;
                this->leg_to_the_left->angular_velocity = 0.0;
            }
        }
        void foot_distance_from_knee_mechanics_logic() {
            // if foot distance is too far from the knee, the spider lifts it up in the air from the ground and places it back below the knee
            if (this->is_in_contact_with_the_ground) {
                if (abs(this->foot_distance_from_neutral_knee_position) > MAX_STABLE_FOOT_DISTANCE_FROM_KNEE) {
                    std::cout << "Spider has to lift leg number: ";
                    std::cout << this->id;
                    std::cout << "\n";
                    this->is_in_contact_with_the_ground = false;
                    this->miliseconds_since_started_to_moveback_knee = 0;
                }
            }
            else {
                // leg in the air
                if (this->miliseconds_since_started_to_moveback_knee >= MILISECONDS_TO_PLACE_FOOT_BACK_BELOW_KNEE) {
                    std::cout << "Spider finished placing leg number: ";
                    std::cout << this->id;
                    std::cout << " back on the ground.\n";
                    // finished manuver, place the leg at its base location.
                    this->is_in_contact_with_the_ground = true;
                    //
                    this->lateral_muscle_state = 0.0;
                    this->lateral_leg_velocity = 0.0;
                    this->foot_distance_from_neutral_knee_position = 0.0;
                    // freely adjuct the angular position since the leg is in the air.
                    this->rotational_muscle_state = 0.0;
                    this->leg_horizontal_angle_delta = 0.0;
                    this->angular_velocity = 0.0;
                }
                else {
                    // this manuver isnt instantanious, it takes some time to move the leg.
                    this->miliseconds_since_started_to_moveback_knee += 10; // is assumed to be run in 100HZ
                }
            }
        }
        void maintain_mechanics_constraints() {
            // Physical constraints:
            // 1). foot distance from knee
            // 2). legs cannot crossover each other. (this assertion turns out to be hard, and i ignore it)
            this->foot_distance_from_knee_mechanics_logic();
            //this->make_sure_neighboring_legs_dont_cross();
        }
};

vector<SpiderLeg> spider_legs = {};
mutex spider_legs_mutex;

void net_function_accept_cruel_fate_and_do_nothing() {
    spider_legs_mutex.lock();
    std::cout << "[Net Actions (r_a, theta_a)] - ";
    for (int i = 0; i < N_LEGS; i++) {
        std::cout << "Leg ";
        std::cout << i;
        std::cout << ": (";
        std::cout << 0;
        std::cout << ", ";
        std::cout << 0;
        std::cout << "), ";
    }
    std::cout << "\n";
    spider_legs_mutex.unlock();
}

void net_function_PID_try_to_maintain_distance_to_knee() {
    // PID controller that tries to maintain distance to knee. no angular logic. no integral part needed - the location is noise but is accurate.
    spider_legs_mutex.lock();
    std::cout << "[Net Actions (r_a, theta_a)] - ";
    for (int i = 0; i < N_LEGS; i++) {
        float lateral_force_to_apply = 0;
        lateral_force_to_apply -= (spider_legs[i].foot_distance_from_neutral_knee_position) * TAU_P;
        lateral_force_to_apply -= spider_legs[i].lateral_leg_velocity * TAU_D;
        std::cout << "Leg ";
        std::cout << i;
        std::cout << ": (";
        std::cout << lateral_force_to_apply;
        std::cout << ", ";
        std::cout << 0;
        std::cout << "), ";
        spider_legs[i].lateral_muscle_state = lateral_force_to_apply;
    }
    std::cout << "\n";
    spider_legs_mutex.unlock();
}

std::map<std::string, void (*)()> funcname_to_func_mapping;

void LoopWhileSpiderStandsAndExecuteWithTimePadding(void (*func_to_execute)(), int miliseconds_to_pad) {
    while (the_spider_is_standing) {
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
    selected_net_function();
}

void MotorsLoopLogic() {
    //std::cout << "MotorsLoopLogic\n";

    spider_legs_mutex.lock();
    for (int i = 0; i < N_LEGS; i++) {
        spider_legs[i].apply_muscle_forces_and_move_the_leg();
        spider_legs[i].maintain_mechanics_constraints();
    }

    std::cout << "[SpiderLeg velocities (r_v, theta_v)] - ";
    for (int i = 0; i < N_LEGS; i++) {
        std::cout << "Leg ";
        std::cout << i;
        std::cout << ": (";
        if (spider_legs[i].is_in_contact_with_the_ground) {
            std::cout << spider_legs[i].lateral_leg_velocity;
            std::cout << ", ";
            std::cout << spider_legs[i].angular_velocity;
        }
        else {
            std::cout << "AIRBORNE";
        }
        std::cout << "), ";
    }
    std::cout << "\n";

    tens_of_miliseconds_survived += 1;
    // The spider falls when too many legs are in the air at a given moment
    int legs_in_contact_with_the_ground = 0;
    for (int i = 0; i < N_LEGS; i++) {
        legs_in_contact_with_the_ground += int(spider_legs[i].is_in_contact_with_the_ground);
    }
    if (legs_in_contact_with_the_ground < N_LEGS_NEEDED_TO_STAY_STANDING) {
        std::cout << "The spider has fallen!\n";
        the_spider_is_standing = false;
    }
    spider_legs_mutex.unlock();
}

void PositionLoopLogic() {
    //std::cout << "PositionLoopLogic\n";

    spider_legs_mutex.lock();
    std::cout << "[Delta Position from base (r, theta)] - ";

    for (int i = 0; i < N_LEGS; i++) {
        spider_legs[i].add_random_noise_to_leg_position();
        //float x, y;
        //tie(x, y) = spider_legs[i].get_leg_position_in_cartesian_coords();
        std::cout << "Leg ";
        std::cout << i;
        std::cout << ": (";
        if (spider_legs[i].is_in_contact_with_the_ground) {
            std::cout << spider_legs[i].foot_distance_from_neutral_knee_position;
            std::cout << ", ";
            std::cout << (spider_legs[i].leg_horizontal_angle_delta);
        }
        else {
            std::cout << "AIRBORNE";
        }
        std::cout << "), ";
    }
    std::cout << "\n";
    spider_legs_mutex.unlock();
}

void MainLoopThreadLogic() {
    //std::cout << "MainLoopThread Initiated.\n";
    LoopWhileSpiderStandsAndExecuteWithTimePadding(&MainLoopLogic, MILLISECONDS_IN_10HZ);
}

void MotorsLoopThreadLogic() {
    //std::cout << "MotorsLoopThread Initiated.\n";
    LoopWhileSpiderStandsAndExecuteWithTimePadding(&MotorsLoopLogic, MILLISECONDS_IN_100HZ);
}

void PositionLoopThreadLogic() {
    //std::cout << "PositionLoopThread Initiated.\n";
    LoopWhileSpiderStandsAndExecuteWithTimePadding(&PositionLoopLogic, MILLISECONDS_IN_250HZ);
}


void init_brain(int n_spider_legs, string selected_net_function_name,
    int DEBUG_SLOW_TIME_FACTOR, float position_noise_factor,
    float max_stable_foot_distance_from_knee, int miliseconds_to_place_foot_back_below_knee,
    float tau_p, float tau_d)
{
    TAU_D = tau_d;
    TAU_P = tau_p;

    if ((n_spider_legs < 4) or (n_spider_legs > 10)) {
        std::cout << "Please select number of legs in range: 4-10\n";
        return;
    }
    N_LEGS = n_spider_legs;
    N_LEGS_NEEDED_TO_STAY_STANDING = 3 + ((N_LEGS - 4) / 2); // for 4 legs we need 3 minimums, and for each two additional legs, one more is needed.
    POSITION_NOISE_FACTOR = position_noise_factor;
    MAX_STABLE_FOOT_DISTANCE_FROM_KNEE = max_stable_foot_distance_from_knee;
    MILISECONDS_TO_PLACE_FOOT_BACK_BELOW_KNEE = miliseconds_to_place_foot_back_below_knee;

    MILLISECONDS_IN_100HZ = DEBUG_SLOW_TIME_FACTOR * 1.0 * 1000 / 100;
    MILLISECONDS_IN_10HZ = DEBUG_SLOW_TIME_FACTOR * 1.0 * 1000 / 10;
    MILLISECONDS_IN_250HZ = DEBUG_SLOW_TIME_FACTOR * 1.0 * 1000 / 250;

    funcname_to_func_mapping["net_function_accept_cruel_fate_and_do_nothing"] = net_function_accept_cruel_fate_and_do_nothing;
    funcname_to_func_mapping["net_function_PID_try_to_maintain_distance_to_knee"] = net_function_PID_try_to_maintain_distance_to_knee;

    selected_net_function = funcname_to_func_mapping[selected_net_function_name];

    // init spider
    for (int id = 0; id < N_LEGS; id++) {
        spider_legs.push_back(SpiderLeg(id, N_LEGS));
    }

    // init left leg neighbor
    for (int id = 0; id < (N_LEGS - 1); id++) {
        spider_legs[id].leg_to_the_left = &(spider_legs[id + 1]);
    }
    // the last leg.
    spider_legs[N_LEGS - 1].leg_to_the_left = &(spider_legs[0]);

    // init right leg neighbor
    for (int id = 1; id < N_LEGS; id++) {
        spider_legs[id].leg_to_the_right = &(spider_legs[id - 1]);
    }
    // the first leg.
    spider_legs[0].leg_to_the_right = &(spider_legs[N_LEGS - 1]);

    thread MainLoopThread(MainLoopThreadLogic);
    thread MotorsLoopThread(MotorsLoopThreadLogic);
    thread PositionLoopThread(PositionLoopThreadLogic);

    MainLoopThread.join();
    MotorsLoopThread.join();
    PositionLoopThread.join();
    std::cout << "Spider survived for: ";
    std::cout << tens_of_miliseconds_survived * 0.1;
    std::cout << " seconds.\n";
    //std::cout << "main() is done.\n";
}

namespace py = pybind11;

PYBIND11_MODULE(my_robot_brain_cpp_logic, m) {
    m.def("init_brain", &init_brain, R"pbdoc(
        docs of init_brain
    )pbdoc");

#ifdef VERSION_INFO
    m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
    m.attr("__version__") = "dev";
#endif
}

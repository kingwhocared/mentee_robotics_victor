# Victor Mentee Robotics project

## Introduction to the world: Spider-Hell. 

My inspiration comes from the requirment that the number of motors is a variable.
What came to my mind was a spider with n legs (although I am pretty sure most spiders have just 8). The spider is placed in on a frozen lake (completly slippery),
and I imagined the spider tries to balance itself on the lake without falling.

The mechanics I invented are as follows:
- The floor is flat (locations are 2-Dimensional)
- The legs (feet) are placed on the unit circle with radius 100, evenly spaced.
- The center of mass is constant at (0, 0).
- The poor spider is shivering from terror, and thus some random noise is added to each of the legs (250Hz).
- For each leg, the knee location is constant (apart from height), and is exactly above the original position. When the foot moves (100Hz), the knee bends accordingly, within some limit, while staying above the original location. When that limit is reached, the spider lifts that leg in the air and moves it to the original location (rebalanced the leg) , but that takes some time and that leg doesn't provide support for the spider while airborne. 
- When a certain number of legs is airborne, the spider is considered to be unstable and falls on the ground. We require 3 legs, and 1 more for each 2 legs the spider has over 4 legs, this is very arbitrary but just what I felt like.
- The legs must not overlap, i.e: crossover eachother. (Note: this logic is unimplmented fully.).
- At 10Hz, the spider alters the muscles states.

(P.S: I am aware this world-model doesn't make sense when it comes to the physics.)

## Code is in the following two files: "my_robot_brain_cpp_logic/src/main.cpp", "main.py"

## Run instruction:
1). run "pip install ./my_robot_brain_cpp_logic"

2). execute main.py with path to configuration yaml (e.g: "python main.py ./brain_configuration.yaml")

3). try different yaml variable values and see how long the spider survives (the number of seconds is printed at the end of the run).

## Configuration file instructions:

n_spider_legs: int, min 4 and max 10.

net_function_name: either "net_function_PID_try_to_maintain_distance_to_knee", which implments a PID control logic,
or "net_function_accept_cruel_fate_and_do_nothing" which keeps the leg muscles frozen.

debug_timeslowdown_factor: int, the time will move that much slower (1 for realtime).

position_noise_factor: int, the bigger, the more noise for the leg positions.

max_stable_foot_distance_from_knee: int, the bigger, the farther the foot can be without having to lift it up in the air.

miliseconds_to_place_foot_back_below_knee: int, how long it takes to move the foot in the air back to original stable position.

tau_p: float, PID variable for proportional element.

tau_d: float, PID variable for differential element.

## 
# Sample output:

please see "sample_output.txt".

# Things I could have done better, but skipped due to time constraints preferences:

1). Object oriented implementation in c++ (have a single brain class with methods, instead of functions.)

2). Better variable naming and conventions.

3). Have the python main.py install the c++ dependencies. 

4). Divide the c++ code into multiple files, instead of having all of the code in a single file.

5). Make the random number (for the noise) generation faster. It is runs 250 times a second, so thats quite a lot.

6). Remove commented out printing statements (I am not 100% done with the project and I might need them later).

7). Have better Git commit/branching/commenting standards and discipline.

8). I have a single mutex for all motors. Perhaps would be better to have a mutex per motor. although i will have to make much more changes, since the printings to std::cout will have to be reworked.

9). I used float for angle variables, and this was a bad idea. Perhaps should have used/created a specialized class for that.

10). I should implement the missing leg horizontal angle constraints and PID controlls, but in 
hindsight, it is highly unlikely that legs will overcross eachother, given that there are max 10 legs. 
Furthermore, although I have implemented most of that logic, it proved much more complicated than expected.

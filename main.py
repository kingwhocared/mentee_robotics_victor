import my_robot_brain_cpp_logic as brain_cpp_logic
import yaml
import sys

if len(sys.argv) < 2:
    path_to_yaml = "./brain_configuration.yaml"
else:
    path_to_yaml = sys.argv[1]

with open(path_to_yaml, 'r') as stream:
    configuration_from_yaml = yaml.safe_load(stream)

n_spider_legs = configuration_from_yaml['n_spider_legs']
net_function_name = configuration_from_yaml['net_function_name']
debug_timeslowdown_factor = configuration_from_yaml['debug_timeslowdown_factor']
position_noise_factor = configuration_from_yaml['position_noise_factor']
max_stable_foot_distance_from_knee = configuration_from_yaml['max_stable_foot_distance_from_knee']
miliseconds_to_place_foot_back_below_knee = configuration_from_yaml['miliseconds_to_place_foot_back_below_knee']
tau_p = configuration_from_yaml['tau_p']
tau_d = configuration_from_yaml['tau_d']

assert net_function_name in {'net_function_accept_cruel_fate_and_do_nothing',
                             'net_function_PID_try_to_maintain_distance_to_knee'}

brain_cpp_logic.init_brain(n_spider_legs, net_function_name, debug_timeslowdown_factor,
                           position_noise_factor, max_stable_foot_distance_from_knee,
                           miliseconds_to_place_foot_back_below_knee, tau_p, tau_d)


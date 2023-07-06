import my_robot_brain_cpp_logic as brain_cpp_logic
import yaml
import sys

if len(sys.argv) < 2:
    path_to_yaml = "./my_robot_brain_cpp_logic/brain_configuration.yaml"
else:
    path_to_yaml = sys.argv[1]

with open(path_to_yaml, 'r') as stream:
    configuration_from_yaml = yaml.safe_load(stream)

n_motors = configuration_from_yaml['n_motors']
net_function_name = configuration_from_yaml['net_function_name']
debug_timeslowdown_factor = configuration_from_yaml['debug_timeslowdown_factor']

brain_cpp_logic.init_brain(n_motors, net_function_name, debug_timeslowdown_factor)


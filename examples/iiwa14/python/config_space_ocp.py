import roboc
import numpy as np
import math


path_to_urdf = "../iiwa_description/urdf/iiwa14.urdf"
robot = roboc.Robot(path_to_urdf)

# Change the limits from the default parameters.
robot.set_joint_effort_limit(np.full(robot.dimu(), 50))
robot.set_joint_velocity_limit(np.full(robot.dimv(), 0.5*math.pi))

# Create a cost function.
cost = roboc.CostFunction()
config_cost = roboc.ConfigurationSpaceCost(robot)
q_ref = np.array([0, 0.5*math.pi, 0, 0.5*math.pi, 0, 0.5*math.pi, 0]) 
config_cost.set_q_ref(q_ref)
config_cost.set_q_weight(np.full(robot.dimv(), 10))
config_cost.set_qf_weight(np.full(robot.dimv(), 10))
config_cost.set_v_weight(np.full(robot.dimv(), 0.01))
config_cost.set_vf_weight(np.full(robot.dimv(), 0.01))
config_cost.set_a_weight(np.full(robot.dimv(), 0.01))
cost.push_back(config_cost)

# Create joint constraints.
constraints           = roboc.Constraints()
joint_position_lower  = roboc.JointPositionLowerLimit(robot)
joint_position_upper  = roboc.JointPositionUpperLimit(robot)
joint_velocity_lower  = roboc.JointVelocityLowerLimit(robot)
joint_velocity_upper  = roboc.JointVelocityUpperLimit(robot)
joint_torques_lower   = roboc.JointTorquesLowerLimit(robot)
joint_torques_upper   = roboc.JointTorquesUpperLimit(robot)
constraints.push_back(joint_position_lower)
constraints.push_back(joint_position_upper)
constraints.push_back(joint_velocity_lower)
constraints.push_back(joint_velocity_upper)
constraints.push_back(joint_torques_lower)
constraints.push_back(joint_torques_upper)

# Create the OCP solver for unconstrained rigid-body systems.
T = 3.0
N = 60
nthreads = 4
t = 0.0
q = np.array([0.5*math.pi, 0, 0.5*math.pi, 0, 0.5*math.pi, 0, 0.5*math.pi]) 
v = np.zeros(robot.dimv())

ocp_solver = roboc.UnconstrOCPSolver(robot, cost, constraints, T, N, nthreads)
ocp_solver.set_solution("q", q)
ocp_solver.set_solution("v", v)
ocp_solver.init_constraints()

print("----- Solves the OCP by Riccati recursion algorithm. -----")
num_iteration = 30
roboc.utils.benchmark.convergence(ocp_solver, t, q, v, num_iteration)

# Solves the OCP by ParNMPC algorithm.
nthreads = 8
parnmpc_solver = roboc.UnconstrParNMPCSolver(robot, cost, constraints, T, N, nthreads)
parnmpc_solver.set_solution("q", q)
parnmpc_solver.set_solution("v", v)
parnmpc_solver.init_constraints()

print("\n----- Solves the OCP by ParNMPC algorithm. -----")
num_iteration = 60
roboc.utils.benchmark.convergence(parnmpc_solver, t, q, v, num_iteration)

viewer = roboc.utils.TrajectoryViewer(path_to_urdf=path_to_urdf, viewer_type='meshcat')
viewer.set_camera_transform_meshcat(camera_tf_vec=[0.5, -3.0, 0.0], zoom=2.0)
viewer.display((T/N), ocp_solver.get_solution('q'))
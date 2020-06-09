#! /usr/bin/env python3

import matplotlib
matplotlib.use('agg')

import sys

from SWEET import *
p = JobGeneration()

p.compile.compiler = 'intel'
p.compile.program = 'swe_sphere'
p.compile.fortran_source = 'enable'

p.compile.plane_or_sphere = 'sphere'
p.compile.plane_spectral_space = 'disable'
p.compile.plane_spectral_dealiasing = 'disable'
p.compile.sphere_spectral_space = 'enable'
p.compile.sphere_spectral_dealiasing = 'enable'

p.compile.rexi_thread_parallel_sum = 'enable'
p.compile.threading = 'off'

p.runtime.space_res_spectral = 16
p.runtime.output_timestep_size = 0.01

p.runtime.gravitation= 1	# gravity
p.runtime.h0 = 100000	# avg height
p.runtime.sphere_rotating_coriolis_omega = 0.000145842	# coriolis effect
p.runtime.sphere_radius = 6371220	# radius

# 3: gaussian breaking dam
# 4: geostrophic balance test case
p.runtime.bench_id = 4


p.runtime.max_simulation_time = 0.001 #math.inf

p.runtime.compute_error = 0


p.runtime.space_res_spectral = 16
p.runtime.normal_mode_analysis = 3

p.runtime.f_sphere = 1


# Smaller values lead to no solution for the vort/div formulation
default_timestep_size=10 #1e-6
default_timesteps=1


#for p.f_sphere in [-1, 0, 1]:
for p.runtime.f_sphere in [0, 1]:
#for p.f_sphere in [0, 1]:
	if p.runtime.f_sphere == -1:
		p.runtime.sphere_rotating_coriolis_omega = 0
		p.runtime.f_sphere = 0

	elif p.runtime.f_sphere == 0:
		# f-sphere
		p.runtime.sphere_rotating_coriolis_omega = 0.000072921	# \Omega coriolis effect

	elif p.runtime.f_sphere == 1:
		p.runtime.sphere_rotating_coriolis_omega = 0.000072921
		p.runtime.sphere_rotating_coriolis_omega = 2.0*p.runtime.f
	else:
		print("ERROR")
		sys.exit(1)
	
	####################################
	# REXI
	####################################
	if 1:
		p.runtime.timestepping_method = 'l_rexi'
		p.runtime.timestepping_order = 0

		p.runtime.timestep_size = default_timestep_size
		p.runtime.max_simulation_time = default_timestep_size*default_timesteps
		p.runtime.max_timesteps_nr = default_timesteps

		p.runtime.rexi_method = 'terry'

		for p.runtime.sphere_extended_modes in [0]:
			for p.runtime.rexi_m in [4, 64, 128, 256]:
				p.gen_script('script'+p.runtime.getUniqueID(p.compile), 'run.sh')

		p.runtime.sphere_extended_modes = 0
		p.runtime.rexi_m = 0

		p.runtime.rexi_method = ''


	####################################
	# RK1
	####################################
	if 1:
		p.runtime.timestepping_method = 'l_erk'
		p.runtime.timestepping_order = 1

		p.runtime.timestep_size = default_timestep_size
		p.runtime.max_simulation_time = default_timestep_size*default_timesteps
		p.runtime.max_timesteps_nr = default_timesteps

		p.runtime.sphere_extended_modes = 0
		p.gen_script('script'+p.runtime.getUniqueID(p.compile), 'run.sh')


	####################################
	# RK2
	####################################
	if 1:
		p.runtime.timestepping_method = 'l_erk'
		p.runtime.timestepping_order = 2

		p.runtime.timestep_size = default_timestep_size
		p.runtime.max_simulation_time = default_timestep_size*default_timesteps
		p.runtime.max_timesteps_nr = default_timesteps

		p.runtime.sphere_extended_modes = 0
		p.gen_script('script'+p.runtime.getUniqueID(p.compile), 'run.sh')


	####################################
	# IRK1
	####################################
	if 1:
		p.runtime.timestepping_method = 'l_irk'
		p.runtime.timestepping_order = 1

		p.runtime.timestep_size = default_timestep_size
		p.runtime.max_simulation_time = default_timestep_size*default_timesteps
		p.runtime.max_timesteps_nr = default_timesteps

		p.runtime.sphere_extended_modes = 0
		p.gen_script('script'+p.runtime.getUniqueID(p.compile), 'run.sh')


	####################################
	# CN
	####################################
	if 1:
		p.runtime.timestepping_method = 'l_cn'
		p.runtime.timestepping_order = 2

		p.runtime.timestep_size = default_timestep_size
		p.runtime.max_simulation_time = default_timestep_size*default_timesteps
		p.runtime.max_timesteps_nr = default_timesteps

		p.runtime.sphere_extended_modes = 0
		p.gen_script('script'+p.runtime.getUniqueID(p.compile), 'run.sh')


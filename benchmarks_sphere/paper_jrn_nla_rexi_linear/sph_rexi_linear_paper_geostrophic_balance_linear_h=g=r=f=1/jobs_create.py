#! /usr/bin/env python3

import sys

from SWEET import *
p = JobGeneration()


#p.cluster.setupTargetMachine("cheyenne")
#p.cluster.pm_space_cores_per_mpi_rank = 1
#p.cluster.pm_time_cores_per_mpi_rank = 1

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




p.runtime.space_res_spectral = 64
p.runtime.output_filename = '-'
p.runtime.timestepping_order = 4


p.runtime.timestepping_method = 'l_erk'
p.runtime.timestepping_order = 2
p.runtime.rexi_m = 256
p.runtime.rexi_h = 0.15
p.runtime.rexi_half_poles = 1
p.runtime.sphere_extended_modes = 2
p.runtime.rexi_normalization = 1

p.runtime.gravitation= 1	# gravity
p.runtime.h0 = 1	# avg height
p.runtime.sphere_rotating_coriolis_omega = 1	# coriolis effect

p.runtime.sphere_radius = 1	# radius

# 10: geostrophic balance test case
p.runtime.bench_id = 10

p.runtime.max_simulation_time = 1


p.runtime.compute_error = 1




####################################
# REXI
####################################

p.runtime.rexi_method = 'terry'

p.runtime.bench_id = 10	# Geostrophic balance benchmark


if False:
	self.gravitation= -1
	self.h0 = -1
	self.sphere_rotating_coriolis_omega = -1
	self.sphere_radius = -1


if True:
	# 10 times larger than RK4 time step size
	p.runtime.timestepping_method = 'l_rexi'
	p.runtime.timestepping_order = 1
	p.runtime.timestep_size = 0.1
	p.runtime.output_timestep_size = 0.1

	p.runtime.rexi_par = 1

	for p.runtime.rexi_half_poles in [0, 1]:
		for p.runtime.rexi_normalization in [1,0]:
			for p.runtime.sphere_extended_modes in [0, 2]:
				for p.runtime.space_res_spectral in [64]:
					for p.runtime.rexi_m in [1, 2, 4, 8, 16, 32, 64, 128, 256]:
						p.gen_script('script'+p.runtime.getUniqueID(p.compile), 'run.sh')

p.runtime.rexi_method = ''


####################################
# RK
####################################

if True:
	p.runtime.timestepping_method = 'l_erk'
	p.runtime.timestepping_order = 2
	p.runtime.timestep_size = 0.01
	p.runtime.output_timestep_size = 0.01

	for p.runtime.space_res_spectral in [64]:
		p.gen_script('script'+p.runtime.getUniqueID(p.compile), 'run.sh')



/*
 * test_sphere_advection.cpp
 *
 *  Created on: 3 Apr 2018
 *      Author: martin
 */


#ifndef SWEET_GUI
	#define SWEET_GUI 1
#endif

#include <sweet/sphere/SphereData_Spectral.hpp>
#if SWEET_GUI
	#include "sweet/VisSweet.hpp"
#endif
#include <benchmarks_sphere/SWESphereBenchmarksCombined.hpp>
#include <sweet/SimulationVariables.hpp>
#include <sweet/sphere/SphereOperators_SphereData.hpp>
#include <sweet/Convert_SphereDataSpectral_To_PlaneData.hpp>
#include <sweet/Convert_SphereDataPhysical_To_PlaneData.hpp>

#include "../programs/advection_sphere/Adv_Sphere_TimeSteppers.hpp"



// Sphere data config
SphereData_Config sphereDataConfigInstance;
SphereData_Config *sphereDataConfig = &sphereDataConfigInstance;

#if SWEET_GUI
	PlaneDataConfig planeDataConfigInstance;
	PlaneDataConfig *planeDataConfig = &planeDataConfigInstance;
#endif

SimulationVariables simVars;


class SimulationInstance
{
public:
	SphereData_Spectral prog_phi_pert;
	SphereData_Spectral prog_phi_pert_t0;	// at t0
	SphereData_Spectral prog_vort, prog_div;

	Adv_Sphere_TimeSteppers timeSteppers;

	SphereOperators_SphereData op;

	/*
	 * LMax error to h0
	 */
	double max_error_h0;
	/*
	 * RMS error to h0
	 */
	double rms_error_h0;


#if SWEET_GUI
	PlaneData viz_plane_data;

	int render_primitive_id = 1;
#endif

	SWESphereBenchmarksCombined sphereBenchmarksCombined;



public:
	SimulationInstance()	:
		prog_phi_pert(sphereDataConfig),
		prog_phi_pert_t0(sphereDataConfig),

		prog_vort(sphereDataConfig),
		prog_div(sphereDataConfig),

		op(sphereDataConfig, &(simVars.sim))

#if SWEET_GUI
		,
		viz_plane_data(planeDataConfig)
#endif
	{
		reset();
	}



	void reset()
	{
		simVars.reset();

		sphereBenchmarksCombined.setup(simVars, op);
		sphereBenchmarksCombined.setupInitialConditions_pert(prog_phi_pert, prog_vort, prog_div);

		prog_phi_pert_t0 = prog_phi_pert;

		// setup sphereDataconfig instance again
		sphereDataConfigInstance.setupAuto(simVars.disc.space_res_physical, simVars.disc.space_res_spectral, simVars.misc.reuse_spectral_transformation_plans);

		timeSteppers.setup(simVars.disc.timestepping_method, op, simVars);

		//simVars.outputConfig();
	}



	void run_timestep()
	{
		if (simVars.timecontrol.current_simulation_time + simVars.timecontrol.current_timestep_size > simVars.timecontrol.max_simulation_time)
			simVars.timecontrol.current_timestep_size = simVars.timecontrol.max_simulation_time - simVars.timecontrol.current_simulation_time;

		timeSteppers.master->run_timestep(
				prog_phi_pert, prog_vort, prog_div,
				simVars.timecontrol.current_timestep_size,
				simVars.timecontrol.current_simulation_time
			);

		double dt = simVars.timecontrol.current_timestep_size;

		// advance in time
		simVars.timecontrol.current_simulation_time += dt;
		simVars.timecontrol.current_timestep_nr++;

		if (simVars.misc.verbosity >= 10)
			std::cout << simVars.timecontrol.current_timestep_nr << ": " << simVars.timecontrol.current_simulation_time/(60*60*24.0) << std::endl;

		max_error_h0 = (prog_phi_pert_t0-prog_phi_pert).getSphereDataPhysical().physical_reduce_max_abs();
		rms_error_h0 = (prog_phi_pert_t0-prog_phi_pert).getSphereDataPhysical().physical_reduce_rms();
	}


	void compute_error()
	{
#if 0
		double t = simVars.timecontrol.current_simulation_time;

		SphereData_Spectral prog_testh(sphereDataConfig);
		prog_testh.physical_update_lambda_array_indices(
			[&](int i, int j, double &io_data)
			{
				double x = (((double)i)/(double)simVars.disc.space_res_physical[0])*simVars.sim.domain_size[0];
				double y = (((double)j)/(double)simVars.disc.space_res_physical[1])*simVars.sim.domain_size[1];

				x -= param_velocity_u*t;
				y -= param_velocity_v*t;

				while (x < 0)
					x += simVars.sim.domain_size[0];

				while (y < 0)
					y += simVars.sim.domain_size[1];

				x = std::fmod(x, simVars.sim.domain_size[0]);
				y = std::fmod(y, simVars.sim.domain_size[1]);

				io_data = SWESphereBenchmarks::return_h(simVars, x, y);
			}
		);

		std::cout << "Lmax Error: " << (prog_phi_pert-prog_testh).reduce_maxAbs() << std::endl;
#endif
	}


	bool should_quit()
	{
		if (simVars.timecontrol.max_timesteps_nr != -1 && simVars.timecontrol.max_timesteps_nr <= simVars.timecontrol.current_timestep_nr)
			return true;

		double diff = std::abs(simVars.timecontrol.max_simulation_time - simVars.timecontrol.current_simulation_time);

		if (	simVars.timecontrol.max_simulation_time != -1 &&
				(
						simVars.timecontrol.max_simulation_time <= simVars.timecontrol.current_simulation_time	||
						diff/simVars.timecontrol.max_simulation_time < 1e-11	// avoid numerical issues in time stepping if current time step is 1e-14 smaller than max time step
				)
			)
			return true;

		return false;
	}

#if SWEET_GUI
	/**
	 * postprocessing of frame: do time stepping
	 */
	void vis_post_frame_processing(int i_num_iterations)
	{
		if (simVars.timecontrol.run_simulation_timesteps)
			for (int i = 0; i < i_num_iterations && !should_quit(); i++)
				run_timestep();

		compute_error();
	}


	void vis_get_vis_data_array(
			const PlaneData **o_dataArray,
			double *o_aspect_ratio,
			int *o_render_primitive_id,
			void **o_bogus_data,
			double *o_viz_min,
			double *o_viz_max
	)
	{
		*o_render_primitive_id = render_primitive_id;
		*o_bogus_data = sphereDataConfig;

		int id = simVars.misc.vis_id % 5;
		switch (id)
		{
		case 0:
			viz_plane_data = Convert_SphereDataSpectral_To_PlaneData::physical_convert(prog_phi_pert, planeDataConfig);
			break;

		case 1:
			viz_plane_data = Convert_SphereDataSpectral_To_PlaneData::physical_convert(prog_vort, planeDataConfig);
			break;

		case 2:
			viz_plane_data = Convert_SphereDataSpectral_To_PlaneData::physical_convert(prog_div, planeDataConfig);
			break;

		case 3:
			{
				SphereData_Physical u(sphereDataConfig);
				SphereData_Physical v(sphereDataConfig);

//				op.robert_vortdiv_to_uv(prog_vort, prog_div, u, v);
				op.vortdiv_to_uv(prog_vort, prog_div, u, v);
				viz_plane_data = Convert_SphereDataPhysical_To_PlaneData::physical_convert(u, planeDataConfig);
			}
			break;

		case 4:
			{
				SphereData_Physical u(sphereDataConfig);
				SphereData_Physical v(sphereDataConfig);

//				op.robert_vortdiv_to_uv(prog_vort, prog_div, u, v);
				op.vortdiv_to_uv(prog_vort, prog_div, u, v);
				viz_plane_data = Convert_SphereDataPhysical_To_PlaneData::physical_convert(v, planeDataConfig);
			}
			break;

		}

		*o_dataArray = &viz_plane_data;
		*o_aspect_ratio = 0.5;
	}



	const char* vis_get_status_string()
	{
		const char* description = "";
		int id = simVars.misc.vis_id % 5;

		switch (id)
		{
		default:
		case 0:
			description = "H";
			break;

		case 1:
			description = "vort";
			break;

		case 2:
			description = "div";
			break;

		case 3:
			description = "u";
			break;

		case 4:
			description = "v";
			break;
		}


		static char title_string[2048];

		//sprintf(title_string, "Time (days): %f (%.2f d), Timestep: %i, timestep size: %.14e, Vis: %s, Mass: %.14e, Energy: %.14e, Potential Entrophy: %.14e",
		sprintf(title_string,
#if SWEET_MPI
				"Rank %i - "
#endif
				"Time: %f (%.2f d), k: %i, dt: %.3e, Vis: %s, TMass: %.6e, TEnergy: %.6e, PotEnstrophy: %.6e, MaxVal: %.6e, MinVal: %.6e ",
#if SWEET_MPI
				mpi_rank,
#endif
				simVars.timecontrol.current_simulation_time,
				simVars.timecontrol.current_simulation_time/(60.0*60.0*24.0),
				simVars.timecontrol.current_timestep_nr,
				simVars.timecontrol.current_timestep_size,
				description,
				simVars.diag.total_mass,
				simVars.diag.total_energy,
				simVars.diag.total_potential_enstrophy,
				viz_plane_data.reduce_max(),
				viz_plane_data.reduce_min()
		);

		return title_string;
	}

	void vis_pause()
	{
		simVars.timecontrol.run_simulation_timesteps = !simVars.timecontrol.run_simulation_timesteps;
	}


	void vis_keypress(int i_key)
	{
		switch(i_key)
		{
		case 'v':
			simVars.misc.vis_id++;
			break;

		case 'V':
			simVars.misc.vis_id--;
			break;

		case 'b':
			render_primitive_id = (render_primitive_id + 1) % 2;
			break;
		}
	}
#endif
};



int main(int i_argc, char *i_argv[])
{
	if (!simVars.setupFromMainParameters(i_argc, i_argv))
	{
		std::cout << std::endl;
		return -1;
	}

	simVars.outputConfig();

	int initial_spectral_modes = simVars.disc.space_res_spectral[0];

	if (simVars.timecontrol.current_timestep_size < 0)
		FatalError("Timestep size not set");


	SphereTimestepping_SemiLagrangian::alpha() = simVars.benchmark.sphere_advection_rotation_angle;

	int max_modes = 256;

	if (simVars.disc.timestepping_order == 1)
		max_modes = 512;
	else if (simVars.disc.timestepping_order == 2)
		max_modes = 256;

	double prev_max_error = -1;
	for (int i = initial_spectral_modes; i <= max_modes; i *= 2)
	{
		simVars.timecontrol.current_timestep_size *= 0.5;

		if (simVars.disc.timestepping_method == "na_sl")
		{
			simVars.disc.space_res_spectral[0] = i;
			simVars.disc.space_res_spectral[1] = i;

			simVars.disc.space_res_physical[0] = 2*i;
			simVars.disc.space_res_physical[1] = i;
		}
		else
		{
			simVars.disc.space_res_spectral[0] = initial_spectral_modes;
			simVars.disc.space_res_spectral[1] = initial_spectral_modes;

			simVars.disc.space_res_physical[0] = 0;
			simVars.disc.space_res_physical[1] = 0;
		}



		sphereDataConfigInstance.setupAuto(simVars.disc.space_res_physical, simVars.disc.space_res_spectral, simVars.misc.reuse_spectral_transformation_plans);

		std::cout << "Testing with " << sphereDataConfigInstance.getUniqueIDString() << std::endl;
		std::cout << "Testing with dt=" << simVars.timecontrol.current_timestep_size << std::endl;

		SimulationInstance simulation;


#if SWEET_GUI
		if (simVars.misc.gui_enabled)
		{
			planeDataConfigInstance.setupAutoSpectralSpace(
					simVars.disc.space_res_physical,
					simVars.misc.reuse_spectral_transformation_plans
				);
			VisSweet<SimulationInstance> visSweet(&simulation);
			return 0;
		}
		else
#endif
		{
//			simulation.reset();
			while (!simulation.should_quit())
				simulation.run_timestep();

			std::cout << "Error compared to initial condition" << std::endl;
			std::cout << "Lmax error: " << simulation.max_error_h0 << std::endl;
			std::cout << "RMS error: " << simulation.rms_error_h0 << std::endl;

			if (prev_max_error >= 0)
			{
				//double conv = (prev_max_error - simulation.max_error) / simulation.max_error;
				double conv = prev_max_error / simulation.max_error_h0;
				std::cout << "Convergence: " << conv << std::endl;

				if (conv*1.1 < std::pow(2.0, (double)simVars.disc.timestepping_order))
					FatalError("Convergence not given!");
			}

			if (simulation.max_error_h0  > 1e10)
				FatalError("Lmax error exceeded threshold!");

			prev_max_error = simulation.max_error_h0;

			std::cout << "*********************************************" << std::endl;
		}
	}

	return 0;
}

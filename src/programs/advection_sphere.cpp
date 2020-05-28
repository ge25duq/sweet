/*
 * Author: Martin Schreiber <SchreiberX@gmail.com>
 * MULE_COMPILE_FILES_AND_DIRS: src/programs/advection_sphere
 * MULE_COMPILE_FILES_AND_DIRS: src/include/benchmarks_sphere/
 */


#ifndef SWEET_GUI
	#define SWEET_GUI 1
#endif

#include <sweet/sphere/SphereData_Spectral.hpp>
#if SWEET_GUI
	#include "sweet/VisSweet.hpp"
#endif
#include <benchmarks_sphere/SWESphereBenchmarks.hpp>
#include <sweet/SimulationVariables.hpp>
#include <sweet/sphere/SphereOperators_SphereData.hpp>
#include <sweet/Convert_SphereDataSpectral_To_PlaneData.hpp>
#include <sweet/Convert_SphereDataPhysical_To_PlaneData.hpp>

#include <sweet/sphere/SphereData_DebugContainer.hpp>

#include "advection_sphere/Adv_Sphere_TimeSteppers.hpp"



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

	SphereData_Physical prog_phi_pert_phys;
//	SphereData_Physical prog_phi_pert_phys_t0;	// at t0

	Adv_Sphere_TimeSteppers timeSteppers;


	SphereOperators_SphereData op;

	bool time_varying_fields;

#if SWEET_GUI
	PlaneData viz_plane_data;

	int render_primitive_id = 1;
#endif

	SWESphereBenchmarks sphereBenchmarks;


public:
	SimulationInstance()	:
		prog_phi_pert(sphereDataConfig),
		prog_phi_pert_t0(sphereDataConfig),

		prog_phi_pert_phys(sphereDataConfig),
//		prog_phi_pert_phys_t0(sphereDataConfig),

		prog_vort(sphereDataConfig),
		prog_div(sphereDataConfig),

		op(sphereDataConfig, &(simVars.sim)),
		time_varying_fields(false)

#if SWEET_GUI
		,
		viz_plane_data(planeDataConfig)
#endif
	{
		reset();
	}


	~SimulationInstance()
	{
		std::cout << "Error compared to initial condition" << std::endl;
		std::cout << "Lmax error spec: " << (prog_phi_pert_t0-prog_phi_pert).toPhys().physical_reduce_max_abs() << std::endl;
//		std::cout << "Lmax error phys: " << (prog_phi_pert_phys_t0-prog_phi_pert_phys).physical_reduce_max_abs() << std::endl;
		//std::cout << "RMS error: " << (prog_phi_pert_t0-prog_phi_pert).toPhys().physical_reduce_rms() << std::endl;
	}



	void reset()
	{
		simVars.reset();

		SphereData_Spectral tmp_vort(sphereDataConfig);
		SphereData_Spectral tmp_div(sphereDataConfig);

		sphereBenchmarks.setup(simVars, op);
		sphereBenchmarks.master->get_initial_state(
				prog_phi_pert, prog_vort, prog_div//,
				//&prog_phi_pert_phys
			);

		time_varying_fields = sphereBenchmarks.master->has_time_varying_state();

		prog_phi_pert_t0 = prog_phi_pert;
//		prog_phi_pert_phys_t0 = prog_phi_pert_phys;

		// setup sphereDataconfig instance again
		sphereDataConfigInstance.setupAuto(simVars.disc.space_res_physical, simVars.disc.space_res_spectral, simVars.misc.reuse_spectral_transformation_plans);

		timeSteppers.setup(simVars.disc.timestepping_method, op, simVars);

		simVars.outputConfig();
	}



	void run_timestep()
	{
		SphereData_DebugContainer::clear();


		if (simVars.timecontrol.current_simulation_time + simVars.timecontrol.current_timestep_size > simVars.timecontrol.max_simulation_time)
			simVars.timecontrol.current_timestep_size = simVars.timecontrol.max_simulation_time - simVars.timecontrol.current_simulation_time;

		/*
		 * Update time varying fields
		 */
		if (time_varying_fields)
			sphereBenchmarks.master->get_reference_state(prog_phi_pert, prog_vort, prog_div, simVars.timecontrol.current_simulation_time);

		timeSteppers.master->run_timestep(
				prog_phi_pert, prog_vort, prog_div,
				simVars.timecontrol.current_timestep_size,
				simVars.timecontrol.current_simulation_time,
				(time_varying_fields ? &sphereBenchmarks : nullptr),
				prog_phi_pert_phys
			);

		double dt = simVars.timecontrol.current_timestep_size;

		// advance in time
		simVars.timecontrol.current_simulation_time += dt;
		simVars.timecontrol.current_timestep_nr++;

		if (simVars.misc.verbosity > 2)
			std::cout << simVars.timecontrol.current_timestep_nr << ": " << simVars.timecontrol.current_simulation_time/(60*60*24.0) << std::endl;

		SphereData_DebugContainer::append(prog_phi_pert_t0-prog_phi_pert, "diff phi0");
//		SphereData_DebugContainer::append(prog_phi_pert_phys_t0-prog_phi_pert_phys, "diff phi0_phys");
	}



	bool should_quit()
	{
		if (simVars.misc.gui_enabled)
			return false;

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
	}


	void vis_get_vis_data_array(
			const PlaneData **o_dataArray,
			double *o_aspect_ratio,
			int *o_render_primitive_id,
			void **o_bogus_data,
			double *o_viz_min,
			double *o_viz_max,
			bool *viz_reset
	)
	{
		*o_render_primitive_id = render_primitive_id;
		*o_bogus_data = sphereDataConfig;

		if (simVars.misc.vis_id < 0)
		{
			int n = -simVars.misc.vis_id-1;
			if (n <  (int)SphereData_DebugContainer().size())
			{
				SphereData_DebugContainer::DataContainer &d = SphereData_DebugContainer().container_data()[n];
				if (d.is_spectral)
					viz_plane_data = Convert_SphereDataSpectral_To_PlaneData::physical_convert(d.data_spectral, planeDataConfig);
				else
					viz_plane_data = Convert_SphereDataPhysical_To_PlaneData::physical_convert(d.data_physical, planeDataConfig);

				*o_dataArray = &viz_plane_data;
				*o_aspect_ratio = 0.5;
				return;
			}
		}


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

				op.vortdiv_to_uv(prog_vort, prog_div, u, v);
				viz_plane_data = Convert_SphereDataPhysical_To_PlaneData::physical_convert(u, planeDataConfig);
			}
			break;

		case 4:
			{
				SphereData_Physical u(sphereDataConfig);
				SphereData_Physical v(sphereDataConfig);

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
		std::string description = "";
		int id = simVars.misc.vis_id % 5;

		bool found = false;
		if (simVars.misc.vis_id < 0)
		{
			int n = -simVars.misc.vis_id-1;

			if (n <  (int)SphereData_DebugContainer().size())
			{
				description = std::string("DEBUG_")+SphereData_DebugContainer().container_data()[n].description;
				found = true;
			}
		}

		if (!found)
		{
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
		}


		static char title_string[2048];

		//sprintf(title_string, "Time (days): %f (%.2f d), Timestep: %i, timestep size: %.14e, Vis: %s, Mass: %.14e, Energy: %.14e, Potential Entrophy: %.14e",
		sprintf(title_string,
				"Time: %f (%.2f d), k: %i, dt: %.3e, Vis: %s, TMass: %.6e, TEnergy: %.6e, PotEnstrophy: %.6e, MaxVal: %.6e, MinVal: %.6e "
				","
				"Colorscale: lowest [Blue... green ... red] highest",
				simVars.timecontrol.current_simulation_time,
				simVars.timecontrol.current_simulation_time/(60.0*60.0*24.0),
				simVars.timecontrol.current_timestep_nr,
				simVars.timecontrol.current_timestep_size,
				description.c_str(),
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

	if (simVars.timecontrol.current_timestep_size < 0)
		SWEETError("Timestep size not set");

	sphereDataConfigInstance.setupAuto(simVars.disc.space_res_physical, simVars.disc.space_res_spectral, simVars.misc.reuse_spectral_transformation_plans);

	SimulationInstance *simulation = new SimulationInstance;

#if SWEET_GUI
	if (simVars.misc.gui_enabled)
	{
		planeDataConfigInstance.setupAutoSpectralSpace(simVars.disc.space_res_physical, simVars.misc.reuse_spectral_transformation_plans);

		VisSweet<SimulationInstance> visSweet(simulation);
	}
	else
#endif
	{
		simulation->reset();
		while (!simulation->should_quit())
		{
			simulation->run_timestep();

			if (simVars.timecontrol.max_simulation_time != -1)
				if (simVars.timecontrol.current_simulation_time > simVars.timecontrol.max_simulation_time)
					break;

			if (simVars.timecontrol.max_timesteps_nr != -1)
				if (simVars.timecontrol.current_timestep_nr > simVars.timecontrol.max_timesteps_nr)
					break;
		}
	}

	delete simulation;

	return 0;
}

/*
 * swe_sphere.cpp
 *
 *  Created on: 15 Aug 2016
 *      Author: Martin Schreiber <SchreiberX@gmail.com>
 */

#if SWEET_GUI
	#include <sweet/VisSweet.hpp>
	#include <sweet/plane/PlaneDataConfig.hpp>
	#include <sweet/plane/PlaneData.hpp>
	#include <sweet/Convert_SphereDataSpectral_To_PlaneData.hpp>
#endif

#include <benchmarks_sphere/SWESphereBenchmarksCombined.hpp>

#include <sweet/sphere/SphereData_Spectral.hpp>
#include <sweet/sphere/SphereData_Physical.hpp>
#include <sweet/sphere/SphereHelpers_Diagnostics.hpp>


#include <sweet/sphere/SphereOperators_SphereData.hpp>
#include <sweet/sphere/SphereOperators_SphereDataComplex.hpp>
#include <sweet/sphere/SphereData_SpectralComplex.hpp>
#include <sweet/Stopwatch.hpp>
#include <sweet/FatalError.hpp>

#include "swe_sphere/SWE_Sphere_TimeSteppers.hpp"
#include "swe_sphere/SWE_Sphere_NormalModeAnalysis.hpp"

#include <sweet/SimulationBenchmarkTiming.hpp>



SimulationVariables simVars;

// Plane data config
SphereData_Config sphereDataConfigInstance;
SphereData_Config sphereDataConfigInstance_nodealiasing;
SphereData_Config *sphereDataConfig = &sphereDataConfigInstance;
SphereData_Config *sphereDataConfig_nodealiasing = &sphereDataConfigInstance_nodealiasing;


#if SWEET_GUI
	PlaneDataConfig planeDataConfigInstance;
	PlaneDataConfig *planeDataConfig = &planeDataConfigInstance;
#endif



/*
 * This allows running REXI including Coriolis-related terms but just by setting f to 0
 */


class SimulationInstance
{
public:
	SphereOperators_SphereData op;
	SphereOperators_SphereData op_nodealiasing;

	SWE_Sphere_TimeSteppers timeSteppers;


	// Diagnostics measures
	int last_timestep_nr_update_diagnostics = -1;

	SphereData_Spectral prog_phi;
	SphereData_Spectral prog_vort;
	SphereData_Spectral prog_div;


	REXI_Terry<> rexi;

#if SWEET_GUI
	PlaneData viz_plane_data;
#endif

	int render_primitive_id = 1;

	SphereHelpers_Diagnostics sphereDiagnostics;

#if SWEET_MPI
	int mpi_rank;
#endif

	// was the output of the time step already done for this simulation state?
	double timestep_last_output_simtime;

	SWESphereBenchmarksCombined sphereBenchmarks;

public:
	SimulationInstance()	:
		op(sphereDataConfig, simVars.sim.sphere_radius),
		op_nodealiasing(sphereDataConfig_nodealiasing, simVars.sim.sphere_radius),
		prog_phi(sphereDataConfig),
		prog_vort(sphereDataConfig),
		prog_div(sphereDataConfig),

#if SWEET_GUI
		viz_plane_data(planeDataConfig),
#endif
		sphereDiagnostics(
				sphereDataConfig,
				simVars,
				simVars.misc.verbosity
		)
	{
#if SWEET_MPI
		MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
#endif
		reset();
	}



	void update_diagnostics()
	{
		// assure, that the diagnostics are only updated for new time steps
		if (last_timestep_nr_update_diagnostics == simVars.timecontrol.current_timestep_nr)
			return;

		sphereDiagnostics.update_phi_vort_div_2_mass_energy_enstrophy(
				op,
				prog_phi,
				prog_vort,
				prog_div,
				simVars
		);
	}



	void reset()
	{
		SimulationBenchmarkTimings::getInstance().main_setup.start();

		simVars.reset();

		// Diagnostics measures
		last_timestep_nr_update_diagnostics = -1;

		simVars.iodata.output_next_sim_seconds = 0;

		if (simVars.timecontrol.current_timestep_size <= 0)
			FatalError("Only fixed time step size supported");

		if (simVars.benchmark.setup_dealiased)
		{
			// use dealiased physical space for setup
			sphereBenchmarks.setup(simVars, op);
			sphereBenchmarks.setupInitialConditions(prog_phi, prog_vort, prog_div);
		}
		else
		{
			// this is not the default since noone uses it
			// use reduced physical space for setup to avoid spurious modes
			SphereData_Spectral prog_phi_nodealiasing(sphereDataConfig_nodealiasing);
			SphereData_Spectral prog_vort_nodealiasing(sphereDataConfig_nodealiasing);
			SphereData_Spectral prog_div_nodealiasing(sphereDataConfig_nodealiasing);

			sphereBenchmarks.setup(simVars, op_nodealiasing);
			sphereBenchmarks.setupInitialConditions(prog_phi_nodealiasing, prog_vort_nodealiasing, prog_div_nodealiasing);

			prog_phi.load_nodealiasing(prog_phi_nodealiasing);
			prog_vort.load_nodealiasing(prog_vort_nodealiasing);
			prog_div.load_nodealiasing(prog_div_nodealiasing);
		}

		/*
		 * SETUP time steppers
		 */
		timeSteppers.setup(simVars.disc.timestepping_method, op, simVars);

		update_diagnostics();

		simVars.diag.backup_reference();

		SimulationBenchmarkTimings::getInstance().main_setup.stop();

		// start at one second in the past to ensure output at t=0
		timestep_last_output_simtime = simVars.timecontrol.current_simulation_time-1.0;

		/*
		 * Output configuration here to ensure that updated variables are included in this output
		 */
#if SWEET_MPI
		if (mpi_rank == 0)
#endif
		{
			simVars.outputConfig();
		}
	}



	/**
	 * Write file to data and return string of file name
	 */
	std::string write_file_csv(
			const SphereData_Spectral &i_sphereData,
			const char* i_name,		///< name of output variable
			bool i_phi_shifted = false
	)
	{
		char buffer[1024];

		// create copy
		SphereData_Physical sphereData = i_sphereData.getSphereDataPhysical();

		const char* filename_template = simVars.iodata.output_file_name.c_str();
		sprintf(buffer, filename_template, i_name, simVars.timecontrol.current_simulation_time*simVars.iodata.output_time_scale);

		if (i_phi_shifted)
			sphereData.physical_file_write_lon_pi_shifted(buffer, "vorticity, lon pi shifted");
		else
			sphereData.physical_file_write(buffer);

		return buffer;
	}



	/**
	 * Write file to data and return string of file name
	 */
	std::string write_file_bin(
			const SphereData_Spectral &i_sphereData,
			const char* i_name
	)
	{
		char buffer[1024];

		SphereData_Spectral sphereData(i_sphereData);
		const char* filename_template = simVars.iodata.output_file_name.c_str();
		sprintf(buffer, filename_template, i_name, simVars.timecontrol.current_simulation_time*simVars.iodata.output_time_scale);
		sphereData.file_write_binary_spectral(buffer);

		return buffer;
	}


	std::string output_reference_filenames;

	void write_file_output()
	{
#if SWEET_MPI
		if (mpi_rank > 0)
			return;
#endif

		if (simVars.iodata.output_file_name.length() == 0)
			return;


		std::cout << "Writing output files as simulation time: " << simVars.timecontrol.current_simulation_time << std::endl;

		if (simVars.iodata.output_file_mode == "csv")
		{
			std::string output_filename;
			SphereData_Spectral h = prog_phi*(1.0/simVars.sim.gravitation);

			output_filename = write_file_csv(prog_phi, "prog_phi");
			output_reference_filenames = output_filename;
			std::cout << " + " << output_filename << " (min: " << h.getSphereDataPhysical().physical_reduce_min() << ", max: " << h.getSphereDataPhysical().physical_reduce_max() << ")" << std::endl;

			output_filename = write_file_csv(h, "prog_h");
			output_reference_filenames += ";"+output_filename;
			std::cout << " + " << output_filename << " (min: " << h.getSphereDataPhysical().physical_reduce_min() << ", max: " << h.getSphereDataPhysical().physical_reduce_max() << ")" << std::endl;

			SphereData_Physical u(sphereDataConfig);
			SphereData_Physical v(sphereDataConfig);

			op.robert_vortdiv_to_uv(prog_vort, prog_div, u, v);

			output_filename = write_file_csv(u, "prog_u");
			output_reference_filenames += ";"+output_filename;
			std::cout << " + " << output_filename << std::endl;

			output_filename = write_file_csv(v, "prog_v");
			output_reference_filenames += ";"+output_filename;
			std::cout << " + " << output_filename << std::endl;

			output_filename = write_file_csv(prog_vort, "prog_vort");
			output_reference_filenames += ";"+output_filename;
			std::cout << " + " << output_filename << std::endl;

			output_filename = write_file_csv(prog_div, "prog_div");
			output_reference_filenames += ";"+output_filename;
			std::cout << " + " << output_filename << std::endl;

			SphereData_Spectral potvort = (prog_phi/simVars.sim.gravitation)*prog_vort;

			output_filename = write_file_csv(potvort, "prog_potvort");
			output_reference_filenames += ";"+output_filename;
			std::cout << " + " << output_filename << std::endl;
		}
		else if (simVars.iodata.output_file_mode == "bin")
		{
			std::string output_filename;

			output_filename = write_file_bin(prog_phi, "prog_phi");
			output_reference_filenames = output_filename;
			std::cout << " + " << output_filename << std::endl;

			output_filename = write_file_bin(prog_vort, "prog_vort");
			output_reference_filenames += ";"+output_filename;
			std::cout << " + " << output_filename << std::endl;

			output_filename = write_file_bin(prog_div, "prog_div");
			output_reference_filenames += ";"+output_filename;
			std::cout << " + " << output_filename << std::endl;
		}
		else
		{
			FatalError("Unknown output file mode '"+simVars.iodata.output_file_mode+"'");
		}
	}



	void timestep_do_output()
	{
		if (simVars.misc.compute_errors)
		{
			if (
					simVars.benchmark.benchmark_name != "williamson2"		&&
					simVars.benchmark.benchmark_name != "williamson2_linear"		&&
					simVars.benchmark.benchmark_name != "geostrophic_balance"		&&
					simVars.benchmark.benchmark_name != "geostrophic_balance_linear"		&&
					simVars.benchmark.benchmark_name != "geostrophic_balance_1"		&&
					simVars.benchmark.benchmark_name != "geostrophic_balance_2"		&&
					simVars.benchmark.benchmark_name != "geostrophic_balance_4"		&&
					simVars.benchmark.benchmark_name != "geostrophic_balance_8"		&&
					simVars.benchmark.benchmark_name != "geostrophic_balance_16"	&&
					simVars.benchmark.benchmark_name != "geostrophic_balance_32"	&&
					simVars.benchmark.benchmark_name != "geostrophic_balance_64"	&&
					simVars.benchmark.benchmark_name != "geostrophic_balance_128"	&&
					simVars.benchmark.benchmark_name != "geostrophic_balance_256"	&&
					simVars.benchmark.benchmark_name != "geostrophic_balance_512"
			)
			{
				std::cout << "Benchmark name: " << simVars.benchmark.benchmark_name << std::endl;
				FatalError("Analytical solution not available for this benchmark");
			}

			SphereData_Spectral anal_solution_phi(sphereDataConfig);
			SphereData_Spectral anal_solution_vort(sphereDataConfig);
			SphereData_Spectral anal_solution_div(sphereDataConfig);

			sphereBenchmarks.setup(simVars, op);
			sphereBenchmarks.setupInitialConditions(anal_solution_phi, anal_solution_vort, anal_solution_div);

			/*
			 * Compute difference
			 */
			SphereData_Spectral diff_phi = prog_phi - anal_solution_phi;
			SphereData_Spectral diff_vort = prog_vort - anal_solution_vort;
			SphereData_Spectral diff_div = prog_div - anal_solution_div;

#if SWEET_MPI
			if (mpi_rank == 0)
#endif
			{
				double error_phi = diff_phi.getSphereDataPhysical().physical_reduce_max_abs();
				double error_vort = diff_vort.getSphereDataPhysical().physical_reduce_max_abs();
				double error_div = diff_div.getSphereDataPhysical().physical_reduce_max_abs();

				std::cout << "[MULE] errors: ";
				std::cout << "simtime=" << simVars.timecontrol.current_simulation_time;
				std::cout << "\terror_linf_phi=" << error_phi;
				std::cout << "\terror_linf_vort=" << error_vort;
				std::cout << "\terror_linf_div=" << error_div;
				std::cout << std::endl;
			}
		}

		write_file_output();

		if (simVars.misc.verbosity > 1)
		{
			update_diagnostics();

#if SWEET_MPI
			if (mpi_rank == 0)
#endif
			{
				// Print header
				if (simVars.timecontrol.current_timestep_nr == 0)
				{
					std::cout << "T\tTOTAL_MASS\tPOT_ENERGY\tKIN_ENERGY\tTOT_ENERGY\tPOT_ENSTROPHY\tREL_TOTAL_MASS\tREL_POT_ENERGY\tREL_KIN_ENERGY\tREL_TOT_ENERGY\tREL_POT_ENSTROPHY";
					std::cout << std::endl;
				}

				// Print simulation time, energy and pot enstrophy
				std::cout << simVars.timecontrol.current_simulation_time << "\t";
				std::cout << simVars.diag.total_mass << "\t";
				std::cout << simVars.diag.potential_energy << "\t";
				std::cout << simVars.diag.kinetic_energy << "\t";
				std::cout << simVars.diag.total_energy << "\t";
				std::cout << simVars.diag.total_potential_enstrophy << "\t";

				std::cout << (simVars.diag.total_mass-simVars.diag.ref_total_mass)/simVars.diag.total_mass << "\t";
				std::cout << (simVars.diag.potential_energy-simVars.diag.ref_potential_energy)/simVars.diag.potential_energy << "\t";
				std::cout << (simVars.diag.kinetic_energy-simVars.diag.ref_kinetic_energy)/simVars.diag.kinetic_energy << "\t";
				std::cout << (simVars.diag.total_energy-simVars.diag.total_energy)/simVars.diag.total_energy << "\t";
				std::cout << (simVars.diag.total_potential_enstrophy-simVars.diag.total_potential_enstrophy)/simVars.diag.total_potential_enstrophy << std::endl;

				static double start_tot_energy = -1;
				if (start_tot_energy == -1)
					start_tot_energy = simVars.diag.total_energy;
			}
		}


		if (simVars.misc.verbosity > 0)
		{
#if SWEET_MPI
			if (mpi_rank == 0)
#endif
				std::cout << "prog_phi min/max:\t" << prog_phi.getSphereDataPhysical().physical_reduce_min() << ", " << prog_phi.getSphereDataPhysical().physical_reduce_max() << std::endl;
		}

		if (simVars.iodata.output_each_sim_seconds > 0)
			while (simVars.iodata.output_next_sim_seconds <= simVars.timecontrol.current_simulation_time)
				simVars.iodata.output_next_sim_seconds += simVars.iodata.output_each_sim_seconds;
	}




public:
	bool timestep_check_output()
	{
#if SWEET_MPI
		if (mpi_rank > 0)
			return false;
#endif

		if (simVars.misc.verbosity > 0)
			std::cout << "." << std::flush;

		// output each time step
		if (simVars.iodata.output_each_sim_seconds < 0)
			return false;

		if (simVars.timecontrol.current_simulation_time == timestep_last_output_simtime)
			return false;

		timestep_last_output_simtime = simVars.timecontrol.current_simulation_time;

		if (simVars.timecontrol.current_simulation_time < simVars.timecontrol.max_simulation_time - simVars.iodata.output_each_sim_seconds*1e-10)
		{
			if (simVars.iodata.output_next_sim_seconds > simVars.timecontrol.current_simulation_time)
				return false;
		}

		if (simVars.misc.verbosity > 0)
			std::cout << std::endl;

		timestep_do_output();

		return true;
	}



public:
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



	bool detect_instability()
	{
#if 0
		double max_abs_value = std::abs(simVars.sim.h0)*2.0*simVars.sim.gravitation;

		if (
				SphereData_Spectral(prog_phi).physical_reduce_max_abs() > max_abs_value &&
				simVars.benchmark.benchmark_id != 4
		)
		{
			std::cerr << "Instability detected (max abs value of h > " << max_abs_value << ")" << std::endl;
			return true;
		}
#endif

		if (prog_phi.getSphereDataPhysical().physical_isAnyNaNorInf())
		{
			std::cerr << "Inf value detected" << std::endl;
			return true;
		}

		return false;
	}



	void run_timestep()
	{
#if SWEET_GUI
		if (simVars.misc.gui_enabled && simVars.misc.normal_mode_analysis_generation == 0)
			timestep_check_output();
#endif

		if (simVars.timecontrol.current_simulation_time + simVars.timecontrol.current_timestep_size > simVars.timecontrol.max_simulation_time)
			simVars.timecontrol.current_timestep_size = simVars.timecontrol.max_simulation_time - simVars.timecontrol.current_simulation_time;

		timeSteppers.master->run_timestep(
				prog_phi, prog_vort, prog_div,
				simVars.timecontrol.current_timestep_size,
				simVars.timecontrol.current_simulation_time
			);

		/*
		 * Add implicit viscosity
		 */
		if (simVars.sim.viscosity != 0)
		{
			double scalar = simVars.sim.viscosity*simVars.timecontrol.current_timestep_size;
			double r = simVars.sim.sphere_radius;

			/*
			 * (1-dt*visc*D2)p(t+dt) = p(t)
			 */
			prog_phi = prog_phi.spectral_solve_helmholtz(1.0, -scalar, r);
			prog_vort = prog_vort.spectral_solve_helmholtz(1.0, -scalar, r);
			prog_div = prog_div.spectral_solve_helmholtz(1.0, -scalar, r);
		}

		// advance time step and provide information to parameters
		simVars.timecontrol.current_simulation_time += simVars.timecontrol.current_timestep_size;
		simVars.timecontrol.current_timestep_nr++;

#if SWEET_GUI
		timestep_check_output();
#endif
	}



	void normalmode_analysis()
	{
		NormalModeAnalysisSphere::normal_mode_analysis(
				prog_phi,
				prog_vort,
				prog_div,
				simVars,
				this,
				&SimulationInstance::run_timestep
			);
	}



#if SWEET_GUI

	/**
	 * postprocessing of frame: do time stepping
	 */
	void vis_post_frame_processing(
			int i_num_iterations
	)
	{
		if (simVars.timecontrol.run_simulation_timesteps)
			for (int i = 0; i < i_num_iterations && !should_quit(); i++)
				run_timestep();
	}



	void vis_get_vis_data_array(
			const PlaneData **o_dataArray,
			double *o_aspect_ratio,
			int *o_render_primitive_id,
			void **o_bogus_data
	)
	{
		// request rendering of sphere
		*o_render_primitive_id = render_primitive_id;
		*o_bogus_data = sphereDataConfig;

		int id = simVars.misc.vis_id % 6;
		switch (id)
		{
			default:
			case 0:
				viz_plane_data = Convert_SphereDataSpectral_To_PlaneData::physical_convert(SphereData_Spectral(prog_phi), planeDataConfig);
				break;

			case 1:
				viz_plane_data = Convert_SphereDataSpectral_To_PlaneData::physical_convert(SphereData_Spectral(prog_vort), planeDataConfig);
				break;

			case 2:
				viz_plane_data = Convert_SphereDataSpectral_To_PlaneData::physical_convert(SphereData_Spectral(prog_div), planeDataConfig);
				break;

			case 3:
				// USE COPY TO AVOID FORWARD/BACKWARD TRANSFORMATION
				viz_plane_data = Convert_SphereDataSpectral_To_PlaneData::physical_convert(SphereData_Spectral(prog_phi)/simVars.sim.gravitation, planeDataConfig);
				break;

			case 4:
			{
				SphereData_Physical u(prog_vort.sphereDataConfig);
				SphereData_Physical v(prog_vort.sphereDataConfig);

				// Don't use Robert, since we're not interested in the Robert formulation here
				op.vortdiv_to_uv(prog_vort, prog_div, u, v);
				viz_plane_data = Convert_SphereDataSpectral_To_PlaneData::physical_convert(u, planeDataConfig);
				break;
			}

			case 5:
			{
				SphereData_Physical u(prog_vort.sphereDataConfig);
				SphereData_Physical v(prog_vort.sphereDataConfig);

				// Don't use Robert, since we're not interested in the Robert formulation here
				op.vortdiv_to_uv(prog_vort, prog_div, u, v);
				viz_plane_data = Convert_SphereDataSpectral_To_PlaneData::physical_convert(v, planeDataConfig);
				break;
			}

		}

		*o_dataArray = &viz_plane_data;
		*o_aspect_ratio = 0.5;
	}



	/**
	 * return status string for window title
	 */
	const char* vis_get_status_string()
	{
		const char* description = "";

		int id = simVars.misc.vis_id % 6;

		switch (id)
		{
		default:
		case 0:
			description = "phi";
			break;

		case 1:
			description = "vort";
			break;

		case 2:
			description = "div";
			break;

		case 3:
			description = "h";
			break;

		case 4:
			description = "u";
			break;

		case 5:
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

		case 'c':
			write_file_output();
			break;

#if 0
case 'C':
			// dump data arrays to VTK
			prog_h.file_physical_saveData_vtk("swe_rexi_dump_h.vtk", "Height");
			prog_u.file_physical_saveData_vtk("swe_rexi_dump_u.vtk", "U-Velocity");
			prog_v.file_physical_saveData_vtk("swe_rexi_dump_v.vtk", "V-Velocity");
			break;

		case 'l':
			// load data arrays
			prog_h.file_physical_loadData("swe_rexi_dump_h.csv", simVars.setup_velocityformulation_progphiuv.input_data_binary);
			prog_u.file_physical_loadData("swe_rexi_dump_u.csv", simVars.setup_velocityformulation_progphiuv.input_data_binary);
			prog_v.file_physical_loadData("swe_rexi_dump_v.csv", simVars.setup_velocityformulation_progphiuv.input_data_binary);
			break;
#endif
		}
	}
#endif
};



int main(int i_argc, char *i_argv[])
{
	// Time counter
	SimulationBenchmarkTimings::getInstance().main.start();

#if __MIC__
	std::cout << "Compiled for MIC" << std::endl;
#endif

#if SWEET_MPI

	#if SWEET_THREADING_SPACE
		int provided;
		MPI_Init_thread(&i_argc, &i_argv, MPI_THREAD_MULTIPLE, &provided);

		if (provided != MPI_THREAD_MULTIPLE)
		{
				std::cerr << "MPI_THREAD_MULTIPLE not available! Try to get an MPI version with multi-threading support or compile without OMP/TBB support. Good bye..." << std::endl;
				exit(-1);
		}
	#else
		MPI_Init(&i_argc, &i_argv);
	#endif

#endif

	//input parameter names (specific ones for this program)
	const char *bogus_var_names[] = {
			nullptr
	};

#if SWEET_MPI
	int mpi_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
#endif

	// Help menu
	if (!simVars.setupFromMainParameters(i_argc, i_argv, bogus_var_names))
	{
#if SWEET_PARAREAL
		simVars.parareal.printOptions();
#endif
		return -1;
	}

	if (simVars.misc.verbosity > 3)
		std::cout << " + setup SH sphere transformations..." << std::endl;

	sphereDataConfigInstance.setupAuto(simVars.disc.space_res_physical, simVars.disc.space_res_spectral, simVars.misc.reuse_spectral_transformation_plans);

	int res_physical_nodealias[2] = {
			2*simVars.disc.space_res_spectral[0],
			simVars.disc.space_res_spectral[1]
		};

	if (simVars.misc.verbosity > 3)
		std::cout << " + setup SH sphere transformations (nodealiasing)..." << std::endl;

	sphereDataConfigInstance_nodealiasing.setupAuto(res_physical_nodealias, simVars.disc.space_res_spectral, simVars.misc.reuse_spectral_transformation_plans);


#if SWEET_GUI
	if (simVars.misc.verbosity > 3)
		std::cout << " + setup FFT plane transformations..." << std::endl;

	planeDataConfigInstance.setupAutoSpectralSpace(simVars.disc.space_res_physical, simVars.misc.reuse_spectral_transformation_plans);
#endif

	std::ostringstream buf;
	buf << std::setprecision(14);

	if (simVars.misc.verbosity > 3)
		std::cout << " + setup finished" << std::endl;

#if SWEET_MPI
	std::cout << "Helo from MPI rank: " << mpi_rank << std::endl;

	// only start simulation and time stepping for first rank
	if (mpi_rank > 0)
	{
		/*
		 * Deactivate all output for ranks larger than the current one
		 */
		simVars.misc.verbosity = 0;
		simVars.iodata.output_each_sim_seconds = -1;
	}
#endif

	{
#if SWEET_MPI
		if (mpi_rank == 0)
#endif
		{
			std::cout << "SPH config string: " << sphereDataConfigInstance.getConfigInformationString() << std::endl;
		}

#if SWEET_PARAREAL
		if (simVars.parareal.enabled)
		{
			/*
			 * Allocate parareal controller and provide class
			 * which implement the parareal features
			 */
			Parareal_Controller_Serial<SimulationInstance> parareal_Controller_Serial;

			// setup controller. This initializes several simulation instances
			parareal_Controller_Serial.benchmark(&simVars.parareal);

			// execute the simulation
			parareal_Controller_Serial.run();
		}
		else
#endif

#if SWEET_GUI // The VisSweet directly calls simulationSWE->reset() and output stuff
		if (simVars.misc.gui_enabled)
		{
			SimulationInstance *simulationSWE = new SimulationInstance;
			VisSweet<SimulationInstance> visSweet(simulationSWE);
			delete simulationSWE;
		}
		else
#endif
		{
			SimulationInstance *simulationSWE = new SimulationInstance;


			if (simVars.misc.normal_mode_analysis_generation > 0)
			{
				simulationSWE->normalmode_analysis();
			}
			else
			{
				// Do first output before starting timer
				simulationSWE->timestep_check_output();

#if SWEET_MPI
				// Start counting time
				if (mpi_rank == 0)
				{
					std::cout << "********************************************************************************" << std::endl;
					std::cout << "Parallel performance information: MPI barrier & timer starts here" << std::endl;
					std::cout << "********************************************************************************" << std::endl;
				}
				MPI_Barrier(MPI_COMM_WORLD);
#endif

				SimulationBenchmarkTimings::getInstance().main_timestepping.start();

				// Main time loop
				while (true)
				{
					// Stop simulation if requested
					if (simulationSWE->should_quit())
						break;

					// Test for some output to be done
					simulationSWE->timestep_check_output();

					// Main call for timestep run
					simulationSWE->run_timestep();

					// Instability
					if (simVars.misc.instability_checks)
					{
#if SWEET_MPI
						if (mpi_rank == 0)
#endif
						{
							if (simulationSWE->detect_instability())
							{
								std::cout << "INSTABILITY DETECTED" << std::endl;
								std::cerr << "INSTABILITY DETECTED" << std::endl;
								// IMPORANT: EXIT IN CASE OF INSTABILITIES
								exit(1);
								break;
							}
						}
					}
				}

				// Stop counting time
				SimulationBenchmarkTimings::getInstance().main_timestepping.stop();

#if SWEET_MPI
				MPI_Barrier(MPI_COMM_WORLD);
#endif

				if (simVars.misc.verbosity > 0)
					std::cout << std::endl;
#if SWEET_MPI
				// Start counting time
				if (mpi_rank == 0)
				{
					std::cout << "********************************************************************************" << std::endl;
					std::cout << "Parallel performance information: timer stopped here" << std::endl;
					std::cout << "********************************************************************************" << std::endl;
				}
#endif

				// Do some output after the time loop
				simulationSWE->timestep_check_output();
			}

			if (simVars.iodata.output_file_name.size() > 0)
				std::cout << "[MULE] reference_filenames: " << simulationSWE->output_reference_filenames << std::endl;

			std::cout << "[MULE] simulation_successfully_finished: 1" << std::endl;

			delete simulationSWE;
		}

		SimulationBenchmarkTimings::getInstance().main.stop();
	}


#if SWEET_MPI
	if (mpi_rank == 0)
#endif
	{
		// End of run output results
		std::cout << std::endl;
		SimulationBenchmarkTimings::getInstance().output();

		std::cout << "***************************************************" << std::endl;
		std::cout << "* Other timing information (direct)" << std::endl;
		std::cout << "***************************************************" << std::endl;
		std::cout << "[MULE] simVars.timecontrol.current_timestep_nr: " << simVars.timecontrol.current_timestep_nr << std::endl;
		std::cout << "[MULE] simVars.timecontrol.current_timestep_size: " << simVars.timecontrol.current_timestep_size << std::endl;
		std::cout << std::endl;
		std::cout << "***************************************************" << std::endl;
		std::cout << "* Other timing information (derived)" << std::endl;
		std::cout << "***************************************************" << std::endl;
		std::cout << "[MULE] simulation_benchmark_timings.time_per_time_step (secs/ts): " << SimulationBenchmarkTimings::getInstance().main_timestepping()/(double)simVars.timecontrol.current_timestep_nr << std::endl;
	}


#if SWEET_MPI
	MPI_Finalize();
#endif

	return 0;
}

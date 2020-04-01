/*
 * SWE_Sphere_REXI.cpp
 *
 *  Created on: 25 Oct 2016
 *      Author: Martin Schreiber <SchreiberX@gmail.com>
 */

#include "SWE_Sphere_TS_l_rexi.hpp"

#include <iostream>
#include <cassert>
#include <utility>
#include <rexi/REXI.hpp>
#include <sweet/sphere/Convert_SphereDataSpectralComplex_to_SphereDataSpectral.hpp>
#include <sweet/sphere/Convert_SphereDataSpectral_to_SphereDataSpectralComplex.hpp>
#include <sweet/SimulationBenchmarkTiming.hpp>

#ifndef SWEET_THREADING_TIME_REXI
#	define SWEET_THREADING_TIME_REXI 1
#endif


/*
 * Compute the REXI sum massively parallel *without* a parallelization with parfor in space
 */
#if SWEET_THREADING_TIME_REXI
#	include <omp.h>
#endif


#ifndef SWEET_MPI
#	define SWEET_MPI 1
#endif



#if SWEET_MPI
#	include <mpi.h>
#endif



SWE_Sphere_TS_l_rexi::SWE_Sphere_TS_l_rexi(
		SimulationVariables &i_simVars,
		SphereOperators_SphereData &i_op
)	:
	simVars(i_simVars),
	simCoeffs(simVars.sim),
	op(i_op),
	sphereDataConfig(i_op.sphereDataConfig),
	sphereDataConfigSolver(nullptr)
{

	#if SWEET_BENCHMARK_TIMINGS
		SimulationBenchmarkTimings::getInstance().rexi.start();
		SimulationBenchmarkTimings::getInstance().rexi_setup.start();
	#endif

	#if !SWEET_USE_LIBFFT
		FatalError("Spectral space required for solvers, use compile option --libfft=enable");
	#endif

	#if SWEET_THREADING_TIME_REXI

		num_local_rexi_par_threads = omp_get_max_threads();

		if (num_local_rexi_par_threads == 0)
		{
			std::cerr << "FATAL ERROR: omp_get_max_threads == 0" << std::endl;
			exit(-1);
		}
	#else
		num_local_rexi_par_threads = 1;
	#endif

	#if SWEET_MPI

		MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
		MPI_Comm_size(MPI_COMM_WORLD, &num_mpi_ranks);

		num_global_threads = num_local_rexi_par_threads * num_mpi_ranks;

	#else

		num_global_threads = num_local_rexi_par_threads;

	#endif


	#if SWEET_BENCHMARK_TIMINGS
		SimulationBenchmarkTimings::getInstance().rexi_setup.stop();
		SimulationBenchmarkTimings::getInstance().rexi.stop();
	#endif
}



void SWE_Sphere_TS_l_rexi::reset()
{
	#if SWEET_BENCHMARK_TIMINGS
		SimulationBenchmarkTimings::getInstance().rexi.start();
		SimulationBenchmarkTimings::getInstance().rexi_setup.start();
	#endif

	for (std::vector<PerThreadVars*>::iterator iter = perThreadVars.begin(); iter != perThreadVars.end(); iter++)
	{
		PerThreadVars* p = *iter;
		delete p;
	}

	perThreadVars.resize(0);

	sphereDataConfigSolver = nullptr;

	#if SWEET_BENCHMARK_TIMINGS
		SimulationBenchmarkTimings::getInstance().rexi_setup.stop();
		SimulationBenchmarkTimings::getInstance().rexi.stop();
	#endif
}



SWE_Sphere_TS_l_rexi::~SWE_Sphere_TS_l_rexi()
{
	#if SWEET_BENCHMARK_TIMINGS
		SimulationBenchmarkTimings::getInstance().rexi.start();
		SimulationBenchmarkTimings::getInstance().rexi_shutdown.start();
	#endif

	for (std::vector<PerThreadVars*>::iterator iter = perThreadVars.begin(); iter != perThreadVars.end(); iter++)
	{
		PerThreadVars* p = *iter;
		delete p;
	}

	#if SWEET_BENCHMARK_TIMINGS
		#if SWEET_MPI

			int num_ranks;
			MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);

			if (num_ranks > 1)
			{
				/*
				 * Send broadcast information from 2nd rank to 1st rank
				 *
				 * This is required in case of buffered broadcasts from the 1st rank
				 * which makes MPI_Bcast to return immediately.
				 */
				if (mpi_rank == 1)
				{
					double data = SimulationBenchmarkTimings::getInstance().rexi_timestepping_broadcast.time;
					MPI_Send(&data, sizeof(double), MPI_BYTE, 0, 0, MPI_COMM_WORLD);
				}

				if (mpi_rank == 0)
				{
					MPI_Status status;
					MPI_Recv(&SimulationBenchmarkTimings::getInstance().rexi_timestepping_broadcast.time, sizeof(double), MPI_BYTE, 1, 0, MPI_COMM_WORLD, &status);
				}

			}
		#endif

		SimulationBenchmarkTimings::getInstance().rexi_shutdown.stop();
		SimulationBenchmarkTimings::getInstance().rexi.stop();
	#endif
}



void SWE_Sphere_TS_l_rexi::p_get_workload_start_end(
		std::size_t &o_start,
		std::size_t &o_end,
		int i_local_thread_id
)
{
	std::size_t max_N = rexi_alphas.size();

	#if SWEET_THREADING_TIME_REXI || SWEET_MPI

		#if SWEET_MPI
			int global_thread_id = i_local_thread_id + num_local_rexi_par_threads*mpi_rank;
		#else
			int global_thread_id = i_local_thread_id;
		#endif

		assert(block_size >= 0);
		assert(global_thread_id >= 0);

		o_start = std::min(max_N, block_size*global_thread_id);
		o_end = std::min(max_N, o_start+block_size);

	#else

		o_start = 0;
		o_end = max_N;

	#endif
}



/**
 * setup the REXI
 */
void SWE_Sphere_TS_l_rexi::setup_new(
		REXI_SimulationVariables &i_rexi,
		const std::string &i_function_name,
		double i_timestep_size,
		bool i_include_coriolis,
		bool i_use_f_sphere
)
{
	setup(
			i_rexi,
			i_function_name,
			i_timestep_size,
			i_use_f_sphere,
			!i_include_coriolis
		);
}

/**
 * setup the REXI
 */
void SWE_Sphere_TS_l_rexi::setup(
		REXI_SimulationVariables &i_rexi,
		const std::string &i_function_name,
		double i_timestep_size,
		bool i_use_f_sphere,
		bool i_no_coriolis
)
{
	no_coriolis = i_no_coriolis;

	rexiSimVars = &i_rexi;

	reset();

	#if SWEET_BENCHMARK_TIMINGS
		SimulationBenchmarkTimings::getInstance().rexi.start();
		SimulationBenchmarkTimings::getInstance().rexi_setup.start();
	#endif

	rexi_use_direct_solution = (rexiSimVars->rexi_method == "direct");

	if (rexi_use_direct_solution)
	{
		if (!no_coriolis)
			FatalError("Direct solution for linear operator with Coriolis effect not available");
	}

	timestep_size = i_timestep_size;
	function_name = i_function_name;

	REXICoefficients<double> rexiCoefficients;

	bool retval = REXI<>::load(
			rexiSimVars,
			function_name,
			rexiCoefficients,
			simVars.misc.verbosity
	);

	if (!retval)
		FatalError(std::string("Phi function '")+function_name+std::string("' not provided"));

	rexi_alphas = rexiCoefficients.alphas;
	rexi_betas = rexiCoefficients.betas;
	rexi_gamma = rexiCoefficients.gamma;

	if (rexi_gamma.real() != 0 || rexi_gamma.imag() != 0)
		FatalError("'rexi_gamma != 0' is Not yet supported");

	rexi_use_sphere_extended_modes = rexiSimVars->use_sphere_extended_modes;
	use_f_sphere = i_use_f_sphere;
	use_rexi_sphere_solver_preallocation = rexiSimVars->sphere_solver_preallocation;

	if (!rexi_use_direct_solution)
	{
		if (rexi_use_sphere_extended_modes == 0)
		{
			sphereDataConfigSolver = sphereDataConfig;
		}
		else
		{
			// Add modes only along latitude since these are the "problematic" modes
			sphereDataConfigInstance.setupAdditionalModes(
					sphereDataConfig,
					rexi_use_sphere_extended_modes,	// TODO: Extend SPH wrapper to also support m != n to set this guy to 0
					rexi_use_sphere_extended_modes,
					simVars.misc.reuse_spectral_transformation_plans
			);

			sphereDataConfigSolver = &sphereDataConfigInstance;
		}

		std::size_t N = rexi_alphas.size();
		block_size = N/num_global_threads;
		if (block_size*num_global_threads != N)
			block_size++;

		perThreadVars.resize(num_local_rexi_par_threads);


		/**
		 * We split the setup from the utilization here.
		 *
		 * This is necessary, since it has to be assured that
		 * the FFTW plans are initialized before using them.
		 */
		if (num_local_rexi_par_threads == 0)
		{
			std::cerr << "FATAL ERROR B: omp_get_max_threads == 0" << std::endl;
			exit(-1);
		}

		#if SWEET_THREADING_SPACE || SWEET_THREADING_TIME_REXI
			if (omp_in_parallel())
			{
				std::cerr << "FATAL ERROR X: in parallel region" << std::endl;
				exit(-1);
			}
		#endif

		// use a kind of serialization of the input to avoid threading conflicts in the ComplexFFT generation
		for (int j = 0; j < num_local_rexi_par_threads; j++)
		{
			#if SWEET_THREADING_TIME_REXI
			#pragma omp parallel for schedule(static,1) default(none) shared(std::cout,j)
			#endif
			for (int local_thread_id = 0; local_thread_id < num_local_rexi_par_threads; local_thread_id++)
			{
				if (local_thread_id != j)
					continue;

				#if SWEET_DEBUG && SWEET_THREADING_TIME_REXI
					if (omp_get_thread_num() != local_thread_id)
					{
						// leave this dummy std::cout in it to avoid the intel compiler removing this part
						std::cout << "ERROR: thread " << omp_get_thread_num() << " number mismatch " << local_thread_id << std::endl;
						exit(-1);
					}
				#endif

				perThreadVars[local_thread_id] = new PerThreadVars;

				std::size_t start, end;
				p_get_workload_start_end(start, end, local_thread_id);
				int local_size = (int)end-(int)start;

				#if SWEET_DEBUG
					if (local_size < 0)
						FatalError("local_size < 0");
				#endif

				perThreadVars[local_thread_id]->alpha.resize(local_size);
				perThreadVars[local_thread_id]->beta_re.resize(local_size);

				perThreadVars[local_thread_id]->accum_phi.setup(sphereDataConfigSolver);
				perThreadVars[local_thread_id]->accum_vort.setup(sphereDataConfigSolver);
				perThreadVars[local_thread_id]->accum_div.setup(sphereDataConfigSolver);

				for (std::size_t n = start; n < end; n++)
				{
					int thread_local_idx = n-start;

					perThreadVars[local_thread_id]->alpha[thread_local_idx] = rexi_alphas[n];
					perThreadVars[local_thread_id]->beta_re[thread_local_idx] = rexi_betas[n];
				}
			}
		}

		p_update_coefficients(false);

		if (num_local_rexi_par_threads == 0)
		{
			std::cerr << "FATAL ERROR C: omp_get_max_threads == 0" << std::endl;
			exit(-1);
		}

	}	// rexi_use_direct_solution

	#if SWEET_BENCHMARK_TIMINGS
		SimulationBenchmarkTimings::getInstance().rexi_setup.stop();
		SimulationBenchmarkTimings::getInstance().rexi.stop();
	#endif
}



void SWE_Sphere_TS_l_rexi::p_update_coefficients(
		bool i_update_rexi
)
{
	if (i_update_rexi)
	{
		REXI<>::load(
				rexiSimVars,
				function_name,
				rexi_alphas,
				rexi_betas,
				rexi_gamma,
				simVars.misc.verbosity
		);
	}

	#if SWEET_THREADING_TIME_REXI
	#pragma omp parallel for schedule(static,1) default(none) shared(std::cout)
	#endif
	for (int local_thread_id = 0; local_thread_id < num_local_rexi_par_threads; local_thread_id++)
	{
		std::size_t start, end;
		p_get_workload_start_end(start, end, local_thread_id);
		int local_size = (int)end-(int)start;

		if (use_rexi_sphere_solver_preallocation)
		{
			perThreadVars[local_thread_id]->rexiSPHRobert_vector.resize(local_size);

			for (std::size_t n = start; n < end; n++)
			{
				int thread_local_idx = n-start;

				perThreadVars[local_thread_id]->rexiSPHRobert_vector[thread_local_idx].setup_vectorinvariant_progphivortdiv(
						sphereDataConfigSolver,
						perThreadVars[local_thread_id]->alpha[thread_local_idx],
						perThreadVars[local_thread_id]->beta_re[thread_local_idx],
						simCoeffs.sphere_radius,
						simCoeffs.sphere_rotating_coriolis_omega,
						simCoeffs.sphere_fsphere_f0,
						simCoeffs.h0 * simCoeffs.gravitation,
						timestep_size,
						use_f_sphere,
						no_coriolis
					);
			}
		}
	}
}




void SWE_Sphere_TS_l_rexi::run_timestep(
	const SphereData_Spectral &i_prog_phi0,
	const SphereData_Spectral &i_prog_vort0,
	const SphereData_Spectral &i_prog_div0,

	SphereData_Spectral &o_prog_phi0,
	SphereData_Spectral &o_prog_vort0,
	SphereData_Spectral &o_prog_div0,

	double i_fixed_dt,		///< if this value is not equal to 0, use this time step size instead of computing one
	double i_simulation_timestamp
)
{
	#if SWEET_BENCHMARK_TIMINGS
		SimulationBenchmarkTimings::getInstance().rexi.start();
		SimulationBenchmarkTimings::getInstance().rexi_timestepping.start();
	#endif

	o_prog_phi0 = i_prog_phi0;
	o_prog_vort0 = i_prog_vort0;
	o_prog_div0 = i_prog_div0;

	run_timestep(o_prog_phi0, o_prog_vort0, o_prog_div0, i_fixed_dt, i_simulation_timestamp);

	#if SWEET_BENCHMARK_TIMINGS
		SimulationBenchmarkTimings::getInstance().rexi_timestepping.stop();
		SimulationBenchmarkTimings::getInstance().rexi.stop();
	#endif
}




/**
 * Solve the REXI of \f$ U(t) = exp(L*t) \f$
 *
 * See
 * 		doc/rexi/understanding_rexi.pdf
 *
 * for further information
 */
void SWE_Sphere_TS_l_rexi::run_timestep(
	SphereData_Spectral &io_prog_phi0,
	SphereData_Spectral &io_prog_vort0,
	SphereData_Spectral &io_prog_div0,

	double i_fixed_dt,		///< if this value is not equal to 0, use this time step size instead of computing one
	double i_simulation_timestamp
)
{
	#if SWEET_BENCHMARK_TIMINGS
		SimulationBenchmarkTimings::getInstance().rexi.start();
		SimulationBenchmarkTimings::getInstance().rexi_timestepping.start();
	#endif

	if (rexi_use_direct_solution)
	{
		// no Coriolis force active

		/*
		 * Using exponential integrators, we must compute an
		 */

		double ir = 1.0/simVars.sim.sphere_radius;
		// avg. geopotential

		double G = -simCoeffs.h0*simCoeffs.gravitation;

		/*
		 * See doc/rexi/rexi_for_swe_on_nonrotating_sphere.pdf
		 */

		SWEET_THREADING_SPACE_PARALLEL_FOR
		for (int m = 0; m <= sphereDataConfig->spectral_modes_m_max; m++)
		{
			std::size_t idx = sphereDataConfig->getArrayIndexByModes(m, m);
			for (int n = m; n <= sphereDataConfig->spectral_modes_n_max; n++)
			{
				double D = -(double)n*((double)n+1.0)*ir*ir;
				D = -D;

				if (D == 0)
				{
					idx++;
					continue;
				}

				std::complex<double> &phi0 = io_prog_phi0.spectral_space_data[idx];
				std::complex<double> &div0 = io_prog_div0.spectral_space_data[idx];

				// TODO: precompute this

				// result will be imaginary only!
				//std::complex<double> sqrt_D = std::sqrt(std::complex<double>(D));
				//std::complex<double> sqrt_G = std::sqrt(std::complex<double>(G));
				std::complex<double> sqrt_DG = std::sqrt(std::complex<double>(D*G));

				// Multiply with Q^{-1}
				std::complex<double> l0 = -sqrt_DG/(2*G) * phi0 + 0.5*div0;
				std::complex<double> l1 = +sqrt_DG/(2*G) * phi0 + 0.5*div0;

				l0 = std::exp(timestep_size*(-sqrt_DG))*l0;
				l1 = std::exp(timestep_size*sqrt_DG)*l1;

				phi0 = -G/sqrt_DG * l0 + G/sqrt_DG* l1;
				div0 = l0 + l1;

				//std::cout << phi0 << std::endl;
				//std::cout << div0 << std::endl;

				idx++;
			}
		}


		#if SWEET_BENCHMARK_TIMINGS
			SimulationBenchmarkTimings::getInstance().rexi_timestepping.stop();
			SimulationBenchmarkTimings::getInstance().rexi.stop();
		#endif

		return;
	} // direct solution

	/*
	 * PREPROCESSING
	 */
	#if SWEET_BENCHMARK_TIMINGS
		SimulationBenchmarkTimings::getInstance().rexi_timestepping_miscprocessing.start();
	#endif

		if (i_fixed_dt <= 0)
			FatalError("Only constant time step size allowed");

		double update_dt_delta = std::abs(timestep_size - i_fixed_dt)/std::max(timestep_size, i_fixed_dt);
		if (update_dt_delta > 1e-9)
		{
			std::cout << "Warning: Reducing time step size from " << i_fixed_dt << " to " << timestep_size << ", threshold " << update_dt_delta << " exceeded" << std::endl;

			std::cout << "timestep_size: " << timestep_size << std::endl;
			std::cout << "i_fixed_dt: " << i_fixed_dt << std::endl;
			std::cout << "a: " << std::abs(timestep_size - i_fixed_dt) << std::endl;
			std::cout << "b: " << std::max(timestep_size, i_fixed_dt) << std::endl;
			std::cout << "c: " << std::abs(timestep_size - i_fixed_dt)/std::max(timestep_size, i_fixed_dt) << std::endl;
			std::cout << "update_dt_delta: " << update_dt_delta << std::endl;

			timestep_size = i_fixed_dt;

			p_update_coefficients(true);
		}

		#if SWEET_REXI_TIMINGS_ADDITIONAL_BARRIERS && SWEET_MPI
			MPI_Barrier(MPI_COMM_WORLD);
		#endif

	#if SWEET_BENCHMARK_TIMINGS
		SimulationBenchmarkTimings::getInstance().rexi_timestepping_miscprocessing.stop();
	#endif

	/*
	 * BROADCAST
	 */

	#if SWEET_BENCHMARK_TIMINGS
		SimulationBenchmarkTimings::getInstance().rexi_timestepping_broadcast.start();
	#endif

		#if SWEET_MPI
			/*
			 * We should measure this for the 2nd rank! And we do so (see later on)
			 */

			std::size_t spectral_data_num_doubles = io_prog_phi0.sphereDataConfig->spectral_array_data_number_of_elements*2;

			MPI_Bcast(io_prog_phi0.spectral_space_data, spectral_data_num_doubles, MPI_DOUBLE, 0, MPI_COMM_WORLD);
			MPI_Bcast(io_prog_vort0.spectral_space_data, spectral_data_num_doubles, MPI_DOUBLE, 0, MPI_COMM_WORLD);
			MPI_Bcast(io_prog_div0.spectral_space_data, spectral_data_num_doubles, MPI_DOUBLE, 0, MPI_COMM_WORLD);

		#endif

	#if SWEET_BENCHMARK_TIMINGS
		SimulationBenchmarkTimings::getInstance().rexi_timestepping_broadcast.stop();
	#endif


	/*
	 * Special handler for REXI without using mode extension and no threading to speedup things
	 */
	#if !SWEET_THREADING_TIME_REXI

		#if SWEET_BENCHMARK_TIMINGS
			SimulationBenchmarkTimings::getInstance().rexi_timestepping_solver.start();
		#endif

			std::size_t start, end;
			p_get_workload_start_end(start, end, 0);

			perThreadVars[0]->accum_phi.spectral_set_zero();
			perThreadVars[0]->accum_vort.spectral_set_zero();
			perThreadVars[0]->accum_div.spectral_set_zero();

			if (simVars.rexi.use_sphere_extended_modes == 0)
			{
				/*
				 * -> No threading
				 * -> No extended modes
				 */

				SphereData_Spectral tmp_prog_phi(sphereDataConfigSolver);
				SphereData_Spectral tmp_prog_vort(sphereDataConfigSolver);
				SphereData_Spectral tmp_prog_div(sphereDataConfigSolver);

				for (std::size_t workload_idx = start; workload_idx < end; workload_idx++)
				{
					int local_idx = workload_idx-start;

					if (use_rexi_sphere_solver_preallocation)
					{
						perThreadVars[0]->rexiSPHRobert_vector[local_idx].solve_vectorinvariant_progphivortdiv(
								io_prog_phi0, io_prog_vort0, io_prog_div0,
								tmp_prog_phi, tmp_prog_vort, tmp_prog_div
							);
					}
					else
					{
						SWERexiTerm_SPHRobert rexiSPHRobert;

						std::complex<double> &alpha = perThreadVars[0]->alpha[local_idx];
						std::complex<double> &beta_re = perThreadVars[0]->beta_re[local_idx];

						rexiSPHRobert.setup_vectorinvariant_progphivortdiv(
								sphereDataConfigSolver,	///< sphere data for input data
								alpha,
								beta_re,

								simCoeffs.sphere_radius,
								simCoeffs.sphere_rotating_coriolis_omega,
								simCoeffs.sphere_fsphere_f0,
								simCoeffs.h0*simCoeffs.gravitation,
								i_fixed_dt,

								use_f_sphere,
								no_coriolis
						);

						rexiSPHRobert.solve_vectorinvariant_progphivortdiv(
								io_prog_phi0, io_prog_vort0, io_prog_div0,
								tmp_prog_phi, tmp_prog_vort, tmp_prog_div
							);
					}

					perThreadVars[0]->accum_phi += tmp_prog_phi;
					perThreadVars[0]->accum_vort += tmp_prog_vort;
					perThreadVars[0]->accum_div += tmp_prog_div;
				}


				io_prog_phi0 = perThreadVars[0]->accum_phi;
				io_prog_vort0 = perThreadVars[0]->accum_vort;
				io_prog_div0 = perThreadVars[0]->accum_div;
			}
			else
			{
				/*
				 * no threading
				 * with extended modes
				 */

				SphereData_Spectral thread_prog_phi0 = io_prog_phi0.spectral_returnWithDifferentModes(sphereDataConfigSolver);
				SphereData_Spectral thread_prog_vort0 = io_prog_vort0.spectral_returnWithDifferentModes(sphereDataConfigSolver);
				SphereData_Spectral thread_prog_div0 = io_prog_div0.spectral_returnWithDifferentModes(sphereDataConfigSolver);

				SphereData_Spectral tmp_prog_phi(sphereDataConfigSolver);
				SphereData_Spectral tmp_prog_vort(sphereDataConfigSolver);
				SphereData_Spectral tmp_prog_div(sphereDataConfigSolver);

				for (std::size_t workload_idx = start; workload_idx < end; workload_idx++)
				{
					int local_idx = workload_idx-start;

					if (use_rexi_sphere_solver_preallocation)
					{
						perThreadVars[0]->rexiSPHRobert_vector[local_idx].solve_vectorinvariant_progphivortdiv(
								thread_prog_phi0, thread_prog_vort0, thread_prog_div0,
								tmp_prog_phi, tmp_prog_vort, tmp_prog_div
							);
					}
					else
					{
						SWERexiTerm_SPHRobert rexiSPHRobert;

						std::complex<double> &alpha = perThreadVars[0]->alpha[local_idx];
						std::complex<double> &beta_re = perThreadVars[0]->beta_re[local_idx];

						rexiSPHRobert.setup_vectorinvariant_progphivortdiv(
								sphereDataConfigSolver,	///< sphere data for input data
								alpha,
								beta_re,

								simCoeffs.sphere_radius,
								simCoeffs.sphere_rotating_coriolis_omega,
								simCoeffs.sphere_fsphere_f0,
								simCoeffs.h0*simCoeffs.gravitation,
								i_fixed_dt,

								use_f_sphere,
								no_coriolis
						);

						rexiSPHRobert.solve_vectorinvariant_progphivortdiv(
								thread_prog_phi0, thread_prog_vort0, thread_prog_div0,
								tmp_prog_phi, tmp_prog_vort, tmp_prog_div
							);
					}

					perThreadVars[0]->accum_phi += tmp_prog_phi;
					perThreadVars[0]->accum_vort += tmp_prog_vort;
					perThreadVars[0]->accum_div += tmp_prog_div;
				}

				io_prog_phi0 = perThreadVars[0]->accum_phi.spectral_returnWithDifferentModes(io_prog_phi0.sphereDataConfig);
				io_prog_vort0 = perThreadVars[0]->accum_vort.spectral_returnWithDifferentModes(io_prog_phi0.sphereDataConfig);
				io_prog_div0 = perThreadVars[0]->accum_div.spectral_returnWithDifferentModes(io_prog_phi0.sphereDataConfig);
			}

		#if SWEET_BENCHMARK_TIMINGS
			SimulationBenchmarkTimings::getInstance().rexi_timestepping_solver.stop();
		#endif


	#else	/* SWEET_THREADING_TIME_REXI */


		if (simVars.rexi.use_sphere_extended_modes == 0)
		{
			#if SWEET_BENCHMARK_TIMINGS
				SimulationBenchmarkTimings::getInstance().rexi_timestepping_solver.start();
			#endif

				#pragma omp parallel for schedule(static,1) default(none) shared(i_fixed_dt, io_prog_phi0, io_prog_vort0, io_prog_div0, std::cout, std::cerr)
				for (int local_thread_id = 0; local_thread_id < num_local_rexi_par_threads; local_thread_id++)
				{
					std::size_t start, end;
					p_get_workload_start_end(start, end, local_thread_id);

					/*
					* Make a copy to ensure that there are no race conditions by converting to physical space
					*/
					SphereData_Spectral thread_io_prog_phi0 = io_prog_phi0;
					SphereData_Spectral thread_io_prog_vort0 = io_prog_vort0;
					SphereData_Spectral thread_io_prog_div0 = io_prog_div0;

					SphereData_Spectral tmp_prog_phi(sphereDataConfigSolver);
					SphereData_Spectral tmp_prog_vort(sphereDataConfigSolver);
					SphereData_Spectral tmp_prog_div(sphereDataConfigSolver);

					perThreadVars[local_thread_id]->accum_phi.spectral_set_zero();
					perThreadVars[local_thread_id]->accum_vort.spectral_set_zero();
					perThreadVars[local_thread_id]->accum_div.spectral_set_zero();


					for (std::size_t workload_idx = start; workload_idx < end; workload_idx++)
					{
						int local_idx = workload_idx-start;

						if (use_rexi_sphere_solver_preallocation)
						{
							perThreadVars[local_thread_id]->rexiSPHRobert_vector[local_idx].solve_vectorinvariant_progphivortdiv(
									thread_io_prog_phi0, thread_io_prog_vort0, thread_io_prog_div0,
									tmp_prog_phi, tmp_prog_vort, tmp_prog_div
								);
						}
						else
						{
							SWERexiTerm_SPHRobert rexiSPHRobert;

							std::complex<double> &alpha = perThreadVars[local_thread_id]->alpha[local_idx];
							std::complex<double> &beta_re = perThreadVars[local_thread_id]->beta_re[local_idx];

							rexiSPHRobert.setup_vectorinvariant_progphivortdiv(
									sphereDataConfigSolver,	///< sphere data for input data
									alpha,
									beta_re,

									simCoeffs.sphere_radius,
									simCoeffs.sphere_rotating_coriolis_omega,
									simCoeffs.sphere_fsphere_f0,
									simCoeffs.h0*simCoeffs.gravitation,
									i_fixed_dt,

									use_f_sphere,
									no_coriolis
							);

							rexiSPHRobert.solve_vectorinvariant_progphivortdiv(
									io_prog_phi0, io_prog_vort0, io_prog_div0,
									tmp_prog_phi, tmp_prog_vort, tmp_prog_div
								);
						}

						perThreadVars[local_thread_id]->accum_phi += tmp_prog_phi;
						perThreadVars[local_thread_id]->accum_vort += tmp_prog_vort;
						perThreadVars[local_thread_id]->accum_div += tmp_prog_div;
					}
				}

			#if SWEET_BENCHMARK_TIMINGS
				SimulationBenchmarkTimings::getInstance().rexi_timestepping_solver.stop();
				SimulationBenchmarkTimings::getInstance().rexi_timestepping_reduce.start();
			#endif

				io_prog_phi0.spectral_set_zero();
				io_prog_vort0.spectral_set_zero();
				io_prog_div0.spectral_set_zero();

				for (int thread_id = 0; thread_id < num_local_rexi_par_threads; thread_id++)
				{
					assert(io_prog_phi0.sphereDataConfig->spectral_array_data_number_of_elements == perThreadVars[0]->accum_phi.sphereDataConfig->spectral_array_data_number_of_elements);

#if 0
					perThreadVars[thread_id]->accum_phi.request_data_physical();
					#pragma omp parallel for schedule(static) default(none) shared(io_prog_phi0, thread_id)
					for (int i = 0; i < io_prog_phi0.sphereDataConfig->physical_array_data_number_of_elements; i++)
						io_prog_phi0.physical_space_data[i] += perThreadVars[thread_id]->accum_phi.physical_space_data[i];

					perThreadVars[thread_id]->accum_vort.request_data_physical();
					#pragma omp parallel for schedule(static) default(none) shared(io_prog_vort0, thread_id)
					for (int i = 0; i < io_prog_vort0.sphereDataConfig->physical_array_data_number_of_elements; i++)
						io_prog_vort0.physical_space_data[i] += perThreadVars[thread_id]->accum_vort.physical_space_data[i];


					perThreadVars[thread_id]->accum_div.request_data_physical();
					#pragma omp parallel for schedule(static) default(none) shared(io_prog_div0, thread_id)
					for (int i = 0; i < io_prog_div0.sphereDataConfig->physical_array_data_number_of_elements; i++)
						io_prog_div0.physical_space_data[i] += perThreadVars[thread_id]->accum_div.physical_space_data[i];
#else
					#pragma omp parallel for schedule(static) default(none) shared(io_prog_phi0, thread_id)
					for (int i = 0; i < io_prog_phi0.sphereDataConfig->spectral_array_data_number_of_elements; i++)
						io_prog_phi0.spectral_space_data[i] += perThreadVars[thread_id]->accum_phi.spectral_space_data[i];

					#pragma omp parallel for schedule(static) default(none) shared(io_prog_vort0, thread_id)
					for (int i = 0; i < io_prog_vort0.sphereDataConfig->spectral_array_data_number_of_elements; i++)
						io_prog_vort0.spectral_space_data[i] += perThreadVars[thread_id]->accum_vort.spectral_space_data[i];

					#pragma omp parallel for schedule(static) default(none) shared(io_prog_div0, thread_id)
					for (int i = 0; i < io_prog_div0.sphereDataConfig->spectral_array_data_number_of_elements; i++)
						io_prog_div0.spectral_space_data[i] += perThreadVars[thread_id]->accum_div.spectral_space_data[i];
#endif
				}

			#if SWEET_BENCHMARK_TIMINGS
				SimulationBenchmarkTimings::getInstance().rexi_timestepping_reduce.stop();
			#endif
			
		}
		else
		{
			#if SWEET_BENCHMARK_TIMINGS
				SimulationBenchmarkTimings::getInstance().rexi_timestepping_solver.start();
			#endif

				#pragma omp parallel for schedule(static,1) default(none) shared(i_fixed_dt, io_prog_phi0, io_prog_vort0, io_prog_div0, std::cout, std::cerr)
				for (int local_thread_id = 0; local_thread_id < num_local_rexi_par_threads; local_thread_id++)
				{
					std::size_t start, end;
					p_get_workload_start_end(start, end, local_thread_id);

					/*
					 * threaded rexi sum 
					 * extended modes
					 */
					SphereData_Spectral thread_prog_phi0(sphereDataConfigSolver);
					SphereData_Spectral thread_prog_vort0(sphereDataConfigSolver);
					SphereData_Spectral thread_prog_div0(sphereDataConfigSolver);

					thread_prog_phi0 = io_prog_phi0.spectral_returnWithDifferentModes(sphereDataConfigSolver);
					thread_prog_vort0 = io_prog_vort0.spectral_returnWithDifferentModes(sphereDataConfigSolver);
					thread_prog_div0 = io_prog_div0.spectral_returnWithDifferentModes(sphereDataConfigSolver);

					SphereData_Spectral tmp_prog_phi(sphereDataConfigSolver);
					SphereData_Spectral tmp_prog_vort(sphereDataConfigSolver);
					SphereData_Spectral tmp_prog_div(sphereDataConfigSolver);

					perThreadVars[local_thread_id]->accum_phi.spectral_set_zero();
					perThreadVars[local_thread_id]->accum_vort.spectral_set_zero();
					perThreadVars[local_thread_id]->accum_div.spectral_set_zero();

					for (std::size_t workload_idx = start; workload_idx < end; workload_idx++)
					{
						int local_idx = workload_idx-start;

						if (use_rexi_sphere_solver_preallocation)
						{
							perThreadVars[local_thread_id]->rexiSPHRobert_vector[local_idx].solve_vectorinvariant_progphivortdiv(
									thread_prog_phi0, thread_prog_vort0, thread_prog_div0,
									tmp_prog_phi, tmp_prog_vort, tmp_prog_div
								);
						}
						else
						{
							SWERexiTerm_SPHRobert rexiSPHRobert;

							std::complex<double> &alpha = perThreadVars[local_thread_id]->alpha[local_idx];
							std::complex<double> &beta_re = perThreadVars[local_thread_id]->beta_re[local_idx];

							rexiSPHRobert.setup_vectorinvariant_progphivortdiv(
									sphereDataConfigSolver,	///< sphere data for input data
									alpha,
									beta_re,

									simCoeffs.sphere_radius,
									simCoeffs.sphere_rotating_coriolis_omega,
									simCoeffs.sphere_fsphere_f0,
									simCoeffs.h0*simCoeffs.gravitation,
									i_fixed_dt,

									use_f_sphere,
									no_coriolis
							);

							rexiSPHRobert.solve_vectorinvariant_progphivortdiv(
									thread_prog_phi0, thread_prog_vort0, thread_prog_div0,
									tmp_prog_phi, tmp_prog_vort, tmp_prog_div
								);
						}

						perThreadVars[local_thread_id]->accum_phi += tmp_prog_phi;
						perThreadVars[local_thread_id]->accum_vort += tmp_prog_vort;
						perThreadVars[local_thread_id]->accum_div += tmp_prog_div;
					}
				}

				assert(io_prog_phi0.sphereDataConfig->spectral_array_data_number_of_elements == sphereDataConfig->spectral_array_data_number_of_elements);

			#if SWEET_BENCHMARK_TIMINGS
				SimulationBenchmarkTimings::getInstance().rexi_timestepping_solver.stop();
				SimulationBenchmarkTimings::getInstance().rexi_timestepping_reduce.start();
			#endif

#if 0
			io_prog_phi0.physical_set_zero();
			io_prog_vort0.physical_set_zero();
			io_prog_div0.physical_set_zero();

			SphereData_Spectral tmp(sphereDataConfig);
			for (int thread_id = 0; thread_id < num_local_rexi_par_threads; thread_id++)
			{
				tmp = perThreadVars[thread_id]->accum_phi.spectral_returnWithDifferentModes(tmp.sphereDataConfig);
				tmp.request_data_physical();
				#pragma omp parallel for schedule(static) default(none) shared(io_prog_phi0, tmp)
				for (int i = 0; i < io_prog_phi0.sphereDataConfig->physical_array_data_number_of_elements; i++)
					io_prog_phi0.physical_space_data[i] += tmp.physical_space_data[i];

				tmp = perThreadVars[thread_id]->accum_vort.spectral_returnWithDifferentModes(tmp.sphereDataConfig);
				tmp.request_data_physical();
				#pragma omp parallel for schedule(static) default(none) shared(io_prog_vort0, tmp)
				for (int i = 0; i < io_prog_vort0.sphereDataConfig->physical_array_data_number_of_elements; i++)
					io_prog_vort0.physical_space_data[i] += tmp.physical_space_data[i];

				tmp = perThreadVars[thread_id]->accum_div.spectral_returnWithDifferentModes(tmp.sphereDataConfig);
				tmp.request_data_physical();
				#pragma omp parallel for schedule(static) default(none) shared(io_prog_div0, tmp)
				for (int i = 0; i < io_prog_div0.sphereDataConfig->physical_array_data_number_of_elements; i++)
					io_prog_div0.physical_space_data[i] += tmp.physical_space_data[i];
			}
#else
			io_prog_phi0.spectral_set_zero();
			io_prog_vort0.spectral_set_zero();
			io_prog_div0.spectral_set_zero();

			SphereData_Spectral tmp(sphereDataConfig);
			for (int thread_id = 0; thread_id < num_local_rexi_par_threads; thread_id++)
			{
				tmp = perThreadVars[thread_id]->accum_phi.spectral_returnWithDifferentModes(tmp.sphereDataConfig);
				#pragma omp parallel for schedule(static) default(none) shared(io_prog_phi0, tmp)
				for (int i = 0; i < io_prog_phi0.sphereDataConfig->spectral_array_data_number_of_elements; i++)
					io_prog_phi0.spectral_space_data[i] += tmp.spectral_space_data[i];

				tmp = perThreadVars[thread_id]->accum_vort.spectral_returnWithDifferentModes(tmp.sphereDataConfig);
				#pragma omp parallel for schedule(static) default(none) shared(io_prog_vort0, tmp)
				for (int i = 0; i < io_prog_vort0.sphereDataConfig->spectral_array_data_number_of_elements; i++)
					io_prog_vort0.spectral_space_data[i] += tmp.spectral_space_data[i];

				tmp = perThreadVars[thread_id]->accum_div.spectral_returnWithDifferentModes(tmp.sphereDataConfig);
				#pragma omp parallel for schedule(static) default(none) shared(io_prog_div0, tmp)
				for (int i = 0; i < io_prog_div0.sphereDataConfig->spectral_array_data_number_of_elements; i++)
					io_prog_div0.spectral_space_data[i] += tmp.spectral_space_data[i];
			}
#endif
			#if SWEET_BENCHMARK_TIMINGS
				SimulationBenchmarkTimings::getInstance().rexi_timestepping_reduce.stop();
			#endif
		}


	#endif	// END SWEET_THREADING_TIME_REXI

	#if SWEET_REXI_TIMINGS_ADDITIONAL_BARRIERS && SWEET_MPI
		#if SWEET_BENCHMARK_TIMINGS
			SimulationBenchmarkTimings::getInstance().rexi_timestepping_miscprocessing.start();
		#endif

			MPI_Barrier(MPI_COMM_WORLD);

		#if SWEET_BENCHMARK_TIMINGS
			SimulationBenchmarkTimings::getInstance().rexi_timestepping_miscprocessing.stop();
		#endif
	#endif


	#if SWEET_MPI

		#if SWEET_BENCHMARK_TIMINGS
			SimulationBenchmarkTimings::getInstance().rexi_timestepping_reduce.start();
		#endif

		#if 1
			/*
			 * Reduction in spectral space
			 *
			 * There was once a reason for this to be done in physical space.
			 * However, eventually, it seems to work also in spectral space.
			 */

			SphereData_Spectral tmp(sphereDataConfig);

// already setup
//			std::size_t spectral_data_num_doubles = io_prog_phi0.sphereDataConfig->spectral_array_data_number_of_elements*2;

			#if SWEET_REXI_ALLREDUCE

				MPI_Allreduce(prog_phi0_phys.spectral_space_data, tmp.spectral_space_data, spectral_data_num_doubles, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
				std::swap(prog_phi0_phys.spectral_space_data, tmp.spectral_space_data);

				MPI_Allreduce(prog_vort0_phys.spectral_space_data, tmp.spectral_space_data, spectral_data_num_doubles, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
				std::swap(prog_vort0.spectral_space_data, tmp.spectral_space_data);

				MPI_Allreduce(prog_div0_phys.spectral_space_data, tmp.spectral_space_data, spectral_data_num_doubles, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
				std::swap(prog_div0.spectral_space_data, tmp.spectral_space_data);

			#else

				MPI_Reduce(io_prog_phi0.spectral_space_data, tmp.spectral_space_data, spectral_data_num_doubles, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
				if (mpi_rank == 0)
					std::swap(io_prog_phi0.spectral_space_data, tmp.spectral_space_data);

				MPI_Reduce(io_prog_vort0.spectral_space_data, tmp.spectral_space_data, spectral_data_num_doubles, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
				if (mpi_rank == 0)
					std::swap(io_prog_vort0.spectral_space_data, tmp.spectral_space_data);

				MPI_Reduce(io_prog_div0.spectral_space_data, tmp.spectral_space_data, spectral_data_num_doubles, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
				if (mpi_rank == 0)
					std::swap(io_prog_div0.spectral_space_data, tmp.spectral_space_data);

			#endif

		#else

			/*
			 * Reduction in physical space
			 */

			SphereData_Physical prog_phi0_phys = io_prog_phi0.getSphereDataPhysical();
			SphereData_Physical prog_vort0_phys = io_prog_vort0.getSphereDataPhysical();
			SphereData_Physical prog_div0_phys = io_prog_div0.getSphereDataPhysical();

			/*
			 * Physical data reduction
			 *
			 * WE HAVE to do the reduction in physical space!
			 *
			 * Comment from Martin to Martin: I forgot why this was necessary :-(
			 */

			SphereData_Physical tmp(sphereDataConfig);

			std::size_t physical_data_num_doubles = prog_phi0_phys.sphereDataConfig->physical_array_data_number_of_elements;

			#if SWEET_REXI_ALLREDUCE

				MPI_Allreduce(prog_phi0_phys.physical_space_data, tmp.physical_space_data, physical_data_num_doubles, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
				std::swap(prog_phi0_phys.physical_space_data, tmp.physical_space_data);

				MPI_Allreduce(prog_vort0_phys.physical_space_data, tmp.physical_space_data, physical_data_num_doubles, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
				std::swap(prog_vort0.physical_space_data, tmp.physical_space_data);

				MPI_Allreduce(prog_div0_phys.physical_space_data, tmp.physical_space_data, physical_data_num_doubles, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
				std::swap(prog_div0.physical_space_data, tmp.physical_space_data);

			#else

				MPI_Reduce(prog_phi0_phys.physical_space_data, tmp.physical_space_data, physical_data_num_doubles, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
				if (mpi_rank == 0)
					std::swap(prog_phi0_phys.physical_space_data, tmp.physical_space_data);

				MPI_Reduce(prog_vort0_phys.physical_space_data, tmp.physical_space_data, physical_data_num_doubles, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
				if (mpi_rank == 0)
					std::swap(prog_vort0_phys.physical_space_data, tmp.physical_space_data);

				MPI_Reduce(prog_div0_phys.physical_space_data, tmp.physical_space_data, physical_data_num_doubles, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
				if (mpi_rank == 0)
					std::swap(prog_div0_phys.physical_space_data, tmp.physical_space_data);

			#endif

			io_prog_phi0.loadSphereDataPhysical(prog_phi0_phys);
			io_prog_vort0.loadSphereDataPhysical(prog_vort0_phys);
			io_prog_div0.loadSphereDataPhysical(prog_div0_phys);

		#endif

		#if SWEET_BENCHMARK_TIMINGS
			SimulationBenchmarkTimings::getInstance().rexi_timestepping_reduce.stop();
		#endif
	#endif



	#if SWEET_BENCHMARK_TIMINGS
		SimulationBenchmarkTimings::getInstance().rexi_timestepping.stop();
		SimulationBenchmarkTimings::getInstance().rexi.stop();
	#endif
}


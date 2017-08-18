/*
 * SWE_Sphere_REXI.cpp
 *
 *  Created on: 25 Oct 2016
 *      Author: Martin Schreiber <M.Schreiber@exeter.ac.uk>
 */

#include "SWE_Sphere_TS_l_rexi.hpp"

#include <rexi/REXI_Terry.hpp>
#include <cassert>
#include <sweet/sphere/Convert_SphereDataComplex_to_SphereData.hpp>
#include <sweet/sphere/Convert_SphereData_to_SphereDataComplex.hpp>


#ifndef SWEET_REXI_THREAD_PARALLEL_SUM
#	define SWEET_REXI_THREAD_PARALLEL_SUM 1
#endif

/*
 * Compute the REXI sum massively parallel *without* a parallelization with parfor in space
 */
#if SWEET_REXI_THREAD_PARALLEL_SUM
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
		SphereOperators &i_op
)	:
	simVars(i_simVars),
	op(i_op),
	sphereDataConfig(i_op.sphereDataConfig),
	sphereDataConfigSolver(nullptr)
{
#if !SWEET_USE_LIBFFT
	std::cerr << "Spectral space required for solvers, use compile option --libfft=enable" << std::endl;
	exit(-1);
#endif


#if SWEET_REXI_THREAD_PARALLEL_SUM

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

#else

	mpi_rank = 0;
	num_mpi_ranks = 1;

#endif

	num_global_threads = num_local_rexi_par_threads * num_mpi_ranks;
}



void SWE_Sphere_TS_l_rexi::reset()
{
	for (std::vector<PerThreadVars*>::iterator iter = perThreadVars.begin(); iter != perThreadVars.end(); iter++)
	{
		PerThreadVars* p = *iter;
		delete p;
	}

	perThreadVars.resize(0);

	sphereDataConfigSolver = nullptr;
}



SWE_Sphere_TS_l_rexi::~SWE_Sphere_TS_l_rexi()
{
	reset();

#if SWEET_BENCHMARK_REXI
	if (mpi_rank == 0)
	{
		std::cout << "STOPWATCH broadcast: " << stopwatch_broadcast() << std::endl;
		std::cout << "STOPWATCH preprocessing: " << stopwatch_preprocessing() << std::endl;
		std::cout << "STOPWATCH reduce: " << stopwatch_reduce() << std::endl;
		std::cout << "STOPWATCH solve_rexi_terms: " << stopwatch_solve_rexi_terms() << std::endl;
	}
#endif
}



void SWE_Sphere_TS_l_rexi::get_workload_start_end(
		std::size_t &o_start,
		std::size_t &o_end
)
{
	std::size_t max_N = rexi.alpha.size();

#if SWEET_REXI_THREAD_PARALLEL_SUM || SWEET_MPI

	#if SWEET_THREADING || SWEET_REXI_THREAD_PARALLEL_SUM
		int local_thread_id = omp_get_thread_num();
	#else
		int local_thread_id = 0;
	#endif

	int global_thread_id = local_thread_id + num_local_rexi_par_threads*mpi_rank;

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
void SWE_Sphere_TS_l_rexi::setup(
		double i_h,						///< sampling size
		int i_M,						///< number of sampling points
		int i_L,						///< number of sampling points for Gaussian approx.
										///< set to 0 for auto detection

		double i_timestep_size,

		bool i_rexi_half,				///< use half-pole reduction

		int i_rexi_use_extended_modes,
		int i_rexi_normalization,
		bool i_use_f_sphere,

		bool i_use_rexi_sphere_preallocation	///< preallocate all rexi coefficients (might fail because of memory limitations)
)
{
	reset();

	M = i_M;
	h = i_h;
	normalization = i_rexi_normalization;
	timestep_size = i_timestep_size;
	rexi_use_extended_modes = i_rexi_use_extended_modes;
	use_f_sphere = i_use_f_sphere;
	use_rexi_preallocation = i_use_rexi_sphere_preallocation;

	if (rexi_use_extended_modes == 0)
	{
		sphereDataConfigSolver = sphereDataConfig;
	}
	else
	{
		// Add modes only along latitude since these are the "problematic" modes
		sphereDataConfigInstance.setupAdditionalModes(
				sphereDataConfig,
				rexi_use_extended_modes,	// TODO: Extend SPH wrapper to also support m != n to set this guy to 0
				rexi_use_extended_modes
		);

		sphereDataConfigSolver = &sphereDataConfigInstance;
	}

	rexi.setup(0, h, M, i_L, i_rexi_half, normalization);

	std::size_t N = rexi.alpha.size();
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

#if SWEET_THREADING || SWEET_REXI_THREAD_PARALLEL_SUM
	if (omp_in_parallel())
	{
		std::cerr << "FATAL ERROR X: in parallel region" << std::endl;
		exit(-1);
	}
#endif

	// use a kind of serialization of the input to avoid threading conflicts in the ComplexFFT generation
	for (int j = 0; j < num_local_rexi_par_threads; j++)
	{
#if SWEET_REXI_THREAD_PARALLEL_SUM
#	pragma omp parallel for schedule(static,1) default(none) shared(std::cout,j)
#endif
		for (int i = 0; i < num_local_rexi_par_threads; i++)
		{
			if (i != j)
				continue;

#if SWEET_THREADING || SWEET_REXI_THREAD_PARALLEL_SUM
			if (omp_get_thread_num() != i)
			{
				// leave this dummy std::cout in it to avoid the intel compiler removing this part
				std::cout << "ERROR: thread " << omp_get_thread_num() << " number mismatch " << i << std::endl;
				exit(-1);
			}
#endif

			perThreadVars[i] = new PerThreadVars;

			std::size_t start, end;
			get_workload_start_end(start, end);
			int local_size = (int)end-(int)start;

#if SWEET_DEBUG
			if (local_size < 0)
				FatalError("local_size < 0");
#endif

			perThreadVars[i]->alpha.resize(local_size);
			perThreadVars[i]->beta_re.resize(local_size);

			perThreadVars[i]->accum_phi.setup(sphereDataConfigSolver);
			perThreadVars[i]->accum_vort.setup(sphereDataConfigSolver);
			perThreadVars[i]->accum_div.setup(sphereDataConfigSolver);

			for (std::size_t n = start; n < end; n++)
			{
				int thread_local_idx = n-start;

				perThreadVars[i]->alpha[thread_local_idx] = rexi.alpha[n];
				perThreadVars[i]->beta_re[thread_local_idx] = rexi.beta_re[n];
			}
		}
	}

	update_coefficients();

	if (num_local_rexi_par_threads == 0)
	{
		std::cerr << "FATAL ERROR C: omp_get_max_threads == 0" << std::endl;
		exit(-1);
	}


#if SWEET_BENCHMARK_REXI
	stopwatch_preprocessing.reset();
	stopwatch_broadcast.reset();
	stopwatch_reduce.reset();
	stopwatch_solve_rexi_terms.reset();
#endif
}



void SWE_Sphere_TS_l_rexi::update_coefficients()
{
	// use a kind of serialization of the input to avoid threading conflicts in the ComplexFFT generation
	for (int j = 0; j < num_local_rexi_par_threads; j++)
	{
#if SWEET_REXI_THREAD_PARALLEL_SUM
#	pragma omp parallel for schedule(static,1) default(none) shared(std::cout,j)
#endif
		for (int i = 0; i < num_local_rexi_par_threads; i++)
		{
			if (i != j)
				continue;

			std::size_t start, end;
			get_workload_start_end(start, end);
			int local_size = (int)end-(int)start;

			if (use_rexi_preallocation)
			{
				perThreadVars[i]->rexiSPHRobert_vector.resize(local_size);

				for (std::size_t n = start; n < end; n++)
				{
					int thread_local_idx = n-start;

					perThreadVars[i]->rexiSPHRobert_vector[thread_local_idx].setup_vectorinvariant_progphivortdiv(
							sphereDataConfigSolver,
							perThreadVars[i]->alpha[thread_local_idx],
							perThreadVars[i]->beta_re[thread_local_idx],
							simVars.sim.earth_radius,
							simVars.sim.coriolis_omega,
							simVars.sim.f0,
							simVars.sim.h0 * simVars.sim.gravitation,
							timestep_size,
							use_f_sphere
						);
				}
			}
		}
	}
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
	SphereData &io_prog_phi0,
	SphereData &io_prog_vort0,
	SphereData &io_prog_div0,

	double i_fixed_dt,		///< if this value is not equal to 0, use this time step size instead of computing one
	double i_simulation_timestamp
)
{
	if (i_fixed_dt <= 0)
		FatalError("Only constant time step size allowed");

	if (timestep_size != i_fixed_dt)
	{
		timestep_size = i_fixed_dt;
		std::cout << "Warning: Reducing time step size from " << i_fixed_dt << " to " << timestep_size << std::endl;
		update_coefficients();
	}


	io_prog_phi0.request_data_spectral();
	io_prog_vort0.request_data_spectral();
	io_prog_div0.request_data_spectral();


#if SWEET_MPI

		/*
		 * TODO: Maybe we should measure this for the 2nd rank!!!
		 * The reason could be since Bcast might already return before the packages were actually received!
		 */
	#if SWEET_BENCHMARK_REXI
		if (mpi_rank == 0)
			stopwatch_broadcast.start();
	#endif

		std::size_t spectral_data_num_doubles = io_prog_phi0.sphereDataConfig->spectral_array_data_number_of_elements*2;

		MPI_Bcast(io_prog_phi0.spectral_space_data, spectral_data_num_doubles, MPI_DOUBLE, 0, MPI_COMM_WORLD);

		if (std::isnan(io_prog_phi0.spectral_get(0,0).real()))
		{
			final_timestep = true;
			return;
		}

		final_timestep = false;

		MPI_Bcast(io_prog_vort0.spectral_space_data, spectral_data_num_doubles, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Bcast(io_prog_div0.spectral_space_data, spectral_data_num_doubles, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	#if SWEET_BENCHMARK_REXI
		if (mpi_rank == 0)
			stopwatch_broadcast.stop();
	#endif

#endif	// SWEET_MPI


#if SWEET_REXI_THREAD_PARALLEL_SUM
#	pragma omp parallel for schedule(static,1) default(none) shared(i_fixed_dt, io_prog_phi0, io_prog_vort0, io_prog_div0, std::cout, std::cerr)
#endif
	for (int thread_id = 0; thread_id < num_local_rexi_par_threads; thread_id++)
	{

#if SWEET_BENCHMARK_REXI
		bool stopwatch_measure = false;
	#if SWEET_REXI_THREAD_PARALLEL_SUM
		if (omp_get_thread_num() == 0)
	#endif
			if (mpi_rank == 0)
				stopwatch_measure = true;
#endif

#if SWEET_BENCHMARK_REXI
		if (stopwatch_measure)
			stopwatch_preprocessing.start();
#endif

		std::size_t start, end;
		get_workload_start_end(start, end);

		/*
		 * DO SUM IN PARALLEL
		 */
		SphereData thread_prog_phi0(sphereDataConfigSolver);
		SphereData thread_prog_vort0(sphereDataConfigSolver);
		SphereData thread_prog_div0(sphereDataConfigSolver);

#if SWEET_DEBUG
		/**
		 * THIS ASSERTION IS VERY IMPORTANT!
		 * OTHERWISE io_prog_*0 will be converted to
		 * spectral space *in parallel* with write
		 * access raceconditions
		 */
		if (	!io_prog_phi0.spectral_space_data_valid	||
				!io_prog_vort0.spectral_space_data_valid	||
				!io_prog_div0.spectral_space_data_valid
		)
		{
			FatalError("SPECTRAL DATA NOT AVAILABLE, BUT REQUIRED!");
		}
#endif

		thread_prog_phi0 = io_prog_phi0.spectral_returnWithDifferentModes(sphereDataConfigSolver);
		thread_prog_vort0 = io_prog_vort0.spectral_returnWithDifferentModes(sphereDataConfigSolver);
		thread_prog_div0 = io_prog_div0.spectral_returnWithDifferentModes(sphereDataConfigSolver);

		SphereData tmp_prog_phi(sphereDataConfigSolver);
		SphereData tmp_prog_vort(sphereDataConfigSolver);
		SphereData tmp_prog_div(sphereDataConfigSolver);

		perThreadVars[thread_id]->accum_phi.spectral_set_zero();
		perThreadVars[thread_id]->accum_vort.spectral_set_zero();
		perThreadVars[thread_id]->accum_div.spectral_set_zero();


		for (std::size_t workload_idx = start; workload_idx < end; workload_idx++)
		{
			int local_idx = workload_idx-start;

			if (use_rexi_preallocation)
			{
				perThreadVars[thread_id]->rexiSPHRobert_vector[local_idx].solve_vectorinvariant_progphivortdiv(
						thread_prog_phi0, thread_prog_vort0, thread_prog_div0,
						tmp_prog_phi, tmp_prog_vort, tmp_prog_div
					);
			}
			else
			{
				SWERexiTerm_SPHRobert rexiSPHRobert;

				std::complex<double> &alpha = perThreadVars[thread_id]->alpha[local_idx];
				std::complex<double> &beta_re = perThreadVars[thread_id]->beta_re[local_idx];

				rexiSPHRobert.setup_vectorinvariant_progphivortdiv(
						sphereDataConfigSolver,	///< sphere data for input data
						alpha,
						beta_re,

						simVars.sim.earth_radius,
						simVars.sim.coriolis_omega,
						simVars.sim.f0,
						simVars.sim.h0*simVars.sim.gravitation,
						i_fixed_dt,

						use_f_sphere
				);

				rexiSPHRobert.solve_vectorinvariant_progphivortdiv(
						thread_prog_phi0, thread_prog_vort0, thread_prog_div0,
						tmp_prog_phi, tmp_prog_vort, tmp_prog_div
					);
			}

			perThreadVars[thread_id]->accum_phi += tmp_prog_phi;
			perThreadVars[thread_id]->accum_vort += tmp_prog_vort;
			perThreadVars[thread_id]->accum_div += tmp_prog_div;
		}

#if SWEET_DEBUG
		if (	!io_prog_phi0.spectral_space_data_valid	||
				!io_prog_vort0.spectral_space_data_valid	||
				!io_prog_div0.spectral_space_data_valid
			)
		{
			FatalError("SPECTRAL DATA NOT AVAILABLE, BUT REQUIRED!");
		}
#endif

#if SWEET_BENCHMARK_REXI
		if (stopwatch_measure)
			stopwatch_solve_rexi_terms.stop();
#endif
	}

#if SWEET_BENCHMARK_REXI
	if (mpi_rank == 0)
		stopwatch_reduce.start();
#endif

#if SWEET_REXI_THREAD_PARALLEL_SUM

	io_prog_phi0.physical_set_zero();
	io_prog_vort0.physical_set_zero();
	io_prog_div0.physical_set_zero();

	for (int thread_id = 0; thread_id < num_local_rexi_par_threads; thread_id++)
	{
		if (rexi_use_extended_modes == 0)
		{
			assert(io_prog_phi0.sphereDataConfig->spectral_array_data_number_of_elements == perThreadVars[0]->accum_phi.sphereDataConfig->spectral_array_data_number_of_elements);

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
		}
		else
		{
			assert(io_prog_phi0.sphereDataConfig->spectral_array_data_number_of_elements == sphereDataConfig->spectral_array_data_number_of_elements);

			SphereData tmp(sphereDataConfig);

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
	}

#else

	io_prog_phi0 = perThreadVars[0]->accum_phi.spectral_returnWithDifferentModes(io_prog_phi0.sphereDataConfig);
	io_prog_vort0 = perThreadVars[0]->accum_vort.spectral_returnWithDifferentModes(io_prog_vort0.sphereDataConfig);
	io_prog_div0 = perThreadVars[0]->accum_div.spectral_returnWithDifferentModes(io_prog_div0.sphereDataConfig);

#endif

	io_prog_phi0.request_data_physical();
	io_prog_vort0.request_data_physical();
	io_prog_div0.request_data_physical();


#if SWEET_MPI
	std::size_t physical_data_num_doubles = io_prog_phi0.sphereDataConfig->physical_array_data_number_of_elements;

	SphereData tmp(sphereDataConfig);

	int retval = MPI_Reduce(io_prog_phi0.physical_space_data, tmp.physical_space_data, physical_data_num_doubles, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	if (retval != MPI_SUCCESS)
	{
		FatalError("MPI Reduce FAILED!");
		exit(1);
	}

	std::swap(io_prog_phi0.physical_space_data, tmp.physical_space_data);

	MPI_Reduce(io_prog_vort0.physical_space_data, tmp.physical_space_data, physical_data_num_doubles, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	std::swap(io_prog_vort0.physical_space_data, tmp.physical_space_data);

	MPI_Reduce(io_prog_div0.physical_space_data, tmp.physical_space_data, physical_data_num_doubles, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
	std::swap(io_prog_div0.physical_space_data, tmp.physical_space_data);
#endif


#if SWEET_BENCHMARK_REXI
	if (mpi_rank == 0)
		stopwatch_reduce.stop();
#endif
}



void SWE_Sphere_TS_l_rexi:: MPI_quitWorkers(
		SphereDataConfig *i_sphereDataConfig
)
{
#if SWEET_MPI

	SphereData dummyData(i_sphereDataConfig);
	dummyData.spectral_set_value(NAN);

	MPI_Bcast(dummyData.spectral_space_data, dummyData.sphereDataConfig->spectral_array_data_number_of_elements*2, MPI_DOUBLE, 0, MPI_COMM_WORLD);

#endif
}


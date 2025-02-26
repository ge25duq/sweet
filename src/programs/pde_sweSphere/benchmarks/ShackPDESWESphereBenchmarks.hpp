/*
 * Author: Martin SCHREIBER <schreiberx@gmail.com>
 */

#ifndef SRC_INCLUDE_SWEET_SHACKS_SHACK_PDE_SWE_SPHERE_BENCHMARK_HPP_
#define SRC_INCLUDE_SWEET_SHACKS_SHACK_PDE_SWE_SPHERE_BENCHMARK_HPP_

#include <string>
#include <iostream>
#include <sweet/core/shacks/ShackInterface.hpp>
#include <sweet/core/sphere/SphereData_Spectral.hpp>
#include <sweet/core/ScalarDataArray.hpp>

/**
 * Values and parameters to setup benchmarks simulations
 */
class ShackPDESWESphereBenchmarks	:
		public sweet::ShackInterface
{
public:
	/// seed for random number generator
	int random_seed = 0;

	/// benchmark scenario
	std::string benchmark_name = "";

	/// rotation angle for advection equation
	double benchmark_sphere_advection_rotation_angle = 0;

	/**
	 * Flag to indicate the presence of topography
	 */
	bool use_topography = false;

	/*
	 * Topography itself
	 */
	sweet::SphereData_Physical h_topography;



	/// Galewsky-benchmark specific: velocity
	double benchmark_galewsky_umax = -1;

	/// Galewsky-benchmark specific: amplitude of bump
	double benchmark_galewsky_hamp = -1;

	/// Galewsky-benchmark specific: latitude coordinate
	double benchmark_galewsky_phi2 = -1;

	std::string benchmark_galewsky_geostrophic_setup = "analytical";


	/*
	 * Get updated velocities for particular point in time
	 */
	void (*getVelocities)(
			sweet::SphereData_Physical&,
			sweet::SphereData_Physical&,
			double i_time,
			ShackPDESWESphereBenchmarks* user_ptr
	) = nullptr;
	ShackPDESWESphereBenchmarks *getVelocitiesUserData = nullptr;

	/**
	 * Callback for special benchmark
	 */
	void (*callback_slComputeDeparture3rdOrder)(
			void *i_this,
			const sweet::ScalarDataArray &i_pos_lon_A,	///< longitude coordinate to compute the velocity for
			const sweet::ScalarDataArray &i_pos_lat_A,	///< latitude coordinate to compute the velocity for
			sweet::ScalarDataArray &o_pos_lon_D,		///< velocity along longitude
			sweet::ScalarDataArray &o_pos_lat_D,		///< velocity along latitude
			double i_dt,
			double i_timestamp_arrival			///< timestamp at arrival point
	);
	void *slComputeDeparture3rdOrderUserData = nullptr;



	void printProgramArguments(const std::string& i_prefix = "")
	{
		std::cout << i_prefix << std::endl;
		std::cout << i_prefix << "SIMULATION SETUP PARAMETERS:" << std::endl;
		std::cout << i_prefix << "	--benchmark-random-seed [int]		random seed for random number generator" << std::endl;
		std::cout << i_prefix << "	--benchmark-name [string]	benchmark name" << std::endl;
		std::cout << i_prefix << "	--benchmark-override-simvars [bool]	Allow overwriting simulation variables by benchmark (default: 1)" << std::endl;
		std::cout << i_prefix << "	--benchmark-setup-dealiased [bool]	Use dealiasing for setup (default: 1)" << std::endl;
		std::cout << i_prefix << "	--benchmark-advection-rotation-angle [float]	Rotation angle for e.g. advection test case" << std::endl;
		std::cout << i_prefix << "	--benchmark-galewsky-geostropic-setup [str]	'analytical'/'numerical' setup of geostropic balance" << std::endl;
		std::cout << i_prefix << std::endl;
	}

	bool processProgramArguments(sweet::ProgramArguments &i_pa)
	{
		i_pa.getArgumentValueByKey("--benchmark-random-seed", random_seed);
		i_pa.getArgumentValueByKey("--benchmark-name", benchmark_name);

		i_pa.getArgumentValueByKey("--benchmark-advection-rotation-angle", benchmark_sphere_advection_rotation_angle);

		i_pa.getArgumentValueByKey("--benchmark-galewsky-umax", benchmark_galewsky_umax);
		i_pa.getArgumentValueByKey("--benchmark-galewsky-hamp", benchmark_galewsky_hamp);
		i_pa.getArgumentValueByKey("--benchmark-galewsky-phi2", benchmark_galewsky_phi2);
		i_pa.getArgumentValueByKey("--benchmark-galewsky-geostropic-setup", benchmark_galewsky_geostrophic_setup);

		if (error.exists())
			return error.forwardWithPositiveReturn(i_pa.error);

		if (random_seed >= 0)
			srandom(random_seed);

		return error.forwardWithPositiveReturn(i_pa.error);
	}

	void printShack(
		const std::string& i_prefix = ""
	)
	{
		std::cout << i_prefix << std::endl;
		std::cout << i_prefix << "BENCHMARK:" << std::endl;
		std::cout << i_prefix << " + benchmark_name: " << benchmark_name << std::endl;
		std::cout << i_prefix << " + benchmark_random_seed: " << random_seed << std::endl;
		std::cout << i_prefix << " + benchmark_sphere_advection_rotation_angle: " << benchmark_sphere_advection_rotation_angle << std::endl;
		std::cout << i_prefix << " + benchmark_galewsky_umax: " << benchmark_galewsky_umax << std::endl;
		std::cout << i_prefix << " + benchmark_galewsky_hamp: " << benchmark_galewsky_hamp << std::endl;
		std::cout << i_prefix << " + benchmark_galewsky_phi2: " << benchmark_galewsky_phi2 << std::endl;
		std::cout << i_prefix << " + benchmark_galewsky_geostrophic_setup: " << benchmark_galewsky_geostrophic_setup << std::endl;
		std::cout << i_prefix << std::endl;
	}
};


#endif


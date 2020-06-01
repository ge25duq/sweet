/*
 * Author: Martin Schreiber <SchreiberX@gmail.com>
 */

#ifndef SRC_PROGRAMS_ADV_SPHERE_REXI_ADV_SPHERE_TS_NA_ERK_HPP_
#define SRC_PROGRAMS_ADV_SPHERE_REXI_ADV_SPHERE_TS_NA_ERK_HPP_

#include "../advection_sphere_benchmarks/BenchmarksSphereAdvection.hpp"
#include <sweet/sphere/SphereData_Spectral.hpp>
#include <sweet/sphere/SphereOperators_SphereData.hpp>
#include <sweet/sphere/SphereTimestepping_ExplicitRK.hpp>
#include <limits>
#include <sweet/SimulationVariables.hpp>

#include "../advection_sphere_timeintegrators/SphereAdvection_TS_interface.hpp"



class SphereAdvection_TS_na_erk	: public SphereAdvection_TS_interface
{
	SimulationVariables &simVars;
	SphereOperators_SphereData &op;

	int timestepping_order;

	const BenchmarksSphereAdvection *sphereBenchmarks;

	SphereTimestepping_ExplicitRK timestepping_rk;

public:
	bool implements_timestepping_method(const std::string &i_timestepping_method);

	std::string string_id();

	void setup_auto();

	std::string get_help();


private:
	void euler_timestep_update(
			const SphereData_Spectral &i_phi,	///< prognostic variables
			const SphereData_Spectral &i_vrt,	///< prognostic variables
			const SphereData_Spectral &i_div,	///< prognostic variables

			SphereData_Spectral &o_phi_t,	///< time updates
			SphereData_Spectral &o_vort_t,	///< time updates
			SphereData_Spectral &o_div_t,	///< time updates

			double i_simulation_timestamp = -1
	);

public:
	SphereAdvection_TS_na_erk(
			SimulationVariables &i_simVars,
			SphereOperators_SphereData &i_op
		);

	void setup(
			int i_order	///< order of RK time stepping method
	);

	void run_timestep(
			SphereData_Spectral &io_phi,	///< prognostic variables
			SphereData_Spectral &io_vrt,	///< prognostic variables
			SphereData_Spectral &io_div,	///< prognostic variables

			double i_fixed_dt,				///< if this value is not equal to 0, use this time step size instead of computing one
			double i_simulation_timestamp,

			// for varying velocity fields
			const BenchmarksSphereAdvection *i_sphereBenchmarks
	);



	virtual ~SphereAdvection_TS_na_erk();
};

#endif /* SRC_PROGRAMS_ADV_PLANE_REXI_ADV_PLANE_TS_LN_ERK_HPP_ */
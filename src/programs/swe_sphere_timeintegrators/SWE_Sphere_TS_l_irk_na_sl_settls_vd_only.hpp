/*
 * SWE_Sphere_TS_l_irk_na_sl_settls_vd_only.hpp
 *
 *  Created on: 01 Apr 2020
 *      Author: Martin Schreiber <SchreiberX@gmail.com>
 *
 *  Based on plane code
 */

#ifndef SRC_PROGRAMS_SWE_SPHERE_TS_L_IRK_NA_SL_SETTLS_VD_ONLY_HPP_
#define SRC_PROGRAMS_SWE_SPHERE_TS_L_IRK_NA_SL_SETTLS_VD_ONLY_HPP_


#include <sweet/SimulationVariables.hpp>
#include <sweet/sphere/SphereData_Physical.hpp>
#include <sweet/sphere/SphereData_Spectral.hpp>
#include <sweet/sphere/SphereOperators_SphereData.hpp>
#include <sweet/sphere/SphereOperators_Sampler_SphereDataPhysical.hpp>
#include <sweet/sphere/SphereTimestepping_SemiLagrangian.hpp>
#include <sweet/sphere/SphereTimestepping_ExplicitRK.hpp>

#include "../swe_sphere_timeintegrators/SWE_Sphere_TS_interface.hpp"
#include "../swe_sphere_timeintegrators/SWE_Sphere_TS_l_irk.hpp"
#include "../swe_sphere_timeintegrators/SWE_Sphere_TS_ln_erk_split_vd.hpp"



class SWE_Sphere_TS_l_irk_na_sl_settls_vd_only	: public SWE_Sphere_TS_interface
{
public:
	bool implements_timestepping_method(const std::string &i_timestepping_method);
	std::string string_id();
	void setup_auto();

private:
	SimulationVariables &simVars;
	SphereOperators_SphereData &op;
	SphereTimestepping_SemiLagrangian semiLagrangian;
	SphereOperators_Sampler_SphereDataPhysical &sphereSampler;

	int timestepping_order;

	SphereData_Spectral U_phi_prev, U_vrt_prev, U_div_prev;

	SWE_Sphere_TS_ln_erk_split_vd* swe_sphere_ts_ln_erk_split_vd__l_erk_1st_order = nullptr;
	SWE_Sphere_TS_l_irk* swe_sphere_ts_l_irk = nullptr;



public:
	SWE_Sphere_TS_l_irk_na_sl_settls_vd_only(
			SimulationVariables &i_simVars,
			SphereOperators_SphereData &i_op,
			bool i_setup_auto = false
		);


	void setup(
			int i_timestepping_order
		);

	void interpolate_departure_point(
			const SphereData_Spectral &i_phi,
			const SphereData_Spectral &i_vrt,
			const SphereData_Spectral &i_div,

			const ScalarDataArray &i_pos_lon_d,
			const ScalarDataArray &i_pos_lat_d,

			SphereData_Spectral &o_phi,
			SphereData_Spectral &o_vrt,
			SphereData_Spectral &o_div
		);

	void run_timestep(
			SphereData_Spectral &io_phi,	///< prognostic variables
			SphereData_Spectral &io_vort,	///< prognostic variables
			SphereData_Spectral &io_div,	///< prognostic variables

			double i_dt = 0,		///< if this value is not equal to 0, use this time step size instead of computing one
			double i_simulation_timestamp = -1
		);

	void run_timestep_2nd_order(
			SphereData_Spectral &io_phi_pert,	///< prognostic variables
			SphereData_Spectral &io_vort,		///< prognostic variables
			SphereData_Spectral &io_div,		///< prognostic variables

			double i_dt = 0,		///< if this value is not equal to 0, use this time step size instead of computing one
			double i_simulation_timestamp = -1
		);

	virtual ~SWE_Sphere_TS_l_irk_na_sl_settls_vd_only();
};

#endif /* SRC_PROGRAMS_SWE_SPHERE_REXI_SWE_SPHERE_TS_L_CN_NA_SL_ND_SETTLS_HPP_ */
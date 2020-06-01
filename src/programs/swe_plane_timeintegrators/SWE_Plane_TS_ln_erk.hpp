/*
 * SWE_Plane_TS_ln_erk.hpp
 *
 *  Created on: 29 May 2017
 *      Author: Martin Schreiber <SchreiberX@gmail.com>
 */

#ifndef SRC_PROGRAMS_SWE_PLANE_TIMEINTEGRATORS_SWE_PLANE_TS_LN_ERK_HPP_
#define SRC_PROGRAMS_SWE_PLANE_TIMEINTEGRATORS_SWE_PLANE_TS_LN_ERK_HPP_

#include <limits>
#include <sweet/plane/PlaneData.hpp>
#include <sweet/plane/PlaneDataTimesteppingExplicitRK.hpp>
#include <sweet/SimulationVariables.hpp>
#include <sweet/plane/PlaneOperators.hpp>

#include "SWE_Plane_TS_interface.hpp"



class SWE_Plane_TS_ln_erk	: public SWE_Plane_TS_interface
{
	SimulationVariables &simVars;
	PlaneOperators &op;

	int timestepping_order;
	PlaneDataTimesteppingExplicitRK timestepping_rk;

	bool use_only_linear_divergence;

private:
	void euler_timestep_update(
			const PlaneData &i_h,	///< prognostic variables
			const PlaneData &i_u,	///< prognostic variables
			const PlaneData &i_v,	///< prognostic variables

			PlaneData &o_h_t,	///< time updates
			PlaneData &o_u_t,	///< time updates
			PlaneData &o_v_t,	///< time updates

			double i_simulation_timestamp = -1
	);

public:
	SWE_Plane_TS_ln_erk(
			SimulationVariables &i_simVars,
			PlaneOperators &i_op
		);

	void setup(
			int i_order,	///< order of RK time stepping method
			bool use_only_linear_divergence
	);

	void run_timestep(
			PlaneData &io_h,	///< prognostic variables
			PlaneData &io_u,	///< prognostic variables
			PlaneData &io_v,	///< prognostic variables

			double i_dt = 0,		///< if this value is not equal to 0, use this time step size instead of computing one
			double i_simulation_timestamp = -1
	);



	virtual ~SWE_Plane_TS_ln_erk();
};

#endif /* SRC_PROGRAMS_SWE_PLANE_TIMEINTEGRATORS_SWE_PLANE_TS_LN_ERK_HPP_ */

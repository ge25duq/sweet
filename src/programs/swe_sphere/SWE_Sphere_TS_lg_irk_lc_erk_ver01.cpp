/*
 * SWE_Sphere_TS_lg_irk_lf_n_erk.cpp
 *
 *  Created on: 11 Nov 2017
 *      Author: Martin Schreiber <SchreiberX@gmail.com>
 */

#include "SWE_Sphere_TS_lg_irk_lc_erk_ver01.hpp"



void SWE_Sphere_TS_lg_irk_lc_erk::run_timestep_pert(
		SphereData_Spectral &io_phi_pert,	///< prognostic variables
		SphereData_Spectral &io_vrt,	///< prognostic variables
		SphereData_Spectral &io_div,	///< prognostic variables

		double i_fixed_dt,			///< if this value is not equal to 0, use this time step size instead of computing one
		double i_simulation_timestamp
)
{
	double gh0 = simVars.sim.gravitation*simVars.sim.h0;
	io_phi_pert += gh0;
	run_timestep_nonpert(io_phi_pert, io_vrt, io_div, i_fixed_dt, i_simulation_timestamp);
	io_phi_pert -= gh0;
}



void SWE_Sphere_TS_lg_irk_lc_erk::run_timestep_nonpert(
		SphereData_Spectral &io_phi,		///< prognostic variables
		SphereData_Spectral &io_vort,	///< prognostic variables
		SphereData_Spectral &io_div,		///< prognostic variables

		double i_dt,		///< if this value is not equal to 0, use this time step size instead of computing one
		double i_simulation_timestamp
)
{
	if (timestepping_order == 1)
	{
		if (version_id == 0)
		{
			// first order IRK for linear
			timestepping_lg_irk.run_timestep_nonpert(
					io_phi, io_vort, io_div,
					i_dt,
					i_simulation_timestamp
				);


			// first order explicit for non-linear
			timestepping_lg_erk_lc_erk.euler_timestep_update_lc(
					io_phi, io_vort, io_div,
					i_dt,
					i_simulation_timestamp
				);
		}
		else if (version_id == 0)
		{
			// first order explicit for non-linear
			timestepping_lg_erk_lc_erk.euler_timestep_update_lc(
					io_phi, io_vort, io_div,
					i_dt,
					i_simulation_timestamp
				);

			// first order IRK for linear
			timestepping_lg_irk.run_timestep_nonpert(
					io_phi, io_vort, io_div,
					i_dt,
					i_simulation_timestamp
				);
		}
	}
	else if (timestepping_order == 2)
	{
		if (version_id == 0)
		{
			// HALF time step for linear part
			timestepping_lg_cn.run_timestep_nonpert(
					io_phi, io_vort, io_div,
					i_dt*0.5,
					i_simulation_timestamp
				);

			// FULL time step for non-linear part
			timestepping_rk_nonlinear.run_timestep(
					&timestepping_lg_erk_lc_erk,
					&SWE_Sphere_TS_lg_erk_lc_erk::euler_timestep_update_lc,	///< pointer to function to compute euler time step updates
					io_phi, io_vort, io_div,
					i_dt,
					timestepping_order,		/// This must be 2nd order accurate to get overall 2nd order accurate method
					i_simulation_timestamp
				);

			// HALF time step for linear part
			timestepping_lg_cn.run_timestep_nonpert(
					io_phi, io_vort, io_div,
					i_dt*0.5,
					i_simulation_timestamp+i_dt*0.5	/* TODO: CHECK THIS, THIS MIGHT BE WRONG!!! */
				);
		}
		else if (version_id == 1)
		{
			// HALF time step for non-linear part
			timestepping_rk_nonlinear.run_timestep(
					&timestepping_lg_erk_lc_erk,
					&SWE_Sphere_TS_lg_erk_lc_erk::euler_timestep_update_lc,	///< pointer to function to compute euler time step updates
					io_phi, io_vort, io_div,
					i_dt*0.5,
					timestepping_order,		/// This must be 2nd order accurate to get overall 2nd order accurate method
					i_simulation_timestamp
				);

			// FULL time step for linear part
			timestepping_lg_cn.run_timestep_nonpert(
					io_phi, io_vort, io_div,
					i_dt,
					i_simulation_timestamp
				);

			// HALF time step for non-linear part
			timestepping_rk_nonlinear.run_timestep(
					&timestepping_lg_erk_lc_erk,
					&SWE_Sphere_TS_lg_erk_lc_erk::euler_timestep_update_lc,	///< pointer to function to compute euler time step updates
					io_phi, io_vort, io_div,
					i_dt*0.5,
					timestepping_order,		/// This must be 2nd order accurate to get overall 2nd order accurate method
					i_simulation_timestamp
				);
		}
		else
		{
			FatalError("Invalid verison id");
		}
	}
	else
	{
		FatalError("Not yet supported!");
	}
}



/*
 * Setup
 */
void SWE_Sphere_TS_lg_irk_lc_erk::setup(
		int i_order,	///< order of RK time stepping method
		int i_version_id
)
{
	version_id = i_version_id;

	timestepping_order = i_order;
	timestep_size = simVars.timecontrol.current_timestep_size;

	if (timestepping_order == 1)
	{
		timestepping_lg_irk.setup(
				1,
				timestep_size*0.5
		);
	}
	else if (timestepping_order == 2)
	{
		if (version_id == 0)
		{
			timestepping_lg_cn.setup(
					simVars.disc.timestepping_crank_nicolson_filter,
					timestep_size*0.5
			);
		}
		else if (version_id == 1)
		{
			timestepping_lg_cn.setup(
					simVars.disc.timestepping_crank_nicolson_filter,
					timestep_size
			);
		}
		else
		{
			FatalError("Invalid version id");
		}
	}
	else
	{
		FatalError("Invalid timestepping order");
	}


	//
	// Only request 1st order time stepping methods for irk and erk
	// These 1st order methods will be combined to higher-order methods in this class
	//
	timestepping_lg_erk_lc_erk.setup(1);
}



SWE_Sphere_TS_lg_irk_lc_erk::SWE_Sphere_TS_lg_irk_lc_erk(
		SimulationVariables &i_simVars,
		SphereOperators_SphereData &i_op
)	:
		simVars(i_simVars),
		op(i_op),
		timestepping_order(-1),
		timestepping_lg_irk(simVars, op),
		timestepping_lg_cn(simVars, op),
		timestepping_lg_erk_lc_erk(simVars, op)
{
}



SWE_Sphere_TS_lg_irk_lc_erk::~SWE_Sphere_TS_lg_irk_lc_erk()
{
}


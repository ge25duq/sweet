/*
 * SWE_Sphere_TS_lg_irk.hpp
 *
 *  Created on: 30 Aug 2016
 *      Author: Martin Schreiber <SchreiberX@gmail.com>
 */


#include "SWE_Sphere_TS_lg_irk.hpp"
#include <complex>
#include <sweet/sphere/app_swe/SWESphBandedMatrixPhysicalReal.hpp>
#include <sweet/sphere/SphereData_Config.hpp>
#include <sweet/sphere/SphereOperators_SphereData.hpp>



bool SWE_Sphere_TS_lg_irk::implements_timestepping_method(const std::string &i_timestepping_method)
{
	if (i_timestepping_method == "lg_irk")
		return true;

	return false;
}



std::string SWE_Sphere_TS_lg_irk::string_id()
{
	return "lg_irk";
}



void SWE_Sphere_TS_lg_irk::setup_auto()
{
	if (simVars.sim.sphere_use_fsphere)
		SWEETError("TODO: Not yet supported");

	setup(
			simVars.disc.timestepping_order,
			simVars.timecontrol.current_timestep_size,
			0,
			simVars.disc.timestepping_crank_nicolson_filter
		);
}



void SWE_Sphere_TS_lg_irk::run_timestep_pert(
		SphereData_Spectral &io_phi_pert,
		SphereData_Spectral &io_vrt,
		SphereData_Spectral &io_div,

		double i_fixed_dt,					///< if this value is not equal to 0, use this time step size instead of computing one
		double i_simulation_timestamp
)
{
	if (timestepping_order == 2)
		lg_erk->run_timestep_pert(io_phi_pert, io_vrt, io_div, i_fixed_dt*0.5, i_simulation_timestamp);

	double gh0 = simVars.sim.gravitation*simVars.sim.h0;
	io_phi_pert += gh0;
	run_timestep_nonpert_private(io_phi_pert, io_vrt, io_div, i_fixed_dt, i_simulation_timestamp);
	io_phi_pert -= gh0;
}



/**
 * Solve a REXI time step for the given initial conditions
 */

void SWE_Sphere_TS_lg_irk::run_timestep_nonpert_private(
		SphereData_Spectral &io_phi,		///< prognostic variables
		SphereData_Spectral &io_vort,	///< prognostic variables
		SphereData_Spectral &io_div,		///< prognostic variables

		double i_fixed_dt,			///< if this value is not equal to 0, use this time step size instead of computing one
		double i_simulation_timestamp
)
{
	if (i_fixed_dt <= 0)
		SWEETError("Only constant time step size allowed");

	if (std::abs(timestep_size - i_fixed_dt)/std::max(timestep_size, i_fixed_dt) > 1e-10)
	{
		std::cout << "Warning: Reducing time step size from " << i_fixed_dt << " to " << timestep_size << std::endl;

		timestep_size = i_fixed_dt;

		update_coefficients();
	}

	SphereData_Spectral phi0 = io_phi;
	SphereData_Spectral vort0 = io_vort;
	SphereData_Spectral div0 = io_div;

	SphereData_Spectral phi(sphereDataConfig);
	SphereData_Spectral vort(sphereDataConfig);
	SphereData_Spectral div(sphereDataConfig);

	{
		SphereData_Spectral rhs = gh*div0 + alpha*phi0;
		phi = rhs.spectral_solve_helmholtz(alpha*alpha, -gh, r);
		io_phi = phi*beta;

		rhs = alpha*div0 + op.laplace(phi0);
		div = rhs.spectral_solve_helmholtz(alpha*alpha, -gh, r);
		io_div = div*beta;

		io_vort = vort0;
	}
}



SWE_Sphere_TS_lg_irk::SWE_Sphere_TS_lg_irk(
		SimulationVariables &i_simVars,
		SphereOperators_SphereData &i_op
)	:
	simVars(i_simVars),
	op(i_op),
	sphereDataConfig(op.sphereDataConfig)
{
}


void SWE_Sphere_TS_lg_irk::update_coefficients()
{
	alpha = -1.0/timestep_size;
	beta = -1.0/timestep_size;

	{
		/*
		 * Crank-Nicolson method:
		 *
		 * (U(t+1) - q dt F(U(t+1))) = (U(t) + q dt F(U(t)))
		 *
		 * with q the CN damping facor with no damping for q=0.5
		 */

		// scale dt by the damping factor to reuse solver structure

		alpha /= crank_nicolson_damping_factor;
		beta /= crank_nicolson_damping_factor;
	}
}



/**
 * Setup the SWE REXI solver with SPH
 */
void SWE_Sphere_TS_lg_irk::setup(
		int i_timestep_order,
		double i_timestep_size,
		int i_extended_modes,
		double i_crank_nicolson_damping_factor
)
{
#if SWEET_DEBUG
	if (i_extended_modes != 0)
		SWEETError("Not supported");
#endif
	timestepping_order = i_timestep_order;
	timestep_size = i_timestep_size;

	if (i_timestep_order == 1)
	{
		// set this to 1 to ignore it
		crank_nicolson_damping_factor = 1.0;
	}
	else if (i_timestep_order == 2)
	{
		crank_nicolson_damping_factor = i_crank_nicolson_damping_factor;
		lg_erk = new SWE_Sphere_TS_lg_erk(simVars, op);
		lg_erk->setup(1);
	}
	else
	{
		SWEETError("Only 1st and 2nd order IRK supported so far with this implementation! Use l_cn if you want to have 2nd order Crank-Nicolson!");
	}


	update_coefficients();

	r = simVars.sim.sphere_radius;
	inv_r = 1.0/r;

	gh = simVars.sim.gravitation*simVars.sim.h0;


}



SWE_Sphere_TS_lg_irk::~SWE_Sphere_TS_lg_irk()
{
	delete lg_erk;
	lg_erk = nullptr;
}

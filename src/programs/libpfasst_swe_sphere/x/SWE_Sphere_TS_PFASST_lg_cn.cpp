/*
 * SWE_Sphere_TS_PFASST_lg_cn.hpp
 *
 *  Created on: 30 Aug 2016
 *      Author: Martin Schreiber <SchreiberX@gmail.com>
 */


#include "SWE_Sphere_TS_PFASST_lg_cn.hpp"
#include <complex>
#include <sweet/sphere/SphereData_Config.hpp>
#include <sweet/sphere/SphereOperators_SphereData.hpp>
#include "../../swe_sphere_timeintegrators/helpers/SWESphBandedMatrixPhysicalReal.hpp"


SWE_Sphere_TS_PFASST_lg_cn::SWE_Sphere_TS_PFASST_lg_cn(
		SimulationVariables &i_simVars,
		SphereOperators_SphereData &i_op
)	:
	simVars(i_simVars),
	op(i_op),
	sphereDataConfig(op.sphereDataConfig)
{
}


/**
 * Setup the SWE REXI solver with SPH
 */
void SWE_Sphere_TS_PFASST_lg_cn::setup(
		double i_crank_nicolson_damping_factor,
		double i_timestep_size
)
{
	crank_nicolson_damping_factor = i_crank_nicolson_damping_factor;

	timestep_size = i_timestep_size;

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

	r = simVars.sim.sphere_radius;
	inv_r = 1.0/r;

	gh = simVars.sim.gravitation*simVars.sim.h0;
}



/**
 * Solve a REXI time step for the given initial conditions
 */

void SWE_Sphere_TS_PFASST_lg_cn::run_timestep_nonpert(
		SphereData_Spectral &io_phi,		///< prognostic variables
		SphereData_Spectral &io_vort,	///< prognostic variables
		SphereData_Spectral &io_div,		///< prognostic variables

		double i_fixed_dt,
		double i_simulation_timestamp
)
{
	if (i_fixed_dt <= 0)
		SWEETError("Only constant time step size allowed");

	SphereData_Spectral phi0 = io_phi;
	SphereData_Spectral vort0 = io_vort;
	SphereData_Spectral div0 = io_div;

	/*
	 * Crank-Nicolson method:
	 *
	 * (U(t+1) - q dt F(U(t+1))) = (U(t) + q dt F(U(t)))
	 *
	 * with q the CN damping facor with no damping for q=0.5
	 */

	SphereData_Spectral o_phi_t(sphereDataConfig);
	SphereData_Spectral o_vort_t(sphereDataConfig);
	SphereData_Spectral o_div_t(sphereDataConfig);

	/*
	 * LINEAR
	 */
	{
		o_phi_t = -gh*div0;
		o_div_t = -op.laplace(phi0);
		o_vort_t.spectral_set_zero();
	}

	double fac = timestep_size*(1.0-crank_nicolson_damping_factor);
	// run single time step for rhs

	phi0 += fac*o_phi_t;
	vort0 += fac*o_vort_t;
	div0 += fac*o_div_t;


	SphereData_Spectral phi(sphereDataConfig);
	SphereData_Spectral vort(sphereDataConfig);
	SphereData_Spectral div(sphereDataConfig);

	{
		SphereData_Spectral rhs = gh*div0 + alpha*phi0;
		phi = rhs.spectral_solve_helmholtz(alpha*alpha, -gh, r);

		vort = 1.0/alpha*(vort0);
		div = -1.0/gh*(phi0 - alpha*phi);
	}

	io_phi = (phi * beta);
	io_vort = (vort * beta);
	io_div = (div * beta);
}


SWE_Sphere_TS_PFASST_lg_cn::~SWE_Sphere_TS_PFASST_lg_cn()
{
}

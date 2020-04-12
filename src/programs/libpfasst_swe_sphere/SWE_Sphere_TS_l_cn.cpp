/*
 * SWE_Sphere_TS_l_cn.hpp
 *
 *  Created on: 30 Aug 2016
 *      Author: Martin Schreiber <SchreiberX@gmail.com>
 */


#include "SWE_Sphere_TS_l_cn.hpp"
#include <complex>
#include <sweet/sphere/app_swe/SWESphBandedMatrixPhysicalReal.hpp>
#include <sweet/sphere/SphereData_Config.hpp>
#include <sweet/sphere/SphereOperators_SphereData.hpp>



SWE_Sphere_TS_l_cn::SWE_Sphere_TS_l_cn(
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
void SWE_Sphere_TS_l_cn::setup(
		double i_crank_nicolson_damping_factor,	// = 0.5,
		double i_timestep_size,
		int i_use_extended_modes
)
{
	crank_nicolson_damping_factor = i_crank_nicolson_damping_factor;

	use_extended_modes = i_use_extended_modes;

	if (use_extended_modes == 0)
	{
		sphereDataConfigSolver = sphereDataConfig;
	}
	else
	{
		// Add modes only along latitude since these are the "problematic" modes
		sphereDataConfigSolverAddedModes.setupAdditionalModes(
				sphereDataConfig,
				use_extended_modes,	// TODO: Extend SPH wrapper to also support m != n to set this guy to 0
				use_extended_modes,
				simVars.misc.reuse_spectral_transformation_plans
		);

		sphereDataConfigSolver = &sphereDataConfigSolverAddedModes;
	}

	timestep_size = i_timestep_size;
	use_f_sphere = simVars.sim.sphere_use_fsphere;

	if (use_f_sphere)
	{
		f0 = simVars.sim.plane_rotating_f0;
		two_coriolis = 0.0;
	}
	else
	{
		f0 = 0.0;
		two_coriolis = 2.0*simVars.sim.sphere_rotating_coriolis_omega;
	}

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

	update_coefficients();
}


void SWE_Sphere_TS_l_cn::update_coefficients()
{
	if (!use_f_sphere)
	{
		sphSolverPhi.setup(sphereDataConfigSolver, 4);
		sphSolverPhi.solver_component_rexi_z1(	(alpha*alpha)*(alpha*alpha), r);
		sphSolverPhi.solver_component_rexi_z2(	2.0*two_coriolis*two_coriolis*alpha*alpha, r);
		sphSolverPhi.solver_component_rexi_z3(	(two_coriolis*two_coriolis)*(two_coriolis*two_coriolis), r);
		sphSolverPhi.solver_component_rexi_z4robert(	-gh*alpha*two_coriolis, r);
		sphSolverPhi.solver_component_rexi_z5robert(	gh/alpha*two_coriolis*two_coriolis*two_coriolis, r);
		sphSolverPhi.solver_component_rexi_z6robert(	gh*2.0*two_coriolis*two_coriolis, r);
		sphSolverPhi.solver_component_rexi_z7(	-gh*alpha*alpha, r);
		sphSolverPhi.solver_component_rexi_z8(	-gh*two_coriolis*two_coriolis, r);

		fg.setup(sphereDataConfig);
		fg.physical_update_lambda_gaussian_grid(
			[&](double lon, double mu, double &o_data)
			{
				o_data = mu*two_coriolis;
			}
		);

		mug.setup(sphereDataConfig);
		mug.physical_update_lambda_gaussian_grid(
			[&](double lon, double mu, double &o_data)
			{
				o_data = mu;
			}
		);
	}
}


/**
 * Solve a REXI time step for the given initial conditions
 */

void SWE_Sphere_TS_l_cn::run_timestep_nonpert(
		SphereData_Spectral &io_phi,		///< prognostic variables
		SphereData_Spectral &io_vort,	///< prognostic variables
		SphereData_Spectral &io_div,		///< prognostic variables

		double i_fixed_dt,			///< if this value is not equal to 0, use this time step size instead of computing one
		double i_simulation_timestamp
)
{
	if (i_fixed_dt <= 0)
		FatalError("SWE_Sphere_TS_l_cn: Only constant time step size allowed");


	if (std::abs(timestep_size - i_fixed_dt)/std::max(timestep_size, i_fixed_dt) > 1e-10)
	{
		std::cout << "Warning: Reducing time step size from " << i_fixed_dt << " to " << timestep_size << std::endl;

		timestep_size = i_fixed_dt;

		update_coefficients();
	}


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
	if (use_f_sphere)
	{
		o_phi_t = -gh*div0;
		o_div_t = -op.laplace(phi0) + f0*vort0;
		o_vort_t = -f0*div0;
	}
	else
	{
		SphereData_Physical ug(sphereDataConfig);
		SphereData_Physical vg(sphereDataConfig);

		op.robert_vortdiv_to_uv(vort0, div0, ug, vg);
		SphereData_Physical phig = phi0.getSphereDataPhysical();

		SphereData_Physical tmpg1 = ug*fg;
		SphereData_Physical tmpg2 = vg*fg;

		op.robert_uv_to_vortdiv(tmpg1, tmpg2, o_div_t, o_vort_t);

		o_vort_t *= -1.0;

		tmpg1 = ug*gh;
		tmpg2 = vg*gh;

		SphereData_Spectral tmpspec(sphereDataConfig);
		op.robert_uv_to_vortdiv(tmpg1,tmpg2, tmpspec, o_phi_t);

		o_phi_t *= -1.0;

		tmpspec.loadSphereDataPhysical(phig);
		o_div_t += -op.laplace(tmpspec);
	}

	double fac = timestep_size*(1.0-crank_nicolson_damping_factor);
	// run single time step for rhs

	phi0 += fac*o_phi_t;
	vort0 += fac*o_vort_t;
	div0 += fac*o_div_t;


	SphereData_Spectral phi(sphereDataConfig);
	SphereData_Spectral vort(sphereDataConfig);
	SphereData_Spectral div(sphereDataConfig);

	if (use_f_sphere)
	{
		SphereData_Spectral rhs = gh*(div0 - f0/alpha*vort0) + (alpha+f0*f0/alpha)*phi0;
		phi = rhs.spectral_solve_helmholtz(alpha*alpha + f0*f0, -gh, r);

		div = -1.0/gh*(phi0 - alpha*phi);
		vort = (1.0/alpha)*(vort0 + f0*(div));
	}
	else
	{
		SphereData_Spectral rhs(sphereDataConfig);

		SphereData_Physical u0g(sphereDataConfig);
		SphereData_Physical v0g(sphereDataConfig);
		op.robert_vortdiv_to_uv(vort0, div0, u0g, v0g);

		SphereData_Physical phi0g = phi0.getSphereDataPhysical();

		SphereData_Physical Fc_k =
				two_coriolis*inv_r*(
						-(-two_coriolis*two_coriolis*mug*mug + alpha*alpha)*u0g
						+ 2.0*alpha*two_coriolis*mug*v0g
				);

		SphereData_Physical foo =
				(gh*(div0.getSphereDataPhysical() - (1.0/alpha)*two_coriolis*mug*vort0.getSphereDataPhysical())) +
				(alpha*phi0g + (1.0/alpha)*two_coriolis*two_coriolis*mug*mug*phi0g);

		SphereData_Physical rhsg =
				alpha*alpha*foo +
				two_coriolis*two_coriolis*mug*mug*foo
				- (gh/alpha)*Fc_k;

		rhs = rhsg;

		phi = sphSolverPhi.solve(rhs.spectral_returnWithDifferentModes(sphereDataConfigSolver)).spectral_returnWithDifferentModes(sphereDataConfig);


		SphereData_Physical u0(sphereDataConfig);
		SphereData_Physical v0(sphereDataConfig);
		op.robert_vortdiv_to_uv(vort0, div0, u0, v0);

		SphereData_Physical gradu(sphereDataConfig);
		SphereData_Physical gradv(sphereDataConfig);
		op.robert_grad_to_vec(phi, gradu, gradv, r);

		SphereData_Physical a = u0 + gradu;
		SphereData_Physical b = v0 + gradv;

		SphereData_Physical k = (two_coriolis*two_coriolis*mug*mug+alpha*alpha);
		SphereData_Physical u = (alpha*a - two_coriolis*mug*(b))/k;
		SphereData_Physical v = (two_coriolis*mug*(a) + alpha*b)/k;

		op.robert_uv_to_vortdiv(u, v, vort, div);
	}

	io_phi = (phi * beta);
	io_vort = (vort * beta);
	io_div = (div * beta);
}


SWE_Sphere_TS_l_cn::~SWE_Sphere_TS_l_cn()
{
}

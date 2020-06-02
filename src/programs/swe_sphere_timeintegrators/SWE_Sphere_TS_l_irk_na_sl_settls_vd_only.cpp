/*
 * Author: Martin Schreiber <SchreiberX@gmail.com>
 */

#include "../swe_sphere_timeintegrators/SWE_Sphere_TS_l_irk_na_sl_settls_vd_only.hpp"



bool SWE_Sphere_TS_l_irk_na_sl_settls_vd_only::implements_timestepping_method(const std::string &i_timestepping_method)
{
	if (i_timestepping_method == "l_irk_na_sl_settls_vd_only")
		return true;

	return false;
}



std::string SWE_Sphere_TS_l_irk_na_sl_settls_vd_only::string_id()
{
	return "l_irk_na_sl_settls_vd_only";
}



void SWE_Sphere_TS_l_irk_na_sl_settls_vd_only::setup_auto()
{
	setup(
		simVars.disc.timestepping_order
	);
}



void SWE_Sphere_TS_l_irk_na_sl_settls_vd_only::run_timestep(
		SphereData_Spectral &io_phi_pert,	///< prognostic variables
		SphereData_Spectral &io_vrt,	///< prognostic variables
		SphereData_Spectral &io_div,	///< prognostic variables

		double i_fixed_dt,			///< if this value is not equal to 0, use this time step size instead of computing one
		double i_simulation_timestamp
)
{
	if (timestepping_order == 1)
	{
		SWEETError("TODO run_timestep_1st_order_pert");
	}
	else if (timestepping_order == 2)
	{
		run_timestep_2nd_order(io_phi_pert, io_vrt, io_div, i_fixed_dt, i_simulation_timestamp);
	}
	else
	{
		SWEETError("Only orders 1/2 supported (ERRID 098nd89eje)");
	}
}



void SWE_Sphere_TS_l_irk_na_sl_settls_vd_only::interpolate_departure_point(
		const SphereData_Spectral &i_phi,
		const SphereData_Spectral &i_vrt,
		const SphereData_Spectral &i_div,

		const ScalarDataArray &i_pos_lon_d,
		const ScalarDataArray &i_pos_lat_d,

		SphereData_Spectral &o_phi,
		SphereData_Spectral &o_vrt,
		SphereData_Spectral &o_div
)
{
	o_phi.setup_if_required(i_phi.sphereDataConfig);
	o_vrt.setup_if_required(i_phi.sphereDataConfig);
	o_div.setup_if_required(i_phi.sphereDataConfig);

	o_phi = sphereSampler.bicubic_scalar_ret_phys(
			i_phi.toPhys(),
			i_pos_lon_d, i_pos_lat_d,
			false,
			simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points,
			simVars.disc.semi_lagrangian_interpolation_limiter
		);

	o_vrt = sphereSampler.bicubic_scalar_ret_phys(
			i_vrt.toPhys(),
			i_pos_lon_d, i_pos_lat_d,
			false,
			simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points,
			simVars.disc.semi_lagrangian_interpolation_limiter
		);

	o_div = sphereSampler.bicubic_scalar_ret_phys(
			i_div.toPhys(),
			i_pos_lon_d, i_pos_lat_d,
			false,
			simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points,
			simVars.disc.semi_lagrangian_interpolation_limiter
		);
}



void SWE_Sphere_TS_l_irk_na_sl_settls_vd_only::run_timestep_2nd_order(
	SphereData_Spectral &io_U_phi,		///< prognostic variables
	SphereData_Spectral &io_U_vrt,		///< prognostic variables
	SphereData_Spectral &io_U_div,		///< prognostic variables

	double i_dt,					///< if this value is not equal to 0, use this time step size instead of computing one
	double i_simulation_timestamp
)
{
	const SphereData_Spectral &U_phi = io_U_phi;
	const SphereData_Spectral &U_vrt = io_U_vrt;
	const SphereData_Spectral &U_div = io_U_div;

	if (i_simulation_timestamp == 0)
	{
		/*
		 * First time step:
		 * Simply backup existing fields for multi-step parts of this algorithm.
		 */
		U_phi_prev = U_phi;
		U_vrt_prev = U_vrt;
		U_div_prev = U_div;
	}


	/*
	 * Step 1) SL
	 * Compute Lagrangian trajectories based on SETTLS.
	 * This depends on V(t-\Delta t) and V(t).
	 *
	 * See Hortal's paper for equation.
	 */
	SphereData_Physical U_u_lon_prev, U_v_lat_prev;
	op.vrtdiv_to_uv(U_vrt_prev, U_div_prev, U_u_lon_prev, U_v_lat_prev);

	SphereData_Physical U_u_lon, U_v_lat;
	op.vrtdiv_to_uv(U_vrt, U_div, U_u_lon, U_v_lat);

	double dt_div_radius = i_dt / simVars.sim.sphere_radius;

	// Calculate departure points
	ScalarDataArray pos_lon_d, pos_lat_d;
	semiLagrangian.semi_lag_departure_points_settls_specialized(
			dt_div_radius*U_u_lon_prev, dt_div_radius*U_v_lat_prev,
			dt_div_radius*U_u_lon, dt_div_radius*U_v_lat,
			pos_lon_d, pos_lat_d		// OUTPUT
	);


	/*
	 * Step 2) Midpoint rule
	 * Put everything together with midpoint rule and solve resulting Helmholtz problem
	 */

	/*
	 * Step 2a) Compute RHS
	 * R = X_D + 1/2 dt L_D + dt N*
	 */

	/*
	 * Compute X_D
	 */
	SphereData_Spectral U_phi_D, U_vrt_D, U_div_D;
	interpolate_departure_point(
			U_phi, U_vrt, U_div,
			pos_lon_d, pos_lat_d,
			U_phi_D, U_vrt_D, U_div_D
		);

	/*
	 * Compute L_D
	 */
	const SphereData_Config *sphereDataConfig = io_U_phi.sphereDataConfig;

	/*
	 * Method 1) First evaluate L, then sample result at departure point
	 */
	SphereData_Spectral L_U_phi(sphereDataConfig, 0), L_U_vrt(sphereDataConfig, 0), L_U_div(sphereDataConfig, 0);

	/*
	 * L_g(U): Linear gravity modes
	 */
	swe_sphere_ts_ln_erk_split_vd__l_erk_1st_order->euler_timestep_update_lg(
			U_phi, U_vrt, U_div,
			L_U_phi, L_U_vrt, L_U_div,
			i_simulation_timestamp
		);

	/*
	 * L_c(U): Linear Coriolis effect
	 */
	swe_sphere_ts_ln_erk_split_vd__l_erk_1st_order->euler_timestep_update_lc(
			U_phi, U_vrt, U_div,
			L_U_phi, L_U_vrt, L_U_div,
			i_simulation_timestamp
		);


	SphereData_Spectral L_U_phi_D, L_U_vrt_D, L_U_div_D;
	interpolate_departure_point(
			L_U_phi, L_U_vrt, L_U_div,
			pos_lon_d, pos_lat_d,
			L_U_phi_D, L_U_vrt_D, L_U_div_D
		);

	/*
	 * Compute R = X_D + 1/2 dt L_D
	 */
	SphereData_Spectral R_phi = U_phi_D + (0.5 * i_dt) * L_U_phi_D;
	SphereData_Spectral R_vrt = U_vrt_D + (0.5 * i_dt) * L_U_vrt_D;
	SphereData_Spectral R_div = U_div_D + (0.5 * i_dt) * L_U_div_D;

	/*
	 * Nonlinear remainder term starts here
	 */
#if 0
	{
		/*
		 * N*(t+0.5dt) = 1/2 ([ 2*N(t) - N(t-dt) ]_D + N(t))
		 *
		 * R += dt*N*(t+0.5dt)
		 */

		/*
		 * Compute
		 * [ 2*N(t) - N(t-dt) ]_D
		 */

		/*
		 * N(t-dt)
		 */
		SphereData_Spectral N_U_phi_prev_nr(sphereDataConfig, 0), N_U_vrt_prev_nr(sphereDataConfig, 0), N_U_div_prev_nr(sphereDataConfig, 0);

		swe_sphere_ts_ln_erk_split_vd__l_erk_1st_order->euler_timestep_update_nr(
				U_phi_prev, U_vrt_prev, U_div_prev,
				N_U_phi_prev_nr, N_U_vrt_prev_nr, N_U_div_prev_nr,
				i_simulation_timestamp-i_dt
		);

		/*
		 * N(t)
		 */
		SphereData_Spectral N_U_phi(sphereDataConfig, 0), N_U_vrt(sphereDataConfig, 0), N_U_div(sphereDataConfig, 0);

		swe_sphere_ts_ln_erk_split_vd__l_erk_1st_order->euler_timestep_update_nr(
				U_phi, U_vrt, U_div,
				N_U_phi, N_U_vrt, N_U_div,
				i_simulation_timestamp
		);

		/*
		 * N(t+dt)_D = [ 2*N(t) - N(t-dt) ]_D
		 */
		SphereData_Spectral N_U_phi_next_D, N_U_vrt_next_D, N_U_div_next_D;
		interpolate_departure_point(
				2.0 * N_U_phi - N_U_phi_prev_nr,
				2.0 * N_U_vrt - N_U_vrt_prev_nr,
				2.0 * N_U_div - N_U_div_prev_nr,

				pos_lon_d, pos_lat_d,
				N_U_phi_next_D, N_U_vrt_next_D, N_U_div_next_D
			);

		/*
		 * Compute midpoint
		 * N(t+dt/2)_m = dt * 1/2 ([ 2*N(t) - N(t-dt) ]_D + N(t)_A)
		 */
		R_phi += (i_dt * 0.5) * (N_U_phi_next_D + N_U_phi);
		R_vrt += (i_dt * 0.5) * (N_U_vrt_next_D + N_U_vrt);
		R_div += (i_dt * 0.5) * (N_U_div_next_D + N_U_div);
	}
#endif

	/*
	 * Step 2b) Solve Helmholtz problem
	 * X - 1/2 dt LX = R
	 */
	swe_sphere_ts_l_irk->run_timestep(
			R_phi, R_vrt, R_div,
			0.5 * i_dt,
			i_simulation_timestamp
		);

	/*
	 * Backup previous variables for multi-step SL method
	 */
	U_phi_prev = U_phi;
	U_vrt_prev = U_vrt;
	U_div_prev = U_div;

	/*
	 * Return new fields stored in R_*
	 */
	io_U_phi = R_phi;
	io_U_vrt = R_vrt;
	io_U_div = R_div;
}



void SWE_Sphere_TS_l_irk_na_sl_settls_vd_only::setup(
		int i_timestepping_order
)
{
	timestepping_order = i_timestepping_order;

	if (timestepping_order != 1 && timestepping_order != 2)
		SWEETError("Invalid time stepping order, must be 1 or 2");

	// Setup semi-lag
	semiLagrangian.setup(op.sphereDataConfig);

	// Initialize with 1st order
	swe_sphere_ts_ln_erk_split_vd__l_erk_1st_order = new SWE_Sphere_TS_ln_erk_split_vd(simVars, op);
	swe_sphere_ts_ln_erk_split_vd__l_erk_1st_order->setup(1, true, true, false, false, false);

	// Initialize with 1st order and half time step size
	swe_sphere_ts_l_irk = new SWE_Sphere_TS_l_irk(simVars, op);
	swe_sphere_ts_l_irk->setup(
			1,
			0.5 * simVars.timecontrol.current_timestep_size,
			simVars.disc.timestepping_crank_nicolson_filter,
			false
		);
}



SWE_Sphere_TS_l_irk_na_sl_settls_vd_only::SWE_Sphere_TS_l_irk_na_sl_settls_vd_only(
			SimulationVariables &i_simVars,
			SphereOperators_SphereData &i_op,
			bool i_setup_auto
		) :
		simVars(i_simVars),
		op(i_op),
		semiLagrangian(simVars),
		sphereSampler(semiLagrangian.sphereSampler)
{
	if (i_setup_auto)
		setup_auto();
}



SWE_Sphere_TS_l_irk_na_sl_settls_vd_only::~SWE_Sphere_TS_l_irk_na_sl_settls_vd_only()
{
	delete swe_sphere_ts_ln_erk_split_vd__l_erk_1st_order;
	delete swe_sphere_ts_l_irk;
}


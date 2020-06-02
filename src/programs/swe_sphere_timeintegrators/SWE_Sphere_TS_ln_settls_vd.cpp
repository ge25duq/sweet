/*
 * SWE_Sphere_TS_ln_settls.cpp
 *
 *  Created on: 24 Sep 2019
 *      Author: Martin Schreiber <SchreiberX@gmail.com>
 *
 *  Changelog:
 *  	2019-10-24: Partly based on plane version
 */

#include "../swe_sphere_timeintegrators/SWE_Sphere_TS_ln_settls_vd.hpp"



bool SWE_Sphere_TS_ln_settls_vd::implements_timestepping_method(const std::string &i_timestepping_method)
{
	/*
	 * Should contain _exp and _settls
	 */
	return (
		!(i_timestepping_method.find("_exp") != std::string::npos)		&&
		(i_timestepping_method.find("_settls") != std::string::npos)	&&
		(i_timestepping_method.find("_vd") != std::string::npos)		&&
		!(i_timestepping_method.find("_only") != std::string::npos)		&&
		true
	);
}


std::string SWE_Sphere_TS_ln_settls_vd::string_id()
{
	return string_id_storage;
}


void SWE_Sphere_TS_ln_settls_vd::run_timestep(
		SphereData_Spectral &io_phi,	///< prognostic variables
		SphereData_Spectral &io_vrt,	///< prognostic variables
		SphereData_Spectral &io_div,	///< prognostic variables

		double i_fixed_dt,			///< if this value is not equal to 0, use this time step size instead of computing one
		double i_simulation_timestamp
)
{
	if (timestepping_order == 2)
	{
		run_timestep_2nd_order(io_phi, io_vrt, io_div, i_fixed_dt, i_simulation_timestamp);
		return;
	}
}


void SWE_Sphere_TS_ln_settls_vd::interpolate_departure_point(
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


#if 1
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
#else
	{
		SphereData_Physical u_tmp, v_tmp;
		op.vrtdiv_to_uv(i_vrt, i_div, u_tmp, v_tmp);

		SphereData_Physical v_tmp_D = sphereSampler.bicubic_scalar_ret_phys(
				u_tmp,
				i_pos_lon_d, i_pos_lat_d,
				true,
				simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points,
				simVars.disc.semi_lagrangian_interpolation_limiter
			);

		SphereData_Physical u_tmp_D = sphereSampler.bicubic_scalar_ret_phys(
				v_tmp,
				i_pos_lon_d, i_pos_lat_d,
				true,
				simVars.disc.semi_lagrangian_sampler_use_pole_pseudo_points,
				simVars.disc.semi_lagrangian_interpolation_limiter
			);

		op.uv_to_vrtdiv(u_tmp_D, v_tmp_D, o_vrt, o_div);
	}
#endif
}


void SWE_Sphere_TS_ln_settls_vd::run_timestep_2nd_order(
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
	SphereData_Spectral L_U_phi_D, L_U_vrt_D, L_U_div_D;
	if (original_linear_operator_sl_treatment)
	{
		/*
		 * Method 1) First evaluate L, then sample result at departure point
		 */
		const SphereData_Config *sphereDataConfig = io_U_phi.sphereDataConfig;
		SphereData_Spectral L_U_phi(sphereDataConfig, 0), L_U_vrt(sphereDataConfig, 0), L_U_div(sphereDataConfig, 0);

		/*
		 * L_g(U): Linear gravity modes
		 */
		swe_sphere_ts_ln_erk_split_vd->euler_timestep_update_lg(
				U_phi, U_vrt, U_div,
				L_U_phi, L_U_vrt, L_U_div,
				i_simulation_timestamp
			);

		if (coriolis_treatment == CORIOLIS_LINEAR)
		{
			/*
			 * L_c(U): Linear Coriolis effect
			 */
			swe_sphere_ts_ln_erk_split_vd->euler_timestep_update_lc(
					U_phi, U_vrt, U_div,
					L_U_phi, L_U_vrt, L_U_div,
					i_simulation_timestamp
				);
		}

		interpolate_departure_point(
				L_U_phi, L_U_vrt, L_U_div,
				pos_lon_d, pos_lat_d,
				L_U_phi_D, L_U_vrt_D, L_U_div_D
			);
	}
	else
	{
		/*
		 * Method 2) First get variables on departure points, then evaluate L
		 */

		SphereData_Spectral U_phi_D, U_vrt_D, U_div_D;
		interpolate_departure_point(
				U_phi, U_vrt, U_div,
				pos_lon_d, pos_lat_d,
				U_phi_D, U_vrt_D, U_div_D
			);

		const SphereData_Config *sphereDataConfig = io_U_phi.sphereDataConfig;
		L_U_phi_D.setup(sphereDataConfig, 0);
		L_U_vrt_D.setup(sphereDataConfig, 0);
		L_U_div_D.setup(sphereDataConfig, 0);

		/*
		 * L_g(U): Linear gravity modes
		 */
		swe_sphere_ts_ln_erk_split_vd->euler_timestep_update_lg(
				U_phi_D, U_vrt_D, U_div_D,
				L_U_phi_D, L_U_vrt_D, L_U_div_D,
				i_simulation_timestamp
			);

		if (coriolis_treatment == CORIOLIS_LINEAR)
		{
			/*
			 * L_c(U): Linear Coriolis effect
			 */
			swe_sphere_ts_ln_erk_split_vd->euler_timestep_update_lc(
					U_phi_D, U_vrt_D, U_div_D,
					L_U_phi_D, L_U_vrt_D, L_U_div_D,
					i_simulation_timestamp
				);
		}
	}

	/*
	 * Compute R = X_D + 1/2 dt L_D
	 */
	SphereData_Spectral R_phi = U_phi_D + (0.5 * i_dt) * L_U_phi_D;
	SphereData_Spectral R_vrt = U_vrt_D + (0.5 * i_dt) * L_U_vrt_D;
	SphereData_Spectral R_div = U_div_D + (0.5 * i_dt) * L_U_div_D;

	/*
	 * Nonlinear remainder term starts here
	 */
	if (1)
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
		const SphereData_Config *sphereDataConfig = io_U_phi.sphereDataConfig;
		SphereData_Spectral N_U_phi_prev_nr(sphereDataConfig, 0), N_U_vrt_prev_nr(sphereDataConfig, 0), N_U_div_prev_nr(sphereDataConfig, 0);

		if (nonlinear_remainder_treatment == NL_REMAINDER_NONLINEAR)
		{
			swe_sphere_ts_ln_erk_split_vd->euler_timestep_update_nr(
					U_phi_prev, U_vrt_prev, U_div_prev,
					N_U_phi_prev_nr, N_U_vrt_prev_nr, N_U_div_prev_nr,
					i_simulation_timestamp-i_dt
			);
		}

		if (coriolis_treatment == CORIOLIS_NONLINEAR)
		{
			swe_sphere_ts_ln_erk_split_vd->euler_timestep_update_lc(
					U_phi_prev, U_vrt_prev, U_div_prev,
					N_U_phi_prev_nr, N_U_vrt_prev_nr, N_U_div_prev_nr,
					i_simulation_timestamp-i_dt
				);
		}

		/*
		 * N(t)
		 */
		SphereData_Spectral N_U_phi_nr(sphereDataConfig, 0);
		SphereData_Spectral N_U_vrt_nr(sphereDataConfig, 0);
		SphereData_Spectral N_U_div_nr(sphereDataConfig, 0);

		if (nonlinear_remainder_treatment == NL_REMAINDER_NONLINEAR)
		{
			swe_sphere_ts_ln_erk_split_vd->euler_timestep_update_nr(
					U_phi, U_vrt, U_div,
					N_U_phi_nr, N_U_vrt_nr, N_U_div_nr,
					i_simulation_timestamp
			);
		}

		if (coriolis_treatment == CORIOLIS_NONLINEAR)
		{
			swe_sphere_ts_ln_erk_split_vd->euler_timestep_update_lc(
					U_phi, U_vrt, U_div,
					N_U_phi_nr, N_U_vrt_nr, N_U_div_nr,
					i_simulation_timestamp-i_dt
				);
		}

		/*
		 * N(t+dt)_D = [ 2*N(t) - N(t-dt) ]_D
		 */
		SphereData_Spectral N_U_phi_next_D, N_U_vrt_next_D, N_U_div_next_D;
		interpolate_departure_point(
				2.0 * N_U_phi_nr - N_U_phi_prev_nr,
				2.0 * N_U_vrt_nr - N_U_vrt_prev_nr,
				2.0 * N_U_div_nr - N_U_div_prev_nr,

				pos_lon_d, pos_lat_d,
				N_U_phi_next_D, N_U_vrt_next_D, N_U_div_next_D
			);

		/*
		 * Compute N*(t+0.5dt) = 1/2 ([ 2*N(t) - N(t-dt) ]_D + N(t))
		 * and add to R terms
		 */
		R_phi += (i_dt * 0.5) * (N_U_phi_next_D + N_U_phi_nr);
		R_vrt += (i_dt * 0.5) * (N_U_vrt_next_D + N_U_vrt_nr);
		R_div += (i_dt * 0.5) * (N_U_div_next_D + N_U_div_nr);
	}

	/*
	 * Step 2b) Solve Helmholtz problem
	 * X - 1/2 dt LX = R
	 */

	if (coriolis_treatment == CORIOLIS_LINEAR)
	{
		swe_sphere_ts_l_irk->run_timestep(
				R_phi, R_vrt, R_div,
				0.5 * i_dt,
				i_simulation_timestamp
			);
	}
	else
	{
		swe_sphere_ts_lg_irk->run_timestep(
				R_phi, R_vrt, R_div,
				0.5 * i_dt,
				i_simulation_timestamp
			);
	}

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






void SWE_Sphere_TS_ln_settls_vd::setup(
		int i_timestepping_order,
		LinearCoriolisTreatment_enum i_coriolis_treatment,
		NLRemainderTreatment_enum i_nonlinear_divergence_treatment,
		bool i_original_linear_operator_sl_treatment
)
{
	coriolis_treatment = i_coriolis_treatment;
	nonlinear_remainder_treatment = i_nonlinear_divergence_treatment;
	timestepping_order = i_timestepping_order;
	original_linear_operator_sl_treatment = i_original_linear_operator_sl_treatment;


	if (timestepping_order != 1 && timestepping_order != 2)
		SWEETError("Invalid time stepping order, must be 1 or 2");

	// Setup sampler for future interpolations
	sphereSampler.setup(op.sphereDataConfig);

	// Setup semi-lag
	semiLagrangian.setup(op.sphereDataConfig);


	swe_sphere_ts_ln_erk_split_vd = nullptr;
	swe_sphere_ts_l_irk = nullptr;
	swe_sphere_ts_lg_irk = nullptr;


	swe_sphere_ts_ln_erk_split_vd = new SWE_Sphere_TS_ln_erk_split_vd(simVars, op);
	swe_sphere_ts_ln_erk_split_vd->setup(1, true, true, true, true, false);

	if (coriolis_treatment == CORIOLIS_LINEAR)
	{
		swe_sphere_ts_l_irk = new SWE_Sphere_TS_l_irk(simVars, op);
		if (timestepping_order == 1)
		{
			swe_sphere_ts_l_irk->setup(
					1,
					simVars.timecontrol.current_timestep_size
				);
		}
		else
		{
			// initialize with 1st order and half time step size
			swe_sphere_ts_l_irk->setup(
					1,
					0.5 * simVars.timecontrol.current_timestep_size
				);
		}
	}
	else
	{
		swe_sphere_ts_lg_irk = new SWE_Sphere_TS_lg_irk(simVars, op);
		if (timestepping_order == 1)
			swe_sphere_ts_lg_irk->setup(
					1,
					simVars.timecontrol.current_timestep_size
				);
		else
			// initialize with 1st order and half time step size
			swe_sphere_ts_lg_irk->setup(
					1,
					0.5 * simVars.timecontrol.current_timestep_size
				);
	}
}



void SWE_Sphere_TS_ln_settls_vd::setup_auto()
{
	SWE_Sphere_TS_ln_settls_vd::LinearCoriolisTreatment_enum linear_coriolis_treatment = SWE_Sphere_TS_ln_settls_vd::CORIOLIS_IGNORE;
	SWE_Sphere_TS_ln_settls_vd::NLRemainderTreatment_enum nonlinear_divergence_treatment = SWE_Sphere_TS_ln_settls_vd::NL_REMAINDER_IGNORE;
	bool original_linear_operator_sl_treatment = true;

	if (simVars.disc.timestepping_method == "ln_settls")
	{
		linear_coriolis_treatment = SWE_Sphere_TS_ln_settls_vd::CORIOLIS_LINEAR;
		nonlinear_divergence_treatment = SWE_Sphere_TS_ln_settls_vd::NL_REMAINDER_NONLINEAR;
		original_linear_operator_sl_treatment = true;
	}
	else
	{
		// Search for Coriolis
		if (simVars.disc.timestepping_method.find("l_irk") != std::string::npos || simVars.disc.timestepping_method.find("l_exp") != std::string::npos)
			linear_coriolis_treatment = SWE_Sphere_TS_ln_settls_vd::CORIOLIS_LINEAR;
		else if (simVars.disc.timestepping_method.find("lc_na_sl") != std::string::npos)
			linear_coriolis_treatment = SWE_Sphere_TS_ln_settls_vd::CORIOLIS_SEMILAGRANGIAN;
		else if (simVars.disc.timestepping_method.find("lc_") != std::string::npos)
			linear_coriolis_treatment = SWE_Sphere_TS_ln_settls_vd::CORIOLIS_NONLINEAR;

		// Search for Nonlinear divergence
		if (simVars.disc.timestepping_method.find("_nr_") != std::string::npos)
			nonlinear_divergence_treatment = SWE_Sphere_TS_ln_settls_vd::NL_REMAINDER_NONLINEAR;

		if (simVars.disc.timestepping_method.find("_ver1") != std::string::npos)
			original_linear_operator_sl_treatment = false;
		else if (simVars.disc.timestepping_method.find("_ver0") != std::string::npos)
			original_linear_operator_sl_treatment = true;
		else
			original_linear_operator_sl_treatment = true;

#if 1
		string_id_storage = "";

		if (linear_coriolis_treatment == SWE_Sphere_TS_ln_settls_vd::CORIOLIS_LINEAR)
			string_id_storage += "l";
		else
			string_id_storage += "lg";

		string_id_storage += "_irk";

		if (linear_coriolis_treatment == SWE_Sphere_TS_ln_settls_vd::CORIOLIS_SEMILAGRANGIAN)
			string_id_storage += "_lc";

		string_id_storage += "_na";

		string_id_storage += "_sl";

		if (linear_coriolis_treatment == SWE_Sphere_TS_ln_settls_vd::CORIOLIS_NONLINEAR)
			string_id_storage += "_lc";

		if (nonlinear_divergence_treatment == SWE_Sphere_TS_ln_settls_vd::NL_REMAINDER_NONLINEAR)
			string_id_storage += "_nr";

		string_id_storage += "_settls";

		if (!original_linear_operator_sl_treatment)
			string_id_storage += "_ver1";

		std::string string_id_storage_ = string_id_storage + "_vd";

		if (simVars.disc.timestepping_method == string_id_storage_)
		{
			string_id_storage = string_id_storage_;
		}
		else
		{
			if (!original_linear_operator_sl_treatment)
			{
				// there must be a ver1 which is likely missing
				std::cerr << "Detected time stepping method: "+string_id_storage_ << std::endl;
				std::cerr << "Provided time stepping method: "+simVars.disc.timestepping_method << std::endl;
				SWEETError("Autodetection of parts of time stepping methods failed!");
			}

			std::string string_id_storage2 = string_id_storage+"_ver0"+"_vd";
			if (simVars.disc.timestepping_method == string_id_storage2)
			{
				string_id_storage = string_id_storage2;
			}
			else
			{
				std::cerr << "Detected time stepping method: "+string_id_storage_ << std::endl;
				std::cerr << "Provided time stepping method: "+simVars.disc.timestepping_method << std::endl;
				std::cerr << "Detected alternative time stepping method: "+string_id_storage2 << std::endl;
				SWEETError("Autodetection of parts of time stepping methods failed!");
			}
		}
	}
#endif

	setup(
			simVars.disc.timestepping_order,
			linear_coriolis_treatment,				// Coriolis treatment
			nonlinear_divergence_treatment,			// Nonlinear divergence treatment
			original_linear_operator_sl_treatment	// original SL linear operator treatment
		);
}



SWE_Sphere_TS_ln_settls_vd::SWE_Sphere_TS_ln_settls_vd(
			SimulationVariables &i_simVars,
			SphereOperators_SphereData &i_op,
			bool i_setup_auto
		) :
		simVars(i_simVars),
		op(i_op),
		semiLagrangian(simVars),

		coriolis_treatment(CORIOLIS_LINEAR),
		nonlinear_remainder_treatment(NL_REMAINDER_NONLINEAR),
		original_linear_operator_sl_treatment(true)
{
	if (i_setup_auto)
		setup_auto();
}

SWE_Sphere_TS_ln_settls_vd::~SWE_Sphere_TS_ln_settls_vd()
{
	delete swe_sphere_ts_ln_erk_split_vd;
	delete swe_sphere_ts_l_irk;
	delete swe_sphere_ts_lg_irk;
}


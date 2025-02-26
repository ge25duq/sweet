/*
 * Author: Martin SCHREIBER <schreiberx@gmail.com>
 */

#ifndef SRC_BENCHMARKS_SPHERE_ADVECTION_NAIR_LAURITZEN_HPP_
#define SRC_BENCHMARKS_SPHERE_ADVECTION_NAIR_LAURITZEN_HPP_

#include "PDEAdvectionSphereBenchmarks_BaseInterface.hpp"
#include <ostream>
#include <sweet/core/shacks/ShackDictionary.hpp>
#include <sweet/core/sphere/SphereOperators.hpp>
#include <sweet/core/sphere/SphereData_Config.hpp>

#include <sweet/core/VectorMath.hpp>


class PDEAdvectionSphereBenchmark_nair_lauritzen_sl	:
		public PDEAdvectionSphereBenchmarks_BaseInterface
{
public:
	PDEAdvectionSphereBenchmark_nair_lauritzen_sl()
	{
	}

	std::string benchmark_name;

	bool implements_benchmark(
			const std::string &i_benchmark_name
		)
	{
		benchmark_name = i_benchmark_name;

		return
				benchmark_name == "nair_lauritzen_case_test"	||
				benchmark_name == "nair_lauritzen_case_1"	||
				benchmark_name == "nair_lauritzen_case_2"	||
				benchmark_name == "nair_lauritzen_case_2_k0.5"	||
				benchmark_name == "nair_lauritzen_case_3"	||
				benchmark_name == "nair_lauritzen_case_4"	||
				benchmark_name == "nair_lauritzen_case_4_ext_2"	||
				benchmark_name == "nair_lauritzen_case_4_ext_3" ||
				false
		;
	}


	void setup_1_shackData()
	{
	}

	void setup_2_withOps(
			sweet::SphereOperators *io_ops
	)
	{
		ops = io_ops;

		shackPDEAdvBenchmark->callback_slComputeDeparture3rdOrder = callback_sl_compute_departure_3rd_order;
		shackPDEAdvBenchmark->slComputeDeparture3rdOrderUserData = this;
	}


	bool has_time_varying_state()
	{
		return true;
	}


	std::string printHelp()
	{
		std::ostringstream stream;
		stream << " * NAIR LAURITZEN SL TEST CASES:" << std::endl;

		stream << "    + 'nair_lauritzen_case_test'" << std::endl;
		stream << "    + 'nair_lauritzen_case_1'" << std::endl;
		stream << "    + 'nair_lauritzen_case_2'" << std::endl;
		stream << "    + 'nair_lauritzen_case_2_k0.5'" << std::endl;
		stream << "    + 'nair_lauritzen_case_3'" << std::endl;
		stream << "    + 'nair_lauritzen_case_4'" << std::endl;
		stream << "    + 'nair_lauritzen_case_4_ext_2'" << std::endl;
		stream << "    + 'nair_lauritzen_case_4_ext_3'" << std::endl;

		return stream.str();
	}


	void getInitialState(
		std::vector<sweet::SphereData_Spectral> &o_prognostic_fields,
		sweet::SphereData_Physical &o_u,
		sweet::SphereData_Physical &o_v
	)
	{
		SWEETAssert(o_prognostic_fields.size() == 1, "Only scalar field supported for this benchmark!");

		getInitialState_Spectral(o_prognostic_fields[0], o_u, o_v);
	}

	void getInitialState_Spectral(
		sweet::SphereData_Spectral &o_phi_pert,
		sweet::SphereData_Physical &o_u,
		sweet::SphereData_Physical &o_v
	)
	{
		/*
		 * Time-varying benchmark case 4 from
		 *
		 * R. Nair, P. Lauritzen "A class of deformational flow
		 * test cases for linear transport problems on the sphere"
		 */

		/*
		 * Setup parameters
		 */

		// use the radius from the command line parameter since this is should work flawless
		//double sphere_radius = 6.37122e6;
		double h0 = 1.0;				// h_max

		if (std::isinf(shackTimestepControl->max_simulation_time))
			shackTimestepControl->max_simulation_time = 12*24*60*60;		// default: 12 days

		// update operators
		ops->setup(ops->sphereDataConfig, shackSphereDataOps);

		double i_lambda0 = M_PI/3;
		double i_theta0 = M_PI;
		double i_lambda1 = -M_PI/3;
		double i_theta1 = M_PI;

		if (benchmark_name == "nair_lauritzen_case_test")
		{
			i_lambda0 = M_PI;
			i_theta0 = M_PI/3;
			i_lambda1 = M_PI;
			i_theta1 = -M_PI/3;
		}
		else if (benchmark_name == "nair_lauritzen_case_1")
		{
			i_lambda0 = M_PI;
			i_theta0 = M_PI/3;
			i_lambda1 = M_PI;
			i_theta1 = -M_PI/3;
		}
		else if (
				benchmark_name == "nair_lauritzen_case_2" ||
				benchmark_name == "nair_lauritzen_case_4" ||
				benchmark_name == "nair_lauritzen_case_4_ext_2"
		)
		{
			i_lambda0 = 5*M_PI/6.0;
			i_theta0 = 0;
			i_lambda1 = 7*M_PI/6.0;
			i_theta1 = 0;
		}
		else if (
				benchmark_name == "nair_lauritzen_case_3" ||
				benchmark_name == "nair_lauritzen_case_4_ext_3"
		)
		{
			i_lambda0 = 3*M_PI/4.0;
			i_theta0 = 0;
			i_lambda1 = 5*M_PI/4.0;
			i_theta1 = 0;
		}


#if 0

		/**
		 * Initial condition for Cosine bell
		 *
		 * DO NOT USE THIS, SINCE IT'S NOT SUFFICIENTLY SMOOTH FOR THE CONVERGENCE BENCHMARKS!!!
		 *
		 * (Section 3.1.1)
		 */
		sweet::SphereData_Physical phi_pert_phys_1(sphereDataConfig);
		{
			// Cosine bells

			sweet::SphereData_Physical phi_pert_phys(sphereDataConfig);
			phi_pert_phys.physical_update_lambda(
				[&](double i_lambda, double i_theta, double &io_data)
				{
					// Constants
					double r = 0.5;
					double b = 0.1;
					double c = 0.9;
					double pi = M_PI;

					// eq between 12 and 13
					double r0 = std::acos(std::sin(i_theta0)*std::sin(i_theta) + std::cos(i_theta0)*(std::cos(i_theta)*std::cos(i_lambda-i_lambda0)));
					double r1 = std::acos(std::sin(i_theta1)*std::sin(i_theta) + std::cos(i_theta1)*(std::cos(i_theta)*std::cos(i_lambda-i_lambda1)));

					io_data = 0;
					// eq. 13
					if (r0 < r)
						io_data = b + c*shackPDEAdvBenchmark->h0/2.0*(1.0 + std::cos(pi*r0/r));
					else if (r1 < r)
						io_data = b + c*shackPDEAdvBenchmark->h0/2.0*(1.0 + std::cos(pi*r1/r));
					else
						io_data = b;
				}
			);

			o_phi_pert = phi_pert_phys;
		}

#else

		double b0 = 5;
		//double b0 = 20;

		/**
		 * Initial condition for smooth scalar field (Gaussian bump)
		 *
		 * (Section 3.1.2)
		 */
		sweet::SphereData_Physical phi_pert_phys_1(ops->sphereDataConfig);
		{
			// Bump 1

			// Caption Figure 1
			double x0[3];
			sweet::VectorMath::point_latlon_to_cartesian__scalar(i_lambda0, i_theta0, x0[0], x0[1], x0[2]);

			phi_pert_phys_1.physical_update_lambda(
				[&](double i_lambda, double i_theta, double &io_data)
				{
					double x[3];
					sweet::VectorMath::point_latlon_to_cartesian__scalar(i_lambda, i_theta, x[0], x[1], x[2]);

					double d =	(x[0] - x0[0])*(x[0] - x0[0]) +
								(x[1] - x0[1])*(x[1] - x0[1]) +
								(x[2] - x0[2])*(x[2] - x0[2]);

					io_data = std::exp(-b0*d)*h0;
				}
			);
		}

		sweet::SphereData_Physical phi_pert_phys_2(ops->sphereDataConfig);
		{
			// Bump 2

			// Caption Figure 1
			double x0[3];
			sweet::VectorMath::point_latlon_to_cartesian__scalar(i_lambda1, i_theta1, x0[0], x0[1], x0[2]);

			phi_pert_phys_2.physical_update_lambda(
				[&](double i_lambda, double i_theta, double &io_data)
				{
					double x[3];
					sweet::VectorMath::point_latlon_to_cartesian__scalar(i_lambda, i_theta, x[0], x[1], x[2]);

					double d =	(x[0] - x0[0])*(x[0] - x0[0]) +
								(x[1] - x0[1])*(x[1] - x0[1]) +
								(x[2] - x0[2])*(x[2] - x0[2]);

					io_data = std::exp(-b0*d)*h0;
				}
			);
		}

		o_phi_pert.loadSphereDataPhysical(phi_pert_phys_1 + phi_pert_phys_2);
#endif

		get_varying_velocities(o_u, o_v, 0);

		return;
	}


	/*
	 * Update fields for time-varying benchmarks
	 */
	void get_varying_velocities(
			sweet::SphereData_Physical &o_u_phys,
			sweet::SphereData_Physical &o_v_phys,
			double i_timestamp = 0
	)
	{
		/*********************************************************************
		 * Time-varying benchmark cases
		 *
		 * R. Nair, P. Lauritzen "A class of deformational flow
		 * test cases for linear transport problems on the sphere"
		 */
		if (benchmark_name == "nair_lauritzen_case_test")
		{
			// time for total deformation
			double T = shackTimestepControl->max_simulation_time;

			// velocity
			double u0 = 2.0*M_PI*shackSphereDataOps->sphere_radius/T;

			// we set k to 2.4 (p. 5)
			double k = 0.5;

			o_u_phys.physical_update_lambda(
				[&](double i_lambda, double i_theta, double &io_data)
				{
					// time varying flow
					io_data = k * std::pow(std::sin(i_lambda/2), 2.0) * std::sin(2.0*i_theta) * std::cos(M_PI*i_timestamp/T);
				}
			);

			o_v_phys.physical_update_lambda(
				[&](double i_lambda, double i_theta, double &io_data)
				{
					// time varying flow
					io_data = k/2.0 * std::sin(i_lambda) * std::cos(i_theta) * std::cos(M_PI*i_timestamp/T);
				}
			);

			o_u_phys *= u0;
			o_v_phys *= u0;
			return;
		}

		if (benchmark_name == "nair_lauritzen_case_1")
		{
			// time for total deformation
			double T = 1;
			double t = i_timestamp;
			double pi = M_PI;

			// we set k to 2.4 (p. 5)
			double k = 2.4;

			/*
			 * Non-dimensionalize
			 */
			// t \in [0;1]
			t /= shackTimestepControl->max_simulation_time;

			// velocity scalar for effects across entire simulation time (e.g. full revelation around sphere)
			// reference solution is computed with T = 5.0 and we resemble it here
			// using radius 1, T=5 would result in u0=1
			double u0 = shackSphereDataOps->sphere_radius*5.0/shackTimestepControl->max_simulation_time;

			using namespace sweet::ScalarDataArray_ops;

			o_u_phys.physical_update_lambda(
				[&](double i_lambda, double i_theta, double &io_data)
				{
					// time varying flow
					io_data = k * pow2(sin(i_lambda/2.0)) * sin(2.0*i_theta) * cos(pi*t/T);
					io_data *= u0;
				}
			);

			o_v_phys.physical_update_lambda(
				[&](double i_lambda, double i_theta, double &io_data)
				{
					// time varying flow
					io_data = k/2.0 * sin(i_lambda) * cos(i_theta) * cos(pi*t/T);
					io_data *= u0;
				}
			);

			return;
		}

		if (benchmark_name == "nair_lauritzen_case_2" || benchmark_name == "nair_lauritzen_case_2_k0.5")
		{
			// time for total deformation
			double T = 1;
			double t = i_timestamp;
			double pi = M_PI;

			// we set k to 2 (p. 5)
			double k = 2;

			/*
			 * Non-dimensionalize
			 */
			// t \in [0;1]
			t /= shackTimestepControl->max_simulation_time;

			// velocity scalar for effects across entire simulation time (e.g. full revelation around sphere)
			// reference solution is computed with T = 5.0 and we resemble it here
			double u0 = shackSphereDataOps->sphere_radius*5.0/shackTimestepControl->max_simulation_time;

			using namespace sweet::ScalarDataArray_ops;

			if (benchmark_name == "nair_lauritzen_case_2_k0.5")
				k = 0.5;

			o_u_phys.physical_update_lambda(
				[&](double i_lambda, double i_theta, double &io_data)
				{
					// time varying flow
					io_data = k * pow2(sin(i_lambda)) * sin(2.0*i_theta) * cos(pi*t/T);
					io_data *= u0;
				}
			);

			o_v_phys.physical_update_lambda(
				[&](double i_lambda, double i_theta, double &io_data)
				{
					// time varying flow
					io_data = k * sin(2.0*i_lambda) * cos(i_theta) * cos(pi*t/T);
					io_data *= u0;
				}
			);

			return;
		}

		if (benchmark_name == "nair_lauritzen_case_3")
		{
			/*
			 * This is a divergent flow!!!
			 */

			// time for total deformation
			double T = 1;
			double t = i_timestamp;
			double pi = M_PI;

			// we set k to 2 (p. 7, "other parameters exactly as given in Case-2")
			double k = 2;

			/*
			 * Non-dimensionalize
			 */
			// t \in [0;1]
			t /= shackTimestepControl->max_simulation_time;

			// velocity scalar for effects across entire simulation time (e.g. full revelation around sphere)
			// reference solution is computed with T = 5.0 and we resemble it here
			double u0 = shackSphereDataOps->sphere_radius*5.0/shackTimestepControl->max_simulation_time;

			using namespace sweet::ScalarDataArray_ops;

			o_u_phys.physical_update_lambda(
				[&](double i_lambda, double i_theta, double &io_data)
				{
					// time varying flow
					io_data = -k * pow2(sin(i_lambda/2.0)) * sin(2.0*i_theta) * pow2(cos(i_theta)) * std::cos(pi*t/T);
					io_data *= u0;
				}
			);

			o_v_phys.physical_update_lambda(
				[&](double i_lambda, double i_theta, double &io_data)
				{
					// time varying flow
					io_data = k/2.0 * sin(i_lambda) * pow3(cos(i_theta)) * cos(pi*t/T);
					io_data *= u0;
				}
			);
			return;
		}

		if (
			benchmark_name == "nair_lauritzen_case_4"	||
			benchmark_name == "nair_lauritzen_case_4_ext_2"
		)
		{

			// time for total deformation
			double T = 1;
			double t = i_timestamp;
			double pi = M_PI;

			// we set k to 2 (p. 5)
			double k = 2;

			/*
			 * Non-dimensionalize
			 */
			// t \in [0;1]
			t /= shackTimestepControl->max_simulation_time;

			// velocity scalar for effects across entire simulation time (e.g. full revelation around sphere)
			// reference solution is computed with T = 5.0 and we resemble it here
			double u0 = shackSphereDataOps->sphere_radius*5.0/shackTimestepControl->max_simulation_time;

			double u0_rot = shackSphereDataOps->sphere_radius/shackTimestepControl->max_simulation_time;

			using namespace sweet::ScalarDataArray_ops;

			if (benchmark_name == "nair_lauritzen_case_2_k0.5")
				k = 0.5;

			o_u_phys.physical_update_lambda(
				[&](double i_lambda, double i_theta, double &io_data)
				{
					io_data = 0;

					// washing machine
					double lambda_prime = i_lambda - 2.0*pi*t / T;
					io_data += u0 * k * pow2(sin(lambda_prime)) * sin(2.0*i_theta) * cos(pi*t/T);

					// add a constant zonal flow
					// non-dimensional version
					io_data += u0_rot*2.0*pi*cos(i_theta)/T;

				}
			);

			o_v_phys.physical_update_lambda(
				[&](double i_lambda, double i_theta, double &io_data)
				{
					io_data = 0;

					// washing machine
					double lambda_prime = i_lambda - 2.0*pi*t / T;
					io_data += k * sin(2.0*lambda_prime) * cos(i_theta) * cos(pi*t/T);
					io_data *= u0;
				}
			);

			return;
		}

		if (benchmark_name == "nair_lauritzen_case_4_ext_3")
		{
			/*
			 * This is a modified case4 version to include the nonlinear divergence
			 */

			/*
			 * This is a divergent flow!!!
			 */

			// time for total deformation
			double T = 1;
			double t = i_timestamp;
			double pi = M_PI;

			// we set k to 2 (p. 7, "other parameters exactly as given in Case-2")
			double k = 2;

			/*
			 * Scale things up for the sphere
			 */
			// total simulation time
			T *= shackTimestepControl->max_simulation_time;

			// velocity scalar for effects across entire simulation time (e.g. full revelation around sphere)
			// reference solution is computed with T = 5.0 and we resemble it here
			double u0 = shackSphereDataOps->sphere_radius*5.0/T;

			using namespace sweet::ScalarDataArray_ops;

			o_u_phys.physical_update_lambda(
				[&](double i_lambda, double i_theta, double &io_data)
				{
					io_data = 0;

					// washing machine
					double lambda_prime = i_lambda - 2.0*pi*t/T;
					io_data += -k * pow2(sin(lambda_prime/2.0)) * sin(2.0*i_theta) * pow2(cos(i_theta)) * std::cos(pi*t/T);
					io_data *= u0;

					// add a constant zonal flow
					// non-dimensional version
					//io_data += 2.0*pi*cos(i_theta)/T;
					io_data += 2.0*pi*cos(i_theta)*shackSphereDataOps->sphere_radius/T;
				}
			);

			o_v_phys.physical_update_lambda(
				[&](double i_lambda, double i_theta, double &io_data)
				{
					io_data = 0;

					// washing machine
					double lambda_prime = i_lambda - 2.0*pi*t / T;
					io_data += k/2.0 * sin(lambda_prime) * pow3(cos(i_theta)) * cos(pi*t/T);
					io_data *= u0;
				}
			);

			return;
		}
	}


	static
	void callback_sl_compute_departure_3rd_order(
			void *i_this,
			const sweet::ScalarDataArray &i_pos_lon_A,	///< longitude coordinate to compute the velocity for
			const sweet::ScalarDataArray &i_pos_lat_A,	///< latitude coordinate to compute the velocity for
			sweet::ScalarDataArray &o_pos_lon_D,		///< velocity along longitude
			sweet::ScalarDataArray &o_pos_lat_D,		///< velocity along latitude
			double i_dt,
			double i_timestamp_arrival			///< timestamp at arrival point
	)
	{
		((PDEAdvectionSphereBenchmark_nair_lauritzen_sl*)i_this)->sl_compute_departure_3rd_order(i_pos_lon_A, i_pos_lat_A, o_pos_lon_D, o_pos_lat_D, i_dt, i_timestamp_arrival);
	}


	/*
	 * Compute the departure points
	 */
	void sl_compute_departure_3rd_order(
			const sweet::ScalarDataArray &i_pos_lon_A,	///< longitude coordinate to compute the velocity for
			const sweet::ScalarDataArray &i_pos_lat_A,	///< latitude coordinate to compute the velocity for
			sweet::ScalarDataArray &o_pos_lon_D,		///< velocity along longitude
			sweet::ScalarDataArray &o_pos_lat_D,		///< velocity along latitude
			double i_dt,
			double i_timestamp_arrival			///< timestamp at arrival point
	)
	{
		if (benchmark_name == "nair_lauritzen_case_1")
		{
			/*********************************************************************
			 * R. Nair, P. Lauritzen "A class of deformational flow
			 * test cases for linear transport problems on the sphere"
			 *
			 * Page 8
			 */

			// time for total deformation
			double pi = M_PI;

			// we set k to 2.4 (p. 5)
			double k = 2.4;


			/*
			 * Get non-dimensionalize variables to stick to the benchmark
			 * whatever resolution we're using
			 *
			 * We rescale everything as if we would execute it with SWEET runtime parameters
			 *   -a 1
			 *   -t 5
			 */

			// Set full time interval to 5 (used in paper)
			double T = 5.0;

			// rescale time interval to [0;5] range
			double t = i_timestamp_arrival*5.0/shackTimestepControl->max_simulation_time;

			// rescale time step size
			double dt = i_dt*5.0/shackTimestepControl->max_simulation_time;


			double omega = pi/T;


			// To directly use notation from paper
			const sweet::ScalarDataArray &lambda = i_pos_lon_A;
			const sweet::ScalarDataArray &theta = i_pos_lat_A;


			// use convenient cos/sin/etc functions
			using namespace sweet::ScalarDataArray_ops;

			// eq. (37)
			sweet::ScalarDataArray u_tilde = 2.0*k*pow2(sin(lambda/2))*sin(theta)*cos(pi*t/T);

			// between eq. (34) and (35)
			sweet::ScalarDataArray v = k/2.0*sin(lambda)*cos(theta)*cos(omega*t);

			// eq. (35)
			o_pos_lon_D =
					lambda
					- dt*u_tilde
					- dt*dt*k*sin(lambda/2.0)*(
							sin(lambda/2)*sin(theta)*sin(omega*t)*omega
							- u_tilde*sin(theta)*cos(omega*t)*cos(lambda/2)
							- v*sin(lambda/2)*cos(theta)*cos(omega*t)
							);

			// eq. (36)
			o_pos_lat_D =
					theta
					- dt*v
					- dt*dt/4.0*k*(
							sin(lambda)*cos(theta)*sin(omega*t)*omega
							- u_tilde*cos(lambda)*cos(theta)*cos(omega*t)
							+ v*sin(lambda)*sin(theta)*cos(omega*t)
						);

			return;
		}

		SWEETError("TODO: Implement it for this test case");
	}


};

#endif

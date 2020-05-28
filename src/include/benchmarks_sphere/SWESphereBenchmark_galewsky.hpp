/*
 * Author: Martin Schreiber <SchreiberX@Gmail.com>
 */

#ifndef SRC_SWE_SPHERE_BENCHMARKS_GALEWSKY_HPP_
#define SRC_SWE_SPHERE_BENCHMARKS_GALEWSKY_HPP_

#include <benchmarks_sphere/SWESphereBenchmarks_helpers.hpp>
#include <benchmarks_sphere/SWESphereBenchmarks_interface.hpp>
#include <sweet/SimulationVariables.hpp>
#include <sweet/sphere/SphereOperators_SphereData.hpp>

#include <libmath/GaussQuadrature.hpp>

class SWESphereBenchmark_galewsky	: public SWESphereBenchmarks_interface
{
	SimulationVariables *simVars = nullptr;
	SphereOperators_SphereData *ops = nullptr;


public:
	SWESphereBenchmark_galewsky()
	{
	}

	std::string benchmark_name;

	bool implements_benchmark(
			const std::string &i_benchmark_name
		)
	{
		benchmark_name = i_benchmark_name;

		return
			i_benchmark_name == "galewsky" ||			///< Standard Galewsky benchmark
			i_benchmark_name == "galewsky_nobump" ||	///< Galewsky benchmark without bumps
			i_benchmark_name == "galewsky_nosetparams"	///< Galewsky benchmark without overriding parameters
		;
	}


	void setup(
			SimulationVariables *i_simVars,
			SphereOperators_SphereData *i_ops
	)
	{
		simVars = i_simVars;
		ops = i_ops;
	}


	std::string get_help()
	{
		std::ostringstream stream;
		stream << "  'galewsky': Galwesky benchmark" << std::endl;
		stream << "  'galewsky_nobump': Galwesky benchmark without any bump" << std::endl;
		stream << "  'galewsky_nosetparams': Galwesky benchmark without setting parameters" << std::endl;
		return stream.str();
	}

	void get_initial_state(
		SphereData_Spectral &o_phi_pert,
		SphereData_Spectral &o_vrt,
		SphereData_Spectral &o_div
	)
	{
		SWESphereBenchmarks_helpers helpers(simVars, ops);
		const SphereData_Config *sphereDataConfig = o_phi_pert.sphereDataConfig;

		if (benchmark_name != "galewsky_nosetparams")
		{
			if (simVars->timecontrol.current_simulation_time == 0)
			{
				std::cout << "!!! WARNING !!!" << std::endl;
				std::cout << "!!! WARNING: Overriding simulation parameters for this benchmark !!!" << std::endl;
				std::cout << "!!! WARNING !!!" << std::endl;
			}

			/// Setup Galewski parameters
			simVars->sim.sphere_rotating_coriolis_omega = 7.292e-5;
			simVars->sim.gravitation = 9.80616;
			simVars->sim.sphere_radius = 6.37122e6;

			// see doc/galewsky_mean_layer_depth/ on how to get this constant.
			// it is NOT 10e3 (see Galewsky paper)
			simVars->sim.h0 = 10158.186170454619;


			/*
			 * Rerun setup to update the operators with the potentially new values
			 */
			ops->setup(o_phi_pert.sphereDataConfig, &(simVars->sim));
		}

		/*
		 * Parameters from Galewsky paper setup
		 */
		double a = simVars->sim.sphere_radius;
		double omega = simVars->sim.sphere_rotating_coriolis_omega;
		double umax = 80.;
		double phi0 = M_PI/7.;
		double phi1 = 0.5*M_PI - phi0;
		double phi2 = 0.25*M_PI;		/// latitude placement of gaussian bump
		double en = std::exp(-4.0/std::pow((phi1-phi0), 2.0));
		double alpha = 1./3.;
		double beta = 1./15.;
		double hamp = 120.;


		if (simVars->benchmark.benchmark_galewsky_umax >= 0)
			umax = simVars->benchmark.benchmark_galewsky_umax;

		if (simVars->benchmark.benchmark_galewsky_hamp >= 0)
			hamp = simVars->benchmark.benchmark_galewsky_hamp;

		if (simVars->benchmark.benchmark_galewsky_phi2 >= 0)
			phi2 = simVars->benchmark.benchmark_galewsky_phi2;

		/*
		 * Setup V=0
		 */
		SphereData_Physical vg(o_phi_pert.sphereDataConfig);
		vg.physical_set_zero();

		auto lambda_u = [&](double phi) -> double
		{
			if (phi >= phi1-1e-5 || phi <= phi0+1e-5)
				return 0.0;
			else
				return umax/en*std::exp(1.0/((phi-phi0)*(phi-phi1)));
		};

		auto lambda_f = [&](double phi) -> double
		{
			return a*lambda_u(phi)*(2.0*omega*std::sin(phi)+(std::tan(phi)/a)*lambda_u(phi));
		};

		/*
		 * Setup U=...
		 * initial velocity along longitude
		 */
		SphereData_Physical ug(o_phi_pert.sphereDataConfig);
		ug.physical_update_lambda(
			[&](double lon, double phi, double &o_data)
			{
				o_data = lambda_u(phi);
			}
		);

		ops->uv_to_vortdiv(ug, vg, o_vrt, o_div);

		bool use_analytical_geostrophic_setup = simVars->misc.comma_separated_tags.find("galewsky_analytical_geostrophic_setup") != std::string::npos;

		if (use_analytical_geostrophic_setup)
		{
			std::cout << "[MULE] use_analytical_geostrophic_setup: 1" << std::endl;
			helpers.computeGeostrophicBalance_nonlinear(
					o_vrt,
					o_div,
					o_phi_pert
			);

			double h0_ = 10e3;
			o_phi_pert = simVars->sim.gravitation * h0_ + o_phi_pert;
			o_phi_pert -= simVars->sim.gravitation*simVars->sim.h0;
		}
		else
		{
			std::cout << "[MULE] use_analytical_geostrophic_setup: 0" << std::endl;

			/*
			 * Initialization of SWE height
			 *
			 * Metric correction terms based on John Thuburn's code
			 */
#if 1
			const unsigned short nlat = sphereDataConfig->physical_num_lat;
			std::vector<double> hg_cached;
			hg_cached.resize(nlat);

			double h_metric_area = 0;
			double hg_sum = 0;
			double int_start, int_end, int_delta;

			int j = sphereDataConfig->physical_num_lat-1;


			// start/end of first integration interval
			{
				assert(sphereDataConfig->lat[j] < 0);

				// start at the south pole
				int_start = -M_PI*0.5;

				// first latitude gaussian point
				int_end = sphereDataConfig->lat[j];

				// 1d area of integration
				int_delta = int_end - int_start;

				assert(int_delta > 0);
				assert(int_delta < 1);

				double hg = GaussQuadrature::integrate5_intervals<double>(int_start, int_end, lambda_f, 20);
				//hg = (int_end+int_start)*0.5;
				hg_cached[j] = hg;

				/*
				 * cos scaling is required for 2D sphere coverage at this latitude
				 *
				 * metric term which computes the area coverage of each point
				 */
				// use integrated average as below instead of the following formulation
				// double mterm = cos((int_start+int_end)*0.5);
				//double mterm = (std::sin(int_end)-std::sin(int_start))*2.0*M_PI;
				double mterm = std::cos(sphereDataConfig->lat[j])*2.0*M_PI;
				assert(mterm > 0);

				hg_sum += hg*mterm;
				h_metric_area += mterm;

				int_start = int_end;
			}
			j--;

			for (; j >= 0; j--)
			{
				double int_end = sphereDataConfig->lat[j];
				int_delta = int_end - int_start;
				assert(int_delta > 0);

				double hg = hg_cached[j+1] + GaussQuadrature::integrate5_intervals<double>(int_start, int_end, lambda_f, 20);

				//hg = (int_end+int_start)*0.5;
				hg_cached[j] = hg;

				// metric term which computes the area coverage of each point
				//double mterm = (std::sin(int_end)-std::sin(int_start))*2.0*M_PI;
				double mterm = std::cos(sphereDataConfig->lat[j])*2.0*M_PI;

				hg_sum += hg*mterm;
				h_metric_area += mterm;

				// continue at the end of the last integration interval
				int_start = int_end;
			}

			// last integration interval
			{
				assert(int_start > 0);
				int_end = M_PI*0.5;

				int_delta = int_end - int_start;
				assert(int_delta > 0);

				// metric term which computes the area coverage of each point
				//double mterm = (std::sin(int_end)-std::sin(int_start))*2.0*M_PI;
				double mterm = std::cos(sphereDataConfig->lat[0])*2.0*M_PI;

				double hg = hg_cached[0] + GaussQuadrature::integrate5_intervals<double>(int_start, int_end, lambda_f, 20);
				//hg = (int_end+int_start)*0.5;
				hg_sum += hg*mterm;
				h_metric_area += mterm;
			}

			assert(h_metric_area > 0);


#if 0
			double h_sum = hg_sum / simVars->sim.gravitation;
			double h_comp_avg = h_sum / h_metric_area;
			// done later on

			/*
			 * From Galewsky et al. paper:
			 * "and the constant h 0 is chosen so that the global mean layer depth is equal to 10 km"
			 */
			double h0 = 10000.0 + h_comp_avg;
			std::cout << "Galewsky benchmark H0 (computed, not used!): " << h0 << std::endl;
#endif

#else

			std::vector<double> hg_cached;
			hg_cached.resize(sphereDataConfig->physical_num_lat);

			double int_start = -M_PI*0.5;
			for (int j = sphereDataConfig->physical_num_lat-1; j >= 0; j--)
			{
				double int_end = sphereDataConfig->lat[j];
				double quad = GaussQuadrature::integrate5_intervals<double>(int_start, int_end, lambda_f, 5);

				if (j == sphereDataConfig->physical_num_lat-1)
					hg_cached[j] = quad;
				else
					hg_cached[j] = hg_cached[j+1] + quad;

				int_start = int_end;


				std::cout << sphereDataConfig->lat[j] << ": " << hg_cached[j] << std::endl;
			}

#endif

			// update data
			SphereData_Physical phig(sphereDataConfig);
			phig.physical_update_lambda_array(
				[&](int i, int j, double &o_data)
				{
					o_data = hg_cached[j];
				}
			);

			o_phi_pert.loadSphereDataPhysical(phig);

			o_phi_pert = -o_phi_pert;
		}

		/*
		 * Now change global mean layer depth to 10km
		 *
		 * From Galewsky et al. paper:
		 * "and the constant h 0 is chosen so that the global mean layer depth is equal to 10 km"
		 */
		o_phi_pert += (simVars->sim.h0 - 10000)*simVars->sim.gravitation;
		simVars->sim.h0 = 10000;

		SphereData_Physical hbump(o_phi_pert.sphereDataConfig);
		if (benchmark_name == "galewsky")
		{
			hbump.physical_update_lambda(
				[&](double lon, double phi, double &o_data)
				{
					o_data = hamp*std::cos(phi)*std::exp(-std::pow((lon-M_PI)/alpha, 2.0))*std::exp(-std::pow((phi2-phi)/beta, 2.0));
				}
			);
			o_phi_pert += hbump*simVars->sim.gravitation;
		}


		std::cout << "phi min: " << o_phi_pert.toPhys().physical_reduce_min() << std::endl;
		std::cout << "phi max: " << o_phi_pert.toPhys().physical_reduce_max() << std::endl;

	}
};

#endif

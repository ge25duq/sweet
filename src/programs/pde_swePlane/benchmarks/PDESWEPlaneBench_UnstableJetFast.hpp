/*
 * SWEUnstableJetFast.hpp
 *
 *  Created on: 05 Mar 2018
 * Author: Pedro Peixoto <pedrosp@ime.usp.br>
 */
#ifndef SWE_PLANE_UNSTABLEJETFAST_HPP_
#define SWE_PLANE_UNSTABLEJETFAST_HPP_


#include <stdlib.h>

#include <cmath>
#include <sweet/core/shacks/ShackDictionary.hpp>
#include <sweet/core/plane/PlaneData_Spectral.hpp>
#include <sweet/core/plane/PlaneData_Physical.hpp>
#include <sweet/libmath/GaussQuadrature.hpp>
#include "PDESWEPlaneBench_BaseInterface.hpp"


/**
 * Implement unstable jet initial conditions
 *
 * Mimics Spherical Mountain Wave test case 5
 *
 *
 **/
class PDESWEPlaneBench_UnstableJetFast	:
		public PDESWEPlaneBench_BaseInterface
{
	double f = shackPDESWEPlane->plane_rotating_f0;
	double g = shackPDESWEPlane->gravitation;
	double sy = shackPlaneDataOps->plane_domain_size[1];

	bool with_bump;

public:
	PDESWEPlaneBench_UnstableJetFast(
			bool i_with_bump = true
	)	:
		with_bump(i_with_bump)
	{
	}



	/*
	 * The depth function is numerically integrated to ensure
	 * balanced initial conditions for the jet
	 *
	 */
	double depth(
			double x,
			double y
	)
	{

		return -(f/g)*sy*GaussQuadrature::integrate5_intervals_adaptive_linear<double>(0, y, u_fun, 10e-13);
		//return -(f/g)*GaussQuadrature::integrate5_intervals<double>(0, y, u_fun, 200);
	}

	/*
	 * Velocity
	 * On (x,y) \in [0,1]x[0,1]
	 */
	static double u_fun(
			double y
	)
	{
		 //power has to be odd to ensure periodicity
		// the larger the thiner the jet
		// Max speed is 50m/s
		return 300.0*std::pow(std::sin(2.0*M_PI*y), 81);
	}

	double u(
			double x,
			double y
	)
	{
		return u_fun(y);

	}

	/*
	 * Depth perturbation (gaussian bumps)
	 * On (x,y) \in [0,1]x[0,1]
	 */
	double bump(
			double x,
			double y
	)
	{
		//double radius = shackDict.setup.radius_scale*sqrt((double)sx*(double)sx+(double)sy*(double)sy);
		double radius = 1.0; //shackDict.setup.radius_scale*sqrt((double)sx*(double)sx+(double)sy*(double)sy);
		double factor = 1000.0;


		// Gaussian Bump top
		double dx = x-0.85;
		double dy = y-0.75;

		dx /= radius;
		dy /= radius;

		double exp1 = std::exp(-factor*(dx*dx + dy*dy));

		// Gaussian Bump bottom
		dx = x-0.15;
		dy = y-0.25;

		dx /= radius;
		dy /= radius;

		double exp2 = std::exp(-factor*(dx*dx + dy*dy));

		double pert = 0.01*shackPDESWEPlane->h0;
		//double pert = 0.000;

		return (pert)*(exp1+exp2);

	}

	void setup_depth(
			sweet::PlaneData_Spectral &o_depth
	)
	{
		// First set for the first column (one vertical slice)

		sweet::PlaneData_Physical depth_phys(o_depth.planeDataConfig);

		for (int j = 0; j < shackPlaneDataOps->space_res_physical[1]; j++)
		{
			int i = 0;
			double x = (((double)i+0.5)/(double)shackPlaneDataOps->space_res_physical[0]); //*shackDict.sim.domain_size[0];
			double y = (((double)j+0.5)/(double)shackPlaneDataOps->space_res_physical[1]); //*shackDict.sim.domain_size[1];

			depth_phys.physical_set_value(j, i, depth(x, y));
		}

		//Now set for other "x" and add bump
		if (with_bump)
		{
			for (int j = 0; j < shackPlaneDataOps->space_res_physical[1]; j++)
			{
				for (int i = 1; i < shackPlaneDataOps->space_res_physical[0]; i++)
				{

					// h - lives in the center of the cell
					// (x,y) \in [0,1]x[0,1]
					double x = (((double)i+0.5)/(double)shackPlaneDataOps->space_res_physical[0]); //*shackDict.sim.domain_size[0];
					double y = (((double)j+0.5)/(double)shackPlaneDataOps->space_res_physical[1]); //*shackDict.sim.domain_size[1];

					depth_phys.physical_set_value(j, i, depth_phys.physical_get(j, 0) + bump(x,y));

				}
			}
		}

		o_depth.loadPlaneDataPhysical(depth_phys);

	}

	void setup_velocity(
			sweet::PlaneData_Spectral &o_u,
			sweet::PlaneData_Spectral &o_v
	)
	{

		sweet::PlaneData_Physical u_phys(o_u.planeDataConfig);
		o_v.spectral_set_zero();

		for (int j = 0; j < shackPlaneDataOps->space_res_physical[1]; j++)
		{
			for (int i = 0; i < shackPlaneDataOps->space_res_physical[0]; i++)
			{

				// (u,v) - lives in the center of the cell
				double x = (((double)i+0.5)/(double)shackPlaneDataOps->space_res_physical[0]); //*shackDict.sim.domain_size[0];
				double y = (((double)j+0.5)/(double)shackPlaneDataOps->space_res_physical[1]); //*shackDict.sim.domain_size[1];
				// (x,y) \in [0,1]x[0,1]
				u_phys.physical_set_value(j, i, u(x, y));
			}
		}

		o_u.loadPlaneDataPhysical(u_phys);
	}


	bool setupBenchmark(
			sweet::PlaneData_Spectral &o_h,
			sweet::PlaneData_Spectral &o_u,
			sweet::PlaneData_Spectral &o_v
	)
	{
		std::cout << "Generating Unstable Jet initial conditions.";
		/*
		 * Setup velocities
		 */
		setup_velocity(o_u,o_v);


		/*
		 * Setup depth function
		 * based on velocities
		 */
		setup_depth(o_h);
		//o_h.file_physical_saveData_ascii("ouput_depth");

		/*
		 * Add perturbation to depth
		 */
		std::cout << "   Done! " << std::endl;

		return true;
	}


};


#endif

/*
 * Author: Martin SCHREIBER <schreiberx@gmail.com>
 */
#ifndef PDE_Advection_PLANE_BENCH_CYLINDER_HPP__
#define PDE_Advection_PLANE_BENCH_CYLINDER_HPP__


#include <stdlib.h>
#include <cmath>
#include <sweet/core/plane/Plane.hpp>

#include <sweet/core/shacksShared/ShackPlaneDataOps.hpp>
#include "ShackPDEAdvectionPlaneBenchmarks.hpp"
#include "PDEAdvectionPlaneBench_BaseInterface.hpp"


/**
 * Setup Cylinder
 */
class PDEAdvectionPlaneBenchCylinder	:
		public PDEAdvectionPlaneBench_BaseInterface
{

public:
	bool setupBenchmark(
			sweet::PlaneData_Spectral &o_h_pert,
			sweet::PlaneData_Spectral &o_u,
			sweet::PlaneData_Spectral &o_v
	)
	{

		double sx = shackPlaneDataOps->plane_domain_size[0];
		double sy = shackPlaneDataOps->plane_domain_size[1];


		sweet::PlaneData_Physical h_pert_phys(o_h_pert.planeDataConfig);
		h_pert_phys.physical_set_zero();
		h_pert_phys.physical_update_lambda_array_indices(
			[&](int i, int j, double &io_data)
			{
				double x = (double)i*(shackPlaneDataOps->plane_domain_size[0]/(double)shackPlaneDataOps->space_res_physical[0]);
				double y = (double)j*(shackPlaneDataOps->plane_domain_size[1]/(double)shackPlaneDataOps->space_res_physical[1]);

				// radial dam break
				double dx = x-shackBenchmarks->object_coord_x*sx;
				double dy = y-shackBenchmarks->object_coord_y*sy;

				double radius = shackBenchmarks->object_scale*sqrt(sx*sx+sy*sy);
				if (dx*dx+dy*dy < radius*radius)
					io_data = 1.0;
				else
					io_data = 0.0;
			}
		);

		o_h_pert.loadPlaneDataPhysical(h_pert_phys);
		o_u.spectral_set_zero();
		o_v.spectral_set_zero();

		return true;
	}
};


#endif

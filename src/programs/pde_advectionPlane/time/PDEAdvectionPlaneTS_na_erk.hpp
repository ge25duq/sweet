/*
 * Author: Martin SCHREIBER <schreiberx@gmail.com>
 */

#ifndef SRC_PDE_ADV_PLANE_TS_NA_ERK_HPP_
#define SRC_PDE_ADV_PLANE_TS_NA_ERK_HPP_

#include "PDEAdvectionPlaneTS_BaseInterface.hpp"
#include <sweet/core/ErrorBase.hpp>
#include <sweet/core/time/TimesteppingExplicitRKPlaneData.hpp>
#include <sweet/core/plane/Plane.hpp>

#include <sweet/core/shacks/ShackDictionary.hpp>
#include <sweet/core/shacksShared/ShackTimestepControl.hpp>
#include "ShackPDEAdvectionPlaneTimeDiscretization.hpp"
#include "../benchmarks/ShackPDEAdvectionPlaneBenchmarks.hpp"


class PDEAdvectionPlaneTS_na_erk	:
		public PDEAdvectionPlaneTS_BaseInterface
{
public:
	int timestepping_order;

	sweet::TimesteppingExplicitRKPlaneData timestepping_rk;


public:
	PDEAdvectionPlaneTS_na_erk();

	~PDEAdvectionPlaneTS_na_erk();

	bool setup(sweet::PlaneOperators *io_ops);

public:
	void euler_timestep_update(
			const sweet::PlaneData_Spectral &i_phi,	///< prognostic variables
			const sweet::PlaneData_Spectral &i_vort,	///< prognostic variables
			const sweet::PlaneData_Spectral &i_div,	///< prognostic variables

			sweet::PlaneData_Spectral &o_phi_t,	///< time updates
			sweet::PlaneData_Spectral &o_vort_t,	///< time updates
			sweet::PlaneData_Spectral &o_div_t,	///< time updates

			double i_simulation_timestamp = -1
	);

public:
	void runTimestep(
			sweet::PlaneData_Spectral &io_phi,	///< prognostic variables
			sweet::PlaneData_Spectral &io_vort,	///< prognostic variables
			sweet::PlaneData_Spectral &io_div,	///< prognostic variables

			double i_dt = 0,
			double i_simulation_timestamp = -1
	);
};

#endif

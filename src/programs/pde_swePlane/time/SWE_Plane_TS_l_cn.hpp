/*
 * Author: Martin SCHREIBER <schreiberx@gmail.com>
 */

#ifndef SRC_PROGRAMS_SWE_PLANE_TS_L_CN_HPP_
#define SRC_PROGRAMS_SWE_PLANE_TS_L_CN_HPP_

#include <limits>
#include <sweet/core/plane/PlaneData_Spectral.hpp>
#include <sweet/core/time/TimesteppingExplicitRKPlaneData.hpp>
#include <sweet/core/shacks/ShackDictionary.hpp>
#include <sweet/core/plane/PlaneOperators.hpp>
#include "SWE_Plane_TS_l_erk.hpp"

#include "PDESWEPlaneTS_BaseInterface.hpp"
#include "SWE_Plane_TS_l_erk.hpp"
#include "SWE_Plane_TS_l_irk.hpp"

#include "PDESWEPlaneTS_BaseInterface.hpp"

class SWE_Plane_TS_l_cn	: public PDESWEPlaneTS_BaseInterface
{
	double crank_nicolson_damping_factor = 0.5;

	SWE_Plane_TS_l_erk ts_l_erk;
	SWE_Plane_TS_l_irk ts_l_irk;

public:
	bool shackRegistration(
			sweet::ShackDictionary *io_shackDict
	);

private:
	void backward_euler_timestep_linear(
			sweet::PlaneData_Spectral &io_h,	///< prognostic variables
			sweet::PlaneData_Spectral &io_u,	///< prognostic variables
			sweet::PlaneData_Spectral &io_v,	///< prognostic variables
			double i_dt
	);



public:

	bool setup(sweet::PlaneOperators *io_ops);

	void runTimestep(
			sweet::PlaneData_Spectral &io_h,	///< prognostic variables
			sweet::PlaneData_Spectral &io_u,	///< prognostic variables
			sweet::PlaneData_Spectral &io_v,	///< prognostic variables

			double i_dt = 0,
			double i_simulation_timestamp = -1
	);

	virtual ~SWE_Plane_TS_l_cn() {}
};

#endif

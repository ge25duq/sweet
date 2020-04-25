/*
 * Adv_Sphere_TS_Interface.hpp
 *
 *  Created on: 1st April 2018
 *      Author: Martin Schreiber <SchreiberX@gmail.com>
 */

#ifndef SRC_PROGRAMS_ADV_SPHERE_TS_INTERFACE_HPP_
#define SRC_PROGRAMS_ADV_SPHERE_TS_INTERFACE_HPP_

#include <sweet/sphere/SphereData_Spectral.hpp>
#include <sweet/sphere/SphereOperators_SphereData.hpp>
#include <limits>
#include <sweet/SimulationVariables.hpp>


class Adv_Sphere_TS_interface
{
public:
	virtual void run_timestep(
			SphereData_Spectral &io_h,	///< prognostic variables
			SphereData_Spectral &io_u,	///< prognostic variables
			SphereData_Spectral &io_v,	///< prognostic variables

			double i_fixed_dt,		///< if this value is not equal to 0, use this time step size instead of computing one
			double i_simulation_timestamp,

			// for varying velocity fields, set to nullptr if velocity fields shouldn't be updated
			const SWESphereBenchmarksCombined *i_sphereBenchmarks,
			SphereData_Physical &io_U_phi_phys
	) = 0;
};

#endif /* SRC_PROGRAMS_ADV_PLANE_REXI_ADV_PLANE_TS_LN_ERK_HPP_ */

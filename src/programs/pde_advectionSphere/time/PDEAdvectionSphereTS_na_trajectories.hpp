/*
 * Author: Martin SCHREIBER <schreiberx@gmail.com>
 */

#ifndef SRC_PROGRAMS_ADV_SPHERE_REXI_ADV_SPHERE_TS_NA_TRAJECTORIES_HPP_
#define SRC_PROGRAMS_ADV_SPHERE_REXI_ADV_SPHERE_TS_NA_TRAJECTORIES_HPP_


#include "../PDEAdvectionSphereBenchmarksCombined.hpp"
#include <limits>
#include <sweet/core/shacks/ShackDictionary.hpp>
#include <sweet/core/sphere/SphereData_Spectral.hpp>
#include <sweet/core/sphere/SphereOperators_Sampler_SphereDataPhysical.hpp>
#include <sweet/core/sphere/SphereOperators.hpp>
#include <sweet/core/time/TimesteppingSemiLagrangianSphereData.hpp>
#include "PDEAdvectionSphereTS_BaseInterface.hpp"


/*
 * This time integration method is based on given benchmark solutions
 *
 * See e.g. R. Nair, P. Lauritzen
 * "A class of deformational flow test cases for linear transport problems on the sphere"
 */
class PDEAdvectionSphereTS_na_trajectories	:
		public PDEAdvectionSphereTS_BaseInterface
{
	int timestepping_order;

	sweet::TimesteppingSemiLagrangianSphereData semiLagrangian;
	sweet::SphereOperators_Sampler_SphereDataPhysical sphereSampler;

	sweet::SphereData_Spectral U_phi_prev;
	sweet::SphereData_Physical U_u_prev, U_v_prev;

public:
	bool testImplementsTimesteppingMethod(const std::string &i_timestepping_method);

	std::string getStringId();

	void printImplementedTimesteppingMethods(
			std::ostream &o_ostream,
			const std::string &i_prefix
	);

public:
	bool setup(
			sweet::SphereOperators *io_ops
	);

private:
	void runTimestep(
			std::vector<sweet::SphereData_Spectral> &io_prognostic_fields,	///< prognostic variables
			sweet::SphereData_Physical &io_u,
			sweet::SphereData_Physical &io_v,

			double i_fixed_dt,
			double i_simulation_timestamp
	);

	void run_timestep_1(
			sweet::SphereData_Spectral &io_prognostic_field,	///< prognostic variables
			sweet::SphereData_Physical &io_u,
			sweet::SphereData_Physical &io_v,

			double i_fixed_dt,
			double i_simulation_timestamp
	);



	virtual ~PDEAdvectionSphereTS_na_trajectories();
};

#endif

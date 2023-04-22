/*
 * Author: Martin SCHREIBER <schreiberx@gmail.com>
 */

#ifndef SRC_PROGRAMS_SWE_SPHERE_REXI_SWE_SPHERE_TS_LG_IRK_LC_NA_ERK_VD_HPP_
#define SRC_PROGRAMS_SWE_SPHERE_REXI_SWE_SPHERE_TS_LG_IRK_LC_NA_ERK_VD_HPP_

#include <sweet/core/sphere/SphereData_Spectral.hpp>
#include <sweet/core/sphere/SphereOperators.hpp>
#include <sweet/core/time/TimesteppingExplicitRKSphereData.hpp>
#include <limits>
#include <sweet/core/shacks/ShackDictionary.hpp>

#include "PDESWESphereTS_BaseInterface.hpp"
#include "PDESWESphereTS_lg_irk.hpp"
#include "PDESWESphereTS_ln_erk_split_vd.hpp"



class PDESWESphereTS_lg_irk_lc_na_erk_vd	: public PDESWESphereTS_BaseInterface
{
public:
	bool setup_auto(
			const std::string &i_timestepping_method,
			sweet::SphereOperators *io_ops
		) override;

	bool setup(
			sweet::SphereOperators *io_ops,
			int i_order,	///< order of RK time stepping method for linear parts
			int i_order2,	///< order of RK time stepping method for non-linear parts
			int i_version_id
	);

public:
	bool implementsTimesteppingMethod(const std::string &i_timestepping_method) override;
	std::string getIDString() override;

	double timestep_size;

	/*
	 * Linear time steppers
	 */
	PDESWESphereTS_lg_irk timestepping_lg_irk;

	/*
	 * Non-linear time steppers
	 */
	PDESWESphereTS_ln_erk_split_vd timestepping_ln_erk_split_vd;

	sweet::TimesteppingExplicitRKSphereData timestepping_rk_nonlinear;

	int version_id;


public:
	bool shackRegistration(
			sweet::ShackDictionary *io_shackDict
	) override
	{
		PDESWESphereTS_BaseInterface::shackRegistration(io_shackDict);

		timestepping_lg_irk.shackRegistration(io_shackDict);
		timestepping_ln_erk_split_vd.shackRegistration(io_shackDict);
		return true;
	}


public:
	PDESWESphereTS_lg_irk_lc_na_erk_vd();

	void runTimestep(
			sweet::SphereData_Spectral &io_phi_pert,
			sweet::SphereData_Spectral &io_vort,
			sweet::SphereData_Spectral &io_div,

			double i_fixed_dt = 0,
			double i_simulation_timestamp = -1
	) override;



	virtual ~PDESWESphereTS_lg_irk_lc_na_erk_vd();
};

#endif
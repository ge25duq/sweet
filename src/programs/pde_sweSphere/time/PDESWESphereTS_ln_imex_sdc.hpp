/*
 * PDESWESphereTS_ln_imex_sdc.hpp
 *
 *  Created on: 7 March 2023
 *      Author: Thibaut LUNET <thibaut.lunet@tuhh.de>
 */

#ifndef SRC_PROGRAMS_SWE_SPHERE_REXI_SWE_SPHERE_TS_LN_IMEX_SDC_HPP_
#define SRC_PROGRAMS_SWE_SPHERE_REXI_SWE_SPHERE_TS_LN_IMEX_SDC_HPP_

#include <sweet/core/sphere/SphereData_Spectral.hpp>
#include <sweet/core/sphere/SphereOperators.hpp>
#include <sweet/core/time/TimesteppingExplicitRKSphereData.hpp>
#include <sweet/core/shacks/ShackDictionary.hpp>

#include <sweet/sdc/ShackSDC.hpp>
#include <sweet/sdc/Storage.hpp>
#include <vector>

#include <sweet/core/dict/DictArrayND.hpp>
#include "PDESWESphereTS_BaseInterface.hpp"
#include "PDESWESphereTS_l_erk_n_erk.hpp"
#include "PDESWESphereTS_l_irk.hpp"


class PDESWESphereTS_ln_imex_sdc	: public PDESWESphereTS_BaseInterface
{
public:
	bool setup_auto(
			const std::string &i_timestepping_method,
			sweet::SphereOperators *io_ops
		) override;

public:
	bool implementsTimesteppingMethod(const std::string &i_timestepping_method) override;
	std::string getIDString() override;

	sweet::ShackSDC* shackSDC;


	/*
	 * Time-steppers for implicit solve (linear) used during sweeps
	 */
	std::vector<PDESWESphereTS_l_irk*> timestepping_l_irk;

	/*
	 * Time-steppers for implicit solve (linear) used during eventual initial sweep
	 */
	std::vector<PDESWESphereTS_l_irk*> timestepping_l_irk_init;

	/*
	 * Time-stepper for tendencies evaluation (linear and non linear)
	 */
	PDESWESphereTS_l_erk_n_erk timestepping_l_erk_n_erk;

	int timestepping_order;


public:
	bool shackRegistration(
			sweet::ShackDictionary *io_shackDict
	) override
	{
		PDESWESphereTS_BaseInterface::shackRegistration(io_shackDict);

		// SDC shack registration
		shackSDC = io_shackDict->getAutoRegistration<sweet::ShackSDC>();
		ERROR_CHECK_WITH_FORWARD_AND_COND_RETURN_BOOLEAN(*io_shackDict);

		return true;
	}

private:
	/*
	 * SDC specific attributes
 	 */
	size_t nNodes=0;  // default value should be 0 for clear and destructor
	size_t nIter;

	std::string initialSweepType;  // Type of initial sweep

	bool diagonal;      // Whether or not using the diagonal implementation
	bool useEndUpdate;  // Whether or not use collocation update for end point

	typedef sweet::DictArrayND<1, double> Vec;
	typedef sweet::DictArrayND<2, double> Mat;

	Vec tau;
	Vec weights;
	Mat qMat;
	Mat qMatDeltaI;
	Mat qMatDeltaE;
	Mat qMatDelta0;

	// Wether or not activate parallel computation for diagonal sweeps
	bool parallel=false;

	/*
	 * Variables used as temporary storage locations during the time step
	 */

	// To store pointers to initial time steps
	SWE_VariableVector ts_u0;

	SDC_NodeStorage ts_linear_tendencies_k0;  		// linear term evaluations
	SDC_NodeStorage ts_nonlinear_tendencies_k0;	// non-linear term evaluations
	SDC_NodeStorage ts_linear_tendencies_k1;  		// linear term evaluations
	SDC_NodeStorage ts_nonlinear_tendencies_k1;	// non-linear term evaluations


	// start of current time step
	double t0;

	// timestep size
	double dt;

	// unique string id
	std::string idString;

	/*
	 * SDC specific methods
 	 */

	// Wrapper evaluating linear terms and storing them in separate variables (eval)
	void eval_linear(const SWE_VariableVector& u, SWE_VariableVector& eval, double t=-1);

	// Wrapper evaluating non-linear terms and storing them in separate variables (eval)
	void eval_nonlinear(const SWE_VariableVector& u, SWE_VariableVector& eval, double t=-1);

	/* Wrapper solving the implicit system built from the linear term :
	 u - dt*L(u) = rhs
	 WARNING : rhs variables are overwritten with u 
	*/ 
	void solveImplicit(SWE_VariableVector& rhs, double dt, int iNode, bool initSweep=false);

	// Perform y <- a*x + y
	void axpy(double a, const SWE_VariableVector& x, SWE_VariableVector& io_y);

	// Initialize nodes values
	void init_sweep();

	// Perform one sweep
	void sweep(size_t k);


public:
	PDESWESphereTS_ln_imex_sdc() : PDESWESphereTS_BaseInterface(), shackSDC(nullptr) {};

	void runTimestep(
			sweet::SphereData_Spectral &io_phi_pert,
			sweet::SphereData_Spectral &io_vort,
			sweet::SphereData_Spectral &io_div,

			double i_fixed_dt = 0,
			double i_simulation_timestamp = -1
	) override;

	bool clear();

	virtual ~PDESWESphereTS_ln_imex_sdc();
};

#endif // SRC_PROGRAMS_SWE_SPHERE_REXI_SWE_SPHERE_TS_LN_IMEX_SDC_HPP_

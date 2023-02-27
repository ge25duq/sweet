/*
 * SWE_Plane_TimeSteppers.hpp
 *
 *  Created on: 29 May 2017
 *      Author: Martin SCHREIBER <schreiberx@gmail.com>
 */

#ifndef SRC_PROGRAMS_SWE_PLANE_REXI_SWE_PLANE_TIMESTEPPERS_HPP_
#define SRC_PROGRAMS_SWE_PLANE_REXI_SWE_PLANE_TIMESTEPPERS_HPP_

#include "SWE_Plane_TS_interface.hpp"
#include "SWE_Plane_TS_l_cn.hpp"
#include "SWE_Plane_TS_l_cn_n_erk.hpp"
#include "SWE_Plane_TS_l_cn_na_sl_nd_settls.hpp"
#include "SWE_Plane_TS_l_direct.hpp"
#include "SWE_Plane_TS_l_erk.hpp"
#include "SWE_Plane_TS_l_erk_n_erk.hpp"
#include "SWE_Plane_TS_l_irk.hpp"
#include "SWE_Plane_TS_l_irk_n_erk.hpp"
#include "SWE_Plane_TS_l_rexi.hpp"
#include "SWE_Plane_TS_l_rexi_n_erk.hpp"
#include "SWE_Plane_TS_l_rexi_n_etdrk.hpp"
#include "SWE_Plane_TS_l_rexi_na_sl_nd_etdrk.hpp"
#include "SWE_Plane_TS_l_rexi_na_sl_nd_settls.hpp"
#include "SWE_Plane_TS_ln_erk.hpp"


/**
 * SWE Plane time steppers
 */
class PDESWEPlaneTimeSteppers
{
public:
	SWE_Plane_TS_ln_erk *ln_erk = nullptr;
	SWE_Plane_TS_l_rexi_n_etdrk *l_rexi_n_etdrk = nullptr;
	SWE_Plane_TS_l_erk *l_erk = nullptr;
	SWE_Plane_TS_l_cn *l_cn = nullptr;
	SWE_Plane_TS_l_erk_n_erk *l_erk_n_erk = nullptr;
	SWE_Plane_TS_l_cn_n_erk *l_cn_n_erk = nullptr;
	SWE_Plane_TS_l_rexi_n_erk *l_rexi_n_erk = nullptr;
	SWE_Plane_TS_l_irk *l_irk = nullptr;
	SWE_Plane_TS_l_rexi *l_rexi = nullptr;
	SWE_Plane_TS_l_direct *l_direct = nullptr;
	SWE_Plane_TS_l_rexi_na_sl_nd_settls *l_rexi_na_sl_nd_settls = nullptr;
	SWE_Plane_TS_l_cn_na_sl_nd_settls *l_cn_na_sl_nd_settls = nullptr;
	SWE_Plane_TS_l_rexi_na_sl_nd_etdrk *l_rexi_na_sl_nd_etdrk = nullptr;
	SWE_Plane_TS_l_irk_n_erk *l_irk_n_erk = nullptr;

	SWE_Plane_TS_interface *master = nullptr;

	bool linear_only = false;

	PDESWEPlaneTimeSteppers()
	{
	}

	void clear()
	{
		if (ln_erk != nullptr)
		{
			delete ln_erk;
			ln_erk = nullptr;
		}

		if (l_rexi_n_etdrk != nullptr)
		{
			delete l_rexi_n_etdrk;
			l_rexi_n_etdrk = nullptr;
		}

		if (l_erk != nullptr)
		{
			delete l_erk;
			l_erk = nullptr;
		}

		if (l_cn != nullptr)
		{
			delete l_cn;
			l_cn = nullptr;
		}

		if (l_erk_n_erk != nullptr)
		{
			delete l_erk_n_erk;
			l_erk_n_erk = nullptr;
		}

		if (l_cn_n_erk != nullptr)
		{
			delete l_cn_n_erk;
			l_cn_n_erk = nullptr;
		}

		if (l_rexi_n_erk != nullptr)
		{
			delete l_rexi_n_erk;
			l_rexi_n_erk = nullptr;
		}

		if (l_irk != nullptr)
		{
			delete l_irk;
			l_irk = nullptr;
		}

		if (l_rexi != nullptr)
		{
			delete l_rexi;
			l_rexi = nullptr;
		}

		if (l_direct != nullptr)
		{
			delete l_direct;
			l_direct = nullptr;
		}

		if (l_rexi_na_sl_nd_settls != nullptr)
		{
			delete l_rexi_na_sl_nd_settls;
			l_rexi_na_sl_nd_settls = nullptr;
		}
		if (l_rexi_na_sl_nd_etdrk != nullptr)
		{
			delete l_rexi_na_sl_nd_etdrk;
			l_rexi_na_sl_nd_etdrk = nullptr;
		}
		if (l_cn_na_sl_nd_settls != nullptr)
		{
			delete l_cn_na_sl_nd_settls;
			l_cn_na_sl_nd_settls = nullptr;
		}

		if (l_irk_n_erk != nullptr)
		{
			delete l_irk_n_erk;
			l_irk_n_erk = nullptr;
		}
	}

	void setup(
			const std::string &i_timestepping_method,
			int &i_timestepping_order,
			int &i_timestepping_order2,

			sweet::PlaneOperators &i_op,
			sweet::ShackDictionary *io_shackDict
	)
	{
		clear();

		/// Always allocate analytical solution
		l_direct = new SWE_Plane_TS_l_direct(io_shackDict, i_op);
		l_direct->setup("phi0");

		if (i_timestepping_method == "l_ld_na_erk")
		{
			ln_erk = new SWE_Plane_TS_ln_erk(io_shackDict, i_op);
			ln_erk->setup(i_timestepping_order, true);

			master = &(SWE_Plane_TS_interface&)*ln_erk;

			linear_only = false;
		}
		if (i_timestepping_method == "ln_erk")
		{
			ln_erk = new SWE_Plane_TS_ln_erk(io_shackDict, i_op);
			ln_erk->setup(i_timestepping_order, false);

			master = &(SWE_Plane_TS_interface&)*ln_erk;

			linear_only = false;
		}
		else if (i_timestepping_method == "l_rexi_ld_na_etdrk")
		{
			l_rexi_n_etdrk = new SWE_Plane_TS_l_rexi_n_etdrk(io_shackDict, i_op);
			l_rexi_n_etdrk->setup(
					io_shackDict.rexi,
					io_shackDict.disc.timestepping_order,
					true
				);

			master = &(SWE_Plane_TS_interface&)*l_rexi_n_etdrk;

			linear_only = false;
		}
		else if (i_timestepping_method == "l_rexi_n_etdrk")
		{
			l_rexi_n_etdrk = new SWE_Plane_TS_l_rexi_n_etdrk(io_shackDict, i_op);
			l_rexi_n_etdrk->setup(
					io_shackDict.rexi,
					io_shackDict.disc.timestepping_order,
					false
				);


			master = &(SWE_Plane_TS_interface&)*l_rexi_n_etdrk;

			linear_only = false;
		}
		else if (i_timestepping_method == "l_cn")
		{

			l_cn= new SWE_Plane_TS_l_cn(io_shackDict, i_op);
			l_cn->setup(io_shackDict.disc.timestepping_crank_nicolson_filter);

			master = &(SWE_Plane_TS_interface&)*l_cn;

			linear_only = true;
		}
		else if (i_timestepping_method == "l_erk")
		{

			l_erk = new SWE_Plane_TS_l_erk(io_shackDict, i_op);
			l_erk->setup(i_timestepping_order);

			master = &(SWE_Plane_TS_interface&)*l_erk;

			linear_only = true;
		}
		else if (i_timestepping_method == "l_erk_na_nd2_erk")
		{
			/*
			 * Special case which treats div(u*h) as u.div(h)
			 */
			l_erk_n_erk = new SWE_Plane_TS_l_erk_n_erk(io_shackDict, i_op);
			l_erk_n_erk->setup(i_timestepping_order, i_timestepping_order2, true);

			master = &(SWE_Plane_TS_interface&)*l_erk_n_erk;
		}
		else if (i_timestepping_method == "l_erk_n_erk")
		{
			l_erk_n_erk = new SWE_Plane_TS_l_erk_n_erk(io_shackDict, i_op);
			l_erk_n_erk->setup(i_timestepping_order, i_timestepping_order2);

			master = &(SWE_Plane_TS_interface&)*l_erk_n_erk;
		}
		else if (i_timestepping_method == "l_cn_na_nd2_erk")
		{
			/*
			 * Special case which treats div(u*h) as u.div(h)
			 */
			l_cn_n_erk = new SWE_Plane_TS_l_cn_n_erk(io_shackDict, i_op);
			l_cn_n_erk->setup(i_timestepping_order, i_timestepping_order2, io_shackDict.disc.timestepping_crank_nicolson_filter, true);

			master = &(SWE_Plane_TS_interface&)*l_cn_n_erk;

			linear_only = false;
		}
		else if (i_timestepping_method == "l_cn_n_erk")
		{

			l_cn_n_erk = new SWE_Plane_TS_l_cn_n_erk(io_shackDict, i_op);
			l_cn_n_erk->setup(i_timestepping_order, i_timestepping_order2, io_shackDict.disc.timestepping_crank_nicolson_filter, false);

			master = &(SWE_Plane_TS_interface&)*l_cn_n_erk;

			linear_only = false;
		}
		else if (i_timestepping_method == "l_rexi_ld_na_erk")
		{

			l_rexi_n_erk = new SWE_Plane_TS_l_rexi_n_erk(io_shackDict, i_op);
			l_rexi_n_erk->setup(
					io_shackDict.rexi,
					i_timestepping_order2,
					true
				);

			master = &(SWE_Plane_TS_interface&)*l_rexi_n_erk;

			linear_only = false;
		}
		else if (i_timestepping_method == "l_rexi_n_erk")
		{

			l_rexi_n_erk = new SWE_Plane_TS_l_rexi_n_erk(io_shackDict, i_op);
			l_rexi_n_erk->setup(
					io_shackDict.rexi,
					i_timestepping_order2,
					false
				);

			master = &(SWE_Plane_TS_interface&)*l_rexi_n_erk;

			linear_only = false;
		}
		else if (i_timestepping_method == "l_irk")
		{
			if (io_shackDict.disc.space_grid_use_c_staggering)
				SWEETError("Staggering not supported for l_irk");

			l_irk = new SWE_Plane_TS_l_irk(io_shackDict, i_op);
			l_irk->setup(i_timestepping_order);

			master = &(SWE_Plane_TS_interface&)*l_irk;

			linear_only = true;
		}
		else if (i_timestepping_method == "l_rexi")
		{
			if (io_shackDict.disc.space_grid_use_c_staggering)
				SWEETError("Staggering not supported for l_rexi");

			l_rexi = new SWE_Plane_TS_l_rexi(io_shackDict, i_op);
			l_rexi->setup(
					io_shackDict.rexi, "phi0", io_shackDict.timecontrol.current_timestep_size
				);
/*

			if (io_shackDict.misc.verbosity > 2)
			{
				std::cout << "ALPHA:" << std::endl;
				for (std::size_t n = 0; n < l_rexi->rexi.alpha.size(); n++)
					std::cout << l_rexi->rexi.alpha[n] << std::endl;

				std::cout << "BETA:" << std::endl;
				for (std::size_t n = 0; n < l_rexi->rexi.beta_re.size(); n++)
					std::cout << l_rexi->rexi.beta_re[n] << std::endl;
			}
*/

			master = &(SWE_Plane_TS_interface&)*l_rexi;

			linear_only = true;
		}
		else if (i_timestepping_method == "l_rexi_na_sl_ld_settls")
		{

			l_rexi_na_sl_nd_settls = new SWE_Plane_TS_l_rexi_na_sl_nd_settls(io_shackDict, i_op);
			l_rexi_na_sl_nd_settls->setup(true);

			master = &(SWE_Plane_TS_interface&)*l_rexi_na_sl_nd_settls;

			linear_only = false;
		}
		else if (i_timestepping_method == "l_rexi_na_sl_nd_settls")
		{

			l_rexi_na_sl_nd_settls = new SWE_Plane_TS_l_rexi_na_sl_nd_settls(io_shackDict, i_op);
			l_rexi_na_sl_nd_settls->setup(false);

			master = &(SWE_Plane_TS_interface&)*l_rexi_na_sl_nd_settls;

			linear_only = false;
		}
		else if (i_timestepping_method == "l_rexi_na_sl_ld_etdrk")
		{
			l_rexi_na_sl_nd_etdrk = new SWE_Plane_TS_l_rexi_na_sl_nd_etdrk(io_shackDict, i_op);
			l_rexi_na_sl_nd_etdrk->setup(io_shackDict.disc.timestepping_order, true);

			master = &(SWE_Plane_TS_interface&)*l_rexi_na_sl_nd_etdrk;

			linear_only = false;
		}
		else if (i_timestepping_method == "l_rexi_na_sl_nd_etdrk")
		{
			l_rexi_na_sl_nd_etdrk = new SWE_Plane_TS_l_rexi_na_sl_nd_etdrk(io_shackDict, i_op);
			l_rexi_na_sl_nd_etdrk->setup(io_shackDict.disc.timestepping_order, false);

			master = &(SWE_Plane_TS_interface&)*l_rexi_na_sl_nd_etdrk;

			linear_only = false;
		}
		else if (i_timestepping_method == "l_cn_na_sl_ld2_settls")
		{
			l_cn_na_sl_nd_settls = new SWE_Plane_TS_l_cn_na_sl_nd_settls(io_shackDict, i_op);

			l_cn_na_sl_nd_settls->setup(true);

			master = &(SWE_Plane_TS_interface&)*l_cn_na_sl_nd_settls;

			linear_only = false;
		}
		else if (i_timestepping_method == "l_cn_na_sl_nd_settls")
		{

			l_cn_na_sl_nd_settls = new SWE_Plane_TS_l_cn_na_sl_nd_settls(io_shackDict, i_op);

			l_cn_na_sl_nd_settls->setup(false);

			master = &(SWE_Plane_TS_interface&)*l_cn_na_sl_nd_settls;

			linear_only = false;
		}
		else if (i_timestepping_method == "l_irk_ld_n_erk")
		{

			l_irk_n_erk = new SWE_Plane_TS_l_irk_n_erk(io_shackDict, i_op);

			l_irk_n_erk->setup(
					i_timestepping_order,
					i_timestepping_order2,
					true
				);

			master = &(SWE_Plane_TS_interface&)*l_irk_n_erk;

			linear_only = false;
		}
		else if (i_timestepping_method == "l_irk_n_erk")
		{

			l_irk_n_erk = new SWE_Plane_TS_l_irk_n_erk(io_shackDict, i_op);

			l_irk_n_erk->setup(
					i_timestepping_order,
					i_timestepping_order2,
					false
				);

			master = &(SWE_Plane_TS_interface&)*l_irk_n_erk;

			linear_only = false;
		}
		else if (i_timestepping_method == "l_direct")
		{
			master = &(SWE_Plane_TS_interface&)*l_direct;

			linear_only = true;
		}
		else //Help menu with list of schemes
		{
			std::cout << "Unknown method: " << i_timestepping_method << std::endl;
			std::cout << "Available --timestepping-method :"  << std::endl;
			std::cout << "      l_direct       : Linear:     Analytical solution to linear SW operator"  << std::endl;
			std::cout << "      l_erk          : Linear:     Explicit RK scheme (supports FD-C staggering)"  << std::endl;
			std::cout << "      l_erk_n_erk    : Non-linear: Linear RK, Non-linear RK, Strang-split"  << std::endl;
			std::cout << "      l_cn           : Linear:     Crank-Nicolson (CN) scheme"  << std::endl;
			std::cout << "      l_cn_n_erk     : Non-linear: Linear CN, Non-linear RK, Strang-split" << std::endl;
			std::cout << "      l_rexi         : Linear:     Pure REXI, our little dog." << std::endl;
			std::cout << "      l_rexi_n_erk   : Non-linear: Linear REXI, Non-linear RK, Strang-split" << std::endl;
			std::cout << "      l_irk          : Linear:     Implicit Euler"  << std::endl;
			std::cout << "      l_irk_n_erk    : Non-linear: Linear Implicit Euler, Non-linear RK, Strang-split"  << std::endl;
			std::cout << "      ln_erk         : Non-linear: Linear and nonlinear solved jointly with erk (supports FD-C staggering)"  << std::endl;
			std::cout << "      l_rexi_na_sl_nd_settls   : Non-linear: Linear Rexi, Advection: Semi-Lag, Nonlinear-diverg: SETTLS"  << std::endl;
			std::cout << "      l_cn_na_sl_nd_settls     : Non-linear: Linear CN, Advection: Semi-Lag, Nonlinear-diverg: SETTLS"  << std::endl;
			std::cout << "      l_rexi_n_etdrk           : Non-linear: Linear REXI, Non-linear: ETDRK"  << std::endl;
			std::cout << "      l_rexi_na_sl_nd_etdrk    : Non-linear: Linear REXI, Advection: Semi-Lag, Nonlinear-diverg: ETDRK"  << std::endl;

			SWEETError("No valid --timestepping-method provided");
		}
	}



	~SWE_Plane_TimeSteppers()
	{
		clear();
	}
};




#endif

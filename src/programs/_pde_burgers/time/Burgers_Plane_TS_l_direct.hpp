/*
 * Burgers_Plane_TS_l_direct.hpp
 *
 *  Created on: 09 August 2017
 */

#ifndef SRC_PROGRAMS_BURGERS_PLANE_TS_L_DIRECT_HPP_
#define SRC_PROGRAMS_BURGERS_PLANE_TS_L_DIRECT_HPP_

#include <limits>
#include <sweet/core/plane/sweet::PlaneData_Spectral.hpp>
#include <sweet/core/plane/PlaneDataSampler.hpp>
#include <sweet/core/shacks/ShackDictionary.hpp>
#include <sweet/core/plane/PlaneOperators.hpp>
#include <sweet/core/plane/PlaneDataGridMapping.hpp>
#include <sweet/core/plane/PlaneStaggering.hpp>

#include "../burgers_timeintegrators/Burgers_Plane_TS_interface.hpp"



#define BURGERS_PLANE_TS_L_DIRECT_QUADPRECISION	0


#if BURGERS_PLANE_TS_L_DIRECT_QUADPRECISION
	#if SWEET_QUADMATH
		#include <quadmath.h>
	#else
		#error "Quad precision not activated"
	#endif
#endif

class Burgers_Plane_TS_l_direct	: public Burgers_Plane_TS_interface
{
	sweet::ShackDictionary &shackDict;
	PlaneOperators &op;

	PlaneDataGridMapping planeDataGridMapping;


#if BURGERS_PLANE_TS_L_DIRECT_QUADPRECISION

	typedef __float128 T;

	static std::complex<T> l_expcplx(std::complex<T> &i_value)
	{
		__complex128 z;
		__real__ z = i_value.real();
		__imag__ z = i_value.imag();

		__complex128 val = cexpq(z);

		return std::complex<double>(crealq(val), cimagq(val));
	}

	inline
	static T l_sqrt(T &i_value)
	{
		return sqrtq(i_value);
	}

	static const std::complex<T> l_sqrtcplx(const std::complex<T> &i_value)
	{
		__complex128 z;
		__real__ z = i_value.real();
		__imag__ z = i_value.imag();

		__complex128 val = csqrtq(z);

		return std::complex<double>(crealq(val), cimagq(val));
	}

	inline
	static T pi2()
	{
		static char *sp;
		static T retval = (T)2.0*strtoflt128("3.1415926535897932384626433832795029", &sp);
		return retval;
	}

#else

	/*
	 * Double precision
	 * Might suffer of numerical double precision limited effects
	 */
	typedef double T;

	std::complex<T> l_expcplx(std::complex<T> &i_value)
	{
		return std::exp(i_value);
	};

	T l_sqrt(T &i_value)
	{
		return l_sqrt(i_value);
	};

	std::complex<T> l_sqrtcplx(std::complex<T> &i_value)
	{
		return std::exp(i_value);
	};


	static T pi2()
	{
		return (T)2.0*(T)M_PI;
	}

#endif


public:
	Burgers_Plane_TS_l_direct(
			sweet::ShackDictionary &i_shackDict,
			PlaneOperators &i_op
		);


	void runTimestep(
			sweet::PlaneData_Spectral &io_u,	///< prognostic variables
			sweet::PlaneData_Spectral &io_v,	///< prognostic variables
			///sweet::PlaneData_Spectral &io_u_prev,	///< prognostic variables
			///sweet::PlaneData_Spectral &io_v_prev,	///< prognostic variables

			double i_dt = 0,
			double i_simulation_timestamp = -1
	);


	void run_timestep_cgrid(
			sweet::PlaneData_Spectral &io_u,		///< prognostic variables
			sweet::PlaneData_Spectral &io_v,		///< prognostic variables

			double i_dt,
			double i_simulation_timestamp
	);


	void run_timestep_agrid(
			sweet::PlaneData_Spectral &io_u,	///< prognostic variables
			sweet::PlaneData_Spectral &io_v,	///< prognostic variables

			double i_dt = 0,
			double i_simulation_timestamp = -1
	);


	void run_timestep_agrid_planedata(
			sweet::PlaneData_Spectral &io_u,	///< prognostic variables
			sweet::PlaneData_Spectral &io_v,	///< prognostic variables

			double i_dt = 0,
			double i_simulation_timestamp = -1
	);



	void run_timestep_agrid_planedatacomplex(
			sweet::PlaneData_Spectral &io_u,	///< prognostic variables
			sweet::PlaneData_Spectral &io_v,	///< prognostic variables

			double i_dt = 0,
			double i_simulation_timestamp = -1
	);

	void setup(
	);

	virtual ~Burgers_Plane_TS_l_direct();
};

#endif

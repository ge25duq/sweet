/*
 * Author: Martin SCHREIBER <schreiberx@gmail.com>
 */

#ifndef SRC_SPHERE_ADVECTION_TIMESTEPPERS_HPP_
#define SRC_SPHERE_ADVECTION_TIMESTEPPERS_HPP_


#include "time/PDEAdvectionSphereTS_BaseInterface.hpp"
#include <sweet/core/ErrorBase.hpp>


/**
 * SWE Plane time steppers
 */
class PDEAdvectionSphereTimeSteppers
{
public:
	sweet::ErrorBase error;
	PDEAdvectionSphereTS_BaseInterface *timestepper = nullptr;

private:
	std::vector<PDEAdvectionSphereTS_BaseInterface*> _registered_integrators;


public:
	void setup_1_registerAllTimesteppers();

private:
	void _timesteppersFreeAll(
			PDEAdvectionSphereTS_BaseInterface *skip_this = nullptr
		);

public:
	PDEAdvectionSphereTimeSteppers();

	void printImplementedTimesteppingMethods(
		std::ostream &o_ostream = std::cout,
		const std::string &i_prefix = ""
	);

	bool setup_2_shackRegistration(
			sweet::ShackDictionary *io_shackDict
	);

	bool setup_3_timestepper(
			const std::string &i_timestepping_method,
			sweet::ShackDictionary *i_shackDict,
			sweet::SphereOperators *io_ops
	);

	void clear();


	~PDEAdvectionSphereTimeSteppers();
};


#endif

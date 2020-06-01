/*
 * Author: Martin Schreiber <SchreiberX@Gmail.com>
 */

#ifndef SRC_BENCHMARKS_SPHERE_VECTOR_ADVECTION_GAUSS_BUMP_HPP_
#define SRC_BENCHMARKS_SPHERE_VECTOR_ADVECTION_GAUSS_BUMP_HPP_

#include "BenchmarksSphereAdvection_interface.hpp"
#include <ostream>
#include <sweet/SWEETMath.hpp>
#include <sweet/SimulationVariables.hpp>
#include <sweet/sphere/SphereOperators_SphereData.hpp>

#include "../swe_sphere_benchmarks/SWESphereBenchmark_williamson_1_advection_gauss_bump.hpp"


class BenchmarksSphereAdvection_vector_advection_gauss_bump	: public BenchmarksSphereAdvection_interface
{
	SimulationVariables *simVars = nullptr;
	SphereOperators_SphereData *ops = nullptr;

	SWESphereBenchmark_williamson_1_advection_gauss_bump benchmark;

public:
	BenchmarksSphereAdvection_vector_advection_gauss_bump()
	{
	}

	bool implements_benchmark(
			const std::string &i_benchmark_name
		)
	{
		return (
				i_benchmark_name == "vector_advection_gauss_bump"	||
				false
			);
	}


	void setup(
			SimulationVariables *i_simVars,
			SphereOperators_SphereData *i_ops
	)
	{
		simVars = i_simVars;
		ops = i_ops;

		benchmark.setup(i_simVars, i_ops);
	}



	std::string get_help()
	{
		std::ostringstream stream;

		stream << " * VECTORIAL ADVECTION TEST CASES:" << std::endl;
		stream << "    + 'vector_advection_gauss_bump'" << std::endl;

		return stream.str();
	}


	/*
	 * Return number of prognostic fields to be used
	 */
	int get_num_prognostic_fields()
	{
		return 3;
	}



	void get_initial_state(
		std::vector<SphereData_Spectral*> &o_prognostic_fields,
		SphereData_Spectral &o_vrt,
		SphereData_Spectral &o_div
	)
	{
		SWEETDebugAssert(o_prognostic_fields.size() == 3, "Only a vectorial field (3 elements) supported for this benchmark!");

		const SphereData_Config *sphereDataConfig = o_prognostic_fields[0]->sphereDataConfig;

		SphereData_Spectral tmp(sphereDataConfig);
		benchmark.get_initial_state(tmp, o_vrt, o_div);

		/*
		 * Setup prognostic fields to k vector (perpendicular to point on sphere)
		 */

		SphereData_Physical x(sphereDataConfig);
		SphereData_Physical y(sphereDataConfig);
		SphereData_Physical z(sphereDataConfig);

		x.physical_update_lambda(
				[&](double lon, double lat, double &o_data)
				{
					double ret[3];
					SWEETMath::latlon_to_cartesian(lon, lat, ret);
					o_data = ret[0];
				}
		);

		y.physical_update_lambda(
				[&](double lon, double lat, double &o_data)
				{
					double ret[3];
					SWEETMath::latlon_to_cartesian(lon, lat, ret);
					o_data = ret[1];
				}
		);

		z.physical_update_lambda(
				[&](double lon, double lat, double &o_data)
				{
					double ret[3];
					SWEETMath::latlon_to_cartesian(lon, lat, ret);
					o_data = ret[2];
				}
		);



		*o_prognostic_fields[0] = x;
		*o_prognostic_fields[1] = y;
		*o_prognostic_fields[2] = z;
	}
};

#endif
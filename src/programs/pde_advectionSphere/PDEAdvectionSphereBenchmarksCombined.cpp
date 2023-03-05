/*
 * Author: Martin SCHREIBER <schreiberx@gmail.com>
 */

#include "PDEAdvectionSphereBenchmarksCombined.hpp"

#include "benchmarks/PDEAdvectionSphereBenchmark_advection_vector_uv_velocities.hpp"
#include "benchmarks/PDEAdvectionSphereBenchmark_advection_vector_uv_gauss_bumps.hpp"
#include "benchmarks/PDEAdvectionSphereBenchmark_advection_vector_3d_normal_vectors.hpp"
#include "benchmarks/PDEAdvectionSphereBenchmark_nair_lauritzen_sl.hpp"
#include "benchmarks/PDEAdvectionSphereBenchmark_williamson_1_advection_cos_bell.hpp"
#include "benchmarks/PDEAdvectionSphereBenchmark_williamson_1_advection_gauss_bump.hpp"
#include "benchmarks/PDEAdvectionSphereBenchmark_zero.hpp"

#include "time/PDEAdvectionSphereTS_BaseInterface.hpp"



PDEAdvectionSphereBenchmarksCombined::PDEAdvectionSphereBenchmarksCombined()	:
	shackDict(nullptr),
	ops(nullptr),
	master(nullptr)
{
}



bool PDEAdvectionSphereBenchmarksCombined::setup(
		sweet::SphereOperators* io_ops
)
{
	ops = io_ops;

	std::string benchmark_name;

	if (shackBenchmarks->benchmark_name == "")
	{
		printAvailableBenchmarks();
		std::cout << std::endl;
		SWEETError("Please choose benchmark");
	}

	benchmarks_register_all();

	/*
	 * Find right one
	 */
	master = nullptr;


	for (std::size_t i = 0; i < _registered_benchmarks.size(); i++)
	{
		PDEAdvectionSphere_Benchmark_BaseInterface *ts = _registered_benchmarks[i];

		if (ts->implements_benchmark(benchmark_name))
		{
			if (master != nullptr)
			{
				std::cout << "Processing " << i+1 << "th element" << std::endl;
				SWEETError(std::string("Duplicate implementation for benchmark ") + benchmark_name);
			}

			std::cout << "Benchmark detection: found match with benchmark id " << i+1 << std::endl;
			ts->setup(shackDict, ops);
			master = ts;
		}
	}

	// Found integrator, freeing others
	benchmarks_free_all(master);

	if (master == nullptr)
	{
		printAvailableBenchmarks();
		std::cout << std::endl;
		SWEETError(std::string("No valid --benchmark-name '") + benchmark_name + std::string("' provided"));
	}

	return true;
}



void PDEAdvectionSphereBenchmarksCombined::clear()
{
	master = nullptr;
	shackDict = nullptr;
	ops = nullptr;

	benchmarks_free_all();
}



bool PDEAdvectionSphereBenchmarksCombined::shackRegistration(
	sweet::ShackDictionary &io_shackDict
)
{
	shackDict = &io_shackDict;

	shackBenchmarks = shackDict->getAutoRegistration<ShackPDEAdvectionSphereBenchmarks>();

	ERROR_CHECK_WITH_RETURN_BOOLEAN(*shackDict);
	return true;
}


void PDEAdvectionSphereBenchmarksCombined::benchmarks_register_all()
{
	_registered_benchmarks.push_back(static_cast<PDEAdvectionSphere_Benchmark_BaseInterface*>(new PDEAdvectionSphereBenchmark_zero));
	_registered_benchmarks.push_back(static_cast<PDEAdvectionSphere_Benchmark_BaseInterface*>(new PDEAdvectionSphereBenchmark_nair_lauritzen_sl));
	_registered_benchmarks.push_back(static_cast<PDEAdvectionSphere_Benchmark_BaseInterface*>(new PDEAdvectionSphereBenchmark_advection_vector_uv_velocities));
	_registered_benchmarks.push_back(static_cast<PDEAdvectionSphere_Benchmark_BaseInterface*>(new PDEAdvectionSphereBenchmark_advection_vector_uv_gauss_bumps));
	_registered_benchmarks.push_back(static_cast<PDEAdvectionSphere_Benchmark_BaseInterface*>(new PDEAdvectionSphereBenchmark_advection_vector_3d_normal_vectors));
	_registered_benchmarks.push_back(static_cast<PDEAdvectionSphere_Benchmark_BaseInterface*>(new PDEAdvectionSphereBenchmark_williamson_1_advection_cos_bell));
	_registered_benchmarks.push_back(static_cast<PDEAdvectionSphere_Benchmark_BaseInterface*>(new PDEAdvectionSphereBenchmark_williamson_1_advection_gauss_bump));
}



void PDEAdvectionSphereBenchmarksCombined::benchmarks_free_all(
		PDEAdvectionSphere_Benchmark_BaseInterface *skip_this
)
{
	for (std::size_t i = 0; i < _registered_benchmarks.size(); i++)
	{
		PDEAdvectionSphere_Benchmark_BaseInterface *ts = _registered_benchmarks[i];

		if (ts == skip_this)
			continue;

		delete ts;
	}

	_registered_benchmarks.clear();
}


void PDEAdvectionSphereBenchmarksCombined::printAvailableBenchmarks()
{
	benchmarks_register_all();
	for (std::size_t i = 0; i < _registered_benchmarks.size(); i++)
	{
		std::cout << _registered_benchmarks[i]->get_help();
		std::cout << std::endl;
	}
	benchmarks_free_all();
}


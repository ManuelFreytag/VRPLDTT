/**
This script defines the python interface for the C++ implemented ALNS solver tool

The user at the python interace should only be allowed to use constructors to change settings
create new data objects and solve.

The following informations are deemed relevant:
	1) Solution objects and their KPIs
	2) Wheel objects and their weights
	3) Data objects and their settings
	4) ALNS objects and all visited solutions with timestamp

The following objects are pickleable:
	1) Data objects (for premature preprocessing)
	2) Solutions (for future evaluation)

*/
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "alns_data.h"
#include "alns.h"
#include "solution.h"
#include "roulette_wheel.h"
#include <Windows.h>

namespace py = pybind11;

PYBIND11_MODULE(ALNSv2, m) {

	// 1) ALNS DATA OBJECT
	py::class_<ALNSData> alns_data(m, "ALNSData");

	// VRPLDTT constructor
	alns_data.def(py::init<int, int, int,
		std::vector<double>, std::vector<double>, std::vector<double>, std::vector<double>,
		std::vector<std::vector<double>>, std::vector<std::vector<double>>,
		double, double, int, int>(),
		py::arg("nr_veh"),
		py::arg("nr_nodes"),
		py::arg("nr_customers"),
		py::arg("demand"),
		py::arg("service_times"),
		py::arg("start_window"),
		py::arg("end_window"),
		py::arg("elevation_m"),
		py::arg("distance_m"),
		py::arg("load_bucket_size") = 0,
		py::arg("nr_load_buckets") = 0,
		py::arg("vehicle_weight") = 140,
		py::arg("vehicle_capacity") = 150);

	// VRPTW constructor
	alns_data.def(py::init<int, int, int,
		std::vector<double>, std::vector<double>, std::vector<double>, std::vector<double>,
		std::vector<std::vector<std::vector<double>>>, int>(),
		py::arg("nr_veh"),
		py::arg("nr_nodes"),
		py::arg("nr_customers"),
		py::arg("demand"),
		py::arg("service_times"),
		py::arg("start_window"),
		py::arg("end_window"),
		py::arg("time_c"),
		py::arg("vehicle_capacity") = 150);


	// Define the pickle functions of the object
	// Must dominant object (VRPLDTT includes all of VRPTW)
	alns_data.def(py::pickle(
		[](const ALNSData &obj) {
		// __getstate__ (of python object, pickle uses those)
		// Must be declared so that it serialize the object
		return py::make_tuple(
			obj.nr_vehicles,
			obj.nr_nodes, 
			obj.nr_customer,
			obj.demand,
			obj.service_times,
			obj.start_window,
			obj.end_window,
			obj.slope_matrix,
			obj.distance_matrix,
			obj.time_cube,
			obj.load_bucket_size,
			obj.vehicle_weight,
			obj.vehicle_cap);
	},
		[](py::tuple t) {
		// __setstate__ (of python object)
		// Must be declared so that it can deserialize
		if (t.size() != 13) {
			throw std::runtime_error("Invalid state!");
		}

		// Create instance
		ALNSData obj = ALNSData(t[0].cast<int>(), // veh
			t[1].cast<int>(), // nodes
			t[2].cast<int>(), // cust
			t[3].cast<std::vector<double>>(), // demand
			t[4].cast<std::vector<double>>(), // service
			t[5].cast<std::vector<double>>(), // start_w
			t[6].cast<std::vector<double>>(), // end_w
			t[7].cast<std::vector<std::vector<double>>>(), // slope
			t[8].cast<std::vector<std::vector<double>>>(), // distance
			t[9].cast<std::vector<std::vector<std::vector<double>>>>(), //time_c
			t[10].cast<double>(), // load_b
			t[11].cast<int>(), // veh_weight
			t[12].cast<int>()); // veh_cap
		return obj;
	}
	));

	alns_data.def_readonly("vehicle_weight", &ALNSData::vehicle_weight);
	alns_data.def_readonly("vehicle_capacity", &ALNSData::vehicle_cap);
	alns_data.def_readonly("add_pseudo_capacity", &ALNSData::add_pseudo_capacity);
	alns_data.def_readonly("load_bucket_size", &ALNSData::load_bucket_size);
	alns_data.def_readonly("nr_vehicles", &ALNSData::nr_vehicles);
	alns_data.def_readonly("nr_nodes", &ALNSData::nr_nodes);
	alns_data.def_readonly("nr_customer", &ALNSData::nr_customer);
	alns_data.def_readonly("customer_demands", &ALNSData::demand);
	alns_data.def_readonly("service_times", &ALNSData::service_times);
	alns_data.def_readonly("start_window", &ALNSData::start_window);
	alns_data.def_readonly("end_window", &ALNSData::end_window);
	alns_data.def_readonly("slope_matrix", &ALNSData::slope_matrix);
	alns_data.def_readonly("time_cube", &ALNSData::time_cube);

	// 2) ALNS SOLVER OBJECT
	py::class_<ALNS> alns_class(m, "ALNS");

	alns_class.def(py::init<ALNSData &,
		std::vector<std::string>,
		std::vector<std::string>,
		int,
		double,
		double,
		double,
		int,
		double,
		double,
		double,
		double,
		double,
		double,
		double,
		double,
		double,
		double,
		double>(),
		py::arg("data_object"),
		py::arg("destroy_operators"),
		py::arg("repair_operators"),
		py::arg("max_time") = 600,
		py::arg("max_iterations") = 10000,
		py::arg("initial_temperature") = 0.01,
		py::arg("cooling_rate") = 0.99975,
		py::arg("wheel_memory_length") = 20,
		py::arg("wheel_parameter") = 0.1,
		py::arg("functor_reward_best") = 33,
		py::arg("functor_reward_accept_better") = 13,
		py::arg("functor_reward_unique") = 9,
		py::arg("functor_reward_divers") = 9,
		py::arg("functor_penalty") = 0,
		py::arg("functor_min_weight") = 1,
		py::arg("random_noise") = 0,
		py::arg("target_inf") = 0.2,
		py::arg("shakeup_log") = 20,
		py::arg("mean_removal_log") = 2);

	// Only some parts of the internal workings relevant
	alns_class.def_readonly("solution", &ALNS::solution);
	alns_class.def_readonly("visited_solutions", &ALNS::visited_solutions);
	alns_class.def_readonly("DestroyWheel", &ALNS::destroy_wheel);
	alns_class.def_readonly("InsertionWheel", &ALNS::insertion_wheel);
	alns_class.def_readonly("capa_error_weight", &ALNS::capa_error_weight);
	alns_class.def_readonly("frame_error_weight", &ALNS::frame_error_weight);
	alns_class.def_readonly("iterations", &ALNS::iterations);
	alns_class.def_readonly("solution_time_ms", &ALNS::solution_time_ms);
	alns_class.def_readonly("value", &ALNS::value);

	// Define the function interface (only solve relevant)
	alns_class.def("solve", &ALNS::solve);

	// -> Rest is irrelevant as its custom input by the user

	// 3) SOLUTION OBJECT
	// Define the solution object that is going to be returned
	py::class_<Solution> solution_obj(m, "Solution");

	// Define Solution constructor (for reevaluating past manual solutions)
	solution_obj.def(py::init<ALNSData &,
		std::vector<std::vector<int>>,
		double,
		double>(),
		py::arg("data_object"),
		py::arg("solutoin_rep"),
		py::arg("capa_error_weight") = 0,
		py::arg("frame_error_weight") = 0);

	// Define Solution pickle function
	solution_obj.def(py::pickle(
		[](const Solution &obj) {
		// __getstate__ (of python object, pickle uses those)
		// Must be declared so that it serialize the object

		return py::make_tuple(
			obj.solution_representation,
			obj.loads,
			obj.arrival_times,
			obj.departure_times,
			obj.driving_time,
			obj.solution_quality,
			obj.capa_error,
			obj.frame_error,
			obj.is_feasible,
			obj.start_times,
			obj.route_driving_times);
	},
		[](py::tuple t) {
		// __setstate__ (of python object)
		// Must be declared so that it can deserialize
		if (t.size() != 11) {
			throw std::runtime_error("Invalid state!");
		}

		// Create empty data instance
		// The reference to the real data object
		// is not provided in the python interace -> does not matter!
		ALNSData data_obj = ALNSData();

		Solution obj = Solution(data_obj, // data
			t[0].cast<std::vector<std::vector<int>>>(), // sol_rep
			t[1].cast<std::vector<double>>(), // loads
			t[2].cast<std::vector<double>>(), // arrival_times
			t[3].cast<std::vector<double>>(), // dep_times
			t[4].cast<double>(), // driving_time
			t[5].cast<double>(), // sol_qual
			t[6].cast<double>(), //capa_e
			t[7].cast<double>(), // frame_error
			t[8].cast<bool>(), // is_feas
			t[9].cast<std::vector<double>>(), // start_times
			t[10].cast<std::vector<double>>()); // route_driving_times
		return obj;
	}
	));


	solution_obj.def_readonly("solution", &Solution::solution_representation);

	// Provide read access to customer based info
	solution_obj.def_readonly("loads", &Solution::loads);
	solution_obj.def_readonly("arrival_times", &Solution::arrival_times);
	solution_obj.def_readonly("departure_times", &Solution::departure_times);

	// Provide read access to KPIs of solution (also infeasible solutions visited)
	solution_obj.def_readonly("quality", &Solution::solution_quality);
	solution_obj.def_readonly("capa_error", &Solution::capa_error);
	solution_obj.def_readonly("frame_error", &Solution::frame_error);
	solution_obj.def_readonly("value", &Solution::driving_time);
	solution_obj.def_readonly("is_feasible", &Solution::is_feasible);

	// Provide access to the route KPIs
	solution_obj.def_readonly("start_times", &Solution::start_times);
	solution_obj.def_readonly("route_driving_times", &Solution::route_driving_times);

	// 4) ROULETTE WHEEL
	// destroy roulette wheel
	py::class_<RouletteWheel> wheel_obj(m, "RouletteWheel");

	wheel_obj.def_readonly("weights", &RouletteWheel::weights);
	wheel_obj.def_readonly("nr_uses", &RouletteWheel::nr_uses);
	
	py::class_<DestroyRouletteWheel, RouletteWheel>(m, "DestroyRouletteWheel");
	py::class_<InsertionRouletteWheel, RouletteWheel>(m, "InsertionRouletteWheel");



#ifdef VERSION_INFO
	m.attr("__version__") = VERSION_INFO;
#else
	m.attr("__version__") = "dev";
#endif
}
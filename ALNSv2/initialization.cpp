#include "solution.h"
#include "alns.h"
#include "tools.h"
#include <vector>

using namespace std;

// this is the global variable that has to be set by either of the init funcions!


/**
Utility function to set the route of an initial solution

Annotation:
All other solution attributes are not set!

@param nr_vehicles
@param nr_customers
*/
vector<vector<int>> route_init_random(
	const int nr_vehicles, 
	const int nr_customers,
	const vector<double> demands,
	const int max_capacity)
{
	// initialize the solution object with multiple routes (nr_vehicles)
	vector<vector<int>> solution_value(nr_vehicles, vector<int>());
	vector<double> vehicle_loads(nr_vehicles, 0.0);

	// create list of node ids
	vector<int> node_ids = tools::range(nr_customers);

	// assign each node to a route randomly also in random order
	// However, only allow this if there is enough capacity!
	while (!node_ids.empty()) {
		// 1) Get random node id
		int node_pos = tools::rand_number(node_ids.size() - 1);
		int node_id = node_ids[node_pos];

		// 2) Assign to random route
		int route_id_start = tools::rand_number(nr_vehicles - 1);

		// iterate over the routes to check where we could insert it
		// the iteration start is created randomlythe next route if there is no place
		bool inserted = false;

		for (int route_id = route_id_start; route_id < nr_vehicles; route_id++) {
			double new_vehicle_load = vehicle_loads[route_id] + demands[node_id];
			if (new_vehicle_load < max_capacity) {
				solution_value[route_id].push_back(node_id);
				vehicle_loads[route_id] = new_vehicle_load;
				inserted = true;
				break;
			}
		}

		// We consider only the route that come after the route
		// -> If we have not inserted it yet try to insert it from the beginning
		if (!inserted) {
			for (int route_id = 0; route_id < route_id_start; route_id++) {
				double new_vehicle_load = vehicle_loads[route_id] + demands[node_id];
				if (new_vehicle_load < max_capacity) {
					solution_value[route_id].push_back(node_id);
					vehicle_loads[route_id] = new_vehicle_load;
					inserted = true;
					break;
				}
			}
		}


		// Check if we inserted the node, if not something is wrong!
		if (inserted) {
			// 3) Remove the node_id from the pool
			node_ids.erase(node_ids.begin() + node_pos);
		}
		else {
			throw std::invalid_argument("Max capacity of all vehicles is not sufficient to allocate all nodes");
		}
	}

	// set this solution
	return solution_value;
}

/**
	Initialization wrapper for a solution object
	The process is:
		1) Generate initial solution
		2) Evalate the solution and set its values
		3) Set the initial run solutions of the alns object

*/
void ALNS::ALNS::initialization() {
	int max_capacity = this->data_obj.vehicle_cap + this->data_obj.add_pseudo_capacity;
	vector<vector<int>> solution_rep;

	// set the initialization value randomly
	solution_rep = route_init_random(this->data_obj.nr_vehicles,
		this->data_obj.nr_customer,
		this->data_obj.demand,
		max_capacity);

	// create solution object
	Solution initial_solution = Solution(this->data_obj, solution_rep, capa_error_weight, frame_error_weight);

	// set all relevant solution objects in the controll structure
	this->running_solution = initial_solution;
	this->current_solution = initial_solution;
}

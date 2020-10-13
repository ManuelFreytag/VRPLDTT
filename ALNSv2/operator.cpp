#include "evaluate.h"
#include "operator.h"
#include "tools.h"
#include <tuple>
#include <functional>
#include <vector>
#include <queue>
#include <algorithm>

using namespace std;

/**
Get the cost of an insertion by
	1) Inserting
	2) Evaluating
	3) Removing
	4) Reevaluating

This is more efficient than copying the previous state of the solution object as a whole,
because the evaluation is done on route basis

Other than in similar problems a decision will impact customer locations before and after the
insertion position as well. (Time windows and load dependencies)

Efficiency annotation:
	- Instead of completely recomputing the solution object we only evaluate the impacted
		route fragement.

	- Instead of copying the solution object / route every iteration we reset the temporary
		adjustments back to normal.
*/
double evaluate_insertion_position(
	Solution &solution_obj,
	const double capa_error_weight,
	const double frame_error_weight,
	const int route_id,
	const int customer_id,
	const int ins_pos)
{
	vector<int> &route = solution_obj.solution_representation[route_id];

	try {
		// perform insertion
		route.insert(route.begin() + ins_pos, customer_id);
		solution_obj.route_chromosome[customer_id] = route_id;

		// reevaluate solution
		solution_obj.evaluate_change(route_id, ins_pos, capa_error_weight, frame_error_weight);

	}
	catch (InfeasibilityException) {
		tools::remove_at(route, ins_pos);
		solution_obj.evaluate_change(route_id, ins_pos - 1, capa_error_weight, frame_error_weight);

		// Use exception to realize when an insertion is completely not possible
		// -> efficiency
		throw InfeasibilityException();
	}

	// Check if performance improved
	double tmp_cost = solution_obj.solution_quality;

	// 3) Reset insertions (Annotation: ins_pos is always inclusive in the removal
	tools::remove_at(route, ins_pos);
	solution_obj.evaluate_change(route_id, ins_pos - 1, capa_error_weight, frame_error_weight);

	return tmp_cost;
}


/**
Get the cost of multiple insertions in the same spot

Efficiency annotation:
	- Instead of completely recomputing the solution object we only evaluate the impacted
		route fragement.

	- Instead of copying the solution object / route every iteration we reset the temporary
		adjustments back to normal.
*/
double evaluate_insertion_chain(
	Solution &solution_obj,
	const double capa_error_weight,
	const double frame_error_weight,
	const int route_id,
	const vector<int> customer_ids,
	const int ins_pos) 
{
	vector<int> &route = solution_obj.solution_representation[route_id];

	int add_pos = 0;
	try {
		// perform insertion
		for (int customer_id : customer_ids) {
			route.insert(route.begin() + ins_pos + add_pos, customer_id);
			solution_obj.route_chromosome[customer_id] = route_id;
			add_pos++;
		}

		// reevaluate solution
		solution_obj.evaluate_change(route_id, ins_pos + add_pos - 1, capa_error_weight, frame_error_weight);

	}
	catch (InfeasibilityException) {
		for (int rem_pos = 0; rem_pos < add_pos; rem_pos++) {
			tools::remove_at(route, ins_pos);
		}
		solution_obj.evaluate_change(route_id, ins_pos - 1, capa_error_weight, frame_error_weight);
		throw InfeasibilityException();
		// Use exception to realize when an insertion is completely not possible
		// -> efficiency
	}

	// Check if performance improved
	double tmp_cost = solution_obj.solution_quality;

	// 3) Reset insertions (Annotation: ins_pos is always inclusive in the removal
	for (int rem_pos = 0; rem_pos < add_pos; rem_pos++) {
		tools::remove_at(route, ins_pos);
	}
	solution_obj.evaluate_change(route_id, ins_pos - 1, capa_error_weight, frame_error_weight);
	return tmp_cost;
}

/**
	Get best insertion position within a route
*/
tuple<double, int, int> get_best_insertion(
	int customer_id,
	Solution &solution_obj,
	double const &capa_error_weight,
	double const &frame_error_weight,
	int route_id = -1)
{
	int start_id;
	int stop_id;

	if (route_id >= 0) {
		start_id = route_id;
		stop_id = route_id + 1;
	}
	else {
		start_id = 0;
		stop_id = solution_obj.solution_representation.size();
	}

	// get the best insertion position!
	double min_cost = std::numeric_limits<double>::max();
	tuple<double, int, int> best_insertion(min_cost, 0, 0);

	for (int rid = start_id; rid < stop_id; rid++) {
		vector<int> &route = solution_obj.solution_representation[rid];

		// First and last position insertion is done correctly implicitly! (depot distance is considered)
		for (unsigned int pos = 0; pos <= route.size(); pos++) {
			try {
				double tmp_cost = evaluate_insertion_position(
					solution_obj,
					capa_error_weight,
					frame_error_weight,
					rid,
					customer_id,
					pos) - solution_obj.solution_quality; // comparison remains iteration independent

				// perform comparision
				if (min_cost > tmp_cost) {
					min_cost = tmp_cost;
					best_insertion = tuple<double, int, int>(tmp_cost, rid, pos);
				}
			}
			catch (InfeasibilityException) {
				// We know that all other insertion positions in that route also exceed max infeasibility!
				break;
			}
		}
	}
	return best_insertion;
}

/**
Get the cost of an removal cost by
	1) Removing
	2) Evaluating
	3) Inserting
	4) Reevaluating

This is more efficient than copying the previous state of the solution object as a whole,
because the evaluation is done on route basis

Other than in similar problems a decision will impact customer locations before and after the
insertion position as well. (Time windows and load dependencies)

Efficiency annotation:
	- Instead of completely recomputing the solution object we only evaluate the impacted
		route fragement.

	- Instead of copying the solution object / route every iteration we reset the temporary
		adjustments back to normal.
*/
double evaluate_removal_position(
	Solution &solution_obj,
	const double capa_error_weight,
	const double frame_error_weight,
	const int route_id,
	const int rem_pos)
{
	vector<int> &route = solution_obj.solution_representation[route_id];

	// 1) perform removal
	int customer_id = route[rem_pos];
	tools::remove_at(route, rem_pos);

	// 2) evaluate change
	solution_obj.evaluate_change(route_id, rem_pos - 1, capa_error_weight, frame_error_weight);

	// check if cost change after removal
	double tmp_cost = solution_obj.solution_quality;

	// 3) perform insertion
	route.insert(route.begin() + rem_pos, customer_id);
	solution_obj.route_chromosome[customer_id] = route_id;

	// 4) reevaluate solution
	solution_obj.evaluate_change(route_id, rem_pos, capa_error_weight, frame_error_weight);

	return tmp_cost;
}

/**
	Random removal operator
	Mean number of customers is set (can be set based on nr of customers as well)
	Probability of acceptance is of equal distribution.
	
	Usage:
		Mainly used for diversification
*/
vector<int> RandomDestroyOperator::operator()(){
	// base init for each destroy operator
	vector<vector<int>> tmp_destroyed_route; // for efficiency reasons allocate
	vector<int> removed_customers;

	for (vector<int> & route : this->solution_obj.solution_representation) {
		vector<int> new_route;

		for (int customer_id : route) {
			int rnd_int = tools::rand_number(this->solution_obj.data_obj.get().nr_customer, 0);

			if (rnd_int > this->mean_removal) {
				new_route.push_back(customer_id);
			}
			else {
				removed_customers.push_back(customer_id);
			}
		}
		tmp_destroyed_route.push_back(new_route); 
	}

	// Set the respective values
	this->solution_obj.solution_representation = tmp_destroyed_route;

	// reevaluate solution (TODO optimize)
	this->solution_obj.evaluate_solution(this->capa_error_weight, this->frame_error_weight);
	return removed_customers;
}

/**
	Remove a full route and redistribute all customers

	Usage:
		Reducation of routes (if necessary)
*/
vector<int> RandomRouteDestroyOperator::operator()() {
	// Get the random route id
	int route_id = tools::rand_number(this->solution_obj.data_obj.get().nr_vehicles-1, 0);

	// save new route
	vector<int> removed_customers = this->solution_obj.solution_representation[route_id]; // call by value per default
	this->solution_obj.solution_representation[route_id].clear(); // destroy route

	// reevaluate solution (TODO: optimize)
	this->solution_obj.evaluate_solution(this->capa_error_weight, this->frame_error_weight);

	return removed_customers;
}


/**
Description
	Remove customers based on the demand they require
	We assume that these customers are of highest priority and biggest levers
	because they impact the all customers after them the most.

Annotation:
	We want to prohibit repetitiveness and stale selection.
	This is why we use the general randomness factor
*/
vector<int> BiggestDemandDestroyOperator::operator()() {
	// Perform randomization!
	ALNSData &data = this->solution_obj.data_obj.get();

	vector<double> skewed_demand_ranks(data.nr_customer);

	int rnd_int = tools::rand_number_normal(this->mean_removal, this->mean_removal / 2);
	rnd_int = max(0, min(data.nr_customer - 1, rnd_int));

	for (int i = 0; i < data.nr_customer; i++) {
		// double bias = VNI;
		// skewed_demand_ranks[i] = this->demand_ranks[i]+(rnd_int * bias);

		double bias = std::pow(UNI, this->rnd_factor);
		skewed_demand_ranks[i] = this->demand_ranks[i] * bias;
	}

	vector<int> sorted_indices = tools::sort_indices(skewed_demand_ranks);

	// return removed customers (increasing order -> take back)
	vector<int> removed_customers(sorted_indices.end()-rnd_int, sorted_indices.end());

	// destroy routes! (with use of chromosomes)
	for (int customer_id : removed_customers) {
		int route_id = solution_obj.route_chromosome[customer_id];
		vector<int> &route = solution_obj.solution_representation[route_id];
		int customer_pos = route_evaluate::get_customer_pos(route, customer_id);
		tools::remove_at(this->solution_obj.solution_representation[route_id], customer_pos);
	}


	this->solution_obj.evaluate_solution(this->capa_error_weight, this->frame_error_weight);
	return removed_customers;
}

/**
Description
	Remove customers based on the travel time they require

Annotation:
	We want to prohibit repetitiveness and stale selection.
	This is why we use the general randomness factor

*/
vector<int> WorstTravelTimeDestroyOperator::operator()() {
	// 1) Get travel times (is not worth to save as seperate attribute)
	ALNSData &data = this->solution_obj.data_obj.get();
	vector<double> travel_times(data.nr_customer);

	for (vector<int> &route : this->solution_obj.solution_representation) {
		if (route.size() > 0) {
			int prev_customer_id = -1;

			for (int customer_id : route) {
				// Add arc TO customer
				int load_level = this->solution_obj.load_levels[customer_id];
				double travel_time = data.time_cube[load_level][prev_customer_id + 1][customer_id + 1];
				travel_times[customer_id] = travel_time;

				// Add arc FROM customer
				if (prev_customer_id > 0) {
					travel_times[prev_customer_id] += travel_time;
				}
				prev_customer_id = customer_id;
			}

			// Add last customer -> depot
			travel_times[prev_customer_id] += data.time_cube[0][prev_customer_id + 1][0];
		}
	}

	// 2) Select customers to remove
	vector<int> travel_time_ranks = tools::get_ranks(travel_times);


	// Perform randomization!
		// uniform distribution
	int rnd_int = tools::rand_number_normal(this->mean_removal, this->mean_removal / 2);
	rnd_int = max(0, min(data.nr_customer - 1, rnd_int));

	vector<double> skewed_travel_ranks(travel_time_ranks.size());

	for (unsigned int i = 0; i < travel_time_ranks.size(); i++) {
		double bias = std::pow(UNI, this->rnd_factor);
		skewed_travel_ranks[i] = travel_time_ranks[i]*bias;
	}

	vector<int> sorted_indices = tools::sort_indices(skewed_travel_ranks);

	// return removed customers (increasing order -> take back)
	vector<int> removed_customers(sorted_indices.end() - rnd_int, sorted_indices.end());

	// 3) Remove customers! (with use of chromosomes)
	for (int customer_id : removed_customers) {
		int route_id = solution_obj.route_chromosome[customer_id];

		// We dont know the correct position
		// Reason for that is because each insertion requires updating
		// The position tracking. We assume that checking it dynamically
		// is more efficient
		vector<int> &route = solution_obj.solution_representation[route_id];
		int customer_pos = route_evaluate::get_customer_pos(route, customer_id);
		tools::remove_at(this->solution_obj.solution_representation[route_id], customer_pos);
	}

	this->solution_obj.evaluate_solution(this->capa_error_weight, this->frame_error_weight);
	return removed_customers;
}

vector<int> WorstRemovalDestroyOperator::operator()() {
	ALNSData &data = this->solution_obj.data_obj.get();

	// 0) Setup
	int nr_removed_customers = tools::rand_number_normal(this->mean_removal, this->mean_removal / 2);
	nr_removed_customers = max(0, min(data.nr_customer - 1, nr_removed_customers));
	vector<int> removed_customers;
	removed_customers.reserve(nr_removed_customers);

	vector<int> candidates = tools::range(data.nr_customer);


	// 1) Calculate best info on customer and route basis
	double overall_max_diff = -std::numeric_limits<double>::max();
	int best_customer_id_pos = 0; // Position in the removed customers list
	int best_route_id = 0;
	int best_rem_pos = 0;

	vector<vector<tuple<double, int>>> best_removals(data.nr_customer, vector<tuple<double, int>>(data.nr_vehicles));

	for (unsigned int customer_id_pos = 0; customer_id_pos < candidates.size(); customer_id_pos++) {
		for (int route_id = 0; route_id < data.nr_vehicles; route_id++) {
			double max_cost_diff = -std::numeric_limits<double>::max();
			int best_route_rem_pos = 0;

			vector<int> &route = this->solution_obj.solution_representation[route_id];

			double tmp_cost = -std::numeric_limits<double>::max();
			for (unsigned int pos = 0; pos < route.size(); pos++) {
				// Increase important because this remains static
				tmp_cost = this->solution_obj.solution_quality - evaluate_removal_position(
					this->solution_obj,
					this->capa_error_weight,
					this->frame_error_weight,
					route_id,
					pos);

				double bias = std::pow(UNI, this->rnd_factor);
				tmp_cost *= bias;

				// perform comparision
				if (max_cost_diff < tmp_cost) {
					max_cost_diff = tmp_cost;
					best_removals[customer_id_pos][route_id] = tuple<double, int>(tmp_cost, pos);

					// only if its a local minimum it can be a global minimum
					if (overall_max_diff < tmp_cost) {
						overall_max_diff = tmp_cost;
						best_customer_id_pos = customer_id_pos;
						best_route_id = route_id;
						best_rem_pos = pos;
					}
				}
			}
		}
	}

	// 2) Iteratively add candidate to removal list
	while (unsigned(nr_removed_customers) < removed_customers.size()) {
		// 2.1) Perform removal
		removed_customers.push_back(candidates[best_customer_id_pos]);
		tools::remove_at(this->solution_obj.solution_representation[best_route_id], best_rem_pos);
		solution_obj.evaluate_change(best_route_id, best_rem_pos - 1, capa_error_weight, frame_error_weight);

		// 2.2) Remove candidate
		tools::remove_at(candidates, best_customer_id_pos);
		tools::remove_at(best_removals, best_customer_id_pos);

		// 2.3) Reevaluate removal costs for that route

		double tmp_cost;
		for (unsigned int customer_id_pos = 0; customer_id_pos < candidates.size(); customer_id_pos++) {
			double best_pos = 0;
			double max_diff = -std::numeric_limits<double>::max();

			for (unsigned int pos = 0; pos < this->solution_obj.solution_representation[best_route_id].size(); pos++) {
				tmp_cost = this->solution_obj.solution_quality - evaluate_removal_position(
					this->solution_obj,
					this->capa_error_weight,
					this->frame_error_weight,
					best_route_id,
					pos);

				double bias = std::pow(UNI, this->rnd_factor);
				tmp_cost *= bias;

				if (max_diff < tmp_cost) {
					max_diff = tmp_cost;
					best_pos = pos;
				}
			}
			best_removals[customer_id_pos][best_route_id] = tuple<double, int>(max_diff, best_pos);
		}

		// 2.4) Get next best candidate
		overall_max_diff = -std::numeric_limits<double>::max();
		for (unsigned int customer_id_pos = 0; customer_id_pos < removed_customers.size(); customer_id_pos++) {
			for (int route_id = 0; route_id < data.nr_vehicles; route_id++) {
				if (overall_max_diff < get<0>(best_removals[customer_id_pos][route_id])) {
					overall_max_diff = get<0>(best_removals[customer_id_pos][route_id]);
					best_customer_id_pos = customer_id_pos;
					best_route_id = route_id;
					best_rem_pos = get<1>(best_removals[customer_id_pos][route_id]);
				}
			}
		}
	}

	return removed_customers;
}

/**
Description
	Remove customers based on proven worst potential of their position
	E.g. Previous best solutions with that node position perform bad -> change pos!

Annotation:
	We want to prohibit repetitiveness and stale selection.
	This is why we use the general randomness factor

*/
vector<int> NodePairDestroyOperator::operator()() {
	// 1) Get the best values of all operators
	ALNSData &data = this->solution_obj.data_obj.get();

	vector<double> historic_perf(data.nr_customer, 0);

	for (vector<int> &route : this->solution_obj.solution_representation) {

		int prev_customer_id = -1; // start at depot
		if (route.size() > 0) {
			for (int customer_id : route) {
				// preceeding route potential
				historic_perf[customer_id] += this->node_pair_potential_matrix[prev_customer_id + 1][customer_id + 1];

				if (prev_customer_id > -1) {
					// succeeding route
					historic_perf[prev_customer_id] += this->node_pair_potential_matrix[prev_customer_id + 1][customer_id + 1];
				}

				prev_customer_id = customer_id;
			}

			// route potential for the last node -> depot
			historic_perf[prev_customer_id] += this->node_pair_potential_matrix[prev_customer_id + 1][0];
		}
	}

	// 2) Select customers to remove
	// 2.1) Get ranks of KPIs
	vector<int> performance_ranks = tools::get_ranks(historic_perf);

	// 2.2) Decide on number of removed customers
	int rnd_int = tools::rand_number_normal(this->mean_removal, this->mean_removal / 2);
	rnd_int = max(0, min(data.nr_customer - 1, rnd_int));

	// 2.3) Perform randomization
	vector<double> skewed_perf_ranks(performance_ranks.size());

	for (unsigned int i = 0; i < performance_ranks.size(); i++) {
		double bias = std::pow(UNI, this->rnd_factor);
		skewed_perf_ranks[i] = performance_ranks[i]*bias;
	}

	// 2.4) Select customers to remove
	vector<int> sorted_indices = tools::sort_indices(skewed_perf_ranks);

	// return removed customers (increasing order -> take back)
	vector<int> removed_customers(sorted_indices.end() - rnd_int, sorted_indices.end());

	// 3) Remove customers! (with use of chromosomes)
	for (int customer_id : removed_customers) {
		int route_id = solution_obj.route_chromosome[customer_id];

		// We dont know the correct position
		// Reason for that is because each insertion requires updating
		// The position tracking. We assume that checking it dynamically
		// is more efficient
		vector<int> &route = solution_obj.solution_representation[route_id];
		int customer_pos = route_evaluate::get_customer_pos(route, customer_id);
		tools::remove_at(this->solution_obj.solution_representation[route_id], customer_pos);
	}

	this->solution_obj.evaluate_solution(this->capa_error_weight, this->frame_error_weight);
	return removed_customers;
}

/**
Implementation of the shaw removal operator as described in Shaw (1998) and Ropke & Pissinger (2005)
*/
vector<int> ShawDestroyOperator::operator()() {
	ALNSData &data = this->solution_obj.data_obj.get();

	// 1) initialize removal list
	int nr_removed_customers = tools::rand_number_normal(this->mean_removal, this->mean_removal / 2);
	nr_removed_customers = max(0, min(data.nr_customer - 1, nr_removed_customers));

	// 1.2) Setup lists
	vector<int> candidates = tools::range(data.nr_customer);
	vector<int> removed_customers;
	removed_customers.reserve(nr_removed_customers);

	// 1.3) Get random customer as start
	int customer_id = tools::rand_number(data.nr_customer-1);
	removed_customers.push_back(customer_id);
	tools::remove_at(candidates, customer_id);

	// 2) Iteratively select new customer and push to list
	int rnd_customer_id;
	int related_cust_id_pos;
	double max_relatedness;

	for (int i = 1; i < nr_removed_customers; i++) {
		max_relatedness = std::numeric_limits<double>::max();

		// 2.1) Get random id
		rnd_customer_id = removed_customers[tools::rand_number(i-1)];

		// 2.2) Find related customer
		for (unsigned int cand_pos = 0; cand_pos < candidates.size(); cand_pos++) {
			int cand_id = candidates[cand_pos];

			// 2.2.1) Get relatedness and perform permutation
			// The distance matrix also includes the depot -> add +1!
			double relatedness = this->distance_weight*this->norm_distance_matrix[rnd_customer_id+1][cand_id+1]
				+ this->window_weight*this->norm_start_window_matrix[rnd_customer_id][cand_id]
				+ this->window_weight*this->norm_end_window_matrix[rnd_customer_id][cand_id]
				+ this->demand_weight*this->norm_demand_matrix[rnd_customer_id][cand_id];

			// Check if its the same route
			if (this->solution_obj.route_chromosome[cand_id] == this->solution_obj.route_chromosome[cand_id]) {
				relatedness += this->vehicle_weight;
			}

			// 2.2.2) Perform permutation
			relatedness *= std::pow(UNI, this->rnd_factor);

			// 2.2.3) Compare maximum relatedness means a relatedness score of 0!
			if (relatedness < max_relatedness) {
				max_relatedness = relatedness;
				related_cust_id_pos = cand_pos;
			}
		}

		// 2.3) Add related customer
		removed_customers.push_back(candidates[related_cust_id_pos]);
		tools::remove_at(candidates, related_cust_id_pos);
	}

	// 3) Remove customers! (with use of chromosomes)
	for (int customer_id : removed_customers) {
		int route_id = solution_obj.route_chromosome[customer_id];

		// We dont know the correct position
		// Reason for that is because each insertion requires updating
		// The position tracking. We assume that checking it dynamically
		// is more efficient
		vector<int> &route = solution_obj.solution_representation[route_id];
		int customer_pos = route_evaluate::get_customer_pos(route, customer_id);
		tools::remove_at(this->solution_obj.solution_representation[route_id], customer_pos);
	}

	this->solution_obj.evaluate_solution(this->capa_error_weight, this->frame_error_weight);
	return removed_customers;
}

/**
	Basic greedy insertion operator.

	The customers are inserted in their best position based on the order in the removal list.

	Annotation:
		- Only positions that obey max infeasibility are considered
		- First and last insertion position are considered
		- Effcient peice vise reevaluation is applied

*/
void BasicGreedyInsertionOperator::operator()(std::vector<int> removed_customers) {
	for (int customer_id : removed_customers) {
		tuple<double, int, int> best_insertion = get_best_insertion(customer_id, this->solution_obj, this->capa_error_weight, this->frame_error_weight);

		// perform insertion
		int best_route_id = std::get<1>(best_insertion);
		int best_route_pos = std::get<2>(best_insertion);

		vector<int> &route = solution_obj.solution_representation[best_route_id];
		route.insert(route.begin() + best_route_pos, customer_id);
		solution_obj.route_chromosome[customer_id] = best_route_id;

		// reevaluate solution
		solution_obj.evaluate_change(best_route_id, best_route_pos, capa_error_weight, frame_error_weight);
	}	
}

void RandomGreedyInsertionOperator::operator()(std::vector<int> removed_customers) {
	while (removed_customers.size() > 0) {
		// setup iteration
		int customer_pos = tools::rand_number(removed_customers.size()-1);
		int customer_id = removed_customers[customer_pos];
		tuple<double, int, int> best_insertion = get_best_insertion(customer_id, this->solution_obj, this->capa_error_weight, this->frame_error_weight);

		// perform insertion
		int best_route_id = std::get<1>(best_insertion);
		int best_route_pos = std::get<2>(best_insertion);

		vector<int> &route = solution_obj.solution_representation[best_route_id];
		route.insert(route.begin() + best_route_pos, customer_id);
		solution_obj.route_chromosome[customer_id] = best_route_id;

		// reevaluate solution
		solution_obj.evaluate_change(best_route_id, best_route_pos, capa_error_weight, frame_error_weight);

		// remove from candidate list
		tools::remove_at(removed_customers, customer_pos);
	}
}

/**
 # TODO
	 Deep greedy checks for each customer at each position whats the best insertion point
	 It is dominant in the solutions that it can find compared to basic greedy.
	 However, it is also a computationally more demanding.

	 1) Calculate all insertion positions
	 2) Perform best insertion
	 3) Remove inserted customer from list
	 4) Reevaluate all insertion positions before and after previous insertion
	 5) Get best insertion position and start from 2)
*/
void DeepGreedyInsertionOperator::operator()(std::vector<int> removed_customers) {
	// Annotation: node_id = customer_id +1
	ALNSData &data = this->solution_obj.data_obj.get();

	// insertion: cost, route_id, position
	tuple<double, int, int> best_insertion(std::numeric_limits<double>::max(), 0, 0);
	int best_customer_id_pos = 0; // Position in the removed customers list

	// 1) Calculate best info on customer and route basis
	vector<vector<tuple<double, int, int>>> best_insertions(removed_customers.size(), vector<tuple<double, int, int>>(data.nr_vehicles));

	tuple<double, int, int> insertion;
	for (unsigned int customer_id_pos = 0; customer_id_pos < removed_customers.size(); customer_id_pos++) {

		for (int route_id = 0; route_id < data.nr_vehicles; route_id++) {
			insertion = get_best_insertion(
				removed_customers[customer_id_pos],
				solution_obj,
				capa_error_weight,
				frame_error_weight,
				route_id);

			best_insertions[customer_id_pos][route_id] = insertion;

			if (std::get<0>(best_insertion) > std::get<0>(insertion)) {
				best_insertion = insertion;
				best_customer_id_pos = customer_id_pos;
			}
		}
	}

	// 2) Iteratively perform insertions with prev information
	while (removed_customers.size() > 0) {
		// 2.1) perform insertion
		int best_customer_id = removed_customers[best_customer_id_pos];
		int best_route_id = std::get<1>(best_insertion);
		int best_route_pos = std::get<2>(best_insertion);


		// reevaluate solution
		vector<int> &route = this->solution_obj.solution_representation[best_route_id];
		route.insert(route.begin() + best_route_pos, best_customer_id);
		this->solution_obj.route_chromosome[best_customer_id] = best_route_id;

		this->solution_obj.evaluate_change(best_route_id, best_route_pos, this->capa_error_weight, this->frame_error_weight);

		// 2.2) ) Remove customer from list
		tools::remove_at(removed_customers, best_customer_id_pos);
		tools::remove_at(best_insertions, best_customer_id_pos);

		// 2.3) Reevaluate the values of the changed route again. (skip if vector is empty)
		for (unsigned int customer_id_pos = 0; customer_id_pos < removed_customers.size(); customer_id_pos++) {
			// Check for insertion for entire route (insertion and add load can fuck up windows)
			insertion = get_best_insertion(
				removed_customers[customer_id_pos],
				solution_obj,
				capa_error_weight,
				frame_error_weight,
				best_route_id);

			best_insertions[customer_id_pos][best_route_id] = insertion;
		}

		best_insertion = tuple<double, int, int>(std::numeric_limits<double>::max(), 0, 0);

		// 2.4) Get the next best insertion customer and position!
		for (unsigned int customer_id_pos = 0; customer_id_pos < removed_customers.size(); customer_id_pos++) {
			for (int route_id = 0; route_id < data.nr_vehicles; route_id++) {
				if (get<0>(best_insertion) > get<0>(best_insertions[customer_id_pos][route_id])) {
					best_insertion = best_insertions[customer_id_pos][route_id];
					best_customer_id_pos = customer_id_pos;
				}
			}
		}
	}
}


/**
	Implementation of the KRegretInsertionOperator
	Compares the insertion cost differences of the k best insertion positions
	and prioritises the positions with the highest regret potential!

	Smart regret layout:
	1) Calculate all regret values for all customers (with best k insertions)
	2) Perform insertion
	3) Reevaluate possible insertion positions (before and after prev. insertion)
	4) Adjust prioritiy queues if necessary
	5) Reevaluate regret value if necessary
	6) Perform next insertion

	-> Almost O(N)!!
*/
void KRegretInsertionOperator::operator()(std::vector<int> removed_customers) {
	ALNSData &data = this->solution_obj.data_obj.get();

	int best_customer_id_pos = 0; // Position in the removed customers list
	// regret, route_id, route_pos
	tuple<double, int, int> best_insertion(-std::numeric_limits<double>::max(), 0, 0);

	// 1) Calculate best info on customer and route basis
	vector<vector<tuple<double, int, int>>> best_insertions(removed_customers.size(), vector<tuple<double, int, int>>(data.nr_vehicles));

	// 1.1) Get best insertion positions
	vector<tuple<double, int, int>> k_best(this->k_regret); // tmp allocation
	tuple<double, int, int> insertion;
	for (unsigned int customer_id_pos = 0; customer_id_pos < removed_customers.size(); customer_id_pos++) {
		for (int route_id = 0; route_id < data.nr_vehicles; route_id++) {

			insertion = get_best_insertion(
				removed_customers[customer_id_pos],
				solution_obj,
				capa_error_weight,
				frame_error_weight,
				route_id);

			best_insertions[customer_id_pos][route_id] = insertion;
		}

		// 1.2) Get regret values
		double regret = 0;
		vector<tuple<double, int, int>> &best_ins = best_insertions[customer_id_pos];
		// Save the k best into a new vector
		std::partial_sort_copy(best_ins.begin(), best_ins.end(), k_best.begin(), k_best.end(), less<tuple<double, int, int>>());

		for (int k = 1; k < this->k_regret; k++) {
			regret += std::get<0>(k_best[k]) - std::get<0>(k_best[k - 1]);
		}

		// 1.3) Get best ins pos
		if (regret > std::get<0>(best_insertion)) {
			best_insertion = tuple<double, int, int>(regret, std::get<1>(k_best[0]), std::get<2>(k_best[0]));
			best_customer_id_pos = customer_id_pos; // Position in the removed customers list
		}
	}


	// 2) Perform insertion and reevaluate
	while (removed_customers.size() > 0) {
		int best_route_id = std::get<1>(best_insertion);
		int best_route_pos = std::get<2>(best_insertion);

		// 2.1) Perform insertion
		int best_customer_id = removed_customers[best_customer_id_pos];

		vector<int> &route = this->solution_obj.solution_representation[best_route_id];
		route.insert(route.begin() + best_route_pos, best_customer_id);
		this->solution_obj.route_chromosome[best_customer_id] = best_route_id;

		// reevaluate solution
		this->solution_obj.evaluate_change(best_route_id, best_route_pos, this->capa_error_weight, this->frame_error_weight);

		// 2.2) Remove customer from list
		tools::remove_at(removed_customers, best_customer_id_pos);
		tools::remove_at(best_insertions, best_customer_id_pos);


		// 2.3) Reevaluate the values of the changed route again. (skip if vector is empty)
		int changed_route_id = best_route_id;

		best_insertion = tuple<double, int, int>(-std::numeric_limits<double>::max(), 0, 0);
		for (unsigned int customer_id_pos = 0; customer_id_pos < removed_customers.size(); customer_id_pos++) {

			insertion = get_best_insertion(
				removed_customers[customer_id_pos],
				solution_obj,
				capa_error_weight,
				frame_error_weight,
				changed_route_id);

			best_insertions[customer_id_pos][changed_route_id] = insertion;

			// 2.5) Reevaluate the regret values!
			double regret = 0;
			vector<tuple<double, int, int>> &best_ins = best_insertions[customer_id_pos];
			std::partial_sort_copy(best_ins.begin(), best_ins.end(), k_best.begin(), k_best.end(), less<tuple<double, int, int>>());

			for (int k = 1; k < this->k_regret; k++) {
				regret += std::get<0>(k_best[k]) - std::get<0>(k_best[k - 1]);
			}

			// 2.6) Get best ins pos
			if (regret > std::get<0>(best_insertion)) {
				best_insertion = tuple<double, int, int>(regret, std::get<1>(k_best[0]), std::get<2>(k_best[0]));
				best_customer_id_pos = customer_id_pos; // Position in the removed customers list
			}
		}
	}
}

/**
	Implementation of the beta-hybrid insertion heuristic

	Process:
	1) Check if the number of customers is lower than beta
	 1.1) Try to insert the customers as a whole
	2) If no position possible or bigger than beta
	 2.1) Perform randomized greedy insertion
*/
void BetaHybridInsertionOperator::operator()(std::vector<int> removed_customers) {
	tuple<double, int, int> best_insertion(std::numeric_limits<double>::max(), -1, -1);

	// 1) Perform beta insertion if possible
	if ((removed_customers.size() <= unsigned(beta) ) & (removed_customers.size() > unsigned(0))) {
		// 1.1) Revert customers list with probability of 0.5
		if (tools::rand_number(1) == 0) {
			std::reverse(removed_customers.begin(), removed_customers.end());
		}

		// 1.2) Check all insertion spots and try to insert them as a bulk
		for (unsigned int route_id = 0; route_id < this->solution_obj.solution_representation.size(); route_id++) {
			vector<int> &route = this->solution_obj.solution_representation[route_id];

			for (unsigned int ins_pos = 0; ins_pos <= route.size(); ins_pos++) {
				try {
					double cost_diff = evaluate_insertion_chain(this->solution_obj,
						this->capa_error_weight,
						this->frame_error_weight,
						route_id,
						removed_customers,
						ins_pos) - this->solution_obj.solution_quality;

					if (std::get<0>(best_insertion) > cost_diff) {
						best_insertion = tuple<double, int, int>(cost_diff, route_id, ins_pos);
					}
				}
				catch (InfeasibilityException) {
					// Max infeasibility is reached -> not insertable in this route!
					break;
				}
			}
		}

		// 1.3) Perform insertion if suitable
		// We know that if the route id is -1 -> No suitible change was made!
		if (std::get<1>(best_insertion) >= 0) {
			int route_id = std::get<1>(best_insertion);
			int route_pos = std::get<2>(best_insertion);

			// perform insertion
			vector<int> &route = this->solution_obj.solution_representation[route_id];

			int add_pos = 0;
			for (int customer_id : removed_customers) {
				route.insert(route.begin() + route_pos + add_pos, customer_id);
				solution_obj.route_chromosome[customer_id] = route_id;
				add_pos++;
			}

			// reevaluate solution
			solution_obj.evaluate_change(route_id, route_pos + add_pos - 1, capa_error_weight, frame_error_weight);
		}
	}

	// 2) Perform randomized greedy insertion if necessary
	if ((std::get<1>(best_insertion) < 0) | (int(removed_customers.size()) > this->beta)) {
		while (removed_customers.size() > 0) {
			// setup iteration
			int customer_pos = tools::rand_number(removed_customers.size() - 1);
			int customer_id = removed_customers[customer_pos];
			best_insertion = get_best_insertion(customer_id, this->solution_obj, this->capa_error_weight, this->frame_error_weight);

			// perform insertion
			int best_route_id = std::get<1>(best_insertion);
			int best_route_pos = std::get<2>(best_insertion);

			vector<int> &route = solution_obj.solution_representation[best_route_id];
			route.insert(route.begin() + best_route_pos, customer_id);
			solution_obj.route_chromosome[customer_id] = best_route_id;

			// reevaluate solution
			solution_obj.evaluate_change(best_route_id, best_route_pos, capa_error_weight, frame_error_weight);

			// remove from candidate list
			tools::remove_at(removed_customers, customer_pos);
		}
	}
}

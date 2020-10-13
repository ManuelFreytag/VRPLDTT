/**
Main part of the ALNS algorithm

This file contains the Simulated Annealing local search framework
All visited solutions are saved with a timestamp to enable future
evaluations and algorithmic trends.
*/
#include "alns.h"
#include "operator.h"
#include "roulette_wheel.h"
#include "tools.h"
#include <math.h>
#include <functional>
#include <algorithm>
#include <iostream>
#include <chrono>
#include <vector>

using namespace std;


// constructors
ALNS::ALNS(ALNSData &data_obj, // We want a normal object interface to easily export it to python
	std::vector<std::string> destroy_operator_names,
	std::vector<std::string> repair_operator_names,
	int max_time,
	double max_iterations,
	double init_temperature,
	double cooling_rate,
	int wheel_memory_length,
	double wheel_parameter,
	double functor_reward_best,
	double functor_reward_accept_better,
	double functor_reward_unique,
	double functor_reward_divers,
	double functor_penalty,
	double functor_min_weight,
	double random_noise,
	double target_inf,
	double shakeup_log,
	double mean_removal_log) :
	max_time(max_time),
	max_iterations(max_iterations),
	init_temperature(init_temperature),
	cooling_rate(cooling_rate),
	wheel_memory_length(wheel_memory_length),
	wheel_parameter(wheel_parameter),
	functor_reward_best(functor_reward_best),
	functor_reward_accept_better(functor_reward_accept_better),
	functor_reward_unique(functor_reward_unique),
	functor_reward_divers(functor_reward_divers),
	functor_penalty(functor_penalty),
	functor_min_weight(functor_min_weight),
	random_noise(random_noise),
	target_inf(target_inf),
	mean_removal_log(mean_removal_log),
	shakeup_log(shakeup_log),
	data_obj(data_obj),
	node_pair_potential_matrix(data_obj.nr_nodes, vector<double>(data_obj.nr_nodes, std::numeric_limits<double>::max())),
	node_pair_useage_matrix(data_obj.nr_nodes, vector<int>(data_obj.nr_nodes, 0)),
	current_solution(data_obj), // We use reference wrapper for each solution object -> no null state -> need to initialize
	running_solution(data_obj),
	solution(data_obj)// This is the null state! (no valid solution object)
{
	this->inf_count = 0;
	this->capa_error_weight = 1;
	this->frame_error_weight = 1;
	this->mean_removal = log(this->data_obj.nr_customer) / log(mean_removal_log);

	// Set the min additional pseudo capacity to guarantee a solution with random initialization
	this->operator_names_d = destroy_operator_names;
	this->operator_names_r = repair_operator_names;

	vector<function<vector<int>()>> destroy_functors = this->get_destroy_functors();

	this->destroy_wheel = DestroyRouletteWheel(destroy_functors,
		wheel_parameter,
		this->operator_names_d.size()*wheel_memory_length,
		functor_min_weight);

	vector<function<void(vector<int>)>> insertion_functors = this->get_insertion_functors();

	this->insertion_wheel = InsertionRouletteWheel(
		insertion_functors,
		wheel_parameter,
		this->operator_names_r.size()*wheel_memory_length,
		functor_min_weight);
}

/**
	Utility function to generate all destroy_functors based on the given config.
	This allows a comprehensive computational experiment with varriing operators!

	Annotation:
	Mostly used as initialization input for the roulette wheel mechanism
*/
vector < function<vector<int>()>> ALNS::get_destroy_functors() {
	// No functors are passed, use random destroy as default
	if (this->operator_names_d.size() == 0) {

		cout << "No destroy operator supplied, take random_destroy as default" << endl;
		this->operator_names_d.push_back(string("random_destroy"));
	}

	vector<function<vector<int>()>> destroy_functors;
	for (string operator_name : this->operator_names_d) {
		if (operator_name == "random_destroy") {
			RandomDestroyOperator* op = new RandomDestroyOperator(
				this->running_solution,
				this->capa_error_weight,
				this->frame_error_weight,
				this->mean_removal);

			destroy_functors.push_back(*op);
		}
		else if (operator_name == "route_destroy") {
			RandomRouteDestroyOperator* op = new RandomRouteDestroyOperator(
				this->running_solution,
				this->capa_error_weight,
				this->frame_error_weight);

			destroy_functors.push_back(*op);
		}
		else if (operator_name == "demand_destroy") {
			BiggestDemandDestroyOperator* op = new BiggestDemandDestroyOperator(
				this->running_solution,
				this->data_obj.demand,
				this->random_noise,
				this->capa_error_weight,
				this->frame_error_weight,
				this->mean_removal);

			destroy_functors.push_back(*op);
		}
		else if (operator_name == "time_destroy") {
			WorstTravelTimeDestroyOperator* op = new WorstTravelTimeDestroyOperator(
				this->running_solution,
				this->random_noise,
				this->capa_error_weight,
				this->frame_error_weight,
				this->mean_removal);

			destroy_functors.push_back(*op);
		}
		else if (operator_name == "worst_destroy") {
			WorstRemovalDestroyOperator* op = new WorstRemovalDestroyOperator(
				this->running_solution,
				this->random_noise,
				this->capa_error_weight,
				this->frame_error_weight,
				this->mean_removal);

			destroy_functors.push_back(*op);
		}
		else if (operator_name == "node_pair_destroy") {
			NodePairDestroyOperator* op = new NodePairDestroyOperator(
				this->running_solution,
				this->node_pair_potential_matrix,
				this->random_noise,
				this->capa_error_weight,
				this->frame_error_weight,
				this->mean_removal);

			destroy_functors.push_back(*op);
		}
		else if (operator_name == "shaw_destroy") {
			ShawDestroyOperator* op = new ShawDestroyOperator(
				this->running_solution,
				9, // distance weight
				3, // window weight
				2, // demadn weight
				5, // veh weight
				this->data_obj.norm_distance_matrix,
				this->data_obj.norm_start_window_matrix,
				this->data_obj.norm_end_window_matrix,
				this->data_obj.norm_demand_matrix,
				this->random_noise,
				this->mean_removal,
				this->capa_error_weight,
				this->frame_error_weight);

			destroy_functors.push_back(*op);
		}
		else if (operator_name == "distance_similarity") {
			ShawDestroyOperator* op = new ShawDestroyOperator(
				this->running_solution,
				1, // distance weight
				0, // window weight
				0, // demadn weight
				0, // veh weight
				this->data_obj.norm_distance_matrix,
				this->data_obj.norm_start_window_matrix,
				this->data_obj.norm_end_window_matrix,
				this->data_obj.norm_demand_matrix,
				this->random_noise,
				this->mean_removal,
				this->capa_error_weight,
				this->frame_error_weight);

			destroy_functors.push_back(*op);
		}
		else if (operator_name == "window_similarity") {
			ShawDestroyOperator* op = new ShawDestroyOperator(
				this->running_solution,
				0, // distance weight
				1, // window weight
				0, // demand weight
				0, // veh weight
				this->data_obj.norm_distance_matrix,
				this->data_obj.norm_start_window_matrix,
				this->data_obj.norm_end_window_matrix,
				this->data_obj.norm_demand_matrix,
				this->random_noise,
				this->mean_removal,
				this->capa_error_weight,
				this->frame_error_weight);

			destroy_functors.push_back(*op);
		}
		else if (operator_name == "demand_similarity") {
		ShawDestroyOperator* op = new ShawDestroyOperator(
			this->running_solution,
			0, // distance weight
			0, // window weight
			1, // demand weight
			0, // veh weight
			this->data_obj.norm_distance_matrix,
			this->data_obj.norm_start_window_matrix,
			this->data_obj.norm_end_window_matrix,
			this->data_obj.norm_demand_matrix,
			this->random_noise,
			this->mean_removal,
			this->capa_error_weight,
			this->frame_error_weight);

		destroy_functors.push_back(*op);
		}
		else {
			throw invalid_argument("At least one of the provided destroy operators unknown");
		}
	}

	return destroy_functors;
}

/**
	Utility function to generate all destroy_functors based on the given config.
	This allows a comprehensive computational experiment with varriing operators!

	Annotation:
	Mostly used as initialization input for the roulette wheel mechanism
*/
vector<function<void(vector<int>)>> ALNS::get_insertion_functors() {
	// No operator supplied, pass greedy as default
	if (this->operator_names_r.size() == 0) {
		cout << "No insertion operator supplied, take basic_greedy as default" << endl;
		this->operator_names_r.push_back(string("basic_greedy"));
	}

	vector<function<void(vector<int>)>> repair_functors;

	for (string operator_name : this->operator_names_r) {
		if (operator_name == "basic_greedy") {
			BasicGreedyInsertionOperator* op = new BasicGreedyInsertionOperator(this->running_solution, this->capa_error_weight, this->frame_error_weight);
			repair_functors.push_back(*op);
		}
		else if (operator_name == "random_greedy") {
			RandomGreedyInsertionOperator* op = new RandomGreedyInsertionOperator(this->running_solution, this->capa_error_weight, this->frame_error_weight);
			repair_functors.push_back(*op);
		}
		else if (operator_name == "deep_greedy") {
			DeepGreedyInsertionOperator *op = new DeepGreedyInsertionOperator(this->running_solution, this->capa_error_weight, this->frame_error_weight);
			repair_functors.push_back(*op);
		}
		else if (operator_name == "2_regret") {
			KRegretInsertionOperator *op = new KRegretInsertionOperator(this->running_solution, 2, this->capa_error_weight, this->frame_error_weight);
			repair_functors.push_back(*op);
		}
		else if (operator_name == "3_regret") {
			KRegretInsertionOperator *op = new KRegretInsertionOperator(this->running_solution, 3, this->capa_error_weight, this->frame_error_weight);
			repair_functors.push_back(*op);
		}
		else if (operator_name == "5_regret") {
			KRegretInsertionOperator *op = new KRegretInsertionOperator(this->running_solution, 5, this->capa_error_weight, this->frame_error_weight);
			repair_functors.push_back(*op);
		}
		else if (operator_name == "beta_hybrid") {
			BetaHybridInsertionOperator* op = new BetaHybridInsertionOperator(this->running_solution, 3, this->capa_error_weight, this->frame_error_weight);
			repair_functors.push_back(*op);
		}
		else {
			throw invalid_argument("At least one of the provided insertion operators unknown");
		}
	}
	return repair_functors;
}

/**
Utility functions to keep all historic informations
up to date.
Relevant for some operators!
*/
void ALNS::update_historic_matrices() {
	for (vector<int> &route : this->running_solution.solution_representation) {

		// start with depot
		int prev_node_id = 0;
		if (route.size() > 0) {
			for (int customer_id : route) {
				// log best positional capabilities of position
				if (this->node_pair_potential_matrix[prev_node_id][customer_id + 1] > this->running_solution.driving_time) {
					this->node_pair_potential_matrix[prev_node_id][customer_id + 1] = this->running_solution.driving_time;
				}

				// log useage matrix of solutions
				this->node_pair_useage_matrix[prev_node_id][customer_id + 1]++;
				prev_node_id = customer_id + 1;
			}

			// last node -> depot
			this->node_pair_potential_matrix[prev_node_id][0] = this->running_solution.driving_time;
			this->node_pair_useage_matrix[prev_node_id][0]++;
		}
	}
}

/**
Utility function to update infeasibiliy weights as demanded
*/
void ALNS::update_weights() {
	double current_inf = this->inf_count / 100;

	// case 1: Not enough infeasibility
	if ((current_inf + 0.05) < this->target_inf) {
		this->capa_error_weight *= 0.85;
		this->frame_error_weight *= 0.85;
	} // case 2: too much infeasibility
	else if ((current_inf - 0.05) > this->target_inf) {
		this->frame_error_weight *= 1.2;
		this->capa_error_weight *= 1.2;
	}

	// Reevaluate all solutions with ne weights
	// (No best solution -> only feasible solutions possibel in there)
	this->current_solution.set_quality(this->capa_error_weight, this->frame_error_weight);
	this->running_solution.set_quality(this->capa_error_weight, this->frame_error_weight);
}

/**
Main function for the ALNS algorithm
Uses all standard input parameter provided in the interface
*/
Solution ALNS::solve() {
	// 1) Initialization
	// 1.0) The operators and wheels have been initialized in the constructor
	// 1.1) Initialization of running solution, current, best solution (S_i, S_c, S_b)
	this->initialization();

	// 1.2) Set initial temperature (based on solution quality)
	double current_temperature = this->init_temperature*this->running_solution.solution_quality; // implicitly depending on the number of customers

	// 1.3) Set current start point and iteration tracking
	int iteration = 0;
	int iteration_wi = 0; // nr iterations without improvement
	int iteration_inf = 0; // current infeasibility pointer for vector
	__int64 start = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count(); // start time
	
	// 1.4) Use prev time stamp for current state tracking
	__int64 prev_time_stamp = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();

	while (((prev_time_stamp - start) / 1000 < this->max_time) && (iteration_wi < this->max_iterations)) {
		// 2) Select operators (based on current weights)
		function<vector<int>()> &destroy_operator = this->destroy_wheel.get_random_operator();
		function<void(vector<int>)> &insertion_operator = this->insertion_wheel.get_random_operator();

		// 3) Apply destroy and insertion operators
		// 3.1) Perform operation
		// We do not discriminate insertion / destroy to avoid overfitting
		__int64 time_stamp = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
		vector<int> removed_customers = destroy_operator();
		insertion_operator(removed_customers);

		// 3.2) Log solution for historic evaluation
		update_historic_matrices();

		// 4) Process solution and evaluate it against other solutions
		double operation_benefit = 0;

		// 4.1) Identify if we already visited the solution (if so -> ignore best comparison)
		// DEP: More info but more mem size_t count_sol = this->visited_solutions.count(this->running_solution);
		size_t count_sol = this->visited_solutions.count(this->running_solution.solution_representation);

		// (if the previous solution is not visited count(0)->save it)
		if (!count_sol) {
			operation_benefit += this->functor_reward_unique;
		}

		// 4.2) Compare running solution with current solution
		double running_quality = this->running_solution.solution_quality;
		double current_quality = this->current_solution.solution_quality;


		if (running_quality < current_quality) {
			// Always accept strictly better solutions
			this->current_solution = this->running_solution;
			operation_benefit += this->functor_reward_accept_better;
		}
		else {
			double diversity_relevance = exp(-(running_quality - current_quality) / current_temperature);

			double diversity = this->running_solution.get_diversity(this->node_pair_useage_matrix, iteration);
			operation_benefit += diversity * diversity_relevance * this->functor_reward_divers;

			// Not better than current -> cannot be better than global best
			// (except in edge cases) because time != quality
			operation_benefit += this->functor_penalty;

			// Randomly accept based on temperature
			double random_int = UNI;

			if (random_int < diversity_relevance) {
				this->current_solution = this->running_solution;
			}
		}

		// 4.3) Evaluate overall solution acceptance
		if ((this->running_solution.driving_time < this->solution.driving_time) & (this->running_solution.is_feasible)) {
			this->solution = this->running_solution;
			operation_benefit += this->functor_reward_best;

			// Reset the w.o. improvement run traits
			iteration_wi=0;

			if (shakeup_log > 0) {
				this->mean_removal = ceil(log(this->data_obj.nr_customer) / log(this->mean_removal_log));
			}
		}
		else {
			// No improvement of overall solution
			iteration_wi++;

			// Seems usefull? Else remvoe it!
			if (shakeup_log > 0) {
				this->mean_removal = ceil((log(iteration_wi + 1) / log(this->shakeup_log))*(log(this->data_obj.nr_customer) / log(this->mean_removal_log)));
			}
		}

		// 5) Prepare for next iteration
		// 5.1) Save visited solutions for future iterations 
		// (if the previous solution is not visited count(0)-> save it)
		if (!count_sol) {
			// Track the solution generation time to analyse later on!
			this->visited_solutions[this->running_solution.solution_representation] = time_stamp;
			// DEPRECTATED: (more mem but more info) this->visited_solutions[this->running_solution] = time_stamp;
		}

		// 5.2) Adjust infeasibility
		if (!this->running_solution.is_feasible) {
			this->inf_count++;
		}

		// Adjust weights if the infeasibility rate is not sufficient
		if (iteration_inf == 99) { // 99 because we start at 0
			this->update_weights();
			this->inf_count = 0;
			iteration_inf = 0;
		}
		else {
			iteration_inf++;
		}

		// 5.2) Update last stats (consider the complete execution time to be fair)
		__int64 execution_time = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - time_stamp + 1;

		this->destroy_wheel.update_stats(operation_benefit/execution_time); // Keep track of last functor implicitly
		this->insertion_wheel.update_stats(operation_benefit/execution_time);

		if (iteration % (this->operator_names_d.size()*wheel_memory_length) == 0) {
			this->destroy_wheel.update_weights();
		}

		if (iteration % (this->operator_names_r.size()*wheel_memory_length) == 0) {
			this->insertion_wheel.update_weights(); // Keep track of the removed and added weights implicitly
		}

		// 5.3) Update cooling rate
		current_temperature *= cooling_rate;
		iteration++;

		// 5.4) Set running solution
		this->running_solution = this->current_solution; // Deep copy (self declared)

		// 5.6) reset time tracking
		prev_time_stamp = time_stamp;
	}

	// Report the final KPIs without service times (standard reporting)
	this->iterations = iteration;
	this->solution_time_ms = prev_time_stamp - start;
	this->value = this->solution.driving_time;

	cout << "INFO:c++: solving (DONE)" << endl;
	return this->solution;
}
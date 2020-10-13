/*
	This is the main object for the adaptive large neighborhood search algorithm


	Annotations:

		Time computation:

		Buckets:
			The load bucketing of the demand levels is done lineary.
			The size of a bucket is selected in the beginning. Each bucket has the same size
			except for the last bucket. The last bucket is of size [total_demand]%[interval_size].

			For simplicity reasons we assume that the weight of the bucket to be the start value of the
			bucket interval. (e.g. 0 if its between 0 and interval_size)

	A detailed description of the ALNS algorithm can be found in the thesis
*/
#pragma once
#include "alns_data.h"
#include "solution.h"
#include "operator.h"
#include "roulette_wheel.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>
#include <chrono>

class ALNS {
private:
	// private attributes (functionality settings)
	// simulated annealing settings
	int const max_time;
	double const max_iterations;
	double const init_temperature;
	double const cooling_rate;
	
	// roulette wheel settings
	int const wheel_memory_length;
	double const wheel_parameter;
	double const functor_min_weight;
	double const functor_reward_best;
	double const functor_reward_unique;
	double const functor_reward_accept_better;
	double const functor_penalty;
	double functor_reward_divers = 9;

	// evaluation settings
	double const target_inf;

	// operator settings
	std::vector<std::string> operator_names_d;
	std::vector<std::string> operator_names_r;


	double const random_noise;

	double const shakeup_log;
	double const mean_removal_log;
	double mean_removal;

	// private dynamic attributes
	double inf_count;
	std::vector<std::vector<double>> node_pair_potential_matrix; // node based!
	std::vector<std::vector<int>> node_pair_useage_matrix; // node based!

	// pointer to all dynamic attributes -> Dont copy the contents!
	Solution current_solution;

public:
	// Necessary runtime public instance
	Solution running_solution;

	// Data interface
	// DEPRECTATED: (more mem but more info) std::unordered_map<Solution, __int64> visited_solutions;
	std::unordered_map<std::vector<std::vector<int>>, __int64> visited_solutions;

	double capa_error_weight;
	double frame_error_weight;

	DestroyRouletteWheel destroy_wheel;
	InsertionRouletteWheel insertion_wheel;

	ALNSData &data_obj; // We use a pointer so that we dont have to copy the complete object!
	Solution solution; // Final solution if once solved!!
	double value=-1;
	int iterations=0;
	int solution_time_ms;


	// constructors
	ALNS(ALNSData &data_obj, // We want a normal object interface to easily export it to python
		std::vector<std::string> destroy_operator_names,
		std::vector<std::string> repair_operator_names,
		int max_time = 600,
		double max_iterations = 10000,
		double init_temperature = 0.001,
		double cooling_rate = 0.99975,
		int wheel_memory_length = 20,
		double wheel_parameter = 0.1,
		double functor_reward_best = 33,
		double functor_reward_accept_better = 13,
		double functor_reward_unique = 9,
		double functor_reward_divers = 9,
		double functor_penalty = 0,
		double functor_min_weight = 1,
		double random_noise = 0,
		double target_inf = 0.2,
		double shakeup_log_log = 20,
		double mean_removal_log = 2);

	// public functions
	void initialization();
	void update_historic_matrices();
	void update_weights();
	std::vector < std::function<std::vector<int>()>> get_destroy_functors();
	std::vector< std::function<void(std::vector<int>)>> get_insertion_functors();
	Solution solve(); // give all tuneable parameters to "solve"
};

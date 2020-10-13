#pragma once
#include "alns_data.h"
#include <vector>
#include <exception>

class Solution {
private:
	// evaluation functions
	void set_chromosomes();
	void set_load_levels();
	void set_solution_time();
	void set_capa_error();
	void set_frame_error();
	void set_is_feasible();

public:
	// ATTRIBUTES
	// The data object to which the solution belongs
	std::reference_wrapper<ALNSData> data_obj; // Must be a poitner because references are not reassignable

	// The solution representation of a solution is a vector of vectors containing ints
	// Each integer represents a customer id starting from 0 and going to nr_customers -1
	std::vector<std::vector<int>> solution_representation;
	std::vector<int> route_chromosome;

	// Customer based info
	std::vector<double> loads;
	std::vector<int> load_levels;
	std::vector<double> arrival_times;
	std::vector<double> departure_times;

	// KPIs of the a solution	// 
	double driving_time;			// 1) Driving time of all routes			
	double capa_error = 0.0;		// 2) Capacity error (capped at a certain max)
	double frame_error = 0.0;		// 3) Potential error of the expected customer time windows
	bool is_feasible;				// 4) Feasibility of the solution
	double solution_quality;		// 5) Solution quality based on the time and infeasibility

	// Route KPIs of the a solution
	std::vector<double> start_times;  // Describe the start time of a ceratinr oute
	std::vector<double> route_driving_times;
	std::vector<double> route_capa_errors;
	std::vector<double> route_frame_errors;
	std::vector<double> route_qualities;

	// FUNCTIONS
	// Base constructor
	Solution(ALNSData &data_obj) : data_obj(data_obj){this->driving_time = std::numeric_limits<double>::max();}; // Default constructor (for init)

	// Main constructor
	Solution(ALNSData &data_obj, // Main Constructor for new solutions
		const std::vector<std::vector<int>> solution_rep,
		const double capa_error_weight,
		const double frame_error_weight);

	// Pickle constructor (ignore aspects)
	Solution(ALNSData &data_obj,
		std::vector<std::vector<int>> solution_representation,
		std::vector<double> loads,
		std::vector<double> arrival_times,
		std::vector<double> departure_times,
		double driving_time,
		double solution_quality,
		double capa_error,
		double frame_error,
		bool is_feasible,
		std::vector<double> start_times,
		std::vector<double> route_driving_times);

	// Deep assignment operator
	Solution& operator=(const Solution &obj);


	bool operator==(Solution const& other) const { // Equality comparision operator (For Hash map look up in case two objects are mapped to an identical key)
		return solution_representation == other.solution_representation;
		// We know that the identical solution representation leads to identical stats. -> only solution rep
	}


	void set_quality(const double capa_error_weight, const double frame_error_weight);

	void evaluate_solution(double capa_error_weight, double frame_error_weight);

	void evaluate_change(
		const int route_id,
		const int ins_pos,
		const double capa_error_weight,
		const double frame_error_weight);

	double get_diversity(std::vector<std::vector<int>> &node_pair_useage_matrix, int iteration);

};


// Define custom exception
struct InfeasibilityException : public std::exception {
	const char * what() const throw () {
		return "Max infeasibility is exceeded. WARNING: Object might be deprecated";
	}
};

/**
	Define the hashing procedure for objects of type "solution"
	Defined in the std namespace so that the hashmap finds it.

	Needed of the efficient solution logging!

	We use the following properties for hasing:
		1) customer_id order
		2) route association of the customer_id
		3) route_size
*/
namespace std {
	template<>
	struct hash<Solution> {
		std::size_t operator()(const Solution& sol) const {
			// starting seed can be arbitrary. (could also be 0)
			std::size_t seed = sol.solution_representation.size();

			for (auto& route : sol.solution_representation) {
				// consider the the size of each subsequent route to avoid bias
				// and ensure a difference between[1,2][3] and [1][2,3]
				seed ^= route.size() + 0x9e3779b9 + (seed << 6) + (seed >> 2);

				for (auto& node_id : route) {
					// basic hashing procedure used in the boost library
					// 0x9e3779b9 is the bias ("magic number") derived by pi
					seed ^= node_id + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				}
			}
			return seed;
		}
	};

	template<>
	struct hash<std::vector<std::vector<int>>> {
		std::size_t operator()(const std::vector<std::vector<int>>& v) const {
			// starting seed can be arbitrary. (could also be 0)
			std::size_t seed = v.size();

			for (auto& route : v) {
				// consider the the size of each subsequent route to avoid bias
				// and ensure a difference between[1,2][3] and [1][2,3]
				seed ^= route.size() + 0x9e3779b9 + (seed << 6) + (seed >> 2);

				for (auto& node_id : route) {
					// basic hashing procedure used in the boost library
					// 0x9e3779b9 is the bias ("magic number") derived by pi
					seed ^= node_id + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				}
			}
			return seed;
		}
	};
}


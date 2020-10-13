/**
	This header file includes the description and interface of all operators
	An operators can be of either type:
		1) Destroy operator
		2) Repair operator

	Every operator must be declared as a functor in order to be useable in the roulette wheel mechanism

	Assumption: The route has been reevaluated after the customers are removed

*/
#pragma once
#include "tools.h"
#include "alns_data.h"
#include "solution.h"
#include <vector>
#include <limits>


class Operator {
public:
	// Utility information
	const double &capa_error_weight;
	const double &frame_error_weight;

	Solution & solution_obj;

	explicit Operator(Solution &sol_obj, 
		double &capa_error_weight, 
		double &frame_error_weight) 
		: solution_obj(sol_obj),
		capa_error_weight(capa_error_weight),
		frame_error_weight(frame_error_weight)
	{}
};


class DestroyOperator : public Operator{
public:
	// default constructor for each destroy operator
	explicit DestroyOperator(Solution & sol_obj,
		double &capa_error_weight,
		double &frame_error_weight) 
		: Operator(sol_obj, capa_error_weight, frame_error_weight){}

	virtual std::vector<int> operator()() = 0; // return removed customers
};


class RepairOperator : public Operator {
public:
	explicit RepairOperator(Solution & sol_obj,
		double &capa_error_weight,
		double &frame_error_weight)
		: Operator(sol_obj, capa_error_weight, frame_error_weight) {}

	virtual void operator()(std::vector<int> removed_customers) = 0;
};

/**
	DESTROY OPERATOR 1
	Radnom destroy
*/
class RandomDestroyOperator : public DestroyOperator {
private:
	double const &mean_removal;		// Average number of customers removed (evenly distributed)

public:
	RandomDestroyOperator(Solution &sol_obj,
		double &capa_error_weight,
		double &frame_error_weight,
		const double &mean_rem) :
		DestroyOperator(sol_obj, capa_error_weight, frame_error_weight),
		mean_removal(mean_rem) {};

	std::vector<int> operator()() override; // TODO
};


/**
	DESTROY OPERATOR 2
	Route destroy
*/
class RandomRouteDestroyOperator : public DestroyOperator {
public:
	RandomRouteDestroyOperator(Solution & sol_obj,
		double &capa_error_weight,
		double &frame_error_weight) :
		DestroyOperator(sol_obj, capa_error_weight, frame_error_weight) {}

	std::vector<int> operator()() override; // TODO
};


/**
	DESTROY OPERATOR 3
	Worst demand removal (random)
*/
class BiggestDemandDestroyOperator : public DestroyOperator {
private:
	const std::vector<int>  demand_ranks; // Ranking based on demand, smallest first
	const double rnd_factor;
	const double &mean_removal;

public:
	BiggestDemandDestroyOperator(Solution & sol_obj,
		std::vector<double> demands,
		double rnd_factor,
		double &capa_error_weight,
		double &frame_error_weight,
		double &mean_rem)
		: DestroyOperator(sol_obj, capa_error_weight, frame_error_weight),
		demand_ranks(tools::get_ranks(demands)),
		rnd_factor(rnd_factor),
		mean_removal(mean_rem){};

	std::vector<int> operator()() override;
};


/**
	DESTROY OPERATOR 4
	Worst travel time removal (random)

	Annotation:
	The travel time ranking is dynamic.
	Tracking this in each solution seems inefficient
	-> computed when needed (in this operator)
*/
class WorstTravelTimeDestroyOperator : public DestroyOperator {
private:
	const double rnd_factor;
	double const &mean_removal;

public:
	WorstTravelTimeDestroyOperator(Solution &sol_obj,
		double rnd_factor,
		double &capa_error_weight,
		double &frame_error_weight,
		double &mean_rem) :
		DestroyOperator(sol_obj, capa_error_weight, frame_error_weight),
		rnd_factor(rnd_factor),
		mean_removal(mean_rem){};

	std::vector<int> operator()() override;
};

/**
	DESTROY OPERATOR 5
	Compare quality before and after removal.
	Select customer with biggest benefit.
	(Reverse greedy insertion)
*/
class WorstRemovalDestroyOperator : public DestroyOperator {
private:
	const double rnd_factor;
	double const &mean_removal;

public:
	WorstRemovalDestroyOperator(Solution &sol_obj,
		double rnd_factor,
		double &capa_error_weight,
		double &frame_error_weight,
		double &mean_rem) :
		DestroyOperator(sol_obj, capa_error_weight, frame_error_weight),
		rnd_factor(rnd_factor),
		mean_removal(mean_rem) {};

	std::vector<int> operator()() override;
};

/**
	DESTROY OPERATOR 6
	History based note-pair removal
*/
class NodePairDestroyOperator : public DestroyOperator {
private:
	std::vector<std::vector<double>> &node_pair_potential_matrix;
	const double rnd_factor;
	double const &mean_removal;

public:
	NodePairDestroyOperator(Solution &sol_obj,
		std::vector<std::vector<double>> &node_pair_potential_matrix,
		double rnd_factor,
		double &capa_error_weight,
		double &frame_error_weight,
		double &mean_rem) :
		DestroyOperator(sol_obj, capa_error_weight, frame_error_weight),
		node_pair_potential_matrix(node_pair_potential_matrix),
		rnd_factor(rnd_factor),
		mean_removal(mean_rem) {};

	std::vector<int> operator()() override;
};

/**
	DESTROY OPERATOR 7
	Shaw removal operator
*/
class ShawDestroyOperator : public DestroyOperator {
private:
	const double distance_weight;
	const double window_weight;
	const double demand_weight;
	const double vehicle_weight;
	const std::vector<std::vector<double>> &norm_distance_matrix;
	const std::vector<std::vector<double>> &norm_start_window_matrix;
	const std::vector<std::vector<double>> &norm_end_window_matrix;
	const std::vector<std::vector<double>> &norm_demand_matrix;
	const double rnd_factor;
	const double &mean_removal;

public:
	ShawDestroyOperator(Solution &sol_obj,
		double distance_weight,
		double window_weight,
		double demand_weight,
		double vehicle_weight,
		std::vector<std::vector<double>> &norm_distance_matrix,
		std::vector<std::vector<double>> &norm_start_window_matrix,
		std::vector<std::vector<double>> &norm_end_window_matrix,
		std::vector<std::vector<double>> &norm_demand_matrix,
		double rnd_factor,
		double &mean_removal,
		double &capa_error_weight,
		double &frame_error_weight) :
		distance_weight(distance_weight),
		window_weight(window_weight),
		vehicle_weight(vehicle_weight),
		demand_weight(demand_weight),
		norm_distance_matrix(norm_distance_matrix),
		norm_start_window_matrix(norm_start_window_matrix),
		norm_end_window_matrix(norm_end_window_matrix),
		norm_demand_matrix(norm_demand_matrix),
		rnd_factor(rnd_factor),
		mean_removal(mean_removal),
		DestroyOperator(sol_obj, capa_error_weight, frame_error_weight) {};

	std::vector<int> operator()() override;
};

/**
	REPAIR OPERATOR 1
	Basic greedy insertion
*/
class BasicGreedyInsertionOperator : public RepairOperator {
public:
	// constructor (Take base info from the ALNS object)
	BasicGreedyInsertionOperator(
		Solution & sol_obj,
		double &capa_error_weight,
		double &frame_error_weight) :
		RepairOperator(sol_obj, capa_error_weight, frame_error_weight) {}

	void operator()(std::vector<int> removed_customers) override;
};

/**
	REPAIR OPERATOR 2
	Random greedy insertion
*/
class RandomGreedyInsertionOperator : public RepairOperator {
public:
	// constructor
	RandomGreedyInsertionOperator(
		Solution &sol_obj,
		double &capa_error_weight,
		double &frame_error_weight):
		RepairOperator(sol_obj, capa_error_weight, frame_error_weight) {}
	
	void operator()(std::vector<int> removed_customers) override;
};

/*
	REPAIR OPERATOR 3
	Deep greedy
*/
class DeepGreedyInsertionOperator : public RepairOperator {
public:
	//constructor
	DeepGreedyInsertionOperator(
		Solution & sol_obj,
		double &capa_error_weight,
		double &frame_error_weight) :
		RepairOperator(sol_obj, capa_error_weight, frame_error_weight) {}

	void operator()(std::vector<int> removed_customers) override;
};


/*
	REPAIR OPERATOR 4
	k-regret insertion

*/
class KRegretInsertionOperator : public RepairOperator {
private:
	// Define the number of elements that we want to calculate the regret value for
	const int k_regret; 

public:
	// constructor
	KRegretInsertionOperator(
		Solution &sol_obj,
		int k_regret,
		double &capa_error_weight,
		double &frame_error_weight) :
		RepairOperator(sol_obj, capa_error_weight, frame_error_weight),
		k_regret(k_regret) {};

	void operator()(std::vector<int> removed_customers) override;
};

/*
	REPAIR OPERATOR 5
	beta-hybrid insertion heuristic

*/
class BetaHybridInsertionOperator : public RepairOperator {
private:
	const int beta;

public:
	BetaHybridInsertionOperator(
		Solution &sol_obj,
		int beta,
		double &capa_error_weight,
		double &frame_error_weight) :
		RepairOperator(sol_obj, capa_error_weight, frame_error_weight),
		beta(beta) {};

	void operator()(std::vector<int> removed_customers) override;
};
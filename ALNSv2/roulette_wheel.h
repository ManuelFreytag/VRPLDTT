#pragma once
#include <vector>
#include <queue>
#include <functional>

class RouletteWheel {
public:
	// wheel parameters
	double wheel_parameter; // impact of historic results
	int memory_length; // Number of iterations considered 
	double min_weight;

	// Adaptive components
	int nr_functors;
	int last_functor_id;
	std::vector<double> weights; // size functors
	std::vector<double> scores; // size of functors
	std::vector<int> nr_uses; // size of functors

	// initialize
	RouletteWheel() {};

	RouletteWheel(int nr_functors, double wheel_par, int memory_length, double min_weight)
	{
		this->wheel_parameter = wheel_par;
		this->memory_length = memory_length;
		this->min_weight = min_weight;

		this->nr_functors = nr_functors;
		this->weights = std::vector<double>(nr_functors, (1.0/nr_functors));
		this->scores = std::vector<double>(nr_functors, 0);
		this->nr_uses = std::vector<int>(nr_functors, 0);
	};

	// match strings to functor objects (simpler)
	int get_random_functor_id();
	void update_stats(double new_score);
	void update_weights();
};


class DestroyRouletteWheel : public RouletteWheel {
private:
	std::vector<std::function<std::vector<int>()>> destroy_functors;

public:
	DestroyRouletteWheel() {};

	DestroyRouletteWheel(std::vector<std::function<std::vector<int>()>> destroy_functors, double wheel_par, int memory_length, double min_weight) : 
		RouletteWheel(destroy_functors.size(), wheel_par, memory_length, min_weight)
	{
		this->destroy_functors = destroy_functors;
	}

	std::function<std::vector<int>()>& get_random_operator();
};


class InsertionRouletteWheel : public RouletteWheel {
private:
	std::vector<std::function<void(std::vector<int>)>> repair_functors;

public:
	InsertionRouletteWheel() {};

	InsertionRouletteWheel(std::vector<std::function<void(std::vector<int>)>> repair_functors, double wheel_par, int memory_length, double min_weight)
		: RouletteWheel(repair_functors.size(), wheel_par, memory_length, min_weight) 
	{
		this->repair_functors = repair_functors;
	}

	std::function<void(std::vector<int>)>& get_random_operator();
};


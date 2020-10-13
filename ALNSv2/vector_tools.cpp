#include "tools.h"
#include <vector>
#include <math.h>


using namespace std;

/**
	create a range containing elements from min to max

	max is not included in the range anymore!
*/
vector<int> tools::range(int max, int min) {
	vector<int> range_vec;
	range_vec.reserve(max - min);

	for (int i = min; i < max; i++) {
		range_vec.push_back(i);
	}

	return range_vec;
}

vector<vector<double>> tools::normalize_matrix_copy(vector<vector<double>> &m, double min, double max) {
	vector<vector<double>> new_m(m.size(), vector<double>(m[0].size()));

	double norm_base = max - min;

	for (unsigned int i = 0; i < m.size(); i++) {
		vector<double> &v2 = m[i];

		for (unsigned int j = 0; j < v2.size(); j++) {
			new_m[i][j] = (m[i][j] - min) / norm_base;
		}
	}
	return new_m;
}

void tools::normalize_matrix(vector<vector<double>> &m, double min, double max) {
	double norm_base = max - min;

	for (unsigned int i = 0; i < m.size(); i++) {
		vector<double> &v2 = m[i];

		for (unsigned int j = 0; j < v2.size(); j++) {
			m[i][j] = (m[i][j] - min) / norm_base;
		}
	}
}

/*
Utility function to generate a normalized distance matrix
Use of min / max normalization
*/
vector<vector<double>> tools::get_norm_distance_matrix(vector<double> &v1) {
	double min = 0;
	double max = 0;

	// 1) Get normal matrix
	vector<vector<double>> distance_matrix(v1.size(), vector<double>(v1.size()));

	for (unsigned int i=0; i < v1.size(); i++) {
		for (unsigned int j = 0; j < v1.size(); j++) {
			double distance = std::abs(v1[i] - v1[j]);

			distance_matrix[i][j] = distance;

			if (distance < min) {
				min = distance;
			}
			
			if (distance > max) {
				max = distance;
			}
		}
	}

	// 2) Normalize values
	tools::normalize_matrix(distance_matrix, min, max);
	return distance_matrix;
}

// reduce 3 dim matrix to 2 dim matrix
std::vector<std::vector<double>> tools::reduce_dim(std::vector<std::vector<std::vector<double>>> &m) {
	vector<vector<double>> new_m(m.size(), vector<double>(m[0].size()*m[0][0].size()));

	for (unsigned int i = 0; i < m.size(); i++) {
		for (unsigned int j = 0; j < m[i].size(); j++) {
			for (unsigned int n = 0; n < m[i][j].size(); n++) {
				new_m[i][m[i][j].size()*j + n] = m[i][j][n];
			}
		}
	}

	return new_m;
}
"""
This file contains the main macro to read the base data and build a valid data object

"""
import logging
import pandas as pd

from tools.bing_maps_api import BingMapsApi
from ALNSv2 import ALNSData

"""
This file contains the main macro to read the base data and build a valid data object

"""
from os.path import isfile, join, exists
import logging
import pandas as pd

from ALNSv2 import ALNSData


def build_data_object(path,
                      nr_vehicles=None,
                      nr_load_buckets=10,
                      weight_interval_size=0,
                      vehicle_capacity=150,
                      vehicle_weight=140):
    # Get slope and distance matrix
    def get_elevation_matrix(elevations):
        """
        Get the elevation matrix for a list elevations (simple)

        Annotation:
        We report the elevation in kms so that it is inline with the km reporting of the distances

        """
        nr_nodes = len(elevations)
        elevation_matrix = [[0 for i in range(nr_nodes)] for j in range(nr_nodes)]

        for i in range(nr_nodes):
            for j in range(i + 1, nr_nodes):
                # altitude difference to get from i to j
                elevation_distance = (elevations[j] - elevations[i])
                elevation_matrix[i][j] = elevation_distance
                elevation_matrix[j][i] = -elevation_distance  # obviously the elevation is shifted now
        return elevation_matrix

    # Some basic input checks
    data = pd.read_csv(path, index_col=0)

    nr_nodes = data.shape[0]
    nr_depots = 1
    nr_customers = nr_nodes - nr_depots
    weight_interval_size = weight_interval_size
    vehicle_weight = vehicle_weight
    vehicle_capacity = vehicle_capacity

    # set standard settings
    if nr_vehicles is None:
        nr_vehicles = nr_customers
    else:
        nr_vehicles = nr_vehicles

    distance_matrix = data.loc[0:20, "0":"20"].values
    logging.info("Distance matrix successfully retrieved")

    # get network information
    altitude = data["elevation"].values
    elevation_matrix = get_elevation_matrix(altitude)

    logging.info("Elevation matrix successfully built")

    # get process information
    demand_array = data["demand"].values[nr_depots:]
    window_start = data["tw a"].values[nr_depots:]
    window_end = data["tw b"].values[nr_depots:]
    service_times = data["s"].values[nr_depots:]

    # Build the data object.  It performs internal preprocessing!
    logging.info("General info retrieved. Preprocessing started.")

    data_object = ALNSData(nr_veh=nr_vehicles,
                           nr_nodes=nr_nodes,
                           nr_customers=nr_customers,
                           demand=demand_array,
                           service_times=service_times,
                           start_window=window_start,
                           end_window=window_end,
                           elevation_m=elevation_matrix,
                           distance_m=distance_matrix,
                           load_bucket_size=weight_interval_size,
                           nr_load_buckets=nr_load_buckets,
                           vehicle_weight=vehicle_weight,
                           vehicle_capacity=vehicle_capacity)

    logging.info("Preprocessing done. Data object built")
    return data_object

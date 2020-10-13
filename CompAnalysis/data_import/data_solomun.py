from CompAnalysis.tools.distance_calc import get_distance_matrix
from ALNSv2 import ALNSData

def build_data_object(file_path):
    """
    Build the data object from a text file
    with syntax introduced by solomun

    http://neo.lcc.uma.es/vrp/vrp-instances/capacitated-vrp-with-time-windows-instances/
    """
    # 1) Parse data
    row_id = 0

    coordinates = [] #
    number_vehicles = 0 #
    nr_nodes = 0 #
    nr_customers = 0 #
    demand_array = [] #
    service_times = [] #
    window_start = [] #
    window_end = [] #
    vehicle_capacity = 0 #

    # Get base data from file
    with open(file_path, 'r') as file:
        for row in file:
            if row_id == 0:
                instance_id = row
            
            if row_id == 4:
                row_data = row.replace("         ", ",").strip().split(",")
                number_vehicles, vehicle_capacity = int(row_data[0]), int(row_data[1])
            
            # start of node data
            if row_id > 8:
                # quick and dirty
                row_data = [x for x in row.replace(" ", ",").split(",") if (x != "") and (x != "\n")]
                if not row_data:
                    break

                coordinates.append((int(row_data[1]), int(row_data[2])))
                
                if row_id > 9:
                    # start of customer data
                    demand_array.append(int(row_data[3]))
                    window_start.append(int(row_data[4]))
                    window_end.append(int(row_data[5]))
                    service_times.append(int(row_data[6]))
        
            nr_customers = len(demand_array)
            nr_nodes = nr_customers + 1
            elevations = [0]*nr_nodes
            row_id += 1
        
    time_cube = [get_distance_matrix(coordinates)] # We must give it a new artifical dimension

    # 2) Build data object
    data_object = ALNSData(nr_veh=number_vehicles,
                       nr_nodes=nr_nodes,
                       nr_customers=nr_customers,
                       demand=demand_array,
                       service_times=service_times,
                       start_window=window_start,
                       end_window=window_end,
                       time_c=time_cube,
                       vehicle_capacity=vehicle_capacity)
    return data_object


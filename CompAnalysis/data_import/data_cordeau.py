from CompAnalysis.tools.distance_calc import get_distance_matrix
from ALNSv2 import ALNSData

def parse_row(row):
    return [float(x) for x in row.replace(" ", ",").replace("\n","").split(",") if x != ""]

def build_data_object(file_path):
    """
    Parse data based on the cordeau syntax
    http://neo.lcc.uma.es/vrp/vrp-instances/description-for-files-of-cordeaus-instances/
    
    Annotation:
    Cordeau passes some irrelevant information because his templates 
    are applicatble for a wide range of problem types (e.g. PVRP, VRPTW, etc.)
    We just ignore some things (e.g. maximum route duration)
    """
    coordinates = []
    service_times = []
    demand_array = []
    window_start = []
    window_end = []
    
    with open(file_path, "r") as file:
        for row_id, row in enumerate(file):
            p_row = parse_row(row)
            # general data row
            if row_id == 0:
                p_type, number_vehicles, nr_customers, nr_days = p_row

            # Route limits
            if row_id == 1:
                route_max_duration, vehicle_capacity = p_row
                
                if route_max_duration > 0:
                    raise ValueError("Max duration not considered in the current ALNS model")
                
            # Depot information
            if row_id > 1:
                coordinates.append((p_row[1], p_row[2]))
                                  
                # customer information
                if row_id > 2:
                    if row != "" and row != "\n":
                        service_times.append(p_row[3])
                        demand_array.append(p_row[4])
                        window_start.append(p_row[-2])
                        window_end.append(p_row[-1])
          
    time_cube = [get_distance_matrix(coordinates)] # We must give it a new artifical dimension

    data_object = ALNSData(nr_veh=int(number_vehicles),
                    nr_nodes=int(nr_customers+1),
                    nr_customers=int(nr_customers),
                    demand=demand_array,
                    service_times=service_times,
                    start_window=window_start,
                    end_window=window_end,
                    time_c=time_cube,
                    vehicle_capacity=int(vehicle_capacity))
    return data_object

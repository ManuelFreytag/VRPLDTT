def get_distance_matrix(coordinates):
    """
    Get the elevation matrix for a list of coordinates
    If no elevations / altitude data is explicitly provided -> request them

    Annotation:
    We report the elevation in kms so that it is inline with the km reporting of the distances

    Args:
        coordinate_list:   List of coordinates. format: [(x, y), (x, y)]

    """
    nr_nodes = len(coordinates)        
    distance_matrix = [[0 for i in range(nr_nodes)] for j in range(nr_nodes)]

    for i in range(nr_nodes):
        for j in range(i+1, nr_nodes):
            # altitude difference to get from i to j
            euc_distance = ((coordinates[i][0] - coordinates[j][0])**2 + (coordinates[i][1] - coordinates[j][1])**2)**0.5
            distance_matrix[i][j] = euc_distance
            distance_matrix[j][i] = euc_distance # obviously the elevation is shifted now
    return distance_matrix

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
        for j in range(i+1, nr_nodes):
            # altitude difference to get from i to j
            elevation_distance = (elevations[j] - elevations[i])/1000
            elevation_matrix[i][j] = elevation_distance
            elevation_matrix[j][i] = -elevation_distance # obviously the elevation is shifted now
    return elevation_matrix
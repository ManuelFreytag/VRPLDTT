"""
Implementation of the BingMapsAPI.
Currently unused but available and usefull when building new data objects
"""
import json
import urllib.request
from urllib.parse import urlencode
import warnings

class BingMapsApi:
    """
    This Class incorporates the most necessary functions to generate new VRP test cases.

    Args:
        api_key:    A valid Bing API key (https://www.bingmapsportal.com/Report)

    """

    def __init__(self, api_key):
        self.api_key = api_key

    @staticmethod
    def __coordinates_to_str(coordinate_tuple):
        return f"{str(coordinate_tuple[0])},{str(coordinate_tuple[1])}"

    @staticmethod
    def __get_data(url):
        """
        Utility function to request data from the REST api via simple URL request.

        """
        request = urllib.request.Request(url)
        response = urllib.request.urlopen(request)
        return json.loads(response.read())["resourceSets"][0]["resources"][0]

    def request_coordinates(self, address_list, **kwargs):
        """
        # TODO
        """
        coordinates = []
        for address in address_list:
            # setup request
            request_dict = {"addressLine": address,
                            "maxResults": 1,
                            **kwargs,
                            "key": self.api_key}
            request_append = urlencode(request_dict)

            # perform REST API call
            url = f"http://dev.virtualearth.net/REST/v1/Locations?&{request_append}"
            return_data = self.__get_data(url)

            if return_data["confidence"] != "High":
                warnings.warn(f"Address confidence for {address} is not high")

            coordinates.append(tuple(return_data["geocodePoints"][0]["coordinates"]))
        return coordinates

    def request_elevation(self, coordinate_list, **kwargs):
        """
        Get a list of elevation values for a given list of coordinate tuples

        Args:
            coordinate_list:    List of tuples in form [(lat1, long1), (lat1, long2)]
            **kwargs:           Other optinal parameters.(https://msdn.microsoft.com/de-de/library/jj158961.aspx)

        Returns:
            elevation_list:     List of integers [a1, a2]
        """
        coordinate_string_list = [self.__coordinates_to_str(coordinates) for coordinates in coordinate_list]
        coordinate_string = ",".join(coordinate_string_list)

        request_dict = {"points": coordinate_string,
                        **kwargs,
                        "key": self.api_key}
        request_append = urlencode(request_dict)

        # perform REST API call
        url = f"http://dev.virtualearth.net/REST/v1/Elevation/List?{request_append}"
        return self.__get_data(url)["elevations"]

    def get_distance_matrix(self, coordinate_list, travel_mode="driving", **kwargs):
        """
        Get the nxn distance matrix for a list of coordinate tuples
        The Bing API supports at max 2500 coordinates at once.

        Args:
            coordinate_list:    List of coordinates. format: [(lat1, long1), (lat2, long2)]
            travel_mode:        Mode that should be used for route finding. Impacts the roads that can be selected
                                Possible: "driving", "walking", "commute"
            **kwargs:           Other optinal parameters.(https://msdn.microsoft.com/en-us/library/mt827298.aspx)

        Returns:
            distance_matrix:    nxn matrix containing the distance matrix from all origins to all destinations

        """
        # setup request
        coordinate_string_list = [self.__coordinates_to_str(coordinates) for coordinates in coordinate_list]
        coordinate_string = ";".join(coordinate_string_list)

        request_dict = {"origins": coordinate_string,
                        "travelMode": travel_mode,
                        **kwargs,
                        "key": self.api_key}
        request_append = urlencode(request_dict)

        # perform REST API call
        url = f"https://dev.virtualearth.net/REST/v1/Routes/DistanceMatrix?{request_append}"
        return_data = self.__get_data(url)

        # do postprocessing (e.g. transform it into array)
        nr_locations = len(coordinate_list)
        distance_matrix = [[0 for i in range(nr_locations)] for j in range(nr_locations)]

        for row in return_data["results"]:
            origin_index = row["originIndex"]
            dest_index = row["destinationIndex"]
            distance = row["travelDistance"]

            distance_matrix[origin_index][dest_index] = distance
        
        return distance_matrix

    def get_elevation_matrix(self, coordinate_list, elevations = None):
        """
        Get the elevation matrix for a list of coordinates
        If no elevations / altitude data is explicitly provided -> request them

        Annotation:
        We report the elevation in kms so that it is inline with the km reporting of the distances

        Args:
            coordinate_list:   List of coordinates. format: [(lat1, long1), (lat2, long2)]
            **kwargs:           Other optinal parameters.(https://msdn.microsoft.com/de-de/library/jj158961.aspx)

        """
        nr_nodes = len(coordinate_list)

        if elevations is None:
            elevations = self.request_elevation(coordinate_list)
        
        elevation_matrix = [[0 for i in range(nr_nodes)] for j in range(nr_nodes)]

        for i in range(nr_nodes):
            for j in range(i+1, nr_nodes):
                # altitude difference to get from i to j
                elevation_distance = (elevations[j] - elevations[i])/1000
                elevation_matrix[i][j] = elevation_distance
                elevation_matrix[j][i] = -elevation_distance # obviously the elevation is shifted now

        return elevation_matrix
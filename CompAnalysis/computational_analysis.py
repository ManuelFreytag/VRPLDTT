"""
This  is the main script for the overall project.
It combines all code fractions of c++ and python
"""
# python
import pickle
import logging
import time
import operator
import multiprocessing
from os import listdir
from os.path import isfile, join

# c++ imports
from ALNSv2 import ALNS
from ALNSv2 import ALNSData

# custom python import
from reg_tree_tuning.parameter_tuning import RegressionTreeParameterTuner

logging.basicConfig(level=logging.INFO)


def _solve_instance(file_name,
                    data_object,
                    heuristic_constructor,
                    kwargs,
                    export=True,
                    export_path="",
                    export_metrics=None):
    """
    Utility function to solve an heuristic instance
    It returns the value attached to the dummy encoded input parameters

    Args:
        data_object:                    Data object
        heuristic_constructor:          Constructor of the heuristic
        man:                            Keywords manager object
        base_kwargs:                    Base keyword arguments
        metric_name:                    Metric on which it should be reported
        export:                         Boolean: Should it be exported or not
        export_path:                    Directory path as export target
        export_metrics:                 List of metrics (of the heuristic object) that should be reported

    Returns:

    """
    # 1) Solve object
    meta_heuristic = heuristic_constructor(data_object, **kwargs)
    meta_heuristic.solve()

    # 2) Export data (with metadata)
    if export:
        if export_metrics is None:
            export_metrics = [metric_name]

        export_data = {"parameter": {**kwargs},
                       "metrics": {exp_m: getattr(meta_heuristic, exp_m) for exp_m in export_metrics},
                       "solution": meta_heuristic.solution}

        with open(join(export_path, file_name), "wb") as f:
            pickle.dump(export_data, f)


def solve_instance(constructor, instance, heuristic_kwargs):
    meta_heuristic = constructor(instance, **heuristic_kwargs)
    meta_heuristic.solve()
    return meta_heuristic.solution


class DataDispenser:
    """
    Utility class to access the data objects for the computational experiment
    """
    code_lookup = {"vrpldtt_fontaine": "vrpldtt_fontaine",
                   "vrpldtt_freytag": "vrpldtt_freytag",
                   "vrptw_solomon": "vrptw_solomon",
                   "vrptw_cordeau": "vrptw_cordeau",
                   "vrptw_gehring_homberger": "vrptw_gehring_homberger"}

    def __init__(self, base_path="C:\\Users\\manuf\\OneDrive\\Dokumente\\Universitaet\\Masterthesis\\data"):
        self.base_path = base_path

    def get_instance(self, instance_type, name, subdir=""):
        """
        Get a necessary dataobject
        Args:
            instance_type: str  Name of the instance author.
                                Allowed values are: "simple" "fontaine", "solomon", "cordeau", "homberger"

            name: str           Instance name to be used

            subdir: str         Optional subdirectory serving as the location of the file

        Returns:

        """

        def read_data(data_syntax, file_name, sub_dir=""):
            alns_data = pickle.load(open(join(self.base_path, data_syntax, "data", sub_dir, file_name), 'rb'))
            return alns_data

        if instance_type == "simple":
            nr_vehicles = 2
            nr_nodes = 4
            nr_customers = 3
            load_bucket_size = 10
            demand = [110, 100, 150]
            service_times = [10, 10, 10]
            start_window = [0, 0, 0]
            end_window = [100, 100, 100]

            distance_matrix = [[0, 3, 2, 3], [3, 0, 1.75, 4], [2, 1.75, 0, 2.5], [3, 4, 2.5, 0]]
            elevation_matrix = [[0.1, 0.2, 0, 0], [0, 0, 0.3, 0], [0, 0, 0, 0], [0, 0, 0, 0]]

            return ALNSData(nr_veh=nr_vehicles,
                            nr_nodes=nr_nodes,
                            nr_customers=nr_customers,
                            demand=demand,
                            service_times=service_times,
                            start_window=start_window,
                            end_window=end_window,
                            elevation_m=elevation_matrix,
                            distance_m=distance_matrix,
                            load_bucket_size=load_bucket_size)

        try:
            return read_data(self.code_lookup[instance_type], file_name=name, sub_dir=subdir)
        except KeyError:
            raise ValueError("instance_type is not known: Valid inputs are: pirmin, solomon, cordeau, homberger")

    def get_all_instances(self, instance_type, subdir=""):
        """
        Utility function return all data in the directory or subdirectory of an instance type.

        Annotation:
            This function is a generator!
            This means you can iterate over it but it will yield one object at a time.
            It also means that its one iteration only!
        """

        def get_object(directory):
            """
            Generator object to return the file paths
            Use generator to avoid memory issues with large directories
            """
            path = join(self.base_path, directory)
            for f in listdir(path):
                if isfile(join(path, f)):
                    yield "file", f
                else:
                    yield "subdir", join(directory, f)

        # 1) Check if the data type is correct
        try:
            inst_code = self.code_lookup[instance_type]
        except KeyError:
            raise ValueError("instance_type is not known")

        data_path = join(inst_code, "data", subdir)

        # 2) Get all the data in the directory and subdirectories
        sub_dirs = [data_path]
        while sub_dirs:
            new_subdir = sub_dirs.pop()
            for f_type, f in get_object(new_subdir):

                if f_type == "file":

                    # try to load the content
                    try:
                        file_obj = pickle.load(open(join(self.base_path, new_subdir, f), 'rb'))
                        yield {"name": f, "content": file_obj}
                    except TypeError:
                        logging.WARN(f"File {f} is not a pickleable object. SKIP")

                else:
                    sub_dirs += [f]

    def save_solution(self, meta_heuristic, instance_type, solution, name, subdir=""):
        with open(join(self.base_path, self.code_lookup[instance_type], "solution", subdir, name), 'wb') as file:
            pickle.dump(solution, file)


if __name__ == '__main__':
    """
    Small test and documentation of the speedup
    """
    # SETTINGS
    # Allowed values: "solve_single", "solve_all", "tuning"
    TASK = "solve_single"  # "tuning"
    SAVE = True
    DATA_BASE_PATH = "C:\\Users\\manuf\\OneDrive\\Dokumente\\Universitaet\\Masterthesis\\data"

    # DATA INIT
    # Get the data object used in the calculation
    disp_obj = DataDispenser(base_path=DATA_BASE_PATH)

    # PERFORM TASKS
    if TASK == "solve_single":
        inst_type = "vrpldtt_freytag"
        object_name = "Fu1_200.pkl"
        subdir = ""

        # "destroy_operators": ["random_destroy", "demand_destroy",
        #                      "node_pair_destroy",
        #                      "shaw_destroy", "distance_similarity"],
        # "repair_operators": ["2_regret", "5_regret", "beta_hybrid"],

        data_object = disp_obj.get_instance(instance_type=inst_type, name=object_name, subdir=subdir)
        # "2_regret", "3_regret", "basic_greedy", "random_greedy", "deep_greedy", "beta_hybrid"
        heuristic_kwargs = {"destroy_operators": ["random_destroy", "route_destroy", "demand_destroy", "time_destroy",
                                                  "node_pair_destroy",
                                                  "shaw_destroy", "worst_destroy", "distance_similarity",
                                                  "window_similarity",
                                                  "demand_similarity"],
                            "repair_operators": ["2_regret", "3_regret", "5_regret", "basic_greedy", "random_greedy",
                                                 "deep_greedy", "beta_hybrid"],
                            "max_time": 1800,
                            "max_iterations": 40000,
                            "initial_temperature": 0.01,
                            "cooling_rate": 0.9999,
                            "wheel_parameter": 0.35,
                            "wheel_memory_length": 10,
                            "functor_reward_best": 50,
                            "functor_reward_accept_better": 100,
                            "functor_reward_divers": 90,
                            "functor_reward_unique": 7,
                            "functor_penalty": -80,
                            "functor_min_weight": 1,
                            "shakeup_log": 10,
                            "mean_removal_log": 3.35,
                            "random_noise": 0.15,
                            "target_inf": 0.65}

        alns_object = ALNS(data_object=data_object,
                           **heuristic_kwargs)

        start = time.time()

        # Get the best feasibile solution
        best_solution = alns_object.solve()
        print(f"Solving took {time.time() - start}")
        print(alns_object.iterations)
        print(alns_object.value)

        # Get the last solution found (independent of feasibility)
        best_quality_solution = max(alns_object.visited_solutions.items(), key=operator.itemgetter(1))[0]

        if SAVE:
            disp_obj.save_solution(inst_type, best_solution, object_name, subdir)

    elif TASK == "solve_all":
        """
        Solve all instances of an instance types with [runs] replications.
        
        All replications are computeted in parallel.
        Each data instance is yielded -> Minimize memory load
        """
        inst_types = ["vrpldtt_fontaine", "vrpldtt_freytag", "vrptw_solomon", "vrptw_gehring_homberger"]
        runs = 10
        pool_size = 5

        heuristic_kwargs = {"destroy_operators": ["random_destroy", "route_destroy", "demand_destroy", "time_destroy",
                                                  "node_pair_destroy",
                                                  "shaw_destroy", "worst_destroy", "distance_similarity",
                                                  "window_similarity",
                                                  "demand_similarity"],
                            "repair_operators": ["2_regret", "3_regret", "5_regret", "basic_greedy", "random_greedy",
                                                 "deep_greedy", "beta_hybrid"],
                            "max_time": 1800,
                            "max_iterations": 10000,
                            "initial_temperature": 0.01,
                            "cooling_rate": 0.9999,
                            "wheel_parameter": 0.35,
                            "wheel_memory_length": 10,
                            "functor_reward_best": 50,
                            "functor_reward_accept_better": 100,
                            "functor_reward_divers": 90,
                            "functor_reward_unique": 7,
                            "functor_penalty": -80,
                            "functor_min_weight": 1,
                            "shakeup_log": 10,
                            "mean_removal_log": 3.35,
                            "random_noise": 0.15,
                            "target_inf": 0.65}

        for inst_type in inst_types:
            for inst in disp_obj.get_all_instances(inst_type):
                data_object = inst["content"]

                iteration_res = 0

                while iteration_res < runs:
                    with multiprocessing.Pool(processes=pool_size) as pool:
                        pool_dat = [[f"{iteration_res + it}_{inst['name']}",
                                     data_object,
                                     ALNS,
                                     heuristic_kwargs,
                                     True,
                                     join(DATA_BASE_PATH, inst_type, "solution", "all_operators"),
                                     ["value", "iterations", "solution_time_ms"]] for it in range(pool_size)]

                        future_solutions = pool.starmap_async(_solve_instance, pool_dat)
                        solutions = future_solutions.get()
                        iteration_res += pool_size


    elif TASK == "tuning":
        start = time.time()

        save_path = join(DATA_BASE_PATH, "1_tuning_results", "vrpldtt", "operators")
        # instances = [disp_obj.get_instance("homberger", file_name, "homberger_600") for file_name in
        #              ["C1_6_1.pkl", "C2_6_6.pkl", "R1_6_8.pkl", "R2_6_3.pkl", "RC1_6_4.pkl", "RC2_6_2.pkl"]]

        instances = [disp_obj.get_instance("freytag", file_name) for file_name in
                     ["Fu1_200.pkl", "Fu2_200.pkl", "Ma3_200.pkl", "Ma2_200.pkl", "Pi1_200.pkl", "Pi3_200.pkl"]]

        base_kwargs = {"max_time": 900,
                       "max_iterations": 10000,
                       "initial_temperature": 0.01,
                       "cooling_rate": 0.9999,
                       "wheel_parameter": 0.35,
                       "wheel_memory_length": 10,
                       "functor_reward_best": 50,
                       "functor_reward_accept_better": 100,
                       "functor_reward_divers": 90,
                       "functor_reward_unique": 7,
                       "functor_penalty": -80,
                       "functor_min_weight": 1,
                       "shakeup_log": 10,
                       "mean_removal_log": 3.35,
                       "random_noise": 0.15,
                       "target_inf": 0.65}

        value_domains = {"destroy_operators": ["random_destroy", "route_destroy", "demand_destroy", "time_destroy",
                                               "node_pair_destroy",
                                               "shaw_destroy", "worst_destroy", "distance_similarity",
                                               "window_similarity",
                                               "demand_similarity"],
                         "repair_operators": ["2_regret", "3_regret", "5_regret", "basic_greedy", "random_greedy",
                                              "deep_greedy", "beta_hybrid"]}

        tuner_obj = RegressionTreeParameterTuner(heuristic_constructor=ALNS,
                                                 instances=instances,
                                                 base_kwargs=base_kwargs,
                                                 value_domains=value_domains,
                                                 metric_name="value",
                                                 n_init_pop=10000,
                                                 iter=500,
                                                 export=SAVE,
                                                 export_path=save_path,
                                                 export_metrics=["value", "iterations", "solution_time_ms"])

        tuned_domains = tuner_obj.tune(processes=12, maxtasksperchild=1, maxtasksperpool=50)

        if SAVE:
            with open(join(save_path, "tuner_obj.pkl"), 'wb') as file:
                pickle.dump(tuner_obj, file)

        print(f"Tuning took {time.time() - start} seconds")
        print(tuned_domains)
    else:
        raise ValueError(f"TASK is not set to a valid input")

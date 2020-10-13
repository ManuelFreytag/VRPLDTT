"""
This script contains the regression tree tuner.

The tuner allows to tune input paramter value domains for a given algorithm.

The algorithm must:
    - Contain a function "solve"

If export = True:
    - Contain an attribute "solution"

Heuristic characteristic:
    - Must be a minimization problem
    - Can have numerical attributes
    - Can have multi choice categorical attributes
    - Can have single choice categorical attributes

Author: Manuel Freytag
"""
# standard imports
import random
import multiprocessing
import pickle
import copy
import logging
import time

# third party imports
import numpy as np
import pandas as pd
from sklearn import tree
from sklearn.tree import _tree

# custom imports
from .kwargs_manager import KwargsManager

from os.path import join

logging.getLogger().setLevel(logging.INFO)


def __export_data(export_data, export_path):
    """
    Utility function to export a solution object
    """
    timestamp = str(time.time()).replace(".", "")
    file_name = f"{timestamp}.pkl"
    with open(join(export_path, file_name), "wb") as f:
        pickle.dump(export_data, f)

    logging.info(f"saved solution file {file_name} to {export_path}")


# Must be outside of class to be pickleable -> parallization
def _solve_instance(data_object,
                    heuristic_constructor,
                    man,
                    base_kwargs,
                    metric_name,
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
    inst = man.get_rnd_instance()
    rnd_kwargs = man.dummy_to_kwargs(inst)

    meta_heuristic = heuristic_constructor(data_object["data"], **base_kwargs, **rnd_kwargs)
    meta_heuristic.solve()

    # 2) Export data (with metadata)
    if export:
        if export_metrics is None:
            export_metrics = [metric_name]

        export_data = {"data_id": data_object["id"],
                       "parameter": {**base_kwargs, **rnd_kwargs},
                       "metrics": {exp_m: getattr(meta_heuristic, exp_m) for exp_m in export_metrics},
                       "solution": meta_heuristic.solution}

        __export_data(export_data, export_path)

    # 3) Return results internally
    inst.append(getattr(meta_heuristic, metric_name))
    return inst


def get_modified_z_scores(y):
    """
    3 options:
     - MAD is 0 -> use MEAN
     - If MEAN is also 0 -> return 0s

    """
    median_y = np.median(y)
    median_absolute_deviation_y = round(np.median([np.abs(y - median_y) for y in y]), 5)

    if median_absolute_deviation_y != 0:
        return [0.6745 * (val - median_y) / median_absolute_deviation_y for val in y]
    else:
        standard_deviation = round(np.std(y), 5)
        if standard_deviation != 0:
            return [0.6745 * (val - median_y) / standard_deviation for val in y]
        else:
            return np.zeros(len(y))


class RegressionTreeParameterTuner:
    """
     Tuner is based on:

    "Bartz-Beielstein, Thomas, Konstantinos E. Parsopoulos, and Michael N. Vrahatis.
    "Analysis of particle swarm optimization using computational statistics."
    Proceedings of the International Conference of Numerical Analysis and Applied Mathematics (ICNAAM 2004). 2004."

    only suitable for minimization problems!
    Note: No non-integer parameter can be tuned with this algorithm version

    Uses modified z-score standardization to make multiple instances comparable.

    1) Calculate solutions of random instances based of the value ranges
    2) Perform regression tree tuning on a selected meta-heuristic
    3) Calculate new values within the best perform range

    How to use:
    - call this function with the parameters you want to tune.
    - Each parameter should be given specifically with the name
    - Value-Ranges for parameters should be provided in a list
    - The meta-heuristic must have a function "solve"

    Args:
        heuristic_constructor:  Object to be tuned. Must have a solve() function
                                After calling the solve() function it must contain a "solution" object.
                                This solution object must have an attribute "value"

        instances:              List of instances that should be randomly selected to solve

        base_kwargs: dict       Base arguments for each run of the heuristic
        value_domains: dict     Value domains of each tunable parameter.

                                List: Either multi choice categorical or numerical
                                Tuple: Single choice categorical domain

        n_init_pop:             Size of the initial data set to avoid cold start

        iter:                   Number of iterations without change in the best branch. Used as stoppage

        export:                 Boolean indication if each solution object should be pickled and saved
                                Must have a pickleable instance "solution" as attribute!

        export_path:            Indicates the directory that should be used to save the solutions

    """
    # Final result descriptions
    x = None
    y = None

    def __init__(self,
                 heuristic_constructor,
                 instances,
                 base_kwargs,
                 value_domains,
                 metric_name="value",
                 reg_tree_kwargs=None,
                 n_init_pop=100,
                 iter=5,
                 export=False,
                 export_path="",
                 export_metrics=None):
        self.heuristic_constructor = heuristic_constructor
        self.instances = [{"id": i, "data": inst} for i, inst in enumerate(instances)]
        self.instance_iteration_tracking = {x: [] for x in range(len(instances))}
        self.base_kwargs = base_kwargs
        self.metric_name = metric_name
        self.n_init_pop = n_init_pop
        self.iter = iter
        self.export = export
        self.export_path = export_path
        self.export_metrics = export_metrics

        # parameters for internal use
        self.__value_domains = value_domains
        self.__iteration_count = 0

        self.man = KwargsManager()
        self.man.fit(value_domains)

        if reg_tree_kwargs is None:
            reg_tree_kwargs = {'criterion': "mse", 'splitter': "best", 'max_depth': None,
                               'min_samples_split': 20, 'min_samples_leaf': 10}

            self.reg_tree = tree.DecisionTreeRegressor(**reg_tree_kwargs)

    def __repr__(self):
        return str(self.__dict__)

    def __solve_instances(self, number_instances, processes=1, maxtasksperchild=1, maxtasksperpool=10):
        """
        Solve the instances in a parallelled way.
        Restart the pool in certain intervalls to force resource reallocation!

        Args:
            processes:        Number of cores that should be used to solve tasks in parallel
            maxtasksperchild: Number of tasks a worker is allowed to perform before restarting
            maxtasksperpool:  Number of tasks a pool of workers is allowed to solve before restarting

        Returns:

        """
        results = []
        while len(results) < number_instances:
            # Get batch size
            pool_size = min(maxtasksperpool, number_instances - len(results))
            logging.info(f"restart pool, task number: {pool_size}")

            # Compute batch and solve heuristic instances
            with multiprocessing.Pool(processes=processes, maxtasksperchild=maxtasksperchild) as pool:
                iteration_kwargs = []

                for x in range(pool_size):
                    data_id = random.randint(0, len(self.instance_iteration_tracking) - 1)
                    iteration_kwargs.append((self.instances[data_id],
                                             self.heuristic_constructor,
                                             self.man,
                                             self.base_kwargs,
                                             self.metric_name,
                                             self.export,
                                             self.export_path,
                                             self.export_metrics))

                    self.instance_iteration_tracking[data_id].append(self.__iteration_count)
                    self.__iteration_count += 1

                future_values = pool.starmap_async(_solve_instance, iteration_kwargs)

                results.extend(future_values.get())

        return results

    def tune(self, processes=1, maxtasksperchild=1, maxtasksperpool=10):
        """
        Tune given value domains.

        Important:
        The object will contain a keywords manager that is tuned in the process
        as well. The next time tune is called it will work with the newly tuned set!

        parallel:               Indication if code should run in parallel.
                                -1:     select only one core
                                0:      select all available cores
                                > 0:    Define number of cores to be used

                                Especially relevant for heuristics with long execution time

        Returns:
            tuned_domains:      Dictionary with reduced rule range
        """

        def optimize_kwargs_manager(tree, kwargs_manager):
            """
            Get the rule-set of a tree of type sklearn.tree.tree.DecisionTreeRegressor
             leading to the leaf with the best value

            Args:
                tree:               Instances of scikit tree
                kwargs_manager:     Current keyword arguments manager whose
                                    domains should be adjusted according to the tree

            """
            # get node with biggest value
            tree_ = tree.tree_

            # first entry of the array is the lower bound, the second is the upper bound

            def recursive_child_rules_quality(node):
                """
                This function recursively iterates through the tree and adjusts the keyword arguments manager
                according to the best leaf found.

                IMPORTANT:
                The last rule is most likely the most strict rule.
                Therefore, if we set one rule once we are not allowed to set it again! (rule_change_indicator)

                Args:
                    node: Child leaf to be selected

                Returns: improved rule_set

                """
                nonlocal kwargs_manager, tree_
                feature_id = tree_.feature[node]
                threshold = tree_.threshold[node]

                if tree_.feature[node] != _tree.TREE_UNDEFINED:
                    values_1, kwargs_man1, rule_change_indicator1 = recursive_child_rules_quality(
                        tree_.children_left[node])
                    values_2, kwargs_man2, rule_change_indicator2 = recursive_child_rules_quality(
                        tree_.children_right[node])

                    # select the best leaf according
                    # set the rule set according to the best leaf
                    if values_1 < values_2:
                        # we must use deep copy to ensure that both the dictionary as the lists are copied instances
                        values, best_kwargs_man, rule_change_indicator = values_1, copy.deepcopy(
                            kwargs_man1), copy.deepcopy(
                            rule_change_indicator1)
                        # check if the rule was already changed if not change and save the change
                        # append new rule (children left means smaller equal -> new upper bound)
                        if not rule_change_indicator[feature_id][1]:
                            best_kwargs_man.attributes[feature_id].val_range[1] = threshold
                            rule_change_indicator[feature_id][1] = True
                    else:
                        values, best_kwargs_man, rule_change_indicator = values_2, copy.deepcopy(
                            kwargs_man2), copy.deepcopy(
                            rule_change_indicator2)
                        # check if the rule was already changed if not change and save the change
                        # append new rule (children left means bigger -> new lower bound)
                        if not rule_change_indicator[feature_id][0]:
                            best_kwargs_man.attributes[feature_id].val_range[0] = threshold
                            rule_change_indicator[feature_id][0] = True
                    return values, best_kwargs_man, rule_change_indicator
                else:  # end of leaf, base version
                    rule_change_indicator = {aid: [False, False] for aid in range(len(kwargs_manager.attributes))}
                    return tree_.value[node][0][0], kwargs_manager, rule_change_indicator

            # build each branch and select the rules of the branch with the better objective value
            # return the new rule set!
            return recursive_child_rules_quality(0)[0:2]

        def check_stoppage(prev_man, curr_man):
            # check if at least one significant change can be observed
            # Bigger or lower
            for aid in range(len(curr_man.attributes)):
                # np conversion allows 0 division (hacky)
                v1 = np.double(curr_man.attributes[aid].val_range[0])
                v2 = np.double(curr_man.attributes[aid].val_range[1])
                v3 = np.double(prev_man.attributes[aid].val_range[0])
                v4 = np.double(prev_man.attributes[aid].val_range[1])

                if (v1 / v3) <= 0.95:
                    return False

                if v3 / v1 <= 0.95:
                    return False

                if (v2 / v4) <= 0.95:
                    return False

                if (v4 / v2) <= 0.95:
                    return False
            return True

        # 0.2) Define new reporting structure
        colnames = [x.__dummyname__ for x in self.man.attributes] + ["value"]

        # 0.3) Init base tree learner

        # 1) HOT START EVALUATION
        # create new instances and solve them
        if processes > -1:
            num_cores = multiprocessing.cpu_count()

            # If the core restriction is less than available cores -> use less
            if (num_cores > processes) and (processes > 0):
                num_cores = processes
        else:
            # parallel is deactivated
            num_cores = 1

        # Get results (in parallel manner)
        # Use maxtaskperchild to free worker resources after each instance.
        # Otherwise it risks a memory error!
        results = self.__solve_instances(number_instances=self.n_init_pop,
                                         processes=num_cores,
                                         maxtasksperchild=maxtasksperchild,
                                         maxtasksperpool=maxtasksperpool)

        # Save and parse results
        df = pd.DataFrame(results, columns=colnames)

        # 3) EVALUATE TILL STOPPAGE!
        x = df.iloc[:, :-1].values
        y = np.zeros(len(df))

        # the modified z-scores must calculated on instance basis for no bias!
        for inst_id in self.instance_iteration_tracking:
            sol_ids = self.instance_iteration_tracking[inst_id]
            y[sol_ids] = get_modified_z_scores(df.iloc[sol_ids, -1].values)

        self.reg_tree.fit(x, y)

        # Reduce the possible value domain based on the tree learner!
        best_leaf_value, self.man = optimize_kwargs_manager(self.reg_tree, self.man)

        while True:
            results = self.__solve_instances(number_instances=self.iter,
                                             processes=processes,
                                             maxtasksperchild=maxtasksperchild,
                                             maxtasksperpool=maxtasksperpool)

            # save the results
            for res in results:
                df = df.append(dict(zip(colnames, res)), ignore_index=True)

            # get the new train data
            x = df.iloc[:, :-1].values
            y = np.zeros(len(df))

            for inst_id in self.instance_iteration_tracking:
                sol_ids = self.instance_iteration_tracking[inst_id]
                y[sol_ids] = get_modified_z_scores(df.iloc[sol_ids, -1].values)

            # repeat process if necessary
            self.reg_tree.fit(x, y)

            # Reset keywords manager to original domains and retrain it
            new_man = KwargsManager()
            new_man.fit(self.__value_domains)
            new_best_leaf_value, new_man = optimize_kwargs_manager(self.reg_tree, new_man)

            # Check if the structure of the tree changed
            if check_stoppage(new_man, self.man):
                break

            self.man = new_man

        self.x = x
        self.y = y
        return self.man.get_domains()

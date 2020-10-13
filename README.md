# COMPONENTS
We distribute the code into 3 mayor functionality components:  
  
1) ALNSLDTT implementation (folder: ALNSv2) (C++)
2) Parameter tuning and benchmarking (folder: CompAnalysis) (Python)
3) Instance preprocessing, pickling, and result analysis (folder: Notebooks)
   (Python - Jupyter notebooks)

Our main application is written in C++ for performance reasons and compiled 
into a .DLL library. The algorithm is exclusively used within Python scripts
for easier data import and visualization.

Jupyter notebooks are used in the preprocessing and result analysis because of
 their easily interactable interface and their documentation potential


# INSTALLATION

## Option 1) [recommended] 
- Windows operating system
- Visual Studio (2015 or higher) 
  + Desktop application package components 
  (C++ compiler, Microsoft distributables etc.)
- Install 32 bit - Python 3.6 or higher 
  (For example anaconda environment through visual studio)
- pybind11 library installed (pip or conda)

-> navigate to ALNSv2 folder and install heuristic with "pip install ."

## Version 2) [Not recommended]
- Install 32 bit - Python 3.6 or higher
- Copy the precompiled package into your Lib\site-packages folder of 
  your python distribution


# HOW TO USE 

After successfully installing the heuristic as a library into your python 
version you can simply import and use it

The CompAnalysis/computational_analysis.py script includes functionality for:
1) Benchmarking
2) Solving single instances
3) Parameter tuning

You need to initialize two objects to solve a problem

## ALNSData
The data object will be the standardized format in which you feed problem
instances to the algorithm. Two constructors are visible to the user: One for 
the VRPTW and one for the VRPLDTT  
  
Required input is:  
- nr_veh
- nr_nodes
- nr_customers
- demand
- service_times
- start_window
- end_window
- elevation_m (only for VRPLDTT)
- distance_m

Optinal arguments are:
- load_bucket_size (default: 0 -> Ignore load dependency (VRPTW)),
- nr_load_buckets (default: 0 -> Ignore load dependency (VRPTW)))
- vehicle_weight (default: 140)
- vehicle_capacity (default: 150)
  
E.g.:
```
kwargs = {#Insert data}
data = ALNSData(**kwargs)
```
  
##  ALNS 
This is the constructor for the algorithm and receives all algorithm 
parameters. Most of the arguments are optional  
- data_object : ALNSData
- destroy_operators : list of strings
- repair_operators : list of strings
- max_time (default=600)
- max_iterations (default: 10000)
- initial_temperature (default: 0.01)
- cooling_rate (default: 0.9999)
- wheel_memory_length (default: 10)
- wheel_parameter (default: 0.35)
- functor_reward_best (default: 50)
- functor_reward_accept_better (default: 100)
- functor_reward_unique (default: 90)
- functor_reward_divers (default: 7),
- functor_penalty (default: -80),
- functor_min_weight (default: 1),
- random_noise (default: 0.35),
- target_inf (default: 0.65),
- shakeup_log (default: 10),
- mean_removal_log (default: 3.35),
- track_accepted (default: False)
  
An object has a .solve method to generate a solution with all parameter settings
and the ALNSData object. This method returns a solution object that is
described by a solution representation and the most important KPIs.  
  
E.g.:  
```
>> kwargs = {#Insert data}
>> data = ALNSData(**kwargs)
>> alns = ALNS(data, ["random_destroy"], ["greedy_insertion"])
>> sol = alns.solve()
>> print(sol.value)
```

**ANNOTATION**  
All of ALNSData, ALNS and Solution objects are pickleable!
Most of the solutions of our solution data is a pickled solution object.

"""
Import pickled solution objects and visualize them
"""
import ALNSv2
import logging
import pickle
from os import listdir, path
from os.path import join

# analysis imports
import matplotlib as mpl

# 1) Rebuild the tree and object!
def get_solutions(path):
    files = []
    for f in listdir(path):
        try:
            raw_file = open(join(path, f), 'rb')
            data = pickle.load(raw_file)
            files.append(data)
        except TypeError:
            logging.WARN(f"File {f} is not a pickleable object. SKIP")
    return files

if __name__ == "__main__":
    path = "C:\\Users\\manuf\\OneDrive\\Dokumente\\Universitaet\\Masterthesis\\data\\1_tuning_results\\init_temp"

    solutions = get_solutions(path)

    [[row["data_id", ]] for row in solutions]
    None
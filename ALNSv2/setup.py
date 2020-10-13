"""
Define the pip setup routine the package!
Install on local environment by:

    Open command line with extended rights
    Navigate to directory
    Insert "pip install ."
"""
import os, sys

from distutils.core import setup, Extension
from distutils import sysconfig

cpp_args = ['-std=c++11', '-stdlib=libc++', '-mmacosx-version-min=10.7']

sfc_module = Extension(
    'ALNSv2', sources = ['module.cpp', 
                         'alns.cpp', 
                         'evaluate.cpp', 
                         'initialization.cpp',
                         'operator.cpp',
                         'preprocessing.cpp',
                         'rand_tools.cpp',
                         'vector_tools.cpp',
                         'roulette_wheel.cpp',
                         'solution.cpp'],
    include_dirs=['pybind11/include'],
    language='c++',
    extra_compile_args = cpp_args,
    )

setup(
    name = 'alnsvrpldtt',
    version = '1.0',    
    description = 'ALNS implementation for the VRPLDTT and VRPTW',
    ext_modules = [sfc_module],
)
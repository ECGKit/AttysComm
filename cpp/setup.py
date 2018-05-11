#!/usr/bin/env python3

"""
setup.py file for AttysComm
"""

from setuptools import setup
from setuptools import Extension
import os
from sys import platform

def read(fname):
    return open(os.path.join(os.path.dirname(__file__), fname)).read()

if platform == "linux" or platform == "linux2":
	attyscomm_module = Extension('_pyattyscomm',
							sources=['pyattyscommPYTHON_wrap.cxx'],
							extra_compile_args=['-std=c++11'],
							libraries=['attyscomm','bluetooth'],
							)
elif platform == "win32":
	attyscomm_module = Extension('_pyattyscomm',
							sources=['pyattyscommPYTHON_wrap.cxx'],
							extra_compile_args=['/DWIN32_LEAN_AND_MEAN'],
                            libraries=['ws2_32'],
							extra_link_args=['Release\\attyscomm_static.lib'],
							)

						   
setup (name = 'pyattyscomm',
       version = '1.2.1b22',
       author      = "Bernd Porr",
       author_email = "bernd@glasgowneuro.tech",
       url = "https://github.com/glasgowneuro/AttysComm",
       description = 'API for the Attys DAQ box (www.attys.tech)',
       long_description=read('README_py'),
       ext_modules = [attyscomm_module],
       py_modules = ["pyattyscomm"],
       license='Apache 2.0',
       classifiers=[
          'Intended Audience :: Developers',
          'Operating System :: POSIX',
		  'Operating System :: Microsoft :: Windows',
          'Programming Language :: Python'
          ]
      )

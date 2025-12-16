# -*- coding: utf-8 -*-
"""
Created on Tue Mar  2 15:39:04 2021

@author: casmi
"""

# import modules
import IPython as IP
IP.get_ipython().magic('reset -sf')



import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
import scipy as sp
import json as json
import scipy.linalg as solve
from matplotlib import colors


f = open('USGS_v3_fieldtest_jan15.csv','r')

x = f.read()
usgs = x[2]


USGS = load('USGS_v3_fieldtest_jan15.csv', skiprows=1)





ARTS = read_csv('ARTS_v3_fieldtest_jan15.csv', header=0, parse_dates=[0], index_col=0, squeeze=True)
print(type(ARTS))
print(ARTS.head())

pyplot.plot(USGS, '-v', color='blue', markersize=8, linewidth=2)
pyplot.plot(ARTS, '--o', color='orange', markersize=8, linewidth=2)
pyplot.show()


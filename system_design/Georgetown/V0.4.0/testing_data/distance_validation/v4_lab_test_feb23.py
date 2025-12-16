# -*- coding: utf-8 -*-
"""
Created on Sat Mar 20 14:08:41 2021

@author: casmi
"""

# import modules
import IPython as IP
IP.get_ipython().magic('reset -sf')


from pandas import read_csv
#from matplotlib import pyplot

import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.dates as md
import datetime as dt
import numpy as np
import scipy as sp
import json as json
import scipy.linalg as solve
from matplotlib import colors


plt.close('all')


# set default fonts and plot colors
plt.rcParams.update({'image.cmap': 'viridis'})
cc = plt.rcParams['axes.prop_cycle'].by_key()['color']
plt.rcParams.update({'font.serif':['Times New Roman', 'Times', 'DejaVu Serif',
 'Bitstream Vera Serif', 'Computer Modern Roman', 'New Century Schoolbook',
 'Century Schoolbook L',  'Utopia', 'ITC Bookman', 'Bookman', 
 'Nimbus Roman No9 L', 'Palatino', 'Charter', 'serif']})
plt.rcParams.update({'font.family':'serif'})
plt.rcParams.update({'font.size': 14})
plt.rcParams.update({'mathtext.rm': 'serif'})
plt.rcParams.update({'mathtext.fontset': 'custom'})
plt.close('all')


#%%



true_values = read_csv('v4_lab_test_feb23_true_values.csv', header=0, parse_dates=[0], index_col=0, squeeze=True)

# true_values_tt = np.linspace(0,12,24) 
# true_values_yy = true_values.values




sensor_values = read_csv('v4_lab_test_feb23_sensor_values.csv', header=0, parse_dates=[0], index_col=0, squeeze=True)
# sensor_values_tt = np.linspace(0,12,24) 
# sensor_values_yy = sensor_values.values


plt.figure(figsize=(6.5,3))
plt.plot(true_values["True distance (m)"], 'v', label='true distance')
plt.plot(sensor_values["Avg sensor reading (m)"], 'o', label='sensor measurement')
plt.legend(framealpha=1)
plt.xlabel('position number')
plt.ylabel('distance (m)')
plt.tight_layout()
plt.grid(True)
plt.savefig('distance.png',dpi=500)

#%%

error = read_csv('v4_lab_test_feb23_error.csv', header=0, parse_dates=[0], index_col=0, squeeze=True)
plt.figure(figsize=(6.5,3))
plt.stem(error["Error (mm)"], markerfmt='o', use_line_collection=True)
#plt.legend(framealpha=1)
plt.xlabel('position number')
plt.ylabel('average error (mm)')
plt.tight_layout()
plt.grid(True)
plt.savefig('error.png',dpi=500)
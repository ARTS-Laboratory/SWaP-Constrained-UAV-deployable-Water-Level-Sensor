# -*- coding: utf-8 -*-
"""
Created on Tue Aug 10 11:52:52 2021

@author: casmi
"""
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from datetime import datetime
import pandas as pd
import pylab as pl
import numpy as np
plt.rcParams["font.family"] = "Times New Roman"
mpl.rcParams.update({'font.size': 14})

import IPython as IP
IP.get_ipython().magic('reset -sf')
plt.close('all')

#%% Using datetimes

df0 = pd.read_csv('v6_longevity.csv',parse_dates=     {"Datetime" : [0,1]})


plt.figure(figsize=(6.5,3.5))
plt.plot(df0['Datetime'], df0['Modified Voltage'])
#plt.legend(framealpha=1)
plt.xlabel('datetime')
plt.ylabel('voltage')
plt.locator_params(axis="x", nbins=5)
plt.xticks(rotation=45)
plt.tight_layout()
plt.grid(True)
plt.savefig('v6_longevity.png', dpi=500)

#%% Using days

df0 = pd.read_csv('v6_longevity.csv')


plt.figure(figsize=(6.5,3.5))
plt.plot(df0['Days'], df0['Modified Voltage'])
#plt.legend(framealpha=1)
plt.xlabel('days')
plt.ylabel('voltage')
#plt.locator_params(axis="x", nbins=10)
#plt.xticks(rotation=45)
plt.tight_layout()
plt.grid(True)
plt.savefig('v6_longevity_days.png', dpi=500)
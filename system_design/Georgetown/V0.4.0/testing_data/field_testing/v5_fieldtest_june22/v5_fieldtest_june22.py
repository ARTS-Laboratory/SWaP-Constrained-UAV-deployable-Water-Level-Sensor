# -*- coding: utf-8 -*-
"""
Created on Wed Jun 23 13:01:46 2021

@author: casmi
"""


import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from datetime import datetime
import pandas as pd
import pylab as pl

import IPython as IP
IP.get_ipython().magic('reset -sf')
plt.close('all')


df0 = pd.read_csv('USGS_june22.csv',parse_dates=     {"Datetime" : [0,1]})
df1 = pd.read_csv('v5_sumterbridge_june22.csv',parse_dates=     {"Datetime" : [0,1]})
df2 = pd.read_csv('v5_maxcybridge_june22.csv',parse_dates=     {"Datetime" : [0,1]})
df3 = pd.read_csv('USGS_june22_maxcy.csv',parse_dates=     {"Datetime" : [0,1]})

plt.figure(figsize=(6.5,3.5))
plt.plot(df0['Datetime'], df0['Stage (cm)'], '-x', label='Sumter USGS' )
plt.plot(df1['Datetime'], df1['Avg Dist (cm)'], '-', label='Sumter bridge')
plt.plot(df2['Datetime'], df2['Avg Dist (cm)'], '--', label='Maxcy bridge')
plt.plot(df3['Datetime'], df3['Stage (cm)'], '-o', label='Maxcy USGS' )
plt.legend(framealpha=1)
plt.xlabel('datetime')
plt.ylabel('distance (cm)')
plt.locator_params(axis="x", nbins=5)
plt.xticks(rotation=45)
plt.tight_layout()
plt.grid(True)
plt.savefig('v5_fieldtest_june22.png', dpi=500)


plt.figure(figsize=(6.5,3.5))
plt.plot(df1['Datetime'], df1['Avg Dist (cm)'], '-', label='Sumter bridge', color='orange')
plt.plot(df2['Datetime'], df2['Avg Dist (cm)'], '--', label='Maxcy bridge', color='green')
plt.legend(framealpha=1)
plt.xlabel('datetime')
plt.ylabel('distance (cm)')
plt.locator_params(axis="x", nbins=5)
plt.xticks(rotation=45)
plt.tight_layout()
plt.grid(True)
plt.savefig('bridge_comparison.png', dpi=500)

plt.figure(figsize=(6.5,3.5))
plt.plot(df0['Datetime'], df0['Stage (cm)'], '-x', label='USGS')
plt.plot(df1['Datetime'], df1['Avg Dist (cm)'], '-', label='Sumter bridge')
plt.legend(framealpha=1)
plt.xlabel('datetime')
plt.ylabel('distance (cm)')
plt.locator_params(axis="x", nbins=5)
plt.xticks(rotation=45)
plt.tight_layout()
plt.grid(True)
plt.savefig('sumter_comparison.png', dpi=500)

plt.figure(figsize=(6.5,3.5))
plt.plot(df3['Datetime'], df3['Stage (cm)'], '-o', label='USGS', color='red' )
plt.plot(df2['Datetime'], df2['Avg Dist (cm)'], '--', label='Maxcy bridge', color='green')
plt.legend(framealpha=1)
plt.xlabel('datetime')
plt.ylabel('distance (cm)')
plt.locator_params(axis="x", nbins=5)
plt.xticks(rotation=45)
plt.tight_layout()
plt.grid(True)
plt.savefig('maxcy_comparison.png', dpi=500)
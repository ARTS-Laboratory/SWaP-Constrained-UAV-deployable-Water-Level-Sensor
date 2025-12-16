# -*- coding: utf-8 -*-
"""
Created on Mon May 17 08:26:23 2021

@author: casmi
"""

import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from datetime import datetime
import pandas as pd
import pylab as pl

import IPython as IP
IP.get_ipython().magic('reset -sf')


df0 = pd.read_csv('USGS_may12_v2.csv',parse_dates=     {"Datetime" : [0,1]})
print(df0)
df = pd.read_csv('teensy_may12.csv',parse_dates=     {"Datetime" : [0,1]})
print(df)
df2 = pd.read_csv('nano_may12.csv',parse_dates=     {"Datetime" : [0,1]})
print(df2)
df.Datetime=pd.to_datetime(df.Datetime)
df.set_index('Datetime')

plt.figure(figsize=(6.5,4))
plt.plot(df0['Datetime'], df0['USGS'], '-', label='USGS' )
plt.plot(df['Datetime'], df['Teensy'], '-v', label='Teensy')
plt.plot(df2['Datetime'], df2['Nano'], '--o', label='Nano')
plt.legend(framealpha=1)
plt.xlabel('time')
plt.ylabel('distance (cm)')
plt.locator_params(axis="x", nbins=5)
#plt.locator_params(axis="y", nbins=2)
plt.tight_layout()
plt.xticks(rotation=45)
plt.grid(True)
plt.savefig('v5_fieldtest_may12.png', dpi=500)

#%% Voltage curves
plt.figure(figsize=(6.5,3))
plt.plot(df['Datetime'], df['Voltage'], '-', label='Teensy')
plt.plot(df2['Datetime'], df2['Voltage'], '-', label='Nano')
plt.legend(framealpha=1)
plt.xlabel('time')
plt.ylabel('volts (V)')
plt.tight_layout()
plt.grid(True)
plt.savefig('voltage_curve_may12.png', dpi=500)

#%% Temperature curves

# df3 = pd.read_csv('nano_temp_voltage.csv', parse_dates={"Datetime" : [0,1]})
# print(df3)

# plt.figure(figsize=(6.5,3))
# plt.plot(df3['Datetime'], df3['Voltage'], '-o', label='voltage')
# plt.plot(df3['Datetime'], df3['Temp'], '-v', label='temperature')
# plt.legend(framealpha=1)
# plt.xlabel('time')
# plt.ylabel('volts (V)')
# plt.tight_layout()
# plt.grid(True)
# plt.savefig('voltage_curve_may12.png', dpi=500)
#!/usr/bin/env python

#@author Jonathan

import ctypes
import struct
import datetime as dt
import numpy as np


COMM_TYPE_BYTE = 0
COMM_TYPE_WORD = 1

def read_waveform(f):

    ret = {}

    # <0> first 16 bytes are file identifier
    _wavedesc = f.read(16)
    wavedesc = ctypes.create_string_buffer(_wavedesc).value.decode()
    if wavedesc != "WAVEDESC":
        print("ERROR: WAVEDESC header not found")

    # <16> next 16 bytes are template name
    _template = f.read(16)
    template = ctypes.create_string_buffer(_template).value.decode()
    if template != "LECROY_1_0":
        print("ERROR: this template ("+template+") cannot be parsed with this code")

    # <32> next 2 bytes (starting at 32) identify size of data
    _comm_type = f.read(2)

    # <34> next 2 bytes (starting at 34) identify endianess of data
    # 0 means big-endian, 1 means little-endian (???)
    _comm_order = f.read(2)
    comm_order = struct.unpack('<H', _comm_order)[0]
    endianess = '@'
    if (comm_order == 0):
        endianess = '>'
    else:
        endianess = '<'

    # see _comm_type
    # 0 means byte, 1 means word (==16 bit)
    comm_type = struct.unpack(endianess+'H', _comm_type)[0]

    # block lengths
    # <36> wave descriptor
    _wave_descriptor_blocklen = f.read(4)
    wave_descriptor_blocklen = struct.unpack(endianess+'l', _wave_descriptor_blocklen)[0]

    # <40> user text descriptor
    _user_text_blocklen = f.read(4)
    user_text_blocklen = struct.unpack(endianess+'l', _user_text_blocklen)[0]

    # <44> trigtime array
    _trigtime_array_blocklen = f.read(4)
    trigtime_array_blocklen = struct.unpack(endianess+'l', _trigtime_array_blocklen)[0]

    # <48> wavearray 1 length
    _wavearray_1_blocklen = f.read(4)
    wavearray_1_blocklen = struct.unpack(endianess+'l', _wavearray_1_blocklen)[0]

    # <52> wavearray 2 length
    _wavearray_2_blocklen = f.read(4)
    wavearray_2_blocklen = struct.unpack(endianess+'l', _wavearray_2_blocklen)[0]

    # <56> instrument name: string of length <= 16 bytes
    _instrument_name = f.read(16)
    instrument_name = ctypes.create_string_buffer(_instrument_name).value.decode()

    # <72> instrument number
    _instrument_number = f.read(4)
    instrument_number = struct.unpack(endianess+'l', _instrument_number)[0]

    # <76> trace label
    _trace_label = f.read(16)
    trace_label = ctypes.create_string_buffer(_trace_label).value.decode()
    ret['trace_label'] = trace_label

    # <92> wave array count: number of actual datapoints in wave arrays
    _wave_array_count = f.read(4)
    wave_array_count = struct.unpack(endianess+'l', _wave_array_count)[0]

    # <96> point per screen
    _points_per_screen = f.read(4)
    points_per_screen = struct.unpack(endianess+'l', _points_per_screen)[0]

    # <100> first valid point
    _first_valid_point = f.read(4)
    first_valid_point = struct.unpack(endianess+'l', _first_valid_point)[0] + 1

    # <104> last valid point
    _last_valid_point = f.read(4)
    last_valid_point = struct.unpack(endianess+'l', _last_valid_point)[0] - 1

    # <108> subarray count
    _subarray_count = f.read(4)
    subarray_count = struct.unpack(endianess+'l', _subarray_count)[0]

    # <112> nominal subarray count
    _nominal_subarray_count = f.read(4)
    nominal_subarray_count = struct.unpack(endianess+'l', _nominal_subarray_count)[0]

    # <116> sweeps per acquisition
    _sweeps_per_acquisition = f.read(4)
    sweeps_per_acquisition = struct.unpack(endianess+'l', _sweeps_per_acquisition)[0]

    # <120> vertical gain
    _vertical_gain = f.read(4)
    vertical_gain = struct.unpack(endianess+'f', _vertical_gain)[0]

    # <124> vertical offset
    _vertical_offset = f.read(4)
    vertical_offset = struct.unpack(endianess+'f', _vertical_offset)[0]

    # <128> max allowed data value
    _max_allowed_value = f.read(2)
    max_allowed_value = struct.unpack(endianess+'h', _max_allowed_value)[0]

    # <130> min allowed data value
    _min_allowed_value = f.read(2)
    min_allowed_value = struct.unpack(endianess+'h', _min_allowed_value)[0]

    # <132> nominal bits
    _nominal_bits = f.read(2)
    nominal_bits = struct.unpack(endianess+'h', _nominal_bits)[0]

    # <134> horizontal sampling interval
    _horizontal_interval = f.read(4)
    horizontal_interval = struct.unpack(endianess+'f', _horizontal_interval)[0]

    # <138> horizontal offset
    _horizontal_offset = f.read(8)
    horizontal_offset = struct.unpack(endianess+'d', _horizontal_offset)[0]

    # <146> pixel offset
    _pixel_offset = f.read(8)
    pixel_offset = struct.unpack(endianess+'d', _pixel_offset)[0]

    # <154> vertical axis unit
    _vertical_unit = f.read(48)
    vertical_unit = ctypes.create_string_buffer(_vertical_unit).value.decode()

    # <202> horizontal axis unit
    _horizontal_unit = f.read(48)
    horizontal_unit = ctypes.create_string_buffer(_horizontal_unit).value.decode()

    # <250> trigger time
    _trigger_time = f.read(16)
    trigger_time_sec, trigger_time_min, trigger_time_h, trigger_time_d, trigger_time_m, \
    trigger_time_y = struct.unpack(endianess+'dbbbbhxx', _trigger_time)
    secs = np.int(trigger_time_sec)
    microsecs = (trigger_time_sec-secs)*1000.0
    trigger_time = dt.datetime(trigger_time_y, trigger_time_m, trigger_time_d, \
                                     trigger_time_h, trigger_time_min, secs, np.int(np.round(microsecs)))

    # <266> acquisition duration
    _acq_duration = f.read(4)
    acq_duration = struct.unpack(endianess+'f', _acq_duration)[0]

    # <270> record type
    _record_type = f.read(2)
    record_type = struct.unpack(endianess+'h', _record_type)[0]

    # <272> processing done
    _processing_done = f.read(2)
    processing_done = struct.unpack(endianess+'h', _processing_done)[0]

    # <274> timebase
    _timebase = f.read(2)
    timebase = struct.unpack(endianess+'h', _timebase)[0]

    # <276> vertical coupling
    _vertical_coupling = f.read(2)
    vertical_coupling = struct.unpack(endianess+'h', _vertical_coupling)[0]

    # <278> probe attenuation
    _probe_attenuation = f.read(4)
    probe_attenuation = struct.unpack(endianess+'f', _probe_attenuation)[0]

    # <282> fixed vertical gain
    _fixed_vertical_gain = f.read(2)
    fixed_vertical_gain = struct.unpack(endianess+'h', _fixed_vertical_gain)[0]

    # <284> bandwidth limit
    _bandwidth_limit = f.read(2)
    bandwidth_limit = struct.unpack(endianess+'h', _bandwidth_limit)[0]

    # <286> vertical vernier
    _vertical_vernier = f.read(4)
    vertical_vernier = struct.unpack(endianess+'f', _vertical_vernier)[0]

    # <290> acquisition vertical offset
    _acq_vertical_offset = f.read(4)
    acq_vertical_offset = struct.unpack(endianess+'f', _acq_vertical_offset)[0]

    # <294> wave source plugin
    _wave_source_plugin = f.read(2)
    wave_source_plugin = struct.unpack(endianess+'h', _wave_source_plugin)[0]

    # <296> wave source channel
    _wave_source_channel = f.read(2)
    wave_source_channel = struct.unpack(endianess+'h', _wave_source_channel)[0]

    # <298> trigger source
    _trigger_source = f.read(2)
    trigger_source = struct.unpack(endianess+'h', _trigger_source)[0]

    # <300> trigger coupling
    _trigger_coupling = f.read(2)
    trigger_coupling = struct.unpack(endianess+'h', _trigger_coupling)[0]

    # <302> trigger slope
    _trigger_slope = f.read(2)
    trigger_slope = struct.unpack(endianess+'h', _trigger_slope)[0]

    # <304> smart trigger
    _smart_trigger = f.read(2)
    smart_trigger = struct.unpack(endianess+'h', _smart_trigger)[0]

    # <306> trigger level
    _trigger_level = f.read(4)
    trigger_level = struct.unpack(endianess+'f', _trigger_level)[0]
    ret['trigger_level']=trigger_level

    # <310> sweeps array 1
    _sweeps_array_1 = f.read(4)
    sweeps_array_1 = struct.unpack(endianess+'l', _sweeps_array_1)[0]

    # <314> sweeps array 2
    _sweeps_array_2 = f.read(4)
    sweeps_array_2 = struct.unpack(endianess+'l', _sweeps_array_2)[0]

    usertext = ''
    if user_text_blocklen>0:

        _usertext = f.read(user_text_blocklen)
        usertext = ctypes.create_string_buffer(_usertext).value.decode()

    # Explanation of the trigger time array (present for sequence waveforms only)
    if trigtime_array_blocklen>0:
        print('not implemented yet')

    wavearray1 = None
    if wavearray_1_blocklen>0:

        _wavearray1 = f.read(wavearray_1_blocklen)

        numPoints = last_valid_point - first_valid_point +1

        if comm_type == COMM_TYPE_BYTE:
            wavearray1 = struct.unpack(endianess+np.str(wave_array_count)+'b', _wavearray1)
        elif comm_type == COMM_TYPE_WORD:
            wavearray1 = struct.unpack(endianess+np.str(wave_array_count)+'h', _wavearray1)
        else:
            print('comm type ' + np.str(comm_type) + ' not understood!')

        wavearray1_valid = wavearray1[first_valid_point:last_valid_point+1]

        wv1_scaled = np.subtract(np.multiply(wavearray1_valid, vertical_gain), acq_vertical_offset)
        wv1_dim_scaled = np.add(np.multiply(range(numPoints), horizontal_interval), horizontal_offset)

        ret['wv1_scaled'] = wv1_scaled
        ret['wv1_dim_scaled'] = wv1_dim_scaled

    if wavearray_2_blocklen>0:
        print('wavearray 2 reading not implemented yet!')

    return ret


filename = 'trace1.000'
wv1=None
with open(filename, 'rb') as f:
    wv1=read_waveform(f)

filename = 'trace2.000'
wv2=None
with open(filename, 'rb') as f:
    wv2=read_waveform(f)

filename = 'trace3.000'
wv3=None
with open(filename, 'rb') as f:
    wv3=read_waveform(f)

filename = 'trace4.000'
wv4=None
with open(filename, 'rb') as f:
    wv4=read_waveform(f)


#%%
import matplotlib.pyplot as plt

f, axarr=plt.subplots(2, sharex=True)

axarr[0].plot(wv1['wv1_dim_scaled']*1e6, wv1['wv1_scaled'], 'r-', label="+RX")
axarr[0].plot(wv2['wv1_dim_scaled']*1e6, wv2['wv1_scaled'], 'b-', label="- RX")
axarr[0].grid(True)
axarr[0].legend(loc='best')
axarr[0].set_ylabel("U / V")

axarr[1].plot(wv3['wv1_dim_scaled']*1e6, wv3['wv1_scaled'], 'r-', label="input")
axarr[1].plot(wv4['wv1_dim_scaled']*1e6, wv4['wv1_scaled'], 'b-', label="output")
axarr[1].set_xlabel("t / $\mu$s")
axarr[1].grid(True)
axarr[1].set_ylabel("U / V")
axarr[1].legend(loc='best')
axarr[1].ticklabel_format(axis='x', scilimits=(-2,2))

plt.tight_layout()

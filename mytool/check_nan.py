from plyfile import PlyData
import numpy as np
import os
from pathlib import Path
import pandas as pd

def read_binary_ply(file_path):
    with open(file_path, 'rb') as f:
        ply_data = PlyData.read(f)
    return ply_data

def read_ascii_ply(file_path):
    ply_data = PlyData.read(file_path)
    return ply_data

in_ply_data = read_ascii_ply('../guassianData/penny/point_cloud/iteration_30000/point_cloud.ply')
out_ply_data = read_binary_ply(f'../guassianData/penny/draco/out.ply')
    
# Access elements and their properties
for element in in_ply_data.elements:
    print(f"Element name: {element.name}")
    for prop in element.properties:
        print(f"Property name: {prop.name}, data type: {prop.dtype}")
        # Access property data using plydata[element.name][prop.name]
        property_data = in_ply_data[element.name][prop.name]
        nan_count = np.count_nonzero(np.isnan(property_data))
        inf_count = np.count_nonzero(np.isinf(property_data))
        # print(f"{nan_count}, {inf_count}")
        property_data = out_ply_data[element.name][prop.name]
        o_nan_count = np.count_nonzero(np.isnan(property_data))
        o_inf_count = np.count_nonzero(np.isinf(property_data))
        print(f"{nan_count-o_nan_count}, {inf_count-o_inf_count}")
        
# # Access elements and their properties
# for element in out_ply_data.elements:
#     print(f"Element name: {element.name}")
#     for prop in element.properties:
#         print(f"Property name: {prop.name}, data type: {prop.dtype}")
#         # Access property data using plydata[element.name][prop.name]
#         property_data = out_ply_data[element.name][prop.name]
#         nan_count = np.count_nonzero(np.isnan(property_data))
#         inf_count = np.count_nonzero(np.isinf(property_data))
#         print(f"{nan_count}, {inf_count}")
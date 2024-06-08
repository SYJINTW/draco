import os
import subprocess
import itertools
import pandas as pd
from pathlib import Path
import create_render_dir_random
import numpy as np

def extract_time_from_decode_log_file(file_path):
    try:
        with open(file_path, 'r') as file:
            log_text = file.read()
            lines = log_text.split('\n')
            for line in lines:
                if "[YC] time:" in line:
                    time_str = line.split("[YC] time: ")[1]
                    time_ms = int(time_str)
                    return time_ms
    except FileNotFoundError:
        print("File not found or unable to read the file.")
        return None

def extract_time_and_size_from_encode_log(file_path):
    try:
        with open(file_path, 'r') as file:
            log_text = file.read()
            time_taken = None
            size_encoded = None
            lines = log_text.split('\n')
            for line in lines:
                if "[YC] time:" in line:
                    time_str = line.split("[YC] time: ")[1]
                    time_taken = int(time_str)
                elif "[YC] size:" in line:
                    size_str = line.split("[YC] size: ")[1].split(" bytes")[0]
                    size_encoded = int(size_str)
            
            return time_taken, size_encoded
    except FileNotFoundError:
        print("File not found or unable to read the file.")
        return None, None

def main(numToCal, scene_names, settings):
    
    for scene_name in scene_names:
        input_dir = Path("..")/"expData"/"draco_input"/scene_name
        save_drc_dir = Path("..")/"expData"/"draco_output_drc"/scene_name
        save_drc_dir.mkdir(parents=True, exist_ok=True)
        save_ply_dir = Path("..")/"expData"/"draco_output_ply"/scene_name
        save_ply_dir.mkdir(parents=True, exist_ok=True)
        save_log_dir = Path("..")/"expData"/"draco_log"/scene_name
        save_log_dir.mkdir(parents=True, exist_ok=True)
        save_csv_dir = Path("..")/"expData"/"draco_csv"/scene_name
        save_csv_dir.mkdir(parents=True, exist_ok=True)
    
        for setting in settings:
            datas = []
            for i in range(numToCal):
                int_setting = setting.astype(int)
                qp_value, qn_value, qfd_value, qfr_value, qo_value, qs_value, qr_value, cl_value, qt_value, qg_value = int_setting
                print(qp_value, qn_value, qfd_value, qfr_value, qo_value, qs_value, qr_value, cl_value, qt_value, qg_value)
                suffix = f"qp{qp_value}_qn{qn_value}_qfd{qfd_value}_qfr{qfr_value}_qo{qo_value}_qs{qs_value}_qr{qr_value}_cl{cl_value}"

                if ~os.path.isfile(save_drc_dir/f"{scene_name}_{suffix}.drc"):
                    os.system(f"../build_dir/draco_encoder -point_cloud \
                                -i {input_dir}/point_cloud.ply \
                                -o {save_drc_dir}/{scene_name}_{suffix}.drc \
                                -qp {qp_value} -qn {qn_value} \
                                -qfd {qfd_value} -qfr {qfr_value} -qo {qo_value} \
                                -qs {qs_value} -qr {qr_value} \
                                -cl {cl_value} \
                                > {save_log_dir}/encode_{suffix}_{i}.log")
                
                if ~os.path.isfile(save_ply_dir/f"{scene_name}_{suffix}.ply"):                  
                    os.system(f"../build_dir/draco_decoder \
                                -i {str(save_drc_dir)}/{scene_name}_{suffix}.drc \
                                -o {str(save_ply_dir)}/{scene_name}_{suffix}.ply \
                                > {save_log_dir}/decode_{suffix}_{i}.log")

                encode_time, encode_size = extract_time_and_size_from_encode_log(save_log_dir/f"encode_{suffix}_{i}.log")
                decode_time = extract_time_from_decode_log_file(save_log_dir/f"decode_{suffix}_{i}.log")
                datas.append([i, qp_value, qn_value, qfd_value, qfr_value, qo_value, qs_value, qr_value, cl_value, encode_time, encode_size, decode_time, suffix])
                
            df = pd.DataFrame(datas, columns=["i", "qp_value", "qn_value", "qfd_value", "qfr_value", "qo_value", "qs_value", "qr_value", "cl_value", "encode_time", "encode_size", "decode_time", "suffix"])
            df.to_csv(save_csv_dir/f"log_{suffix}.csv", index=False)
            print(f"{scene_name} {suffix} done")
                
if __name__ == "__main__":
    
    numToCal = 1
    scene_names = ["lego"] # ["drjohnson", "playroom", "train", "truck"]
    file_path = '../../random_data/lego_19.npy' # random_arrays_44_20
    
    settings = np.load(file_path)
    main(numToCal, scene_names, settings) 
    create_render_dir_random.main(numToCal, scene_names, settings)  
    print(f"========= Done Main =========")
    print(f"scene_names: {scene_names}")
    print(f"file_path: {file_path}")  
    

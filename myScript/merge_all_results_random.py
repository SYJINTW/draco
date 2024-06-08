import os
import pandas as pd
import itertools
from pathlib import Path
import numpy as np
from plyfile import PlyData, PlyElement

def getNumOf3DGS(file_path):
    plydata = PlyData.read(str(file_path))
    return (np.asarray(plydata.elements[0]["x"])).size

def main(numToCal, scene_names, settings,
        gzip=False, bzip2=False):
    
    merged_df = pd.DataFrame()
    
    # draco
    for scene_name in scene_names:
        numOfPoint = getNumOf3DGS(Path(f'..')/"expData"/"raw_ply"/scene_name/"point_cloud.ply")
        original_file_size=(Path(f'..')/"expData"/"raw_ply"/scene_name/"point_cloud.ply").stat().st_size
        for setting in settings:
            int_setting = setting.astype(int)
            qp_value, qn_value, qfd_value, qfr_value, qo_value, qs_value, qr_value, cl_value, qt_value, qg_value = int_setting
            print(qp_value, qn_value, qfd_value, qfr_value, qo_value, qs_value, qr_value, cl_value, qt_value, qg_value)
            suffix = f"qp{qp_value}_qn{qn_value}_qfd{qfd_value}_qfr{qfr_value}_qo{qo_value}_qs{qs_value}_qr{qr_value}_cl{cl_value}"
            csv_file_path = Path("..")/"expData"/"draco_csv"/scene_name/f"log_{suffix}.csv"
            if os.path.exists(csv_file_path):
                df = pd.read_csv(csv_file_path)
                df = df.iloc[:numToCal]
                df['alg'] = 'draco'
                df['scene_name'] = scene_name
                df['original_size'] = original_file_size
                df['numOf3DGS'] = numOfPoint
                if merged_df.empty:
                    merged_df = df
                else:
                    merged_df = pd.concat([merged_df, df], axis=0)
            else:
                continue
    
    if not merged_df.empty:          
        save_dir = Path("..")/"expData"/"results"
        save_dir.mkdir(parents=True, exist_ok=True)
        merged_df.to_csv(save_dir/"lego_19.csv", index=False)

if __name__ == "__main__":
    
    numToCal = 1
    scene_names = ["lego"] # ["drjohnson", "playroom", "train", "truck"]
    file_path = '../../random_data/lego_19.npy' # random_arrays_0_20 random_arrays_4_20 random_arrays_44_20 spark_arrays
    
    settings = np.load(file_path)
    main(numToCal, scene_names, settings) 
    print(f"========= Done Merge =========")
    print(f"scene_names: {scene_names}")
    print(f"file_path: {file_path}")  

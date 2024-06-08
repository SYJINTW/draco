import os
import pandas as pd
import itertools
from pathlib import Path
import numpy as np
from plyfile import PlyData, PlyElement

def getNumOf3DGS(file_path):
    plydata = PlyData.read(str(file_path))
    return (np.asarray(plydata.elements[0]["x"])).size
    

def main(numToCal, scene_names, 
        qp_values, 
        qn_values, 
        qfd_values, qfr_values, qo_values, 
        qs_values, qr_values, 
        cl_values, 
        qt_values, qg_values,
        gzip=False, bzip2=False):
    
    GS = qfd_values
    SH = qs_values
    
    merged_df = pd.DataFrame()
    
    # draco
    for scene_name in scene_names:
        numOfPoint = getNumOf3DGS(Path(f'..')/"expData"/"raw_ply"/scene_name/"point_cloud.ply")
        original_file_size=(Path(f'..')/"expData"/"raw_ply"/scene_name/"point_cloud.ply").stat().st_size
        for qp_value in qp_values:
            for qn_value in qn_values:
                for GS_value in GS:
                    for SH_value in SH:
                        for cl_value in cl_values:                            
                            qfr_value = qo_value = qfd_value = GS_value
                            qr_value = qs_value = SH_value
                            suffix = f"qp{qp_value}_qn{qn_value}_qfd{qfd_value}_qfr{qfd_value}_qo{qfd_value}_qs{qs_value}_qr{qs_value}_cl{cl_value}"
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
    # gzip
    if gzip:
        gzip_compression_levels = [1, 3, 5, 7, 9]
        for scene_name in scene_names:
            numOfPoint = getNumOf3DGS(Path(f'..')/"expData"/"raw_ply"/scene_name/"point_cloud.ply")
            original_file_size=(Path(f'..')/"expData"/"raw_ply"/scene_name/"point_cloud.ply").stat().st_size
            qp_value = qn_value = qfd_value = qfr_value = qo_value = qs_value = qr_value = "x"
            for cl_value in gzip_compression_levels:
                suffix = f"qp{qp_value}_qn{qn_value}_qfd{qfd_value}_qfr{qfr_value}_qo{qo_value}_qs{qs_value}_qr{qr_value}_cl{cl_value}"
                csv_file_path = Path("..")/"expData"/"gzip_csv"/scene_name/f"log_{suffix}.csv"
                if os.path.exists(csv_file_path):
                    df = pd.read_csv(csv_file_path)
                    df['alg'] = 'gzip'
                    df['scene_name'] = scene_name
                    df['original_size'] = original_file_size
                    df['numOf3DGS'] = numOfPoint
                    if merged_df.empty:
                        merged_df = df
                    else:
                        merged_df = pd.concat([merged_df, df], axis=0)
                else:
                    continue
    # bzip2
    if bzip2:
        bzip2_compression_levels = [1, 3, 5, 7, 9]
        for scene_name in scene_names:
            numOfPoint = getNumOf3DGS(Path(f'..')/"expData"/"raw_ply"/scene_name/"point_cloud.ply")
            original_file_size=(Path(f'..')/"expData"/"raw_ply"/scene_name/"point_cloud.ply").stat().st_size
            qp_value = qn_value = qfd_value = qfr_value = qo_value = qs_value = qr_value = "y"
            for cl_value in bzip2_compression_levels:
                suffix = f"qp{qp_value}_qn{qn_value}_qfd{qfd_value}_qfr{qfr_value}_qo{qo_value}_qs{qs_value}_qr{qr_value}_cl{cl_value}"
                csv_file_path = Path("..")/"expData"/"bzip2_csv"/scene_name/f"log_{suffix}.csv"
                if os.path.exists(csv_file_path):
                    df = pd.read_csv(csv_file_path)
                    df['alg'] = 'bzip2'
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
        merged_df.to_csv(save_dir/"position.csv", index=False)

if __name__ == "__main__":
    
    numToCal = 1 # unuse
    scene_names = ["drjohnson"] # ["drjohnson", "playroom", "train", "truck"]
    qp_values = [4, 9, 16, 23, 30] # [0], [4, 9, 16, 23, 30] --> [4, 5, 7, 10, 14, 19, 30]
    qn_values = [1] # [0], [1] normal don't change this
    qfd_values = qfr_values = qo_values = [4, 9, 16, 23, 30] # [0], [2, 9, 16, 23, 30] --> [4, 5, 7, 10, 14, 19, 30]
    qs_values = qr_values = [4, 9, 16, 23, 30] # [0], [4, 9, 16, 23, 30] --> [4, 5, 7, 10, 14, 19, 30]
    cl_values = [7] # [3, 5, 7, 9]
    
    print(f"numToCal: {numToCal} (unuse)")
    print(f"scene_names: {scene_names}")
    print(f"qp_values: {qp_values}")
    print(f"qn_values: {qn_values}")
    print(f"qfd_values = qfr_values = qo_values: {qfd_values}")
    print(f"qs_values = qr_values: {qs_values}")
    print(f"cl_values: {cl_values}")
    
    qt_values = [0] # unuse
    qg_values = [0] # unuse
    
    main(numToCal, scene_names, 
        qp_values, 
        qn_values, 
        qfd_values, qfr_values, qo_values, 
        qs_values, qr_values, 
        cl_values, 
        qt_values, qg_values,
        gzip=False, bzip2=False)

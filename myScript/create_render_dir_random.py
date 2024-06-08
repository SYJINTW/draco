import os
import itertools
import shutil
from pathlib import Path
import numpy as np

def main(numToCal, scene_names, settings):  
    
    for scene_name in scene_names:
        decoded_ply_dir = Path("..")/"expData"/"draco_output_ply"/scene_name
        pretrain_model_dir = Path("..")/".."/"gaussian-splatting"/"output"/f"{scene_name}"
        for setting in settings:
            int_setting = setting.astype(int)
            qp_value, qn_value, qfd_value, qfr_value, qo_value, qs_value, qr_value, cl_value, qt_value, qg_value = int_setting
            print(qp_value, qn_value, qfd_value, qfr_value, qo_value, qs_value, qr_value, cl_value, qt_value, qg_value)
            suffix = f"qp{qp_value}_qn{qn_value}_qfd{qfd_value}_qfr{qfr_value}_qo{qo_value}_qs{qs_value}_qr{qr_value}_cl{cl_value}"
            decoded_ply_path = decoded_ply_dir/f"{scene_name}_{suffix}.ply"
            
            if os.path.exists(decoded_ply_path) and os.path.exists(pretrain_model_dir):
                save_dir = Path("..")/".."/"gaussian-splatting"/"output"/f"{scene_name}_{suffix}"
                save_dir.mkdir(parents=True, exist_ok=True)
                ply_save_dir = save_dir/"point_cloud"/"iteration_30000"
                ply_save_dir.mkdir(parents=True, exist_ok=True)
                
                # copy necessary file
                shutil.copyfile(pretrain_model_dir/"input.ply", save_dir/"input.ply")
                shutil.copyfile(pretrain_model_dir/"cameras.json", save_dir/"cameras.json")
                # copy the decoded point cloud
                shutil.copyfile(decoded_ply_path, ply_save_dir/"point_cloud.ply")
                
                # create new cfg_args
                with open(save_dir/"cfg_args", 'w') as f:
                    f.write(f"Namespace(data_device='cuda', eval=True, images='images', model_path='./output/{scene_name}_{suffix}', resolution=-1, sh_degree=3, source_path='./data/{scene_name}', white_background=False)")

if __name__ == "__main__":   
    numToCal = 1
    scene_names = ["drjohnson"] # ["drjohnson", "playroom", "train", "truck"]
    settings = np.load('../../random_data/random_arrays_4_20.npy')
    
    main(numToCal, scene_names, settings) 

import os
import itertools
import shutil
from pathlib import Path

def main(numToCal, scene_names, qp_values, qn_values, qfd_values, qfr_values, qo_values, qs_values, qr_values, cl_values, qt_values, qg_values):
    
    GS = qfd_values
    SH = qs_values
    
    for scene_name in scene_names:
        for qp_value in qp_values:
            for qn_value in qn_values:
                for GS_value in GS:
                    for SH_value in SH:
                        for cl_value in cl_values: 
                            decoded_ply_dir = Path("..")/"expData"/"draco_output_ply"/scene_name
                            pretrain_model_dir = Path("..")/".."/"gaussian-splatting"/"output"/f"{scene_name}"
                            
                            qfr_value = qo_value = qfd_value = GS_value
                            qr_value = qs_value = SH_value
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
    numToCal = 1 # unuse
    scene_names = ["lego"] # ["drjohnson", "playroom", "train", "truck"]
    qp_values = [4, 9, 16, 23, 30] # [0], [4, 9, 16, 23, 30] --> [4, 5, 7, 10, 14, 19, 30]
    qn_values = [1] # [0], [1] normal don't change this
    qfd_values = qfr_values = qo_values = [16] # [0], [4, 9, 16, 23, 30] --> [4, 5, 7, 10, 14, 19, 30]
    qs_values = qr_values = [16] # [0], [4, 9, 16, 23, 30] --> [4, 5, 7, 10, 14, 19, 30]
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
        qt_values, qg_values)

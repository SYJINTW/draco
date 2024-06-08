import os
import json
from pathlib import Path
import argparse

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-dp", "--decoder_path", type=str)
    parser.add_argument("-jp", "--json_path", type=str)
    args = parser.parse_args()
    
    with open(args.json_path) as f:
        data = json.load(f)
    
    start_dir_path = Path(data["start_dir_path"])
    # _3dgs_input_path = start_dir_path/data["3dgs_input_path"]
    draco_output_path = start_dir_path/data["draco_output_path"]
    _3dgs_output_path = start_dir_path/data["3dgs_output_path"]
    log_dir_path = start_dir_path/data["log_dir_path"]
    
    _3dgs_output_path.parent.mkdir(parents=True, exist_ok=True)
    log_dir_path.mkdir(parents=True, exist_ok=True)
    # print(draco_output_path)
    # print(_3dgs_output_path)
    
    os.system(f'{args.decoder_path} -point_cloud \
                -i {draco_output_path} \
                -o {_3dgs_output_path} \
                > {log_dir_path}/decode.log')
    
    print("\n[YC] Finish Draco decode.")
    
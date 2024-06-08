import os
import json
from pathlib import Path
import argparse

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-ep", "--encoder_path", type=str)
    parser.add_argument("-jp", "--json_path", type=str)
    args = parser.parse_args()
    
    with open(args.json_path) as f:
        data = json.load(f)
    
    start_dir_path = Path(data["start_dir_path"])
    _3dgs_input_path = start_dir_path/data["3dgs_input_path"]
    draco_output_path = start_dir_path/data["draco_output_path"]
    log_dir_path = start_dir_path/data["log_dir_path"]
    
    draco_output_path.parent.mkdir(parents=True, exist_ok=True)
    log_dir_path.mkdir(parents=True, exist_ok=True)
    # print(_3dgs_input_path)
    # print(draco_output_path)
    
    os.system(f'{args.encoder_path} -point_cloud \
                -i {_3dgs_input_path} \
                -o {draco_output_path} \
                -qp {data["qp"]} -qn {data["qn"]} \
                -qfd {data["qfd"]} \
                -qfr1 {data["qfr1"]} -qfr2 {data["qfr2"]} -qfr3 {data["qfr3"]} \
                -qo {data["qo"]} \
                -qs {data["qs"]} -qr {data["qr"]} \
                -cl {data["cl"]} \
                > {log_dir_path}/encode.log')
    
    print("\n[YC] Finish Draco encode.")
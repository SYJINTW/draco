import subprocess
import os
import time
import shutil
import pandas as pd
from pathlib import Path

def compress_with_gzip(file_dir, save_dir, compressNum=6):
    file_path = file_dir/"point_cloud.ply"  
    try:
        start_time = time.time()
        os.system(f"gzip -fk -{compressNum} {file_path}")
        end_time = time.time()
        # print(f"File {file_path} compressed with gzip.")
        shutil.move(str(file_dir/"point_cloud.ply.gz"), str(save_dir/"point_cloud.ply.gz"))
        file_size = os.path.getsize(str(save_dir/"point_cloud.ply.gz"))
        return (end_time - start_time) * 1000, file_size
    except Exception as e:
        print(f"Compression failed: {e}")
        return -1


def decompress_with_gzip(file_dir, save_dir):
    file_path = file_dir/"point_cloud.ply.gz"
    try:
        start_time = time.time()
        os.system(f"gzip -fdk {file_path}")
        end_time = time.time()
        # print(f"File {file_path} decompressed.")
        shutil.move(str(file_dir/"point_cloud.ply"), str(save_dir/"point_cloud.ply"))
        return (end_time - start_time) * 1000
    except Exception as e:
        print(f"Decompression failed: {e}")
        return -1

def compress_with_bzip2(file_dir, save_dir, compressNum=6):
    file_path = file_dir/"point_cloud.ply"  
    try:
        start_time = time.time()
        os.system(f"bzip2 -fk -{compressNum} {file_path}")
        end_time = time.time()
        # print(f"File {file_path} compressed with gzip.")
        shutil.move(str(file_dir/"point_cloud.ply.bz2"), str(save_dir/"point_cloud.ply.bz2"))
        file_size = os.path.getsize(str(save_dir/"point_cloud.ply.bz2"))
        return (end_time - start_time) * 1000, file_size
    except Exception as e:
        print(f"Compression failed: {e}")
        return -1


def decompress_with_bzip2(file_dir, save_dir):
    file_path = file_dir/"point_cloud.ply.bz2"
    try:
        start_time = time.time()
        os.system(f"bzip2 -fdk {file_path}")
        end_time = time.time()
        # print(f"File {file_path} decompressed.")
        shutil.move(str(file_dir/"point_cloud.ply"), str(save_dir/"point_cloud.ply"))
        return (end_time - start_time) * 1000
    except Exception as e:
        print(f"Decompression failed: {e}")
        return -1


def main(numToCal, scene_name, gzip, bzip2, gzip_compression_levels, bzip2_compression_levels):
    
    input_dir = Path("..")/"expData"/"raw_ply"/scene_name
    
    
    
    if gzip:
        save_gzip_dir = Path("..")/"expData"/"gzip_output_gzip"/scene_name
        save_gzip_dir.mkdir(parents=True, exist_ok=True)
        save_gzip_ply_dir = Path("..")/"expData"/"gzip_output_ply"/scene_name
        save_gzip_ply_dir.mkdir(parents=True, exist_ok=True)
        save_gzip_csv_dir = Path("..")/"expData"/"gzip_csv"/scene_name
        save_gzip_csv_dir.mkdir(parents=True, exist_ok=True)
        
        qp_value = qn_value = qfd_value = qfr_value = qo_value = qs_value = qr_value = "x" 
        for cl_value in gzip_compression_levels:
            suffix = f"qp{qp_value}_qn{qn_value}_qfd{qfd_value}_qfr{qfr_value}_qo{qo_value}_qs{qs_value}_qr{qr_value}_cl{cl_value}"
            datas = []
            for i in range(numToCal):
                # Compress file using gzip
                compress_time, encode_size = compress_with_gzip(input_dir, save_gzip_dir, cl_value)
                decompress_time = decompress_with_gzip(save_gzip_dir, save_gzip_ply_dir)
                # print(compress_time, decompress_time)
                datas.append([i, qp_value, qn_value, qfd_value, qfr_value, qo_value, qs_value, qr_value, cl_value, compress_time, encode_size, decompress_time, suffix])
            df = pd.DataFrame(datas, columns=["i", "qp_value", "qn_value", "qfd_value", "qfr_value", "qo_value", "qs_value", "qr_value", "cl_value", "encode_time", "encode_size", "decode_time", "suffix"])
            df.to_csv(save_gzip_csv_dir/f"log_{suffix}.csv", index=False)
    
    if bzip2:
        save_bzip2_dir = Path("..")/"expData"/"bzip2_output_bzip2"/scene_name
        save_bzip2_dir.mkdir(parents=True, exist_ok=True)
        save_bzip2_ply_dir = Path("..")/"expData"/"bzip2_output_ply"/scene_name
        save_bzip2_ply_dir.mkdir(parents=True, exist_ok=True)
        save_bzip2_csv_dir = Path("..")/"expData"/"bzip2_csv"/scene_name
        save_bzip2_csv_dir.mkdir(parents=True, exist_ok=True)
        
        qp_value = qn_value = qfd_value = qfr_value = qo_value = qs_value = qr_value = "y" 
        for cl_value in bzip2_compression_levels:
            suffix = f"qp{qp_value}_qn{qn_value}_qfd{qfd_value}_qfr{qfr_value}_qo{qo_value}_qs{qs_value}_qr{qr_value}_cl{cl_value}"
            datas = []
            for i in range(numToCal):
                # Compress file using gzip
                compress_time, encode_size = compress_with_bzip2(input_dir, save_bzip2_dir, cl_value)
                decompress_time = decompress_with_bzip2(save_bzip2_dir, save_bzip2_ply_dir)
                # print(compress_time, decompress_time)
                datas.append([i, qp_value, qn_value, qfd_value, qfr_value, qo_value, qs_value, qr_value, cl_value, compress_time, encode_size, decompress_time, suffix])
            df = pd.DataFrame(datas, columns=["i", "qp_value", "qn_value", "qfd_value", "qfr_value", "qo_value", "qs_value", "qr_value", "cl_value", "encode_time", "encode_size", "decode_time", "suffix"])
            df.to_csv(save_bzip2_csv_dir/f"log_{suffix}.csv", index=False)
    
            
if __name__ == "__main__":
    
    numToCal = 10
    # scene_names = ["drjohnson", "playroom", "train", "truck"]
    scene_names = ["truck"]
    gzip = True
    bzip2 = True
    gzip_compression_levels = [1, 3, 5, 7, 9] # (1-9)
    bzip2_compression_levels = [1, 3, 5, 7, 9] # (1-9)
    print(scene_names)
    
    for scene_name in scene_names:
        main(numToCal, scene_name, gzip, bzip2, gzip_compression_levels, bzip2_compression_levels)


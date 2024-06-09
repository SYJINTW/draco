# Draco Compression for 3DGS

This project is contributed to Draco 3D data compression (https://github.com/google/draco).

The original project didn't support 3D Gaussain Splatting compression. So, we proposed a 3D compression method based on the point cloud compressing method in Draco.
There are no comparison between the original and our proposed, but we show the figure of R-D curve by using our method to compress 3DGS frame. 
![RDCurve](https://github.com/SYJINTW/draco/blob/main/figs/bear_20_all_RD_PSNR_ori.png?raw=true)  
More details are shown in the paper.

 

## Build
### Build in Linux (Ubuntu)
```bash
mkdir build_dir && cd build_dir
cmake ../
make
```

## Usage
### Encode
**Simple**  
```bash
./build_dir/draco_encoder -point_cloud \
-i ./myData/ficus_3dgs.ply \
-o ./myData/ficus_3dgs_compressed.drc
```

**More complex setup**  
```bash
./build_dir/draco_encoder -point_cloud \
-i ./myData/ficus_3dgs.ply \
-o ./myData/ficus_3dgs_compressed.drc \
-qp 16 \
-qfd 16 -qfr1 16 -qfr2 16 -qfr3 16 \
-qo 16 \
-qs 16 -qr 16 \
-cl 10
```

### Decode
```bash
./build_dir/draco_decoder \
-i ./myData/ficus_3dgs_compress.drc \
-o ./myData/ficus_3dgs_distorted.ply
```

### For experiment
**Encode (Using my_encode.py and json file)**  
```bash
python my_encoder.py -jp ../myJson/template_sh0.json
```

**Decode (Using my_decode.py and json file)**  
```bash
python my_decoder.py -jp ../myJson/template.json
```

**Configuration json file**  
Based on the template json file at `myJson/template.json`

# Draco Compression for 3DGS

1. The name of the open-source project to which you contributed.

2. The problem you identified in the original project.

3. A detailed description of your proposed solution.

4. Comparison of your method & original method.
 

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

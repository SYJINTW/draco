import torch
from torch import nn
import numpy as np
from plyfile import PlyData, PlyElement
from pathlib import Path
# external lib
# from scene.gaussian_model import GaussianModel

def load_gaussian_ply(path: Path) -> list:
    plydata = PlyData.read(str(path))

    xyz = np.stack((np.asarray(plydata.elements[0]["x"]),
                    np.asarray(plydata.elements[0]["y"]),
                    np.asarray(plydata.elements[0]["z"])),  axis=1)
    
    opacities = np.asarray(plydata.elements[0]["opacity"])[..., np.newaxis]

    features_dc = np.zeros((xyz.shape[0], 3, 1))
    features_dc[:, 0, 0] = np.asarray(plydata.elements[0]["f_dc_0"])
    features_dc[:, 1, 0] = np.asarray(plydata.elements[0]["f_dc_1"])
    features_dc[:, 2, 0] = np.asarray(plydata.elements[0]["f_dc_2"])

    extra_f_names = [p.name for p in plydata.elements[0].properties if p.name.startswith("f_rest_")]
    extra_f_names = sorted(extra_f_names, key = lambda x: int(x.split('_')[-1]))
    assert len(extra_f_names)==3*(3 + 1) ** 2 - 3
    features_extra = np.zeros((xyz.shape[0], len(extra_f_names)))
    for idx, attr_name in enumerate(extra_f_names):
        features_extra[:, idx] = np.asarray(plydata.elements[0][attr_name])
    # Reshape (P,F*SH_coeffs) to (P, F, SH_coeffs except DC)
    features_extra = features_extra.reshape((features_extra.shape[0], 3, (3 + 1) ** 2 - 1))

    scale_names = [p.name for p in plydata.elements[0].properties if p.name.startswith("scale_")]
    scale_names = sorted(scale_names, key = lambda x: int(x.split('_')[-1]))
    scales = np.zeros((xyz.shape[0], len(scale_names)))
    for idx, attr_name in enumerate(scale_names):
        scales[:, idx] = np.asarray(plydata.elements[0][attr_name])

    rot_names = [p.name for p in plydata.elements[0].properties if p.name.startswith("rot")]
    rot_names = sorted(rot_names, key = lambda x: int(x.split('_')[-1]))
    rots = np.zeros((xyz.shape[0], len(rot_names)))
    for idx, attr_name in enumerate(rot_names):
        rots[:, idx] = np.asarray(plydata.elements[0][attr_name])

    results = [nn.Parameter(torch.tensor(xyz, dtype=torch.float, device="cuda").requires_grad_(True)), 
               nn.Parameter(torch.tensor(features_dc, dtype=torch.float, device="cuda").transpose(1, 2).contiguous().requires_grad_(True)),
               nn.Parameter(torch.tensor(features_extra, dtype=torch.float, device="cuda").transpose(1, 2).contiguous().requires_grad_(True)),
               nn.Parameter(torch.tensor(opacities, dtype=torch.float, device="cuda").requires_grad_(True)),
               nn.Parameter(torch.tensor(scales, dtype=torch.float, device="cuda").requires_grad_(True)),
               nn.Parameter(torch.tensor(rots, dtype=torch.float, device="cuda").requires_grad_(True))
               ]
    return results


def save_gaussian_ply_for_draco(_xyz, _features_dc, _features_rest, _opacity, _scaling, _rotation, path: Path, ascii=False):
        
        xyz = _xyz.detach().cpu().numpy()
        normals = np.zeros_like(xyz)
        f_dc = _features_dc.detach().transpose(1, 2).flatten(start_dim=1).contiguous().cpu().numpy()
        f_rest = _features_rest.detach().transpose(1, 2).flatten(start_dim=1).contiguous().cpu().numpy()
        opacities = _opacity.detach().cpu().numpy()
        scale = _scaling.detach().cpu().numpy()
        rotation = _rotation.detach().cpu().numpy()

        l = ['x', 'y', 'z', 'nx', 'ny', 'nz']
        # All channels except the 3 DC
        for i in range(_features_dc.shape[1]*_features_dc.shape[2]):
            l.append('f_dc_{}'.format(i))
        for i in range(_features_rest.shape[1]*_features_rest.shape[2]):
            l.append('f_rest_{}'.format(i))
        l.append('opacity')
        for i in range(_scaling.shape[1]):
            l.append('scale_{}'.format(i))
        for i in range(_rotation.shape[1]):
            l.append('rot_{}'.format(i))

        dtype_full = [(attribute, 'f4') for attribute in l]

        elements = np.empty(xyz.shape[0], dtype=dtype_full)
        attributes = np.concatenate((xyz, normals, f_dc, f_rest, opacities, scale, rotation), axis=1)
        elements[:] = list(map(tuple, attributes))
        el = PlyElement.describe(elements, 'vertex')
        
        if ascii:
            PlyData([el], text=True).write(str(path))
        else:
            PlyData([el]).write(str(path))

if __name__ == "__main__":
    load_ply_path = Path("..")/"mytest"/"gaussian_input"/"point_cloud.ply"
    output_ply_path = Path("..")/"mytest"/"draco_input"/"point_cloud.ply"
    
    results = load_gaussian_ply(load_ply_path)
    save_gaussian_ply_for_draco(results[0], results[1], results[2], results[3], results[4], results[5], output_ply_path)


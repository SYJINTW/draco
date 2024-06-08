// Copyright 2016 The Draco Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "draco/io/ply_encoder.h"

#include <memory>
#include <sstream>

#include "draco/io/file_writer_factory.h"
#include "draco/io/file_writer_interface.h"

namespace draco {

PlyEncoder::PlyEncoder()
    : out_buffer_(nullptr), in_point_cloud_(nullptr), in_mesh_(nullptr) {}

bool PlyEncoder::EncodeToFile(const PointCloud &pc,
                              const std::string &file_name) {
  std::unique_ptr<FileWriterInterface> file =
      FileWriterFactory::OpenWriter(file_name);
  if (!file) {
    return false;  // File couldn't be opened.
  }
  // Encode the mesh into a buffer.
  EncoderBuffer buffer;
  if (!EncodeToBuffer(pc, &buffer)) {
    return false;
  }
  //! [YC] note: important
  // Write the buffer into the file.
  file->Write(buffer.data(), buffer.size());
  return true;
}

bool PlyEncoder::EncodeToFile(const Mesh &mesh, const std::string &file_name) {
  in_mesh_ = &mesh;
  return EncodeToFile(static_cast<const PointCloud &>(mesh), file_name);
}

bool PlyEncoder::EncodeToBuffer(const PointCloud &pc,
                                EncoderBuffer *out_buffer) {
  in_point_cloud_ = &pc;
  out_buffer_ = out_buffer;
  if (!EncodeInternal()) {
    return ExitAndCleanup(false);
  }
  return ExitAndCleanup(true);
}

bool PlyEncoder::EncodeToBuffer(const Mesh &mesh, EncoderBuffer *out_buffer) {
  in_mesh_ = &mesh;
  return EncodeToBuffer(static_cast<const PointCloud &>(mesh), out_buffer);
}
bool PlyEncoder::EncodeInternal() {
  // Write PLY header.
  // TODO(ostava): Currently works only for xyz positions and rgb(a) colors.
  std::stringstream out;
  out << "ply" << std::endl;
  out << "format binary_little_endian 1.0" << std::endl;
  out << "element vertex " << in_point_cloud_->num_points() << std::endl;

  const int pos_att_id =
      in_point_cloud_->GetNamedAttributeId(GeometryAttribute::POSITION);
  int normal_att_id =
      in_point_cloud_->GetNamedAttributeId(GeometryAttribute::NORMAL);
  int tex_coord_att_id =
      in_point_cloud_->GetNamedAttributeId(GeometryAttribute::TEX_COORD);
  const int color_att_id =
      in_point_cloud_->GetNamedAttributeId(GeometryAttribute::COLOR);
  //! [YC] start: Get attribute id
  const int f_dc_att_id = in_point_cloud_->GetNamedAttributeId(GeometryAttribute::F_DC);
  // const int f_rest_att_id = in_point_cloud_->GetNamedAttributeId(GeometryAttribute::F_REST); 
  const int f_rest_1_att_id = in_point_cloud_->GetNamedAttributeId(GeometryAttribute::F_REST_1);
  const int f_rest_2_att_id = in_point_cloud_->GetNamedAttributeId(GeometryAttribute::F_REST_2);
  const int f_rest_3_att_id = in_point_cloud_->GetNamedAttributeId(GeometryAttribute::F_REST_3); 
  const int opacity_att_id = in_point_cloud_->GetNamedAttributeId(GeometryAttribute::OPACITY); 
  const int scale_att_id = in_point_cloud_->GetNamedAttributeId(GeometryAttribute::SCALE); 
  const int rot_att_id = in_point_cloud_->GetNamedAttributeId(GeometryAttribute::ROT); 
  //! [YC] end

  if (pos_att_id < 0) {
    return false;
  }

  // Ensure normals are 3 component. Don't encode them otherwise.
  if (normal_att_id >= 0 &&
      in_point_cloud_->attribute(normal_att_id)->num_components() != 3) {
    normal_att_id = -1;
  }

  // Ensure texture coordinates have only 2 components. Don't encode them
  // otherwise. TODO(ostava): Add support for 3 component normals (uvw).
  if (tex_coord_att_id >= 0 &&
      in_point_cloud_->attribute(tex_coord_att_id)->num_components() != 2) {
    tex_coord_att_id = -1;
  }

  out << "property " << GetAttributeDataType(pos_att_id) << " x" << std::endl;
  out << "property " << GetAttributeDataType(pos_att_id) << " y" << std::endl;
  out << "property " << GetAttributeDataType(pos_att_id) << " z" << std::endl;
  if (normal_att_id >= 0) {
    out << "property " << GetAttributeDataType(normal_att_id) << " nx"
        << std::endl;
    out << "property " << GetAttributeDataType(normal_att_id) << " ny"
        << std::endl;
    out << "property " << GetAttributeDataType(normal_att_id) << " nz"
        << std::endl;
  }
  if (color_att_id >= 0) {
    const auto *const attribute = in_point_cloud_->attribute(color_att_id);
    if (attribute->num_components() > 0) {
      out << "property " << GetAttributeDataType(color_att_id) << " red"
          << std::endl;
    }
    if (attribute->num_components() > 1) {
      out << "property " << GetAttributeDataType(color_att_id) << " green"
          << std::endl;
    }
    if (attribute->num_components() > 2) {
      out << "property " << GetAttributeDataType(color_att_id) << " blue"
          << std::endl;
    }
    if (attribute->num_components() > 3) {
      out << "property " << GetAttributeDataType(color_att_id) << " alpha"
          << std::endl;
    }
  }
  
  //! [YC] start: Add properties to header. Here have order issue
  if (f_dc_att_id >= 0) {
    out << "property " << GetAttributeDataType(f_dc_att_id) << " f_dc_0"
        << std::endl;
    out << "property " << GetAttributeDataType(f_dc_att_id) << " f_dc_1"
        << std::endl;
    out << "property " << GetAttributeDataType(f_dc_att_id) << " f_dc_2"
        << std::endl;
  }
  // if (f_rest_att_id >= 0) {
  //   for(int i = 0; i < 45; i++){
  //     out << "property " << GetAttributeDataType(f_dc_att_id) << " f_rest_" << i
  //         << std::endl;
  //   }
  // }
  if (f_rest_1_att_id >= 0) {
    for(int i = 0; i < 9; i++){
      out << "property " << GetAttributeDataType(f_rest_1_att_id) << " f_rest_" << i
          << std::endl;
    }
  }
  if (f_rest_2_att_id >= 0) {
    for(int i = 9; i < 24; i++){ // 24 = 9 + 15
      out << "property " << GetAttributeDataType(f_rest_2_att_id) << " f_rest_" << i
          << std::endl;
    }
  }
  if (f_rest_3_att_id >= 0) {
    for(int i = 24; i < 45; i++){ // 45 = 9 + 15 + 21
      out << "property " << GetAttributeDataType(f_rest_3_att_id) << " f_rest_" << i
          << std::endl;
    }
  }
  if (opacity_att_id >= 0) {
    out << "property " << GetAttributeDataType(opacity_att_id) << " opacity"
        << std::endl;
  }
  if (scale_att_id >= 0) {
    out << "property " << GetAttributeDataType(scale_att_id) << " scale_0"
        << std::endl;
    out << "property " << GetAttributeDataType(scale_att_id) << " scale_1"
        << std::endl;
    out << "property " << GetAttributeDataType(scale_att_id) << " scale_2"
        << std::endl;
  }
  if (rot_att_id >= 0) {
    out << "property " << GetAttributeDataType(rot_att_id) << " rot_0"
        << std::endl;
    out << "property " << GetAttributeDataType(rot_att_id) << " rot_1"
        << std::endl;
    out << "property " << GetAttributeDataType(rot_att_id) << " rot_2"
        << std::endl;
    out << "property " << GetAttributeDataType(rot_att_id) << " rot_3"
        << std::endl;
  }
  //! [YC] end

  if (in_mesh_) {
    out << "element face " << in_mesh_->num_faces() << std::endl;
    out << "property list uchar int vertex_indices" << std::endl;
    if (tex_coord_att_id >= 0) {
      // Texture coordinates are usually encoded in the property list (one value
      // per corner).
      out << "property list uchar " << GetAttributeDataType(tex_coord_att_id)
          << " texcoord" << std::endl;
    }
  }
  out << "end_header" << std::endl;

  // Not very efficient but the header should be small so just copy the stream
  // to a string.
  const std::string header_str = out.str();
  // printf("[YC] Encode header\n"); // [YC] add: print to check
  buffer()->Encode(header_str.data(), header_str.length());

  // Store point attributes.
  const int num_points = in_point_cloud_->num_points();
  printf("[YC] num_points: %d\n", num_points); // [YC] add: check print
  for (PointIndex v(0); v < num_points; ++v) {
    // printf("[YC] point: %d\n", v); // [YC] add: check print
    // printf("[YC] pos_att\n"); // [YC] add: check print
    const auto *const pos_att = in_point_cloud_->attribute(pos_att_id);
    buffer()->Encode(pos_att->GetAddress(pos_att->mapped_index(v)),
                     pos_att->byte_stride());
    
    if (normal_att_id >= 0) {
      // printf("[YC] normal_att_id\n"); // [YC] add: check print
      const auto *const normal_att = in_point_cloud_->attribute(normal_att_id);
      buffer()->Encode(normal_att->GetAddress(normal_att->mapped_index(v)),
                       normal_att->byte_stride());
    }
    if (color_att_id >= 0) {
      // printf("[YC] color_att_id\n"); // [YC] add: check print
      const auto *const color_att = in_point_cloud_->attribute(color_att_id);
      buffer()->Encode(color_att->GetAddress(color_att->mapped_index(v)),
                       color_att->byte_stride());
    }
    
    //! [YC] start: Write the vertex value to buffer
    if (f_dc_att_id >= 0) {
      // printf("[YC] f_dc_att_id\n"); // [YC] add: check print
      const auto *const f_dc_att = in_point_cloud_->attribute(f_dc_att_id);
      buffer()->Encode(f_dc_att->GetAddress(f_dc_att->mapped_index(v)),
                       f_dc_att->byte_stride());
    }
    // if (f_rest_att_id >= 0) {
    //   // printf("[YC] f_rest_att_id\n"); // [YC] add: check print
    //   const auto *const f_rest_att = in_point_cloud_->attribute(f_rest_att_id);
    //   buffer()->Encode(f_rest_att->GetAddress(f_rest_att->mapped_index(v)),
    //                    f_rest_att->byte_stride());
    // }
    if (f_rest_1_att_id >= 0) {
      // printf("[YC] f_rest_att_id\n"); // [YC] add: check print
      const auto *const f_rest_1_att = in_point_cloud_->attribute(f_rest_1_att_id);
      buffer()->Encode(f_rest_1_att->GetAddress(f_rest_1_att->mapped_index(v)),
                       f_rest_1_att->byte_stride());
    }
    if (f_rest_2_att_id >= 0) {
      // printf("[YC] f_rest_att_id\n"); // [YC] add: check print
      const auto *const f_rest_2_att = in_point_cloud_->attribute(f_rest_2_att_id);
      buffer()->Encode(f_rest_2_att->GetAddress(f_rest_2_att->mapped_index(v)),
                       f_rest_2_att->byte_stride());
    }
    if (f_rest_3_att_id >= 0) {
      // printf("[YC] f_rest_att_id\n"); // [YC] add: check print
      const auto *const f_rest_3_att = in_point_cloud_->attribute(f_rest_3_att_id);
      buffer()->Encode(f_rest_3_att->GetAddress(f_rest_3_att->mapped_index(v)),
                       f_rest_3_att->byte_stride());
    }
    if (opacity_att_id >= 0) {
      // printf("[YC] opacity_att_id\n"); // [YC] add: check print
      const auto *const opacity_att = in_point_cloud_->attribute(opacity_att_id);
      buffer()->Encode(opacity_att->GetAddress(opacity_att->mapped_index(v)),
                       opacity_att->byte_stride());
    }
    if (scale_att_id >= 0) {
      // printf("[YC] scale_att_id\n"); // [YC] add: check print
      const auto *const scale_att = in_point_cloud_->attribute(scale_att_id);
      buffer()->Encode(scale_att->GetAddress(scale_att->mapped_index(v)),
                       scale_att->byte_stride());
    }
    if (rot_att_id >= 0) {
      // printf("[YC] rot_att_id\n"); // [YC] add: check print
      const auto *const rot_att = in_point_cloud_->attribute(rot_att_id);
      buffer()->Encode(rot_att->GetAddress(rot_att->mapped_index(v)),
                       rot_att->byte_stride());
    }
    //! [YC] end
  }

  if (in_mesh_) {
    // Write face data.
    for (FaceIndex i(0); i < in_mesh_->num_faces(); ++i) {
      // Write the number of face indices (always 3).
      buffer()->Encode(static_cast<uint8_t>(3));

      const auto &f = in_mesh_->face(i);
      for (int c = 0; c < 3; ++c) {
        if (f[c] >= num_points) {
          // Invalid point stored on the |in_mesh_| face.
          return false;
        }
        buffer()->Encode(f[c]);
      }

      if (tex_coord_att_id >= 0) {
        // Two coordinates for every corner -> 6.
        buffer()->Encode(static_cast<uint8_t>(6));

        const auto *const tex_att =
            in_point_cloud_->attribute(tex_coord_att_id);
        for (int c = 0; c < 3; ++c) {
          buffer()->Encode(tex_att->GetAddress(tex_att->mapped_index(f[c])),
                           tex_att->byte_stride());
        }
      }
    }
  }
  return true;
}

bool PlyEncoder::ExitAndCleanup(bool return_value) {
  in_mesh_ = nullptr;
  in_point_cloud_ = nullptr;
  out_buffer_ = nullptr;
  return return_value;
}

const char *PlyEncoder::GetAttributeDataType(int attribute) {
  // TODO(ostava): Add support for more types.
  switch (in_point_cloud_->attribute(attribute)->data_type()) {
    case DT_FLOAT32:
      return "float";
    case DT_UINT8:
      return "uchar";
    case DT_INT32:
      return "int";
    default:
      break;
  }
  return nullptr;
}

}  // namespace draco

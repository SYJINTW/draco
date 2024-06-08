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
#include "draco/io/ply_decoder.h"

#include "draco/core/macros.h"
#include "draco/core/status.h"
#include "draco/io/file_utils.h"
#include "draco/io/ply_property_reader.h"

namespace draco {
namespace {
int64_t CountNumTriangles(const PlyElement &face_element,
                          const PlyProperty &vertex_indices) {
  int64_t num_triangles = 0;
  for (int i = 0; i < face_element.num_entries(); ++i) {
    const int64_t list_size = vertex_indices.GetListEntryNumValues(i);
    if (list_size < 3) {
      // Correctly encoded ply files don't have less than three vertices.
      continue;
    }
    num_triangles += list_size - 2;
  }
  return num_triangles;
}
}  // namespace

PlyDecoder::PlyDecoder() : out_mesh_(nullptr), out_point_cloud_(nullptr) {}

Status PlyDecoder::DecodeFromFile(const std::string &file_name,
                                  Mesh *out_mesh) {
  out_mesh_ = out_mesh;
  return DecodeFromFile(file_name, static_cast<PointCloud *>(out_mesh));
}

Status PlyDecoder::DecodeFromFile(const std::string &file_name,
                                  PointCloud *out_point_cloud) {
  std::vector<char> data;
  if (!ReadFileToBuffer(file_name, &data)) {
    return Status(Status::DRACO_ERROR, "Unable to read input file.");
  }
  buffer_.Init(data.data(), data.size());
  return DecodeFromBuffer(&buffer_, out_point_cloud);
}

Status PlyDecoder::DecodeFromBuffer(DecoderBuffer *buffer, Mesh *out_mesh) {
  out_mesh_ = out_mesh;
  return DecodeFromBuffer(buffer, static_cast<PointCloud *>(out_mesh));
}

Status PlyDecoder::DecodeFromBuffer(DecoderBuffer *buffer,
                                    PointCloud *out_point_cloud) {
  out_point_cloud_ = out_point_cloud;
  buffer_.Init(buffer->data_head(), buffer->remaining_size());
  return DecodeInternal();
}

Status PlyDecoder::DecodeInternal() {
  PlyReader ply_reader;
  DRACO_RETURN_IF_ERROR(ply_reader.Read(buffer()));
  // First, decode the connectivity data.
  if (out_mesh_)
    DRACO_RETURN_IF_ERROR(DecodeFaceData(ply_reader.GetElementByName("face")));
  // Decode all attributes.
  DRACO_RETURN_IF_ERROR(
      DecodeVertexData(ply_reader.GetElementByName("vertex")));
  // In case there are no faces this is just a point cloud which does
  // not require deduplication.
  if (out_mesh_ && out_mesh_->num_faces() != 0) {
#ifdef DRACO_ATTRIBUTE_VALUES_DEDUPLICATION_SUPPORTED
    if (!out_point_cloud_->DeduplicateAttributeValues()) {
      return Status(Status::DRACO_ERROR,
                    "Could not deduplicate attribute values");
    }
#endif
#ifdef DRACO_ATTRIBUTE_INDICES_DEDUPLICATION_SUPPORTED
    out_point_cloud_->DeduplicatePointIds();
#endif
  }
  return OkStatus();
}

Status PlyDecoder::DecodeFaceData(const PlyElement *face_element) {
  // We accept point clouds now.
  if (face_element == nullptr) {
    return Status(Status::INVALID_PARAMETER, "face_element is null");
  }
  const PlyProperty *vertex_indices =
      face_element->GetPropertyByName("vertex_indices");
  if (vertex_indices == nullptr) {
    // The property name may be named either "vertex_indices" or "vertex_index".
    vertex_indices = face_element->GetPropertyByName("vertex_index");
  }
  if (vertex_indices == nullptr || !vertex_indices->is_list()) {
    return Status(Status::DRACO_ERROR, "No faces defined");
  }

  // Allocate faces.
  out_mesh_->SetNumFaces(CountNumTriangles(*face_element, *vertex_indices));
  const int64_t num_polygons = face_element->num_entries();

  PlyPropertyReader<PointIndex::ValueType> vertex_index_reader(vertex_indices);
  Mesh::Face face;
  FaceIndex face_index(0);
  for (int i = 0; i < num_polygons; ++i) {
    const int64_t list_offset = vertex_indices->GetListEntryOffset(i);
    const int64_t list_size = vertex_indices->GetListEntryNumValues(i);
    if (list_size < 3) {
      continue;  // All invalid polygons are skipped.
    }

    // Triangulate polygon assuming the polygon is convex.
    const int64_t num_triangles = list_size - 2;
    face[0] = vertex_index_reader.ReadValue(static_cast<int>(list_offset));
    for (int64_t ti = 0; ti < num_triangles; ++ti) {
      for (int64_t c = 1; c < 3; ++c) {
        face[c] = vertex_index_reader.ReadValue(
            static_cast<int>(list_offset + ti + c));
      }
      out_mesh_->SetFace(face_index, face);
      face_index++;
    }
  }
  out_mesh_->SetNumFaces(face_index.value());
  return OkStatus();
}

template <typename DataTypeT>
bool PlyDecoder::ReadPropertiesToAttribute(
    const std::vector<const PlyProperty *> &properties,
    PointAttribute *attribute, int num_vertices) {
  std::vector<std::unique_ptr<PlyPropertyReader<DataTypeT>>> readers;
  readers.reserve(properties.size());
  for (int prop = 0; prop < properties.size(); ++prop) {
    readers.push_back(std::unique_ptr<PlyPropertyReader<DataTypeT>>(
        new PlyPropertyReader<DataTypeT>(properties[prop])));
  }
  std::vector<DataTypeT> memory(properties.size());
  for (PointIndex::ValueType i = 0; i < static_cast<uint32_t>(num_vertices);
       ++i) {
    for (int prop = 0; prop < properties.size(); ++prop) {
      memory[prop] = readers[prop]->ReadValue(i);
    }
    attribute->SetAttributeValue(AttributeValueIndex(i), memory.data());
  }
  return true;
}

//! [YC] note: important
Status PlyDecoder::DecodeVertexData(const PlyElement *vertex_element) {
  if (vertex_element == nullptr) {
    return Status(Status::INVALID_PARAMETER, "vertex_element is null");
  }
  // TODO(b/34330853): For now, try to load x,y,z vertices, red,green,blue,alpha
  // colors, and nx,ny,nz normals. We need to add other properties later.
  const PlyProperty *const x_prop = vertex_element->GetPropertyByName("x");
  const PlyProperty *const y_prop = vertex_element->GetPropertyByName("y");
  const PlyProperty *const z_prop = vertex_element->GetPropertyByName("z");
  if (!x_prop || !y_prop || !z_prop) {
    // Currently, we require 3 vertex coordinates (this should be generalized
    // later on).
    return Status(Status::INVALID_PARAMETER, "x, y, or z property is missing");
  }
  const PointIndex::ValueType num_vertices = vertex_element->num_entries();
  out_point_cloud_->set_num_points(num_vertices);
  // Decode vertex positions.
  {
    // All properties must have the same type.
    if (x_prop->data_type() != y_prop->data_type() ||
        y_prop->data_type() != z_prop->data_type()) {
      return Status(Status::INVALID_PARAMETER,
                    "x, y, and z properties must have the same type");
    }
    // TODO(ostava): For now assume the position types are float32 or int32.
    const DataType dt = x_prop->data_type();
    if (dt != DT_FLOAT32 && dt != DT_INT32) {
      return Status(Status::INVALID_PARAMETER,
                    "x, y, and z properties must be of type float32 or int32");
    }

    GeometryAttribute va;
    va.Init(GeometryAttribute::POSITION, nullptr, 3, dt, false,
            DataTypeLength(dt) * 3, 0);
    const int att_id = out_point_cloud_->AddAttribute(va, true, num_vertices);
    std::vector<const PlyProperty *> properties;
    properties.push_back(x_prop);
    properties.push_back(y_prop);
    properties.push_back(z_prop);
    if (dt == DT_FLOAT32) {
      ReadPropertiesToAttribute<float>(
          properties, out_point_cloud_->attribute(att_id), num_vertices);
    } else if (dt == DT_INT32) {
      ReadPropertiesToAttribute<int32_t>(
          properties, out_point_cloud_->attribute(att_id), num_vertices);
    }
  }

  //! [YC] start: Skip normal
  // Decode normals if present.
  const PlyProperty *const n_x_prop = vertex_element->GetPropertyByName("nx");
  const PlyProperty *const n_y_prop = vertex_element->GetPropertyByName("ny");
  const PlyProperty *const n_z_prop = vertex_element->GetPropertyByName("nz");
  if (n_x_prop != nullptr && n_y_prop != nullptr && n_z_prop != nullptr) {
    // For now, all normal properties must be set and of type float32
    if (n_x_prop->data_type() == DT_FLOAT32 &&
        n_y_prop->data_type() == DT_FLOAT32 &&
        n_z_prop->data_type() == DT_FLOAT32) {
      PlyPropertyReader<float> x_reader(n_x_prop);
      PlyPropertyReader<float> y_reader(n_y_prop);
      PlyPropertyReader<float> z_reader(n_z_prop);
      GeometryAttribute va;
      va.Init(GeometryAttribute::NORMAL, nullptr, 3, DT_FLOAT32, false,
              sizeof(float) * 3, 0);
      const int att_id = out_point_cloud_->AddAttribute(va, true, num_vertices);
      for (PointIndex::ValueType i = 0; i < num_vertices; ++i) {
        std::array<float, 3> val;
        val[0] = x_reader.ReadValue(i);
        val[1] = y_reader.ReadValue(i);
        val[2] = z_reader.ReadValue(i);
        out_point_cloud_->attribute(att_id)->SetAttributeValue(
            AttributeValueIndex(i), &val[0]);
      }
    }
  }
  //! [YC] end

  //! [YC] start: Add for 3D gaussian
  // Decode f_dc (Order 0)
  const PlyProperty *const f_dc_0 = vertex_element->GetPropertyByName("f_dc_0");
  const PlyProperty *const f_dc_1 = vertex_element->GetPropertyByName("f_dc_1");
  const PlyProperty *const f_dc_2 = vertex_element->GetPropertyByName("f_dc_2");
  if (f_dc_0 != nullptr && f_dc_1 != nullptr && f_dc_2 != nullptr) {
    // For now, all normal properties must be set and of type float32
    if (f_dc_0->data_type() == DT_FLOAT32 &&
        f_dc_1->data_type() == DT_FLOAT32 &&
        f_dc_2->data_type() == DT_FLOAT32) {
      PlyPropertyReader<float> x_reader(f_dc_0);
      PlyPropertyReader<float> y_reader(f_dc_1);
      PlyPropertyReader<float> z_reader(f_dc_2);
      GeometryAttribute va;
      va.Init(GeometryAttribute::F_DC, nullptr, 3, DT_FLOAT32, false,
              sizeof(float) * 3, 0);
      const int att_id = out_point_cloud_->AddAttribute(va, true, num_vertices);
      for (PointIndex::ValueType i = 0; i < num_vertices; ++i) {
        std::array<float, 3> val;
        val[0] = x_reader.ReadValue(i);
        val[1] = y_reader.ReadValue(i);
        val[2] = z_reader.ReadValue(i);
        out_point_cloud_->attribute(att_id)->SetAttributeValue(
            AttributeValueIndex(i), &val[0]);
      }
    }
  }

  // // Decode f_rest (all together). Will not be safe because without any check
  // GeometryAttribute va;
  // va.Init(GeometryAttribute::F_REST_1, nullptr, 45, DT_FLOAT32, false, sizeof(float) * 45, 0);
  // const int att_id = out_point_cloud_->AddAttribute(va, true, num_vertices);
  // for (PointIndex::ValueType i = 0; i < num_vertices; ++i) {
  //   std::array<float, 45> val;
  //   for(int restNum = 0; restNum < 45; restNum++){
  //     const PlyProperty *const f_rest_x = vertex_element->GetPropertyByName("f_rest_" + std::to_string(restNum));
  //     PlyPropertyReader<float> reader(f_rest_x);
  //     val[restNum] = reader.ReadValue(i);
  //   }
  //   out_point_cloud_->attribute(att_id)->SetAttributeValue(
  //   AttributeValueIndex(i), &val[0]);
  // }
  
  // Decode f_rest (Order 1). (3 * 3 = 9) Will not be safe because without any check
  const PlyProperty *const order1_prop = vertex_element->GetPropertyByName("f_rest_0");
  if (order1_prop != nullptr){
    GeometryAttribute va;
    va.Init(GeometryAttribute::F_REST_1, nullptr, 9, DT_FLOAT32, false, sizeof(float) * 9, 0);
    const int att_id_1 = out_point_cloud_->AddAttribute(va, true, num_vertices);
    for (PointIndex::ValueType i = 0; i < num_vertices; ++i) {
      std::array<float, 9> val;
      for(int restNum = 0; restNum < 9; restNum++){
        const PlyProperty *const f_rest_x = vertex_element->GetPropertyByName("f_rest_" + std::to_string(restNum));
        PlyPropertyReader<float> reader(f_rest_x);
        val[restNum] = reader.ReadValue(i);
      }
      out_point_cloud_->attribute(att_id_1)->SetAttributeValue(
        AttributeValueIndex(i), &val[0]);
    }
  }

  // Decode f_rest (Order 2) (5 * 3 = 15)
  const PlyProperty *const order2_prop = vertex_element->GetPropertyByName("f_rest_15");
  if (order2_prop != nullptr){
    GeometryAttribute va_2;
    const int numOfAtt_2 = 15; // numOfAtt == number of attribute
    va_2.Init(GeometryAttribute::F_REST_2, nullptr, numOfAtt_2, DT_FLOAT32, false, sizeof(float) * numOfAtt_2, 0);
    const int att_id_2 = out_point_cloud_->AddAttribute(va_2, true, num_vertices);
    for (PointIndex::ValueType i = 0; i < num_vertices; ++i) {
      std::array<float, numOfAtt_2> val;
      for(int restNum = 9; restNum < (9 + numOfAtt_2); restNum++){ // 24 = 9 + 15
        const PlyProperty *const f_rest_x = vertex_element->GetPropertyByName("f_rest_" + std::to_string(restNum));
        PlyPropertyReader<float> reader(f_rest_x);
        val[restNum - 9] = reader.ReadValue(i);
      }
      out_point_cloud_->attribute(att_id_2)->SetAttributeValue(
        AttributeValueIndex(i), &val[0]);
    }
  }

  // Decode f_rest (Order 3) (7 * 3 = 21)
  const PlyProperty *const order3_prop = vertex_element->GetPropertyByName("f_rest_24");
  if (order3_prop != nullptr){
    GeometryAttribute va_3;
    const int numOfAtt_3 = 21; // numOfAtt == number of attribute
    va_3.Init(GeometryAttribute::F_REST_3, nullptr, numOfAtt_3, DT_FLOAT32, false, sizeof(float) * numOfAtt_3, 0);
    const int att_id_3 = out_point_cloud_->AddAttribute(va_3, true, num_vertices);
    for (PointIndex::ValueType i = 0; i < num_vertices; ++i) {
      std::array<float, numOfAtt_3> val;
      for(int restNum = 24; restNum < (24 + numOfAtt_3); restNum++){ // 45 = 9 + 15 + 21
        const PlyProperty *const f_rest_x = vertex_element->GetPropertyByName("f_rest_" + std::to_string(restNum));
        PlyPropertyReader<float> reader(f_rest_x);
        val[restNum - 24] = reader.ReadValue(i);
      }
      out_point_cloud_->attribute(att_id_3)->SetAttributeValue(
      AttributeValueIndex(i), &val[0]);
    }
  }

  // Decode opacity
  const PlyProperty *const opacity = vertex_element->GetPropertyByName("opacity");
  if (opacity != nullptr) {
    // For now, all normal properties must be set and of type float32
    if (opacity->data_type() == DT_FLOAT32) {
      PlyPropertyReader<float> x_reader(opacity);
      GeometryAttribute va;
      va.Init(GeometryAttribute::OPACITY, nullptr, 1, DT_FLOAT32, false,
              sizeof(float) * 1, 0);
      const int att_id = out_point_cloud_->AddAttribute(va, true, num_vertices);
      for (PointIndex::ValueType i = 0; i < num_vertices; ++i) {
        std::array<float, 1> val;
        val[0] = x_reader.ReadValue(i);
        out_point_cloud_->attribute(att_id)->SetAttributeValue(
            AttributeValueIndex(i), &val[0]);
      }
    }
  }
  
  // Decode scale
  const PlyProperty *const scale_0 = vertex_element->GetPropertyByName("scale_0");
  const PlyProperty *const scale_1 = vertex_element->GetPropertyByName("scale_1");
  const PlyProperty *const scale_2 = vertex_element->GetPropertyByName("scale_2");
  if (scale_0 != nullptr && scale_1 != nullptr && scale_2 != nullptr) {
    // For now, all normal properties must be set and of type float32
    if (scale_0->data_type() == DT_FLOAT32 &&
        scale_1->data_type() == DT_FLOAT32 &&
        scale_2->data_type() == DT_FLOAT32) {
      PlyPropertyReader<float> x_reader(scale_0);
      PlyPropertyReader<float> y_reader(scale_1);
      PlyPropertyReader<float> z_reader(scale_2);
      GeometryAttribute va;
      va.Init(GeometryAttribute::SCALE, nullptr, 3, DT_FLOAT32, false,
              sizeof(float) * 3, 0);
      const int att_id = out_point_cloud_->AddAttribute(va, true, num_vertices);
      for (PointIndex::ValueType i = 0; i < num_vertices; ++i) {
        std::array<float, 3> val;
        val[0] = x_reader.ReadValue(i);
        val[1] = y_reader.ReadValue(i);
        val[2] = z_reader.ReadValue(i);
        out_point_cloud_->attribute(att_id)->SetAttributeValue(
            AttributeValueIndex(i), &val[0]);
      }
    }
  }

  // Decode rot
  const PlyProperty *const rot_0 = vertex_element->GetPropertyByName("rot_0");
  const PlyProperty *const rot_1 = vertex_element->GetPropertyByName("rot_1");
  const PlyProperty *const rot_2 = vertex_element->GetPropertyByName("rot_2");
  const PlyProperty *const rot_3 = vertex_element->GetPropertyByName("rot_3");
  if (rot_0 != nullptr && rot_1 != nullptr && rot_2 != nullptr && rot_3 != nullptr) {
    // For now, all normal properties must be set and of type float32
    if (rot_0->data_type() == DT_FLOAT32 &&
        rot_1->data_type() == DT_FLOAT32 &&
        rot_2->data_type() == DT_FLOAT32 &&
        rot_3->data_type() == DT_FLOAT32) {
      PlyPropertyReader<float> x_reader(rot_0);
      PlyPropertyReader<float> y_reader(rot_1);
      PlyPropertyReader<float> z_reader(rot_2);
      PlyPropertyReader<float> w_reader(rot_3);
      GeometryAttribute va;
      va.Init(GeometryAttribute::ROT, nullptr, 4, DT_FLOAT32, false,
              sizeof(float) * 4, 0);
      const int att_id = out_point_cloud_->AddAttribute(va, true, num_vertices);
      for (PointIndex::ValueType i = 0; i < num_vertices; ++i) {
        std::array<float, 4> val;
        val[0] = x_reader.ReadValue(i);
        val[1] = y_reader.ReadValue(i);
        val[2] = z_reader.ReadValue(i);
        val[3] = w_reader.ReadValue(i);
        out_point_cloud_->attribute(att_id)->SetAttributeValue(
            AttributeValueIndex(i), &val[0]);
      }
    }
  }

  //! [YC] end

  // Decode color data if present.
  int num_colors = 0;
  const PlyProperty *const r_prop = vertex_element->GetPropertyByName("red");
  const PlyProperty *const g_prop = vertex_element->GetPropertyByName("green");
  const PlyProperty *const b_prop = vertex_element->GetPropertyByName("blue");
  const PlyProperty *const a_prop = vertex_element->GetPropertyByName("alpha");
  if (r_prop) {
    ++num_colors;
  }
  if (g_prop) {
    ++num_colors;
  }
  if (b_prop) {
    ++num_colors;
  }
  if (a_prop) {
    ++num_colors;
  }

  if (num_colors) {
    std::vector<std::unique_ptr<PlyPropertyReader<uint8_t>>> color_readers;
    const PlyProperty *p;
    if (r_prop) {
      p = r_prop;
      // TODO(ostava): For now ensure the data type of all components is uint8.
      DRACO_DCHECK_EQ(true, p->data_type() == DT_UINT8);
      if (p->data_type() != DT_UINT8) {
        return Status(Status::INVALID_PARAMETER,
                      "Type of 'red' property must be uint8");
      }
      color_readers.push_back(std::unique_ptr<PlyPropertyReader<uint8_t>>(
          new PlyPropertyReader<uint8_t>(p)));
    }
    if (g_prop) {
      p = g_prop;
      // TODO(ostava): For now ensure the data type of all components is uint8.
      DRACO_DCHECK_EQ(true, p->data_type() == DT_UINT8);
      if (p->data_type() != DT_UINT8) {
        return Status(Status::INVALID_PARAMETER,
                      "Type of 'green' property must be uint8");
      }
      color_readers.push_back(std::unique_ptr<PlyPropertyReader<uint8_t>>(
          new PlyPropertyReader<uint8_t>(p)));
    }
    if (b_prop) {
      p = b_prop;
      // TODO(ostava): For now ensure the data type of all components is uint8.
      DRACO_DCHECK_EQ(true, p->data_type() == DT_UINT8);
      if (p->data_type() != DT_UINT8) {
        return Status(Status::INVALID_PARAMETER,
                      "Type of 'blue' property must be uint8");
      }
      color_readers.push_back(std::unique_ptr<PlyPropertyReader<uint8_t>>(
          new PlyPropertyReader<uint8_t>(p)));
    }
    if (a_prop) {
      p = a_prop;
      // TODO(ostava): For now ensure the data type of all components is uint8.
      DRACO_DCHECK_EQ(true, p->data_type() == DT_UINT8);
      if (p->data_type() != DT_UINT8) {
        return Status(Status::INVALID_PARAMETER,
                      "Type of 'alpha' property must be uint8");
      }
      color_readers.push_back(std::unique_ptr<PlyPropertyReader<uint8_t>>(
          new PlyPropertyReader<uint8_t>(p)));
    }

    GeometryAttribute va;
    va.Init(GeometryAttribute::COLOR, nullptr, num_colors, DT_UINT8, true,
            sizeof(uint8_t) * num_colors, 0);
    const int32_t att_id =
        out_point_cloud_->AddAttribute(va, true, num_vertices);
    for (PointIndex::ValueType i = 0; i < num_vertices; ++i) {
      std::array<uint8_t, 4> val;
      for (int j = 0; j < num_colors; j++) {
        val[j] = color_readers[j]->ReadValue(i);
      }
      out_point_cloud_->attribute(att_id)->SetAttributeValue(
          AttributeValueIndex(i), &val[0]);
    }
  }

  return OkStatus();
}

}  // namespace draco

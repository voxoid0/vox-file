/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Joel Becker
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include "vox_file.h"
#include <fstream>
#include <sstream>
#include <vector>

using namespace magicavoxel;
using namespace std;

// Reads a little-endian uint32 from a (binary) file
uint32_t le_u32read(ifstream& file) {
  char data[4];
  file.read(data, 4);
  return (static_cast<uint8_t>(data[0])) |
         (static_cast<uint8_t>(data[1]) << 8) |
         (static_cast<uint8_t>(data[2]) << 16) |
         (static_cast<uint8_t>(data[3]) << 24);
}

// Reads a little-endian int32 from a (binary) file.
int32_t le_i32read(ifstream& file) {
  return static_cast<int32_t>(le_u32read(file));
}

int read_byte(ifstream& file) {
  char byte;
  file.read(&byte, 1);
  return static_cast<int>(byte);
}

uint8_t read_uint8(ifstream& file) {
  return static_cast<uint8_t>(read_byte(file));
}


VoxFile::VoxFile(bool load_dense, bool load_sparse, bool remove_hidden_voxels)
    : load_dense_(load_dense),
      load_sparse_(load_sparse),
      remove_hidden_voxels_(remove_hidden_voxels),
      cur_size_{0, 0, 0},
      palette_(kDefaultPalette) {}

void VoxFile::Load(const std::string& path) {
  ifstream file(path, ios::in | ios::binary);
  dense_models_.clear();
  ReadId(file, "VOX ");
  auto version = le_i32read(file);

  // Read MAIN chunk. If the file has other chunks beyond MAIN, we are ignoring
  // them currently. (Current 3.x format appears to only have MAIN though, with
  // its child chunks.)
  ReadChunk(file);  

  for (auto& model : dense_models_) {
    model.palette() = palette_;
  }
}

void VoxFile::ReadId(ifstream& file, const string& id) const {
  if (id.length() != 4) throw std::logic_error("ID must be 4 characters");

  char fid[4];
  file.read(fid, 4);

  if (fid[0] != id[0] || fid[1] != id[1] || fid[2] != id[2] ||
      fid[3] != id[3]) {
    stringstream ss;
    ss << "Chunk ID Mismatch. Expected '" << id << "' but found '" << fid[0]
       << fid[1] << fid[2] << fid[3] << "'";
    throw VoxException(ss.str());
  }
}

void VoxFile::ReadChunk(ifstream& file) {
  char chunk_id_chars[5];
  file.read(chunk_id_chars, 4);
  chunk_id_chars[4] = '\0';
  string chunk_id(chunk_id_chars);

  const uint32_t contents_size = le_u32read(file);
  const uint32_t children_size = le_u32read(file);
  const uint32_t contents_start = static_cast<uint32_t>(file.tellg());

  if (chunk_id == "MAIN")
    ReadMainChunk(file, contents_size, children_size);
  else if (chunk_id == "SIZE")
    ReadSizeChunk(file, contents_size, children_size);
  else if (chunk_id == "XYZI")
    ReadXyziChunk(file, contents_size, children_size);
  else if (chunk_id == "RGBA")
    ReadRgbaChunk(file, contents_size, children_size);
  //else if (chunk_id == "MATT")

  // RIFF format enforces even byte boundaries between chunks.
  // if (contents_size & 1) ++contents_size;

  const uint32_t next_chunk_pos =
      contents_start + contents_size + children_size;
  file.seekg(next_chunk_pos);
}

void VoxFile::ReadMainChunk(std::ifstream& file, uint32_t contents_size,
                            uint32_t children_size) {
  // skip contents, to get to the beginning of the children
  file.seekg(contents_size, ios_base::cur);

  uint32_t end_pos = static_cast<uint32_t>(file.tellg()) + children_size;
  while (file.tellg() < end_pos) {
    ReadChunk(file);
  }
}

void VoxFile::ReadSizeChunk(std::ifstream& file, uint32_t contents_size,
                            uint32_t children_size) {
  cur_size_ = {le_u32read(file), le_u32read(file), le_u32read(file)};
}

void VoxFile::RemoveHiddenVoxels(VoxDenseModel& dense, VoxSparseModel& sparse, const vector<Voxel>& voxels)
{
  int n_removed = 0;
  for (const auto& voxel : voxels) {
    // If voxel is surrounded by non-empty cells on all 6 sides, and it is not
    // along one of the sides of the model (e.g. x == 0), we can remove it.
    if (voxel.x > 0 && dense.voxel(voxel.x - 1, voxel.y, voxel.z) &&
        voxel.x < dense.size().x - 1 &&
        dense.voxel(voxel.x + 1, voxel.y, voxel.z) && voxel.y > 0 &&
        dense.voxel(voxel.x, voxel.y - 1, voxel.z) &&
        voxel.y < dense.size().y - 1 &&
        dense.voxel(voxel.x, voxel.y + 1, voxel.z) && voxel.z > 0 &&
        dense.voxel(voxel.x, voxel.y, voxel.z - 1) &&
        voxel.z < dense.size().z - 1 &&
        dense.voxel(voxel.x, voxel.y, voxel.z + 1)) {
      dense.voxel(voxel.x, voxel.y, voxel.z) = 0;
      ++n_removed;
    } else {
      sparse.voxels().push_back(voxel);
    }
  }
}

void VoxFile::ReadXyziChunk(std::ifstream& file, uint32_t contents_size,
                            uint32_t children_size) {
  VoxDenseModel dense(cur_size_);
  VoxSparseModel sparse(cur_size_);
  vector<Voxel> voxels;
  uint32_t n_voxels = le_u32read(file);

  for (uint32_t i = 0; i < n_voxels; ++i) {
    Voxel voxel{read_uint8(file), read_uint8(file), read_uint8(file),
                read_uint8(file)};
    //sparse.voxels().push_back(voxel);
    voxels.push_back(voxel);
    dense.voxel(voxel.x, voxel.y, voxel.z) = voxel.color;
  }

  if (remove_hidden_voxels_) {
    RemoveHiddenVoxels(dense, sparse, voxels);
  } else {
    sparse.voxels().assign(voxels.begin(), voxels.end());
  }

  if (load_dense_) dense_models_.push_back(dense);
  if (load_sparse_) sparse_models_.push_back(sparse);
}

void VoxFile::ReadRgbaChunk(std::ifstream& file, uint32_t contents_size,
                            uint32_t children_size) {
  for (int i = 1; i < 256; ++i) {
    palette_[i].r = read_byte(file);
    palette_[i].g = read_byte(file);
    palette_[i].b = read_byte(file);
    palette_[i].a = read_byte(file);
  }
}

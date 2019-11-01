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

#ifndef VOX_FILE_H
#define VOX_FILE_H

#include <array>
#include <fstream>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace magicavoxel {

class VoxFile;
class VoxDenseModel;
class VoxSparseModel;
class VoxException;
struct Voxel;
struct Color;
struct Vec3i;

// 3D size. x, y, z is the width, height, depth...or width, depth, height...
// it's just more straightforward to use the axis names.
using Size = Vec3i;


// Exception thrown when there is a problem reading/writing a .vox file
class VoxException : public std::exception {
 public:
  explicit VoxException(const std::string& message) : message_(message) {}

  char const* what() const override { return message_.c_str(); }

 private:
  // const char* const message_;
  const std::string message_;
};

struct Vec3i {
  uint32_t x, y, z;
};


// RGBA color, as four bytes ranging from 0 to 255.
struct Color {
  Color() = default;
  constexpr Color(uint32_t val)
      : r(val & 0xff),
        g((val >> 8) & 0xff),
        b((val >> 16) & 0xff),
        a((val >> 24) & 0xff) {}
  constexpr Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a)
      : r(_r), g(_g), b(_b), a(_a) {}

  uint8_t r, g, b, a;
};

// A MagicaVoxel color palette: 256 RGBA values.
using Palette = std::array<Color, 256>;  // std::array<Color, 256>;

// MagicaVoxel's default color palette for .vox files, used if no palette is
// included in the file.
constexpr Palette kDefaultPalette{
    {0xffffffff, 0xffffffcc, 0xffffff99, 0xffffff66, 0xffffff33, 0xffffff00,
     0xffffccff, 0xffffcccc, 0xffffcc99, 0xffffcc66, 0xffffcc33, 0xffffcc00,
     0xffff99ff, 0xffff99cc, 0xffff9999, 0xffff9966, 0xffff9933, 0xffff9900,
     0xffff66ff, 0xffff66cc, 0xffff6699, 0xffff6666, 0xffff6633, 0xffff6600,
     0xffff33ff, 0xffff33cc, 0xffff3399, 0xffff3366, 0xffff3333, 0xffff3300,
     0xffff00ff, 0xffff00cc, 0xffff0099, 0xffff0066, 0xffff0033, 0xffff0000,
     0xffccffff, 0xffccffcc, 0xffccff99, 0xffccff66, 0xffccff33, 0xffccff00,
     0xffccccff, 0xffcccccc, 0xffcccc99, 0xffcccc66, 0xffcccc33, 0xffcccc00,
     0xffcc99ff, 0xffcc99cc, 0xffcc9999, 0xffcc9966, 0xffcc9933, 0xffcc9900,
     0xffcc66ff, 0xffcc66cc, 0xffcc6699, 0xffcc6666, 0xffcc6633, 0xffcc6600,
     0xffcc33ff, 0xffcc33cc, 0xffcc3399, 0xffcc3366, 0xffcc3333, 0xffcc3300,
     0xffcc00ff, 0xffcc00cc, 0xffcc0099, 0xffcc0066, 0xffcc0033, 0xffcc0000,
     0xff99ffff, 0xff99ffcc, 0xff99ff99, 0xff99ff66, 0xff99ff33, 0xff99ff00,
     0xff99ccff, 0xff99cccc, 0xff99cc99, 0xff99cc66, 0xff99cc33, 0xff99cc00,
     0xff9999ff, 0xff9999cc, 0xff999999, 0xff999966, 0xff999933, 0xff999900,
     0xff9966ff, 0xff9966cc, 0xff996699, 0xff996666, 0xff996633, 0xff996600,
     0xff9933ff, 0xff9933cc, 0xff993399, 0xff993366, 0xff993333, 0xff993300,
     0xff9900ff, 0xff9900cc, 0xff990099, 0xff990066, 0xff990033, 0xff990000,
     0xff66ffff, 0xff66ffcc, 0xff66ff99, 0xff66ff66, 0xff66ff33, 0xff66ff00,
     0xff66ccff, 0xff66cccc, 0xff66cc99, 0xff66cc66, 0xff66cc33, 0xff66cc00,
     0xff6699ff, 0xff6699cc, 0xff669999, 0xff669966, 0xff669933, 0xff669900,
     0xff6666ff, 0xff6666cc, 0xff666699, 0xff666666, 0xff666633, 0xff666600,
     0xff6633ff, 0xff6633cc, 0xff663399, 0xff663366, 0xff663333, 0xff663300,
     0xff6600ff, 0xff6600cc, 0xff660099, 0xff660066, 0xff660033, 0xff660000,
     0xff33ffff, 0xff33ffcc, 0xff33ff99, 0xff33ff66, 0xff33ff33, 0xff33ff00,
     0xff33ccff, 0xff33cccc, 0xff33cc99, 0xff33cc66, 0xff33cc33, 0xff33cc00,
     0xff3399ff, 0xff3399cc, 0xff339999, 0xff339966, 0xff339933, 0xff339900,
     0xff3366ff, 0xff3366cc, 0xff336699, 0xff336666, 0xff336633, 0xff336600,
     0xff3333ff, 0xff3333cc, 0xff333399, 0xff333366, 0xff333333, 0xff333300,
     0xff3300ff, 0xff3300cc, 0xff330099, 0xff330066, 0xff330033, 0xff330000,
     0xff00ffff, 0xff00ffcc, 0xff00ff99, 0xff00ff66, 0xff00ff33, 0xff00ff00,
     0xff00ccff, 0xff00cccc, 0xff00cc99, 0xff00cc66, 0xff00cc33, 0xff00cc00,
     0xff0099ff, 0xff0099cc, 0xff009999, 0xff009966, 0xff009933, 0xff009900,
     0xff0066ff, 0xff0066cc, 0xff006699, 0xff006666, 0xff006633, 0xff006600,
     0xff0033ff, 0xff0033cc, 0xff003399, 0xff003366, 0xff003333, 0xff003300,
     0xff0000ff, 0xff0000cc, 0xff000099, 0xff000066, 0xff000033, 0xffee0000,
     0xffdd0000, 0xffbb0000, 0xffaa0000, 0xff880000, 0xff770000, 0xff550000,
     0xff440000, 0xff220000, 0xff110000, 0xff00ee00, 0xff00dd00, 0xff00bb00,
     0xff00aa00, 0xff008800, 0xff007700, 0xff005500, 0xff004400, 0xff002200,
     0xff001100, 0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088,
     0xff000077, 0xff000055, 0xff000044, 0xff000022, 0xff000011, 0xffeeeeee,
     0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777, 0xff555555,
     0xff444444, 0xff222222, 0xff111111, 0xff000000}};

// Dense representation of a voxel model. That is, a three-dimensional array of
// color values, where each color value is a (byte) index into a Palette (array
// of RGBA color values).
class VoxDenseModel {
 public:
  explicit VoxDenseModel(const Size& size, std::vector<uint8_t> voxels,
                        const Palette& palette)
      : size_(size), voxels_(std::move(voxels)), palette_(palette) {}

  explicit VoxDenseModel(const Size& size,
                        const Palette& palette = kDefaultPalette)
      : size_(size) {
    voxels_.resize(size.x * size.y * size.z);
  }

  const Size& size() const noexcept { return size_; }
  Palette& palette() noexcept { return palette_; }
  const Palette& palette() const noexcept { return palette_; }
  uint8_t voxel(int x, int y, int z) const {
    return voxels_.at(x + (y * size_.x) + (z * size_.x * size_.y));
  }
  uint8_t& voxel(int x, int y, int z) {
    return voxels_.at(x + (y * size_.x) + (z * size_.x * size_.y));
  }
  std::vector<uint8_t>& data() noexcept { return voxels_; }

 private:
  const Size size_;
  std::vector<uint8_t> voxels_;
  Palette palette_;
};

// A voxel which has an x, y, z location, and a color value (index into palette)
struct Voxel
{
  uint8_t x, y, z;
  uint8_t color;
};

// Sparse representation of a voxel model. That is, a list of voxels, each
// containing its x,y,z location and color value (index into palette).
// For models with less than 1/4 of its voxels used, this will use less
// memory, but also can be easier or faster for processing, since empty
// voxels never need to be visited.
class VoxSparseModel
{
public:
  explicit VoxSparseModel(const Size& size, const Palette& palette = kDefaultPalette)
    : size_(size),
      palette_(palette)
  {}

  explicit VoxSparseModel(const Size& size, std::vector<Voxel> voxels, const Palette& palette)
    : size_(size),
      voxels_(std::move(voxels)),
      palette_(palette)
  {}

  const Size& size() const noexcept { return size_; }
  const std::vector<Voxel>& voxels() const noexcept { return voxels_; }
  std::vector<Voxel>& voxels() noexcept { return voxels_; }
  const Palette& palette() const noexcept { return palette_; }

 private:
  const Size size_;
  std::vector<Voxel> voxels_;
  Palette palette_;
};

// Used to load a .vox file of the MagicaVoxel format, into memory, as either
// dense models, sparse models, or both.
//
// File format information source:
// https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox.txt
class VoxFile final {
 public:
  // load_dense: if true, loads the models as dense models, accessible via denseModels()
  // load_sparse: if true, loads the models as sparse models, accessible via sparseModels()
  // remove_hidden_voxels: if true, removes voxels that can never be visible (its 6 sides
  //   are covered by other (non-empty) voxels.
  explicit VoxFile(bool load_dense = true, bool load_sparse = true,
                   bool remove_hidden_voxels = true);

  ~VoxFile() = default;
  VoxFile(const VoxFile&) = default;
  VoxFile(VoxFile&&) = default;
  VoxFile& operator=(const VoxFile&) = default;
  VoxFile& operator=(VoxFile&&) = default;

  // Clears any previously-loaded data and loads the models and (optional)
  // palette from the file at the given path.
  void Load(const std::string& path);

  std::vector<VoxDenseModel>& denseModels() noexcept { return dense_models_; }
  std::vector<VoxSparseModel>& sparseModels() noexcept { return sparse_models_; }

 private:
  // Reads a 4-character ID from the file, and asserts that it matches the given one.
  void ReadId(std::ifstream& file, const std::string& id) const;

  // Reads the next chunk (RIFF-like structure)
  void ReadChunk(std::ifstream& file);
  void ReadMainChunk(std::ifstream& file, uint32_t contents_size,
                     uint32_t children_size);
  void ReadSizeChunk(std::ifstream& file, uint32_t contents_size,
                     uint32_t children_size);
  void ReadXyziChunk(std::ifstream& file, uint32_t contents_size,
                     uint32_t children_size);
  void ReadRgbaChunk(std::ifstream& file, uint32_t contents_size,
                     uint32_t children_size);
  void RemoveHiddenVoxels(VoxDenseModel& dense, VoxSparseModel& sparse,
                          const std::vector<Voxel>& voxels);
 private:
  bool load_dense_;
  bool load_sparse_;
  bool remove_hidden_voxels_;

  // Stores the size read from the last SIZE chunk, which indicates the size
  // of the next model (XYZI chunk) in the file.
  Size cur_size_;
  std::vector<VoxDenseModel> dense_models_;
  std::vector<VoxSparseModel> sparse_models_;

  // Palette usd by the models. (If it is possible to have more than one palette
  // in a .vox file, we do not support it currently; but I don't believe it is
  // possible.)
  Palette palette_;
};

}  // namespace magicavoxel
#endif

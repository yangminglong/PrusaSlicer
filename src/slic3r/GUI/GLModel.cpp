#include "libslic3r/libslic3r.h"
#include "GLModel.hpp"

#include "3DScene.hpp"
#include "GUI_App.hpp"
#include "GLShader.hpp"
#if ENABLE_GLMODEL_STATISTICS
#include "Plater.hpp"
#include "GLCanvas3D.hpp"
#endif // ENABLE_GLMODEL_STATISTICS

#include "libslic3r/TriangleMesh.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/Polygon.hpp"
#if ENABLE_LEGACY_OPENGL_REMOVAL
#include "libslic3r/BuildVolume.hpp"
#include "libslic3r/Geometry/ConvexHull.hpp"
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

#if ENABLE_GLMODEL_STATISTICS
#include <imgui/imgui_internal.h>
#endif // ENABLE_GLMODEL_STATISTICS

#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string/predicate.hpp>

#if ENABLE_LEGACY_OPENGL_REMOVAL
#if ENABLE_SMOOTH_NORMALS
#include <igl/per_face_normals.h>
#include <igl/per_corner_normals.h>
#include <igl/per_vertex_normals.h>
#endif // ENABLE_SMOOTH_NORMALS
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

#include <GL/glew.h>

namespace Slic3r {
namespace GUI {

#if ENABLE_LEGACY_OPENGL_REMOVAL
#if ENABLE_SMOOTH_NORMALS
static void smooth_normals_corner(const TriangleMesh& mesh, std::vector<stl_normal>& normals)
{
    using MapMatrixXfUnaligned = Eigen::Map<const Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor | Eigen::DontAlign>>;
    using MapMatrixXiUnaligned = Eigen::Map<const Eigen::Matrix<int, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor | Eigen::DontAlign>>;

    std::vector<Vec3f> face_normals = its_face_normals(mesh.its);

    Eigen::MatrixXd vertices = MapMatrixXfUnaligned(mesh.its.vertices.front().data(),
        Eigen::Index(mesh.its.vertices.size()), 3).cast<double>();
    Eigen::MatrixXi indices = MapMatrixXiUnaligned(mesh.its.indices.front().data(),
        Eigen::Index(mesh.its.indices.size()), 3);
    Eigen::MatrixXd in_normals = MapMatrixXfUnaligned(face_normals.front().data(),
        Eigen::Index(face_normals.size()), 3).cast<double>();
    Eigen::MatrixXd out_normals;

    igl::per_corner_normals(vertices, indices, in_normals, 1.0, out_normals);

    normals = std::vector<stl_normal>(mesh.its.vertices.size());
    for (size_t i = 0; i < mesh.its.indices.size(); ++i) {
        for (size_t j = 0; j < 3; ++j) {
            normals[mesh.its.indices[i][j]] = out_normals.row(i * 3 + j).cast<float>();
        }
    }
}
#endif // ENABLE_SMOOTH_NORMALS
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

#if ENABLE_LEGACY_OPENGL_REMOVAL
void GLModel::Geometry::add_vertex(const Vec2f& position)
{
    assert(format.vertex_layout == EVertexLayout::P2);
    vertices.emplace_back(position.x());
    vertices.emplace_back(position.y());
}

void GLModel::Geometry::add_vertex(const Vec2f& position, const Vec2f& tex_coord)
{
    assert(format.vertex_layout == EVertexLayout::P2T2);
    vertices.emplace_back(position.x());
    vertices.emplace_back(position.y());
    vertices.emplace_back(tex_coord.x());
    vertices.emplace_back(tex_coord.y());
}

void GLModel::Geometry::add_vertex(const Vec3f& position)
{
    assert(format.vertex_layout == EVertexLayout::P3);
    vertices.emplace_back(position.x());
    vertices.emplace_back(position.y());
    vertices.emplace_back(position.z());
}

void GLModel::Geometry::add_vertex(const Vec3f& position, const Vec2f& tex_coord)
{
    assert(format.vertex_layout == EVertexLayout::P3T2);
    vertices.emplace_back(position.x());
    vertices.emplace_back(position.y());
    vertices.emplace_back(position.z());
    vertices.emplace_back(tex_coord.x());
    vertices.emplace_back(tex_coord.y());
}

void GLModel::Geometry::add_vertex(const Vec3f& position, const Vec3f& normal)
{
    assert(format.vertex_layout == EVertexLayout::P3N3);
    vertices.emplace_back(position.x());
    vertices.emplace_back(position.y());
    vertices.emplace_back(position.z());
    vertices.emplace_back(normal.x());
    vertices.emplace_back(normal.y());
    vertices.emplace_back(normal.z());
}

void GLModel::Geometry::add_index(unsigned int id)
{
    indices.emplace_back(id);
}

void GLModel::Geometry::add_line(unsigned int id1, unsigned int id2)
{
    indices.emplace_back(id1);
    indices.emplace_back(id2);
}

void GLModel::Geometry::add_triangle(unsigned int id1, unsigned int id2, unsigned int id3)
{
    indices.emplace_back(id1);
    indices.emplace_back(id2);
    indices.emplace_back(id3);
}

Vec2f GLModel::Geometry::extract_position_2(size_t id) const
{
    const size_t p_stride = position_stride_floats(format);
    if (p_stride != 2) {
        assert(false);
        return { FLT_MAX, FLT_MAX };
    }

    if (vertices_count() <= id) {
        assert(false);
        return { FLT_MAX, FLT_MAX };
    }

    const float* start = &vertices[id * vertex_stride_floats(format) + position_offset_floats(format)];
    return { *(start + 0), *(start + 1) };
}

Vec3f GLModel::Geometry::extract_position_3(size_t id) const
{
    const size_t p_stride = position_stride_floats(format);
    if (p_stride != 3) {
        assert(false);
        return { FLT_MAX, FLT_MAX, FLT_MAX };
    }

    if (vertices_count() <= id) {
        assert(false);
        return { FLT_MAX, FLT_MAX, FLT_MAX };
    }

    const float* start = &vertices[id * vertex_stride_floats(format) + position_offset_floats(format)];
    return { *(start + 0), *(start + 1), *(start + 2) };
}

Vec3f GLModel::Geometry::extract_normal_3(size_t id) const
{
    const size_t n_stride = normal_stride_floats(format);
    if (n_stride != 3) {
        assert(false);
        return { FLT_MAX, FLT_MAX, FLT_MAX };
    }

    if (vertices_count() <= id) {
        assert(false);
        return { FLT_MAX, FLT_MAX, FLT_MAX };
    }

    const float* start = &vertices[id * vertex_stride_floats(format) + normal_offset_floats(format)];
    return { *(start + 0), *(start + 1), *(start + 2) };
}

Vec2f GLModel::Geometry::extract_tex_coord_2(size_t id) const
{
    const size_t t_stride = tex_coord_stride_floats(format);
    if (t_stride != 2) {
        assert(false);
        return { FLT_MAX, FLT_MAX };
    }

    if (vertices_count() <= id) {
        assert(false);
        return { FLT_MAX, FLT_MAX };
    }

    const float* start = &vertices[id * vertex_stride_floats(format) + tex_coord_offset_floats(format)];
    return { *(start + 0), *(start + 1) };
}

void GLModel::Geometry::set_vertex(size_t id, const Vec3f& position, const Vec3f& normal)
{
    assert(format.vertex_layout == EVertexLayout::P3N3);
    assert(id < vertices_count());
    if (id < vertices_count()) {
        float* start = &vertices[id * vertex_stride_floats(format)];
        *(start + 0) = position.x();
        *(start + 1) = position.y();
        *(start + 2) = position.z();
        *(start + 3) = normal.x();
        *(start + 4) = normal.y();
        *(start + 5) = normal.z();
    }
}

void GLModel::Geometry::set_index(size_t id, unsigned int index)
{
    assert(id < indices_count());
    if (id < indices_count())
        indices[id] = index;
}

unsigned int GLModel::Geometry::extract_index(size_t id) const
{
    if (indices_count() <= id) {
        assert(false);
        return -1;
    }

    return indices[id];
}

void GLModel::Geometry::remove_vertex(size_t id)
{
    assert(id < vertices_count());
    if (id < vertices_count()) {
        const size_t stride = vertex_stride_floats(format);
        std::vector<float>::const_iterator it = vertices.begin() + id * stride;
        vertices.erase(it, it + stride);
    }
}

size_t GLModel::Geometry::vertex_stride_floats(const Format& format)
{
    switch (format.vertex_layout)
    {
    case EVertexLayout::P2:   { return 2; }
    case EVertexLayout::P2T2: { return 4; }
    case EVertexLayout::P3:   { return 3; }
    case EVertexLayout::P3T2: { return 5; }
    case EVertexLayout::P3N3: { return 6; }
    default:                  { assert(false); return 0; }
    };
}

size_t GLModel::Geometry::position_stride_floats(const Format& format)
{
    switch (format.vertex_layout)
    {
    case EVertexLayout::P2:
    case EVertexLayout::P2T2: { return 2; }
    case EVertexLayout::P3:
    case EVertexLayout::P3T2:
    case EVertexLayout::P3N3: { return 3; }
    default:                  { assert(false); return 0; }
    };
}

size_t GLModel::Geometry::position_offset_floats(const Format& format)
{
    switch (format.vertex_layout)
    {
    case EVertexLayout::P2:
    case EVertexLayout::P2T2:
    case EVertexLayout::P3:
    case EVertexLayout::P3T2:
    case EVertexLayout::P3N3: { return 0; }
    default:                  { assert(false); return 0; }
    };
}

size_t GLModel::Geometry::normal_stride_floats(const Format& format)
{
    switch (format.vertex_layout)
    {
    case EVertexLayout::P3N3: { return 3; }
    default:                  { assert(false); return 0; }
    };
}

size_t GLModel::Geometry::normal_offset_floats(const Format& format)
{
    switch (format.vertex_layout)
    {
    case EVertexLayout::P3N3: { return 3; }
    default:                  { assert(false); return 0; }
    };
}

size_t GLModel::Geometry::tex_coord_stride_floats(const Format& format)
{
    switch (format.vertex_layout)
    {
    case EVertexLayout::P2T2:
    case EVertexLayout::P3T2: { return 2; }
    default:                  { assert(false); return 0; }
    };
}

size_t GLModel::Geometry::tex_coord_offset_floats(const Format& format)
{
    switch (format.vertex_layout)
    {
    case EVertexLayout::P2T2: { return 2; }
    case EVertexLayout::P3T2: { return 3; }
    default:                  { assert(false); return 0; }
    };
}

size_t GLModel::Geometry::index_stride_bytes(const Geometry& data)
{
    switch (data.index_type)
    {
    case EIndexType::UINT:   { return sizeof(unsigned int); }
    case EIndexType::USHORT: { return sizeof(unsigned short); }
    case EIndexType::UBYTE:  { return sizeof(unsigned char); }
    default:                 { assert(false); return 0; }
    };
}

bool GLModel::Geometry::has_position(const Format& format)
{
    switch (format.vertex_layout)
    {
    case EVertexLayout::P2:
    case EVertexLayout::P2T2:
    case EVertexLayout::P3:
    case EVertexLayout::P3T2:
    case EVertexLayout::P3N3: { return true; }
    default:                  { assert(false); return false; }
    };
}

bool GLModel::Geometry::has_normal(const Format& format)
{
    switch (format.vertex_layout)
    {
    case EVertexLayout::P2:
    case EVertexLayout::P2T2:
    case EVertexLayout::P3:
    case EVertexLayout::P3T2: { return false; }
    case EVertexLayout::P3N3: { return true; }
    default:                  { assert(false); return false; }
    };
}

bool GLModel::Geometry::has_tex_coord(const Format& format)
{
    switch (format.vertex_layout)
    {
    case EVertexLayout::P2T2:
    case EVertexLayout::P3T2: { return true; }
    case EVertexLayout::P2:
    case EVertexLayout::P3:
    case EVertexLayout::P3N3: { return false; }
    default:                  { assert(false); return false; }
    };
}
#else
size_t GLModel::Geometry::vertices_count() const
{
    size_t ret = 0;
    for (const Entity& entity : entities) {
        ret += entity.positions.size();
    }
    return ret;
}

size_t GLModel::Geometry::indices_count() const
{
    size_t ret = 0;
    for (const Entity& entity : entities) {
        ret += entity.indices.size();
    }
    return ret;
}
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

#if ENABLE_GLMODEL_STATISTICS
GLModel::Statistics GLModel::s_statistics;
#endif // ENABLE_GLMODEL_STATISTICS

#if ENABLE_LEGACY_OPENGL_REMOVAL
void GLModel::init_from(Geometry&& data)
#else
void GLModel::init_from(const Geometry& data)
#endif // ENABLE_LEGACY_OPENGL_REMOVAL
{
#if ENABLE_LEGACY_OPENGL_REMOVAL
    if (is_initialized()) {
        // call reset() if you want to reuse this model
        assert(false);
        return;
    }

    if (data.vertices.empty() || data.indices.empty()) {
        assert(false);
        return;
    }

    m_render_data.geometry = std::move(data);

    // update bounding box
    for (size_t i = 0; i < vertices_count(); ++i) {
        const size_t position_stride = Geometry::position_stride_floats(data.format);
        if (position_stride == 3)
            m_bounding_box.merge(m_render_data.geometry.extract_position_3(i).cast<double>());
        else if (position_stride == 2) {
            const Vec2f position = m_render_data.geometry.extract_position_2(i);
            m_bounding_box.merge(Vec3f(position.x(), position.y(), 0.0f).cast<double>());
        }
    }
#else
    if (!m_render_data.empty()) // call reset() if you want to reuse this model
        return;

    for (const Geometry::Entity& entity : data.entities) {
        if (entity.positions.empty() || entity.indices.empty())
            continue;

        assert(entity.normals.empty() || entity.normals.size() == entity.positions.size());

        RenderData rdata;
        rdata.type = entity.type;
        rdata.color = entity.color;

        // vertices/normals data
        std::vector<float> vertices(6 * entity.positions.size());
        for (size_t i = 0; i < entity.positions.size(); ++i) {
            const size_t offset = i * 6;
            ::memcpy(static_cast<void*>(&vertices[offset]), static_cast<const void*>(entity.positions[i].data()), 3 * sizeof(float));
            if (!entity.normals.empty())
                ::memcpy(static_cast<void*>(&vertices[3 + offset]), static_cast<const void*>(entity.normals[i].data()), 3 * sizeof(float));
        }

        // indices data
        std::vector<unsigned int> indices = entity.indices;

        rdata.indices_count = static_cast<unsigned int>(indices.size());

        // update bounding box
        for (size_t i = 0; i < entity.positions.size(); ++i) {
            m_bounding_box.merge(entity.positions[i].cast<double>());
        }

        send_to_gpu(rdata, vertices, indices);
        m_render_data.emplace_back(rdata);
    }
#endif // ENABLE_LEGACY_OPENGL_REMOVAL
}

#if ENABLE_LEGACY_OPENGL_REMOVAL
#if ENABLE_SMOOTH_NORMALS
void GLModel::init_from(const TriangleMesh& mesh, bool smooth_normals)
{
    if (smooth_normals) {
        if (is_initialized()) {
            // call reset() if you want to reuse this model
            assert(false);
            return;
        }

        if (mesh.its.vertices.empty() || mesh.its.indices.empty()) {
            assert(false);
            return;
        }

        std::vector<stl_normal> normals;
        smooth_normals_corner(mesh, normals);

        const indexed_triangle_set& its = mesh.its;
        Geometry& data = m_render_data.geometry;
        data.format = { Geometry::EPrimitiveType::Triangles, Geometry::EVertexLayout::P3N3, GLModel::Geometry::index_type(3 * its.indices.size()) };
        data.reserve_vertices(3 * its.indices.size());
        data.reserve_indices(3 * its.indices.size());

        // vertices
        for (size_t i = 0; i < its.vertices.size(); ++i) {
            data.add_vertex(its.vertices[i], normals[i]);
        }

        // indices
        for (size_t i = 0; i < its.indices.size(); ++i) {
            const stl_triangle_vertex_indices& idx = its.indices[i];
            if (data.format.index_type == GLModel::Geometry::EIndexType::USHORT)
                data.add_ushort_triangle((unsigned short)idx(0), (unsigned short)idx(1), (unsigned short)idx(2));
            else
                data.add_uint_triangle((unsigned int)idx(0), (unsigned int)idx(1), (unsigned int)idx(2));
        }

        // update bounding box
        for (size_t i = 0; i < vertices_count(); ++i) {
            m_bounding_box.merge(m_render_data.geometry.extract_position_3(i).cast<double>());
        }
    }
    else
        init_from(mesh.its);
}
#else
void GLModel::init_from(const TriangleMesh& mesh)
{
    init_from(mesh.its);
}
#endif // ENABLE_SMOOTH_NORMALS

void GLModel::init_from(const indexed_triangle_set& its)
#else
void GLModel::init_from(const indexed_triangle_set& its, const BoundingBoxf3 &bbox)
#endif // ENABLE_LEGACY_OPENGL_REMOVAL
{
#if ENABLE_LEGACY_OPENGL_REMOVAL
    if (is_initialized()) {
        // call reset() if you want to reuse this model
        assert(false);
        return;
    }

    if (its.vertices.empty() || its.indices.empty()){
        assert(false);
        return;
    }

    Geometry& data = m_render_data.geometry;
    data.format = { Geometry::EPrimitiveType::Triangles, Geometry::EVertexLayout::P3N3 };
    data.reserve_vertices(3 * its.indices.size());
    data.reserve_indices(3 * its.indices.size());

    // vertices + indices
    unsigned int vertices_counter = 0;
    for (uint32_t i = 0; i < its.indices.size(); ++i) {
        const stl_triangle_vertex_indices face = its.indices[i];
        const stl_vertex                  vertex[3] = { its.vertices[face[0]], its.vertices[face[1]], its.vertices[face[2]] };
        const stl_vertex                  n = face_normal_normalized(vertex);
        for (size_t j = 0; j < 3; ++j) {
            data.add_vertex(vertex[j], n);
        }
        vertices_counter += 3;
        data.add_triangle(vertices_counter - 3, vertices_counter - 2, vertices_counter - 1);
    }

    // update bounding box
    for (size_t i = 0; i < vertices_count(); ++i) {
        m_bounding_box.merge(data.extract_position_3(i).cast<double>());
    }
#else
    if (!m_render_data.empty()) // call reset() if you want to reuse this model
        return;

    RenderData data;
    data.type = EPrimitiveType::Triangles;

    std::vector<float> vertices = std::vector<float>(18 * its.indices.size());
    std::vector<unsigned int> indices = std::vector<unsigned int>(3 * its.indices.size());

    unsigned int vertices_count = 0;
    for (uint32_t i = 0; i < its.indices.size(); ++i) {
        stl_triangle_vertex_indices face      = its.indices[i];
        stl_vertex                  vertex[3] = { its.vertices[face[0]], its.vertices[face[1]], its.vertices[face[2]] };
        stl_vertex                  n         = face_normal_normalized(vertex);
        for (size_t j = 0; j < 3; ++ j) {
            size_t offset = i * 18 + j * 6;
            ::memcpy(static_cast<void*>(&vertices[offset]), static_cast<const void*>(vertex[j].data()), 3 * sizeof(float));
            ::memcpy(static_cast<void*>(&vertices[3 + offset]), static_cast<const void*>(n.data()), 3 * sizeof(float));
        }
        for (size_t j = 0; j < 3; ++j)
            indices[i * 3 + j] = vertices_count + j;
        vertices_count += 3;
    }

    data.indices_count = static_cast<unsigned int>(indices.size());
    m_bounding_box = bbox;

    send_to_gpu(data, vertices, indices);
    m_render_data.emplace_back(data);
#endif // ENABLE_LEGACY_OPENGL_REMOVAL
}

#if !ENABLE_LEGACY_OPENGL_REMOVAL
void GLModel::init_from(const indexed_triangle_set& its)
{
    init_from(its, bounding_box(its));
}
#endif // !ENABLE_LEGACY_OPENGL_REMOVAL

void GLModel::init_from(const Polygons& polygons, float z)
{
#if ENABLE_LEGACY_OPENGL_REMOVAL
    if (is_initialized()) {
        // call reset() if you want to reuse this model
        assert(false);
        return;
    }

    if (polygons.empty()) {
        assert(false);
        return;
    }

    Geometry& data = m_render_data.geometry;
    data.format = { Geometry::EPrimitiveType::Lines, Geometry::EVertexLayout::P3 };

    size_t segments_count = 0;
    for (const Polygon& polygon : polygons) {
        segments_count += polygon.points.size();
    }

    data.reserve_vertices(2 * segments_count);
    data.reserve_indices(2 * segments_count);

    // vertices + indices
    unsigned int vertices_counter = 0;
    for (const Polygon& poly : polygons) {
        for (size_t i = 0; i < poly.points.size(); ++i) {
            const Point& p0 = poly.points[i];
            const Point& p1 = (i == poly.points.size() - 1) ? poly.points.front() : poly.points[i + 1];
            data.add_vertex(Vec3f(unscale<float>(p0.x()), unscale<float>(p0.y()), z));
            data.add_vertex(Vec3f(unscale<float>(p1.x()), unscale<float>(p1.y()), z));
            vertices_counter += 2;
            data.add_line(vertices_counter - 2, vertices_counter - 1);
        }
    }

    // update bounding box
    for (size_t i = 0; i < vertices_count(); ++i) {
        m_bounding_box.merge(data.extract_position_3(i).cast<double>());
    }
#else
    auto append_polygon = [](const Polygon& polygon, float z, GUI::GLModel::Geometry& data) {
        if (!polygon.empty()) {
            GUI::GLModel::Geometry::Entity entity;
            entity.type = GUI::GLModel::EPrimitiveType::LineLoop;
            // contour
            entity.positions.reserve(polygon.size() + 1);
            entity.indices.reserve(polygon.size() + 1);
            unsigned int id = 0;
            for (const Point& p : polygon) {
                Vec3f position = unscale(p.x(), p.y(), 0.0).cast<float>();
                position.z() = z;
                entity.positions.emplace_back(position);
                entity.indices.emplace_back(id++);
            }
            data.entities.emplace_back(entity);
        }
    };

    Geometry init_data;
    for (const Polygon& polygon : polygons) {
        append_polygon(polygon, z, init_data);
    }
    init_from(init_data);
#endif // ENABLE_LEGACY_OPENGL_REMOVAL
}

bool GLModel::init_from_file(const std::string& filename)
{
    if (!boost::filesystem::exists(filename))
        return false;

    if (!boost::algorithm::iends_with(filename, ".stl"))
        return false;

    Model model;
    try {
        model = Model::read_from_file(filename);
    }
    catch (std::exception&) {
        return false;
    }

#if ENABLE_LEGACY_OPENGL_REMOVAL
    init_from(model.mesh());
#else
    const TriangleMesh& mesh = model.mesh();
    init_from(mesh.its, mesh.bounding_box());
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

    m_filename = filename;

    return true;
}

#if !ENABLE_LEGACY_OPENGL_REMOVAL
void GLModel::set_color(int entity_id, const ColorRGBA& color)
{
    for (size_t i = 0; i < m_render_data.size(); ++i) {
        if (entity_id == -1 || static_cast<int>(i) == entity_id)
            m_render_data[i].color = color;
    }
}

ColorRGBA GLModel::get_color(size_t entity_id) const
{
    if (entity_id < 0 || entity_id >= m_render_data.size()) return ColorRGBA{};
    return m_render_data[entity_id].color;
}
#endif // !ENABLE_LEGACY_OPENGL_REMOVAL

void GLModel::reset()
{
#if ENABLE_LEGACY_OPENGL_REMOVAL
    // release gpu memory
    if (m_render_data.ibo_id > 0) {
        glsafe(::glDeleteBuffers(1, &m_render_data.ibo_id));
        m_render_data.ibo_id = 0;
#if ENABLE_GLMODEL_STATISTICS
        s_statistics.gpu_memory.indices.current -= indices_size_bytes();
#endif // ENABLE_GLMODEL_STATISTICS
    }
    if (m_render_data.vbo_id > 0) {
        glsafe(::glDeleteBuffers(1, &m_render_data.vbo_id));
        m_render_data.vbo_id = 0;
#if ENABLE_GLMODEL_STATISTICS
        s_statistics.gpu_memory.vertices.current -= vertices_size_bytes();
#endif // ENABLE_GLMODEL_STATISTICS
    }

    m_render_data.vertices_count = 0;
    m_render_data.indices_count  = 0;
    m_render_data.geometry.vertices = std::vector<float>();
    m_render_data.geometry.indices  = std::vector<unsigned int>();
#else
    for (RenderData& data : m_render_data) {
        // release gpu memory
        if (data.ibo_id > 0)
            glsafe(::glDeleteBuffers(1, &data.ibo_id));
        if (data.vbo_id > 0)
            glsafe(::glDeleteBuffers(1, &data.vbo_id));
    }

    m_render_data.clear();
#endif // ENABLE_LEGACY_OPENGL_REMOVAL
    m_bounding_box = BoundingBoxf3();
    m_filename = std::string();
}

#if ENABLE_LEGACY_OPENGL_REMOVAL
static GLenum get_primitive_mode(const GLModel::Geometry::Format& format)
{
    switch (format.type)
    {
    case GLModel::Geometry::EPrimitiveType::Points:        { return GL_POINTS; }
    default:
    case GLModel::Geometry::EPrimitiveType::Triangles:     { return GL_TRIANGLES; }
    case GLModel::Geometry::EPrimitiveType::TriangleStrip: { return GL_TRIANGLE_STRIP; }
    case GLModel::Geometry::EPrimitiveType::TriangleFan:   { return GL_TRIANGLE_FAN; }
    case GLModel::Geometry::EPrimitiveType::Lines:         { return GL_LINES; }
    case GLModel::Geometry::EPrimitiveType::LineStrip:     { return GL_LINE_STRIP; }
    case GLModel::Geometry::EPrimitiveType::LineLoop:      { return GL_LINE_LOOP; }
    }
}

static GLenum get_index_type(const GLModel::Geometry& data)
{
    switch (data.index_type)
    {
    default:
    case GLModel::Geometry::EIndexType::UINT:   { return GL_UNSIGNED_INT; }
    case GLModel::Geometry::EIndexType::USHORT: { return GL_UNSIGNED_SHORT; }
    case GLModel::Geometry::EIndexType::UBYTE:  { return GL_UNSIGNED_BYTE; }
    }
}

void GLModel::render() 
#else
void GLModel::render() const
#endif // ENABLE_LEGACY_OPENGL_REMOVAL
{
#if ENABLE_LEGACY_OPENGL_REMOVAL
    render(std::make_pair<size_t, size_t>(0, indices_count()));
#else
    GLShaderProgram* shader = wxGetApp().get_current_shader();

    for (const RenderData& data : m_render_data) {
        if (data.vbo_id == 0 || data.ibo_id == 0)
            continue;

        GLenum mode;
        switch (data.type)
        {
        default:
        case EPrimitiveType::Triangles: { mode = GL_TRIANGLES; break; }
        case EPrimitiveType::Lines:     { mode = GL_LINES; break; }
        case EPrimitiveType::LineStrip: { mode = GL_LINE_STRIP; break; }
        case EPrimitiveType::LineLoop:  { mode = GL_LINE_LOOP; break; }
        }

        glsafe(::glBindBuffer(GL_ARRAY_BUFFER, data.vbo_id));
        glsafe(::glVertexPointer(3, GL_FLOAT, 6 * sizeof(float), (const void*)0));
        glsafe(::glNormalPointer(GL_FLOAT, 6 * sizeof(float), (const void*)(3 * sizeof(float))));

        glsafe(::glEnableClientState(GL_VERTEX_ARRAY));
        glsafe(::glEnableClientState(GL_NORMAL_ARRAY));

        if (shader != nullptr)
            shader->set_uniform("uniform_color", data.color);
        else
            glsafe(::glColor4fv(data.color.data()));

        glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo_id));
        glsafe(::glDrawElements(mode, static_cast<GLsizei>(data.indices_count), GL_UNSIGNED_INT, (const void*)0));
        glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

        glsafe(::glDisableClientState(GL_NORMAL_ARRAY));
        glsafe(::glDisableClientState(GL_VERTEX_ARRAY));

        glsafe(::glBindBuffer(GL_ARRAY_BUFFER, 0));
    }
#endif // ENABLE_LEGACY_OPENGL_REMOVAL
}

#if ENABLE_LEGACY_OPENGL_REMOVAL
void GLModel::render(const std::pair<size_t, size_t>& range)
{
    if (m_render_disabled)
        return;

    if (range.second == range.first)
        return;

    GLShaderProgram* shader = wxGetApp().get_current_shader();
    if (shader == nullptr)
        return;

    // sends data to gpu if not done yet
    if (m_render_data.vbo_id == 0 || m_render_data.ibo_id == 0) {
        if (m_render_data.geometry.vertices_count() > 0 && m_render_data.geometry.indices_count() > 0 && !send_to_gpu())
            return;
    }

    const Geometry& data = m_render_data.geometry;

    const GLenum mode = get_primitive_mode(data.format);
    const GLenum index_type = get_index_type(data);

    const size_t vertex_stride_bytes = Geometry::vertex_stride_bytes(data.format);
    const bool position = Geometry::has_position(data.format);
    const bool normal = Geometry::has_normal(data.format);
    const bool tex_coord = Geometry::has_tex_coord(data.format);

    glsafe(::glBindBuffer(GL_ARRAY_BUFFER, m_render_data.vbo_id));

#if ENABLE_GL_SHADERS_ATTRIBUTES
    int position_id = -1;
    int normal_id = -1;
    int tex_coord_id = -1;
#endif // ENABLE_GL_SHADERS_ATTRIBUTES

    if (position) {
#if ENABLE_GL_SHADERS_ATTRIBUTES
        position_id = shader->get_attrib_location("v_position");
        if (position_id != -1) {
            glsafe(::glVertexAttribPointer(position_id, Geometry::position_stride_floats(data.format), GL_FLOAT, GL_FALSE, vertex_stride_bytes, (const void*)Geometry::position_offset_bytes(data.format)));
            glsafe(::glEnableVertexAttribArray(position_id));
        }
#else
        glsafe(::glVertexPointer(Geometry::position_stride_floats(data.format), GL_FLOAT, vertex_stride_bytes, (const void*)Geometry::position_offset_bytes(data.format)));
        glsafe(::glEnableClientState(GL_VERTEX_ARRAY));
#endif // ENABLE_GL_SHADERS_ATTRIBUTES
    }
    if (normal) {
#if ENABLE_GL_SHADERS_ATTRIBUTES
        normal_id = shader->get_attrib_location("v_normal");
        if (normal_id != -1) {
            glsafe(::glVertexAttribPointer(normal_id, Geometry::normal_stride_floats(data.format), GL_FLOAT, GL_FALSE, vertex_stride_bytes, (const void*)Geometry::normal_offset_bytes(data.format)));
            glsafe(::glEnableVertexAttribArray(normal_id));
        }
#else
        glsafe(::glNormalPointer(GL_FLOAT, vertex_stride_bytes, (const void*)Geometry::normal_offset_bytes(data.format)));
        glsafe(::glEnableClientState(GL_NORMAL_ARRAY));
#endif // ENABLE_GL_SHADERS_ATTRIBUTES
    }
    if (tex_coord) {
#if ENABLE_GL_SHADERS_ATTRIBUTES
        tex_coord_id = shader->get_attrib_location("v_tex_coord");
        if (tex_coord_id != -1) {
            glsafe(::glVertexAttribPointer(tex_coord_id, Geometry::tex_coord_stride_floats(data.format), GL_FLOAT, GL_FALSE, vertex_stride_bytes, (const void*)Geometry::tex_coord_offset_bytes(data.format)));
            glsafe(::glEnableVertexAttribArray(tex_coord_id));
        }
#else
        glsafe(::glTexCoordPointer(Geometry::tex_coord_stride_floats(data.format), GL_FLOAT, vertex_stride_bytes, (const void*)Geometry::tex_coord_offset_bytes(data.format)));
        glsafe(::glEnableClientState(GL_TEXTURE_COORD_ARRAY));
#endif // ENABLE_GL_SHADERS_ATTRIBUTES
    }

    shader->set_uniform("uniform_color", data.color);

    glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_render_data.ibo_id));
    glsafe(::glDrawElements(mode, range.second - range.first, index_type, (const void*)(range.first * Geometry::index_stride_bytes(data))));
    glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

#if ENABLE_GL_SHADERS_ATTRIBUTES
    if (tex_coord_id != -1)
        glsafe(::glDisableVertexAttribArray(tex_coord_id));
    if (normal_id != -1)
        glsafe(::glDisableVertexAttribArray(normal_id));
    if (position_id != -1)
        glsafe(::glDisableVertexAttribArray(position_id));
#else
    if (tex_coord)
        glsafe(::glDisableClientState(GL_TEXTURE_COORD_ARRAY));
    if (normal)
        glsafe(::glDisableClientState(GL_NORMAL_ARRAY));
    if (position)
        glsafe(::glDisableClientState(GL_VERTEX_ARRAY));
#endif // ENABLE_GL_SHADERS_ATTRIBUTES

    glsafe(::glBindBuffer(GL_ARRAY_BUFFER, 0));

#if ENABLE_GLMODEL_STATISTICS
    ++s_statistics.render_calls;
#endif // ENABLE_GLMODEL_STATISTICS
}
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

#if ENABLE_LEGACY_OPENGL_REMOVAL
void GLModel::render_instanced(unsigned int instances_vbo, unsigned int instances_count)
#else
void GLModel::render_instanced(unsigned int instances_vbo, unsigned int instances_count) const
#endif // ENABLE_LEGACY_OPENGL_REMOVAL
{
    if (instances_vbo == 0 || instances_count == 0)
        return;

    GLShaderProgram* shader = wxGetApp().get_current_shader();
#if ENABLE_LEGACY_OPENGL_REMOVAL
    if (shader == nullptr || !boost::algorithm::iends_with(shader->get_name(), "_instanced"))
        return;

    // vertex attributes
    const GLint position_id = shader->get_attrib_location("v_position");
    const GLint normal_id   = shader->get_attrib_location("v_normal");
    if (position_id == -1 || normal_id == -1)
        return;

    // instance attributes
    const GLint offset_id = shader->get_attrib_location("i_offset");
    const GLint scales_id = shader->get_attrib_location("i_scales");
    if (offset_id == -1 || scales_id == -1)
        return;

    if (m_render_data.vbo_id == 0 || m_render_data.ibo_id == 0) {
        if (!send_to_gpu())
            return;
    }
#else
    assert(shader == nullptr || boost::algorithm::iends_with(shader->get_name(), "_instanced"));

    // vertex attributes
    GLint position_id = (shader != nullptr) ? shader->get_attrib_location("v_position") : -1;
    GLint normal_id = (shader != nullptr) ? shader->get_attrib_location("v_normal") : -1;
    assert(position_id != -1 && normal_id != -1);

    // instance attributes
    GLint offset_id = (shader != nullptr) ? shader->get_attrib_location("i_offset") : -1;
    GLint scales_id = (shader != nullptr) ? shader->get_attrib_location("i_scales") : -1;
    assert(offset_id != -1 && scales_id != -1);
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

    glsafe(::glBindBuffer(GL_ARRAY_BUFFER, instances_vbo));
#if ENABLE_LEGACY_OPENGL_REMOVAL
    const size_t instance_stride = 5 * sizeof(float);
    glsafe(::glVertexAttribPointer(offset_id, 3, GL_FLOAT, GL_FALSE, instance_stride, (const void*)0));
    glsafe(::glEnableVertexAttribArray(offset_id));
    glsafe(::glVertexAttribDivisor(offset_id, 1));

    glsafe(::glVertexAttribPointer(scales_id, 2, GL_FLOAT, GL_FALSE, instance_stride, (const void*)(3 * sizeof(float))));
    glsafe(::glEnableVertexAttribArray(scales_id));
    glsafe(::glVertexAttribDivisor(scales_id, 1));
#else
    if (offset_id != -1) {
        glsafe(::glVertexAttribPointer(offset_id, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (GLvoid*)0));
        glsafe(::glEnableVertexAttribArray(offset_id));
        glsafe(::glVertexAttribDivisor(offset_id, 1));
    }
    if (scales_id != -1) {
        glsafe(::glVertexAttribPointer(scales_id, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (GLvoid*)(3 * sizeof(float))));
        glsafe(::glEnableVertexAttribArray(scales_id));
        glsafe(::glVertexAttribDivisor(scales_id, 1));
    }
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

#if ENABLE_LEGACY_OPENGL_REMOVAL
    const Geometry& data = m_render_data.geometry;

    const GLenum mode = get_primitive_mode(data.format);
    const GLenum index_type = get_index_type(data);

    const size_t vertex_stride_bytes = Geometry::vertex_stride_bytes(data.format);
    const bool position = Geometry::has_position(data.format);
    const bool normal   = Geometry::has_normal(data.format);

    glsafe(::glBindBuffer(GL_ARRAY_BUFFER, m_render_data.vbo_id));

    if (position) {
        glsafe(::glVertexAttribPointer(position_id, Geometry::position_stride_floats(data.format), GL_FLOAT, GL_FALSE, vertex_stride_bytes, (const void*)Geometry::position_offset_bytes(data.format)));
        glsafe(::glEnableVertexAttribArray(position_id));
    }

    if (normal) {
        glsafe(::glVertexAttribPointer(normal_id, Geometry::normal_stride_floats(data.format), GL_FLOAT, GL_FALSE, vertex_stride_bytes, (const void*)Geometry::normal_offset_bytes(data.format)));
        glsafe(::glEnableVertexAttribArray(normal_id));
    }

    shader->set_uniform("uniform_color", data.color);

    glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_render_data.ibo_id));
    glsafe(::glDrawElementsInstanced(mode, indices_count(), index_type, (const void*)0, instances_count));
    glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

    if (normal)
        glsafe(::glDisableVertexAttribArray(normal_id));
    if (position)
        glsafe(::glDisableVertexAttribArray(position_id));

    glsafe(::glDisableVertexAttribArray(scales_id));
    glsafe(::glDisableVertexAttribArray(offset_id));
#else
    for (const RenderData& data : m_render_data) {
        if (data.vbo_id == 0 || data.ibo_id == 0)
            continue;

        GLenum mode;
        switch (data.type)
        {
        default:
        case EPrimitiveType::Triangles: { mode = GL_TRIANGLES; break; }
        case EPrimitiveType::Lines:     { mode = GL_LINES; break; }
        case EPrimitiveType::LineStrip: { mode = GL_LINE_STRIP; break; }
        case EPrimitiveType::LineLoop:  { mode = GL_LINE_LOOP; break; }
        }

        if (shader != nullptr)
            shader->set_uniform("uniform_color", data.color);
        else
            glsafe(::glColor4fv(data.color.data()));

        glsafe(::glBindBuffer(GL_ARRAY_BUFFER, data.vbo_id));
        if (position_id != -1) {
            glsafe(::glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (GLvoid*)0));
            glsafe(::glEnableVertexAttribArray(position_id));
        }
        if (normal_id != -1) {
            glsafe(::glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (GLvoid*)(3 * sizeof(float))));
            glsafe(::glEnableVertexAttribArray(normal_id));
        }

        glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo_id));
        glsafe(::glDrawElementsInstanced(mode, static_cast<GLsizei>(data.indices_count), GL_UNSIGNED_INT, (const void*)0, instances_count));
        glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));

        if (normal_id != -1)
            glsafe(::glDisableVertexAttribArray(normal_id));
        if (position_id != -1)
            glsafe(::glDisableVertexAttribArray(position_id));
    }

    if (scales_id != -1)
        glsafe(::glDisableVertexAttribArray(scales_id));
    if (offset_id != -1)
        glsafe(::glDisableVertexAttribArray(offset_id));
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

    glsafe(::glBindBuffer(GL_ARRAY_BUFFER, 0));

#if ENABLE_GLMODEL_STATISTICS
    ++s_statistics.render_instanced_calls;
#endif // ENABLE_GLMODEL_STATISTICS
}

#if ENABLE_LEGACY_OPENGL_REMOVAL
bool GLModel::send_to_gpu()
{
    if (m_render_data.vbo_id > 0 || m_render_data.ibo_id > 0) {
        assert(false);
        return false;
    }

    Geometry& data = m_render_data.geometry;
    if (data.vertices.empty() || data.indices.empty()) {
        assert(false);
        return false;
    }

    // vertices
    glsafe(::glGenBuffers(1, &m_render_data.vbo_id));
    glsafe(::glBindBuffer(GL_ARRAY_BUFFER, m_render_data.vbo_id));
    glsafe(::glBufferData(GL_ARRAY_BUFFER, data.vertices_size_bytes(), data.vertices.data(), GL_STATIC_DRAW));
    glsafe(::glBindBuffer(GL_ARRAY_BUFFER, 0));
    m_render_data.vertices_count = vertices_count();
#if ENABLE_GLMODEL_STATISTICS
    s_statistics.gpu_memory.vertices.current += data.vertices_size_bytes();
    s_statistics.gpu_memory.vertices.max = std::max(s_statistics.gpu_memory.vertices.current, s_statistics.gpu_memory.vertices.max);
#endif // ENABLE_GLMODEL_STATISTICS
    data.vertices = std::vector<float>();

    // indices
    glsafe(::glGenBuffers(1, &m_render_data.ibo_id));
    glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_render_data.ibo_id));
    const size_t indices_count = data.indices.size();
    if (m_render_data.vertices_count <= 256) {
        // convert indices to unsigned char to save gpu memory
        std::vector<unsigned char> reduced_indices(indices_count);
        for (size_t i = 0; i < indices_count; ++i) {
            reduced_indices[i] = (unsigned char)data.indices[i];
        }
        data.index_type = Geometry::EIndexType::UBYTE;
        glsafe(::glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_count * sizeof(unsigned char), reduced_indices.data(), GL_STATIC_DRAW));
        glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    }
    else if (m_render_data.vertices_count <= 65536) {
        // convert indices to unsigned short to save gpu memory
        std::vector<unsigned short> reduced_indices(indices_count);
        for (size_t i = 0; i < data.indices.size(); ++i) {
            reduced_indices[i] = (unsigned short)data.indices[i];
        }
        data.index_type = Geometry::EIndexType::USHORT;
        glsafe(::glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_count * sizeof(unsigned short), reduced_indices.data(), GL_STATIC_DRAW));
        glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    }
    else {
        data.index_type = Geometry::EIndexType::UINT;
        glsafe(::glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.indices_size_bytes(), data.indices.data(), GL_STATIC_DRAW));
        glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    }
    m_render_data.indices_count = indices_count;
#if ENABLE_GLMODEL_STATISTICS
    s_statistics.gpu_memory.indices.current += data.indices_size_bytes();
    s_statistics.gpu_memory.indices.max = std::max(s_statistics.gpu_memory.indices.current, s_statistics.gpu_memory.indices.max);
#endif // ENABLE_GLMODEL_STATISTICS
    data.indices = std::vector<unsigned int>();

    return true;
}

#if ENABLE_GLMODEL_STATISTICS
void GLModel::render_statistics()
{
    static const float offset = 175.0f;
    ImGuiWrapper& imgui = *wxGetApp().imgui();

    auto add_memory = [&imgui](const std::string& label, int64_t memory) {
        auto format_string = [memory](const std::string& units, float value) {
            return std::to_string(memory) + " bytes (" +
                Slic3r::float_to_string_decimal_point(float(memory) * value, 3)
                + " " + units + ")";
        };

        static const float kb = 1024.0f;
        static const float inv_kb = 1.0f / kb;
        static const float mb = 1024.0f * kb;
        static const float inv_mb = 1.0f / mb;
        static const float gb = 1024.0f * mb;
        static const float inv_gb = 1.0f / gb;
        imgui.text_colored(ImGuiWrapper::COL_AC_MAINBLUE, label);
        ImGui::SameLine(offset);
        if (static_cast<float>(memory) < mb)
            imgui.text(format_string("KB", inv_kb));
        else if (static_cast<float>(memory) < gb)
            imgui.text(format_string("MB", inv_mb));
        else
            imgui.text(format_string("GB", inv_gb));
    };

    auto add_counter = [&imgui](const std::string& label, int64_t counter) {
        imgui.text_colored(ImGuiWrapper::COL_AC_MAINBLUE, label);
        ImGui::SameLine(offset);
        imgui.text(std::to_string(counter));
    };

    imgui.set_next_window_pos(0.5f * wxGetApp().plater()->get_current_canvas3D()->get_canvas_size().get_width(), 0.0f, ImGuiCond_Once, 0.5f, 0.0f);
    ImGui::SetNextWindowSizeConstraints({ 300.0f, 100.0f }, { 600.0f, 900.0f });
    imgui.begin(std::string("GLModel Statistics"), ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);
    ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());

    add_counter(std::string("Render calls:"), s_statistics.render_calls);
    add_counter(std::string("Render instanced calls:"), s_statistics.render_instanced_calls);

    if (ImGui::CollapsingHeader("GPU memory")) {
        ImGui::Indent(10.0f);
        if (ImGui::CollapsingHeader("Vertices")) {
            add_memory(std::string("Current:"), s_statistics.gpu_memory.vertices.current);
            add_memory(std::string("Max:"), s_statistics.gpu_memory.vertices.max);
        }
        if (ImGui::CollapsingHeader("Indices")) {
            add_memory(std::string("Current:"), s_statistics.gpu_memory.indices.current);
            add_memory(std::string("Max:"), s_statistics.gpu_memory.indices.max);
        }
        ImGui::Unindent(10.0f);
    }

    imgui.end();
}
#endif // ENABLE_GLMODEL_STATISTICS

#else
void GLModel::send_to_gpu(RenderData& data, const std::vector<float>& vertices, const std::vector<unsigned int>& indices)
{
    assert(data.vbo_id == 0);
    assert(data.ibo_id == 0);

    // vertex data -> send to gpu
    glsafe(::glGenBuffers(1, &data.vbo_id));
    glsafe(::glBindBuffer(GL_ARRAY_BUFFER, data.vbo_id));
    glsafe(::glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW));
    glsafe(::glBindBuffer(GL_ARRAY_BUFFER, 0));

    // indices data -> send to gpu
    glsafe(::glGenBuffers(1, &data.ibo_id));
    glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo_id));
    glsafe(::glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW));
    glsafe(::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

#if ENABLE_LEGACY_OPENGL_REMOVAL
template<typename Fn>
inline bool all_vertices_inside(const GLModel::Geometry& geometry, Fn fn)
{
    const size_t position_stride_floats = geometry.position_stride_floats(geometry.format);
    const size_t position_offset_floats = geometry.position_offset_floats(geometry.format);
    assert(position_stride_floats == 3);
    if (geometry.vertices.empty() || position_stride_floats != 3)
        return false;

    for (auto it = geometry.vertices.begin(); it != geometry.vertices.end(); ) {
        it += position_offset_floats;
        if (!fn({ *it, *(it + 1), *(it + 2) }))
            return false;
        it += (geometry.vertex_stride_floats(geometry.format) - position_offset_floats - position_stride_floats);
    }
    return true;
}

bool contains(const BuildVolume& volume, const GLModel& model, bool ignore_bottom)
{
    static constexpr const double epsilon = BuildVolume::BedEpsilon;
    switch (volume.type()) {
    case BuildVolume::Type::Rectangle:
    {
        BoundingBox3Base<Vec3d> build_volume = volume.bounding_volume().inflated(epsilon);
        if (volume.max_print_height() == 0.0)
            build_volume.max.z() = std::numeric_limits<double>::max();
        if (ignore_bottom)
            build_volume.min.z() = -std::numeric_limits<double>::max();
        const BoundingBoxf3& model_box = model.get_bounding_box();
        return build_volume.contains(model_box.min) && build_volume.contains(model_box.max);
    }
    case BuildVolume::Type::Circle:
    {
        const Geometry::Circled& circle = volume.circle();
        const Vec2f c = unscaled<float>(circle.center);
        const float r = unscaled<double>(circle.radius) + float(epsilon);
        const float r2 = sqr(r);
        return volume.max_print_height() == 0.0 ?
            all_vertices_inside(model.get_geometry(), [c, r2](const Vec3f& p) { return (to_2d(p) - c).squaredNorm() <= r2; }) :

            all_vertices_inside(model.get_geometry(), [c, r2, z = volume.max_print_height() + epsilon](const Vec3f& p) { return (to_2d(p) - c).squaredNorm() <= r2 && p.z() <= z; });
    }
    case BuildVolume::Type::Convex:
        //FIXME doing test on convex hull until we learn to do test on non-convex polygons efficiently.
    case BuildVolume::Type::Custom:
        return volume.max_print_height() == 0.0 ?
            all_vertices_inside(model.get_geometry(), [&volume](const Vec3f& p) { return Geometry::inside_convex_polygon(volume.top_bottom_convex_hull_decomposition_bed(), to_2d(p).cast<double>()); }) :
            all_vertices_inside(model.get_geometry(), [&volume, z = volume.max_print_height() + epsilon](const Vec3f& p) { return Geometry::inside_convex_polygon(volume.top_bottom_convex_hull_decomposition_bed(), to_2d(p).cast<double>()) && p.z() <= z; });
    default:
        return true;
    }
}
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

GLModel::Geometry stilized_arrow(unsigned int resolution, float tip_radius, float tip_height, float stem_radius, float stem_height)
{
#if !ENABLE_LEGACY_OPENGL_REMOVAL
    auto append_vertex = [](GLModel::Geometry::Entity& entity, const Vec3f& position, const Vec3f& normal) {
        entity.positions.emplace_back(position);
        entity.normals.emplace_back(normal);
    };
    auto append_indices = [](GLModel::Geometry::Entity& entity, unsigned int v1, unsigned int v2, unsigned int v3) {
        entity.indices.emplace_back(v1);
        entity.indices.emplace_back(v2);
        entity.indices.emplace_back(v3);
    };
#endif // !ENABLE_LEGACY_OPENGL_REMOVAL

    resolution = std::max<unsigned int>(4, resolution);

    GLModel::Geometry data;
#if ENABLE_LEGACY_OPENGL_REMOVAL
    data.format = { GLModel::Geometry::EPrimitiveType::Triangles, GLModel::Geometry::EVertexLayout::P3N3 };
    data.reserve_vertices(6 * resolution + 2);
    data.reserve_indices(6 * resolution * 3);
#else
    GLModel::Geometry::Entity entity;
    entity.type = GLModel::EPrimitiveType::Triangles;
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

    const float angle_step = 2.0f * float(PI) / float(resolution);
    std::vector<float> cosines(resolution);
    std::vector<float> sines(resolution);

    for (unsigned int i = 0; i < resolution; ++i) {
        const float angle = angle_step * float(i);
        cosines[i] = ::cos(angle);
        sines[i] = -::sin(angle);
    }

    const float total_height = tip_height + stem_height;

    // tip vertices/normals
#if ENABLE_LEGACY_OPENGL_REMOVAL
    data.add_vertex(Vec3f(0.0f, 0.0f, total_height), (Vec3f)Vec3f::UnitZ());
    for (unsigned int i = 0; i < resolution; ++i) {
        data.add_vertex(Vec3f(tip_radius * sines[i], tip_radius * cosines[i], stem_height), Vec3f(sines[i], cosines[i], 0.0f));
    }

    // tip triangles
    for (unsigned int i = 0; i < resolution; ++i) {
        const unsigned int v3 = (i < resolution - 1) ? i + 2 : 1;
        data.add_triangle(0, i + 1, v3);
    }

    // tip cap outer perimeter vertices
    for (unsigned int i = 0; i < resolution; ++i) {
        data.add_vertex(Vec3f(tip_radius * sines[i], tip_radius * cosines[i], stem_height), (Vec3f)(-Vec3f::UnitZ()));
    }

    // tip cap inner perimeter vertices
    for (unsigned int i = 0; i < resolution; ++i) {
        data.add_vertex(Vec3f(stem_radius * sines[i], stem_radius * cosines[i], stem_height), (Vec3f)(-Vec3f::UnitZ()));
    }

    // tip cap triangles
    for (unsigned int i = 0; i < resolution; ++i) {
        const unsigned int v2 = (i < resolution - 1) ? i + resolution + 2 : resolution + 1;
        const unsigned int v3 = (i < resolution - 1) ? i + 2 * resolution + 2 : 2 * resolution + 1;
        data.add_triangle(i + resolution + 1, v3, v2);
        data.add_triangle(i + resolution + 1, i + 2 * resolution + 1, v3);
    }

    // stem bottom vertices
    for (unsigned int i = 0; i < resolution; ++i) {
        data.add_vertex(Vec3f(stem_radius * sines[i], stem_radius * cosines[i], stem_height), Vec3f(sines[i], cosines[i], 0.0f));
    }

    // stem top vertices
    for (unsigned int i = 0; i < resolution; ++i) {
        data.add_vertex(Vec3f(stem_radius * sines[i], stem_radius * cosines[i], 0.0f), Vec3f(sines[i], cosines[i], 0.0f));
    }

    // stem triangles
    for (unsigned int i = 0; i < resolution; ++i) {
        const unsigned int v2 = (i < resolution - 1) ? i + 3 * resolution + 2 : 3 * resolution + 1;
        const unsigned int v3 = (i < resolution - 1) ? i + 4 * resolution + 2 : 4 * resolution + 1;
        data.add_triangle(i + 3 * resolution + 1, v3, v2);
        data.add_triangle(i + 3 * resolution + 1, i + 4 * resolution + 1, v3);
    }

    // stem cap vertices
    data.add_vertex((Vec3f)Vec3f::Zero(), (Vec3f)(-Vec3f::UnitZ()));
    for (unsigned int i = 0; i < resolution; ++i) {
        data.add_vertex(Vec3f(stem_radius * sines[i], stem_radius * cosines[i], 0.0f), (Vec3f)(-Vec3f::UnitZ()));
    }

    // stem cap triangles
    for (unsigned int i = 0; i < resolution; ++i) {
        const unsigned int v3 = (i < resolution - 1) ? i + 5 * resolution + 3 : 5 * resolution + 2;
        data.add_triangle(5 * resolution + 1, v3, i + 5 * resolution + 2);
    }
#else
    append_vertex(entity, { 0.0f, 0.0f, total_height }, Vec3f::UnitZ());
    for (int i = 0; i < resolution; ++i) {
        append_vertex(entity, { tip_radius * sines[i], tip_radius * cosines[i], stem_height }, { sines[i], cosines[i], 0.0f });
    }

    // tip triangles
    for (int i = 0; i < resolution; ++i) {
        const int v3 = (i < resolution - 1) ? i + 2 : 1;
        append_indices(entity, 0, i + 1, v3);
    }

    // tip cap outer perimeter vertices
    for (int i = 0; i < resolution; ++i) {
        append_vertex(entity, { tip_radius * sines[i], tip_radius * cosines[i], stem_height }, -Vec3f::UnitZ());
    }

    // tip cap inner perimeter vertices
    for (int i = 0; i < resolution; ++i) {
        append_vertex(entity, { stem_radius * sines[i], stem_radius * cosines[i], stem_height }, -Vec3f::UnitZ());
    }

    // tip cap triangles
    for (int i = 0; i < resolution; ++i) {
        const int v2 = (i < resolution - 1) ? i + resolution + 2 : resolution + 1;
        const int v3 = (i < resolution - 1) ? i + 2 * resolution + 2 : 2 * resolution + 1;
        append_indices(entity, i + resolution + 1, v3, v2);
        append_indices(entity, i + resolution + 1, i + 2 * resolution + 1, v3);
    }

    // stem bottom vertices
    for (int i = 0; i < resolution; ++i) {
        append_vertex(entity, { stem_radius * sines[i], stem_radius * cosines[i], stem_height }, { sines[i], cosines[i], 0.0f });
    }

    // stem top vertices
    for (int i = 0; i < resolution; ++i) {
        append_vertex(entity, { stem_radius * sines[i], stem_radius * cosines[i], 0.0f }, { sines[i], cosines[i], 0.0f });
    }

    // stem triangles
    for (int i = 0; i < resolution; ++i) {
        const int v2 = (i < resolution - 1) ? i + 3 * resolution + 2 : 3 * resolution + 1;
        const int v3 = (i < resolution - 1) ? i + 4 * resolution + 2 : 4 * resolution + 1;
        append_indices(entity, i + 3 * resolution + 1, v3, v2);
        append_indices(entity, i + 3 * resolution + 1, i + 4 * resolution + 1, v3);
    }

    // stem cap vertices
    append_vertex(entity, Vec3f::Zero(), -Vec3f::UnitZ());
    for (int i = 0; i < resolution; ++i) {
        append_vertex(entity, { stem_radius * sines[i], stem_radius * cosines[i], 0.0f }, -Vec3f::UnitZ());
    }

    // stem cap triangles
    for (int i = 0; i < resolution; ++i) {
        const int v3 = (i < resolution - 1) ? i + 5 * resolution + 3 : 5 * resolution + 2;
        append_indices(entity, 5 * resolution + 1, v3, i + 5 * resolution + 2);
    }

    data.entities.emplace_back(entity);
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

    return data;
}

GLModel::Geometry circular_arrow(unsigned int resolution, float radius, float tip_height, float tip_width, float stem_width, float thickness)
{
#if !ENABLE_LEGACY_OPENGL_REMOVAL
    auto append_vertex = [](GLModel::Geometry::Entity& entity, const Vec3f& position, const Vec3f& normal) {
        entity.positions.emplace_back(position);
        entity.normals.emplace_back(normal);
    };
    auto append_indices = [](GLModel::Geometry::Entity& entity, unsigned int v1, unsigned int v2, unsigned int v3) {
        entity.indices.emplace_back(v1);
        entity.indices.emplace_back(v2);
        entity.indices.emplace_back(v3);
    };
#endif // !ENABLE_LEGACY_OPENGL_REMOVAL

    resolution = std::max<unsigned int>(2, resolution);

    GLModel::Geometry data;
#if ENABLE_LEGACY_OPENGL_REMOVAL
    data.format = { GLModel::Geometry::EPrimitiveType::Triangles, GLModel::Geometry::EVertexLayout::P3N3 };
    data.reserve_vertices(8 * (resolution + 1) + 30);
    data.reserve_indices((8 * resolution + 16) * 3);
#else
    GLModel::Geometry::Entity entity;
    entity.type = GLModel::EPrimitiveType::Triangles;
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

    const float half_thickness = 0.5f * thickness;
    const float half_stem_width = 0.5f * stem_width;
    const float half_tip_width = 0.5f * tip_width;

    const float outer_radius = radius + half_stem_width;
    const float inner_radius = radius - half_stem_width;
    const float step_angle = 0.5f * float(PI) / float(resolution);

#if ENABLE_LEGACY_OPENGL_REMOVAL
    // tip
    // top face vertices
    data.add_vertex(Vec3f(0.0f, outer_radius, half_thickness), (Vec3f)Vec3f::UnitZ());
    data.add_vertex(Vec3f(0.0f, radius + half_tip_width, half_thickness), (Vec3f)Vec3f::UnitZ());
    data.add_vertex(Vec3f(-tip_height, radius, half_thickness), (Vec3f)Vec3f::UnitZ());
    data.add_vertex(Vec3f(0.0f, radius - half_tip_width, half_thickness), (Vec3f)Vec3f::UnitZ());
    data.add_vertex(Vec3f(0.0f, inner_radius, half_thickness), (Vec3f)Vec3f::UnitZ());

    // top face triangles
    data.add_triangle(0, 1, 2);
    data.add_triangle(0, 2, 4);
    data.add_triangle(4, 2, 3);

    // bottom face vertices
    data.add_vertex(Vec3f(0.0f, outer_radius, -half_thickness), (Vec3f)(-Vec3f::UnitZ()));
    data.add_vertex(Vec3f(0.0f, radius + half_tip_width, -half_thickness), (Vec3f)(-Vec3f::UnitZ()));
    data.add_vertex(Vec3f(-tip_height, radius, -half_thickness), (Vec3f)(-Vec3f::UnitZ()));
    data.add_vertex(Vec3f(0.0f, radius - half_tip_width, -half_thickness), (Vec3f)(-Vec3f::UnitZ()));
    data.add_vertex(Vec3f(0.0f, inner_radius, -half_thickness), (Vec3f)(-Vec3f::UnitZ()));

    // bottom face triangles
    data.add_triangle(5, 7, 6);
    data.add_triangle(5, 9, 7);
    data.add_triangle(9, 8, 7);

    // side faces vertices
    data.add_vertex(Vec3f(0.0f, outer_radius, -half_thickness), (Vec3f)Vec3f::UnitX());
    data.add_vertex(Vec3f(0.0f, radius + half_tip_width, -half_thickness), (Vec3f)Vec3f::UnitX());
    data.add_vertex(Vec3f(0.0f, outer_radius, half_thickness), (Vec3f)Vec3f::UnitX());
    data.add_vertex(Vec3f(0.0f, radius + half_tip_width, half_thickness), (Vec3f)Vec3f::UnitX());

    Vec3f normal(-half_tip_width, tip_height, 0.0f);
    normal.normalize();
    data.add_vertex(Vec3f(0.0f, radius + half_tip_width, -half_thickness), normal);
    data.add_vertex(Vec3f(-tip_height, radius, -half_thickness), normal);
    data.add_vertex(Vec3f(0.0f, radius + half_tip_width, half_thickness), normal);
    data.add_vertex(Vec3f(-tip_height, radius, half_thickness), normal);

    normal = { -half_tip_width, -tip_height, 0.0f };
    normal.normalize();
    data.add_vertex(Vec3f(-tip_height, radius, -half_thickness), normal);
    data.add_vertex(Vec3f(0.0f, radius - half_tip_width, -half_thickness), normal);
    data.add_vertex(Vec3f(-tip_height, radius, half_thickness), normal);
    data.add_vertex(Vec3f(0.0f, radius - half_tip_width, half_thickness), normal);

    data.add_vertex(Vec3f(0.0f, radius - half_tip_width, -half_thickness), (Vec3f)Vec3f::UnitX());
    data.add_vertex(Vec3f(0.0f, inner_radius, -half_thickness), (Vec3f)Vec3f::UnitX());
    data.add_vertex(Vec3f(0.0f, radius - half_tip_width, half_thickness), (Vec3f)Vec3f::UnitX());
    data.add_vertex(Vec3f(0.0f, inner_radius, half_thickness), (Vec3f)Vec3f::UnitX());

    // side face triangles
    for (unsigned int i = 0; i < 4; ++i) {
        const unsigned int ii = i * 4;
        data.add_triangle(10 + ii, 11 + ii, 13 + ii);
        data.add_triangle(10 + ii, 13 + ii, 12 + ii);
    }

    // stem
    // top face vertices
    for (unsigned int i = 0; i <= resolution; ++i) {
        const float angle = float(i) * step_angle;
        data.add_vertex(Vec3f(inner_radius * ::sin(angle), inner_radius * ::cos(angle), half_thickness), (Vec3f)Vec3f::UnitZ());
    }

    for (unsigned int i = 0; i <= resolution; ++i) {
        const float angle = float(i) * step_angle;
        data.add_vertex(Vec3f(outer_radius * ::sin(angle), outer_radius * ::cos(angle), half_thickness), (Vec3f)Vec3f::UnitZ());
    }

    // top face triangles
    for (unsigned int i = 0; i < resolution; ++i) {
        data.add_triangle(26 + i, 27 + i, 27 + resolution + i);
        data.add_triangle(27 + i, 28 + resolution + i, 27 + resolution + i);
    }

    // bottom face vertices
    for (unsigned int i = 0; i <= resolution; ++i) {
        const float angle = float(i) * step_angle;
        data.add_vertex(Vec3f(inner_radius * ::sin(angle), inner_radius * ::cos(angle), -half_thickness), (Vec3f)(-Vec3f::UnitZ()));
    }

    for (unsigned int i = 0; i <= resolution; ++i) {
        const float angle = float(i) * step_angle;
        data.add_vertex(Vec3f(outer_radius * ::sin(angle), outer_radius * ::cos(angle), -half_thickness), (Vec3f)(-Vec3f::UnitZ()));
    }

    // bottom face triangles
    for (unsigned int i = 0; i < resolution; ++i) {
        data.add_triangle(28 + 2 * resolution + i, 29 + 3 * resolution + i, 29 + 2 * resolution + i);
        data.add_triangle(29 + 2 * resolution + i, 29 + 3 * resolution + i, 30 + 3 * resolution + i);
    }

    // side faces vertices and triangles
    for (unsigned int i = 0; i <= resolution; ++i) {
        const float angle = float(i) * step_angle;
        const float c = ::cos(angle);
        const float s = ::sin(angle);
        data.add_vertex(Vec3f(inner_radius * s, inner_radius * c, -half_thickness), Vec3f(-s, -c, 0.0f));
    }

    for (unsigned int i = 0; i <= resolution; ++i) {
        const float angle = float(i) * step_angle;
        const float c = ::cos(angle);
        const float s = ::sin(angle);
        data.add_vertex(Vec3f(inner_radius * s, inner_radius * c, half_thickness), Vec3f(-s, -c, 0.0f));
    }

    unsigned int first_id = 26 + 4 * (resolution + 1);
    for (unsigned int i = 0; i < resolution; ++i) {
        const unsigned int ii = first_id + i;
        data.add_triangle(ii, ii + 1, ii + resolution + 2);
        data.add_triangle(ii, ii + resolution + 2, ii + resolution + 1);
    }

    data.add_vertex(Vec3f(inner_radius, 0.0f, -half_thickness), (Vec3f)(-Vec3f::UnitY()));
    data.add_vertex(Vec3f(outer_radius, 0.0f, -half_thickness), (Vec3f)(-Vec3f::UnitY()));
    data.add_vertex(Vec3f(inner_radius, 0.0f, half_thickness), (Vec3f)(-Vec3f::UnitY()));
    data.add_vertex(Vec3f(outer_radius, 0.0f, half_thickness), (Vec3f)(-Vec3f::UnitY()));

    first_id = 26 + 6 * (resolution + 1);
    data.add_triangle(first_id, first_id + 1, first_id + 3);
    data.add_triangle(first_id, first_id + 3, first_id + 2);

    for (int i = resolution; i >= 0; --i) {
        const float angle = float(i) * step_angle;
        const float c = ::cos(angle);
        const float s = ::sin(angle);
        data.add_vertex(Vec3f(outer_radius * s, outer_radius * c, -half_thickness), Vec3f(s, c, 0.0f));
    }

    for (int i = resolution; i >= 0; --i) {
        const float angle = float(i) * step_angle;
        const float c = ::cos(angle);
        const float s = ::sin(angle);
        data.add_vertex(Vec3f(outer_radius * s, outer_radius * c, +half_thickness), Vec3f(s, c, 0.0f));
    }

    first_id = 30 + 6 * (resolution + 1);
    for (unsigned int i = 0; i < resolution; ++i) {
        const unsigned int ii = first_id + i;
        data.add_triangle(ii, ii + 1, ii + resolution + 2);
        data.add_triangle(ii, ii + resolution + 2, ii + resolution + 1);
    }
#else
    // tip
    // top face vertices
    append_vertex(entity, { 0.0f, outer_radius, half_thickness }, Vec3f::UnitZ());
    append_vertex(entity, { 0.0f, radius + half_tip_width, half_thickness }, Vec3f::UnitZ());
    append_vertex(entity, { -tip_height, radius, half_thickness }, Vec3f::UnitZ());
    append_vertex(entity, { 0.0f, radius - half_tip_width, half_thickness }, Vec3f::UnitZ());
    append_vertex(entity, { 0.0f, inner_radius, half_thickness }, Vec3f::UnitZ());

    // top face triangles
    append_indices(entity, 0, 1, 2);
    append_indices(entity, 0, 2, 4);
    append_indices(entity, 4, 2, 3);

    // bottom face vertices
    append_vertex(entity, { 0.0f, outer_radius, -half_thickness }, -Vec3f::UnitZ());
    append_vertex(entity, { 0.0f, radius + half_tip_width, -half_thickness }, -Vec3f::UnitZ());
    append_vertex(entity, { -tip_height, radius, -half_thickness }, -Vec3f::UnitZ());
    append_vertex(entity, { 0.0f, radius - half_tip_width, -half_thickness }, -Vec3f::UnitZ());
    append_vertex(entity, { 0.0f, inner_radius, -half_thickness }, -Vec3f::UnitZ());

    // bottom face triangles
    append_indices(entity, 5, 7, 6);
    append_indices(entity, 5, 9, 7);
    append_indices(entity, 9, 8, 7);

    // side faces vertices
    append_vertex(entity, { 0.0f, outer_radius, -half_thickness }, Vec3f::UnitX());
    append_vertex(entity, { 0.0f, radius + half_tip_width, -half_thickness }, Vec3f::UnitX());
    append_vertex(entity, { 0.0f, outer_radius, half_thickness }, Vec3f::UnitX());
    append_vertex(entity, { 0.0f, radius + half_tip_width, half_thickness }, Vec3f::UnitX());

    Vec3f normal(-half_tip_width, tip_height, 0.0f);
    normal.normalize();
    append_vertex(entity, { 0.0f, radius + half_tip_width, -half_thickness }, normal);
    append_vertex(entity, { -tip_height, radius, -half_thickness }, normal);
    append_vertex(entity, { 0.0f, radius + half_tip_width, half_thickness }, normal);
    append_vertex(entity, { -tip_height, radius, half_thickness }, normal);

    normal = Vec3f(-half_tip_width, -tip_height, 0.0f);
    normal.normalize();
    append_vertex(entity, { -tip_height, radius, -half_thickness }, normal);
    append_vertex(entity, { 0.0f, radius - half_tip_width, -half_thickness }, normal);
    append_vertex(entity, { -tip_height, radius, half_thickness }, normal);
    append_vertex(entity, { 0.0f, radius - half_tip_width, half_thickness }, normal);

    append_vertex(entity, { 0.0f, radius - half_tip_width, -half_thickness }, Vec3f::UnitX());
    append_vertex(entity, { 0.0f, inner_radius, -half_thickness }, Vec3f::UnitX());
    append_vertex(entity, { 0.0f, radius - half_tip_width, half_thickness }, Vec3f::UnitX());
    append_vertex(entity, { 0.0f, inner_radius, half_thickness }, Vec3f::UnitX());

    // side face triangles
    for (int i = 0; i < 4; ++i) {
        const int ii = i * 4;
        append_indices(entity, 10 + ii, 11 + ii, 13 + ii);
        append_indices(entity, 10 + ii, 13 + ii, 12 + ii);
    }

    // stem
    // top face vertices
    for (int i = 0; i <= resolution; ++i) {
        const float angle = static_cast<float>(i) * step_angle;
        append_vertex(entity, { inner_radius * ::sin(angle), inner_radius * ::cos(angle), half_thickness }, Vec3f::UnitZ());
    }

    for (int i = 0; i <= resolution; ++i) {
        const float angle = static_cast<float>(i) * step_angle;
        append_vertex(entity, { outer_radius * ::sin(angle), outer_radius * ::cos(angle), half_thickness }, Vec3f::UnitZ());
    }

    // top face triangles
    for (int i = 0; i < resolution; ++i) {
        append_indices(entity, 26 + i, 27 + i, 27 + resolution + i);
        append_indices(entity, 27 + i, 28 + resolution + i, 27 + resolution + i);
    }

    // bottom face vertices
    for (int i = 0; i <= resolution; ++i) {
        const float angle = static_cast<float>(i) * step_angle;
        append_vertex(entity, { inner_radius * ::sin(angle), inner_radius * ::cos(angle), -half_thickness }, -Vec3f::UnitZ());
    }

    for (int i = 0; i <= resolution; ++i) {
        const float angle = static_cast<float>(i) * step_angle;
        append_vertex(entity, { outer_radius * ::sin(angle), outer_radius * ::cos(angle), -half_thickness }, -Vec3f::UnitZ());
    }

    // bottom face triangles
    for (int i = 0; i < resolution; ++i) {
        append_indices(entity, 28 + 2 * resolution + i, 29 + 3 * resolution + i, 29 + 2 * resolution + i);
        append_indices(entity, 29 + 2 * resolution + i, 29 + 3 * resolution + i, 30 + 3 * resolution + i);
    }

    // side faces vertices and triangles
    for (int i = 0; i <= resolution; ++i) {
        const float angle = static_cast<float>(i) * step_angle;
        const float c = ::cos(angle);
        const float s = ::sin(angle);
        append_vertex(entity, { inner_radius * s, inner_radius * c, -half_thickness }, { -s, -c, 0.0f });
    }

    for (int i = 0; i <= resolution; ++i) {
        const float angle = static_cast<float>(i) * step_angle;
        const float c = ::cos(angle);
        const float s = ::sin(angle);
        append_vertex(entity, { inner_radius * s, inner_radius * c, half_thickness }, { -s, -c, 0.0f });
    }

    int first_id = 26 + 4 * (resolution + 1);
    for (int i = 0; i < resolution; ++i) {
        const int ii = first_id + i;
        append_indices(entity, ii, ii + 1, ii + resolution + 2);
        append_indices(entity, ii, ii + resolution + 2, ii + resolution + 1);
    }

    append_vertex(entity, { inner_radius, 0.0f, -half_thickness }, -Vec3f::UnitY());
    append_vertex(entity, { outer_radius, 0.0f, -half_thickness }, -Vec3f::UnitY());
    append_vertex(entity, { inner_radius, 0.0f, half_thickness }, -Vec3f::UnitY());
    append_vertex(entity, { outer_radius, 0.0f, half_thickness }, -Vec3f::UnitY());

    first_id = 26 + 6 * (resolution + 1);
    append_indices(entity, first_id, first_id + 1, first_id + 3);
    append_indices(entity, first_id, first_id + 3, first_id + 2);

    for (int i = resolution; i >= 0; --i) {
        const float angle = static_cast<float>(i) * step_angle;
        const float c = ::cos(angle);
        const float s = ::sin(angle);
        append_vertex(entity, { outer_radius * s, outer_radius * c, -half_thickness }, { s, c, 0.0f });
    }

    for (int i = resolution; i >= 0; --i) {
        const float angle = static_cast<float>(i) * step_angle;
        const float c = ::cos(angle);
        const float s = ::sin(angle);
        append_vertex(entity, { outer_radius * s, outer_radius * c, +half_thickness }, { s, c, 0.0f });
    }

    first_id = 30 + 6 * (resolution + 1);
    for (int i = 0; i < resolution; ++i) {
        const int ii = first_id + i;
        append_indices(entity, ii, ii + 1, ii + resolution + 2);
        append_indices(entity, ii, ii + resolution + 2, ii + resolution + 1);
    }

    data.entities.emplace_back(entity);
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

    return data;
}

GLModel::Geometry straight_arrow(float tip_width, float tip_height, float stem_width, float stem_height, float thickness)
{
#if !ENABLE_LEGACY_OPENGL_REMOVAL
    auto append_vertex = [](GLModel::Geometry::Entity& entity, const Vec3f& position, const Vec3f& normal) {
        entity.positions.emplace_back(position);
        entity.normals.emplace_back(normal);
    };
    auto append_indices = [](GLModel::Geometry::Entity& entity, unsigned int v1, unsigned int v2, unsigned int v3) {
        entity.indices.emplace_back(v1);
        entity.indices.emplace_back(v2);
        entity.indices.emplace_back(v3);
    };
#endif // !ENABLE_LEGACY_OPENGL_REMOVAL

    GLModel::Geometry data;
#if ENABLE_LEGACY_OPENGL_REMOVAL
    data.format = { GLModel::Geometry::EPrimitiveType::Triangles, GLModel::Geometry::EVertexLayout::P3N3 };
    data.reserve_vertices(42);
    data.reserve_indices(72);
#else
    GLModel::Geometry::Entity entity;
    entity.type = GLModel::EPrimitiveType::Triangles;
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

    const float half_thickness = 0.5f * thickness;
    const float half_stem_width = 0.5f * stem_width;
    const float half_tip_width = 0.5f * tip_width;
    const float total_height = tip_height + stem_height;

#if ENABLE_LEGACY_OPENGL_REMOVAL
    // top face vertices
    data.add_vertex(Vec3f(half_stem_width, 0.0f, half_thickness), (Vec3f)Vec3f::UnitZ());
    data.add_vertex(Vec3f(half_stem_width, stem_height, half_thickness), (Vec3f)Vec3f::UnitZ());
    data.add_vertex(Vec3f(half_tip_width, stem_height, half_thickness), (Vec3f)Vec3f::UnitZ());
    data.add_vertex(Vec3f(0.0f, total_height, half_thickness), (Vec3f)Vec3f::UnitZ());
    data.add_vertex(Vec3f(-half_tip_width, stem_height, half_thickness), (Vec3f)Vec3f::UnitZ());
    data.add_vertex(Vec3f(-half_stem_width, stem_height, half_thickness), (Vec3f)Vec3f::UnitZ());
    data.add_vertex(Vec3f(-half_stem_width, 0.0f, half_thickness), (Vec3f)Vec3f::UnitZ());

    // top face triangles
    data.add_triangle(0, 1, 6);
    data.add_triangle(6, 1, 5);
    data.add_triangle(4, 5, 3);
    data.add_triangle(5, 1, 3);
    data.add_triangle(1, 2, 3);

    // bottom face vertices
    data.add_vertex(Vec3f(half_stem_width, 0.0f, -half_thickness), (Vec3f)(-Vec3f::UnitZ()));
    data.add_vertex(Vec3f(half_stem_width, stem_height, -half_thickness), (Vec3f)(-Vec3f::UnitZ()));
    data.add_vertex(Vec3f(half_tip_width, stem_height, -half_thickness), (Vec3f)(-Vec3f::UnitZ()));
    data.add_vertex(Vec3f(0.0f, total_height, -half_thickness), (Vec3f)(-Vec3f::UnitZ()));
    data.add_vertex(Vec3f(-half_tip_width, stem_height, -half_thickness), (Vec3f)(-Vec3f::UnitZ()));
    data.add_vertex(Vec3f(-half_stem_width, stem_height, -half_thickness), (Vec3f)(-Vec3f::UnitZ()));
    data.add_vertex(Vec3f(-half_stem_width, 0.0f, -half_thickness), (Vec3f)(-Vec3f::UnitZ()));

    // bottom face triangles
    data.add_triangle(7, 13, 8);
    data.add_triangle(13, 12, 8);
    data.add_triangle(12, 11, 10);
    data.add_triangle(8, 12, 10);
    data.add_triangle(9, 8, 10);

    // side faces vertices
    data.add_vertex(Vec3f(half_stem_width, 0.0f, -half_thickness), (Vec3f)Vec3f::UnitX());
    data.add_vertex(Vec3f(half_stem_width, stem_height, -half_thickness), (Vec3f)Vec3f::UnitX());
    data.add_vertex(Vec3f(half_stem_width, 0.0f, half_thickness), (Vec3f)Vec3f::UnitX());
    data.add_vertex(Vec3f(half_stem_width, stem_height, half_thickness), (Vec3f)Vec3f::UnitX());

    data.add_vertex(Vec3f(half_stem_width, stem_height, -half_thickness), (Vec3f)(-Vec3f::UnitY()));
    data.add_vertex(Vec3f(half_tip_width, stem_height, -half_thickness), (Vec3f)(-Vec3f::UnitY()));
    data.add_vertex(Vec3f(half_stem_width, stem_height, half_thickness), (Vec3f)(-Vec3f::UnitY()));
    data.add_vertex(Vec3f(half_tip_width, stem_height, half_thickness), (Vec3f)(-Vec3f::UnitY()));

    Vec3f normal(tip_height, half_tip_width, 0.0f);
    normal.normalize();
    data.add_vertex(Vec3f(half_tip_width, stem_height, -half_thickness), normal);
    data.add_vertex(Vec3f(0.0f, total_height, -half_thickness), normal);
    data.add_vertex(Vec3f(half_tip_width, stem_height, half_thickness), normal);
    data.add_vertex(Vec3f(0.0f, total_height, half_thickness), normal);

    normal = { -tip_height, half_tip_width, 0.0f };
    normal.normalize();
    data.add_vertex(Vec3f(0.0f, total_height, -half_thickness), normal);
    data.add_vertex(Vec3f(-half_tip_width, stem_height, -half_thickness), normal);
    data.add_vertex(Vec3f(0.0f, total_height, half_thickness), normal);
    data.add_vertex(Vec3f(-half_tip_width, stem_height, half_thickness), normal);

    data.add_vertex(Vec3f(-half_tip_width, stem_height, -half_thickness), (Vec3f)(-Vec3f::UnitY()));
    data.add_vertex(Vec3f(-half_stem_width, stem_height, -half_thickness), (Vec3f)(-Vec3f::UnitY()));
    data.add_vertex(Vec3f(-half_tip_width, stem_height, half_thickness), (Vec3f)(-Vec3f::UnitY()));
    data.add_vertex(Vec3f(-half_stem_width, stem_height, half_thickness), (Vec3f)(-Vec3f::UnitY()));

    data.add_vertex(Vec3f(-half_stem_width, stem_height, -half_thickness), (Vec3f)(-Vec3f::UnitX()));
    data.add_vertex(Vec3f(-half_stem_width, 0.0f, -half_thickness), (Vec3f)(-Vec3f::UnitX()));
    data.add_vertex(Vec3f(-half_stem_width, stem_height, half_thickness), (Vec3f)(-Vec3f::UnitX()));
    data.add_vertex(Vec3f(-half_stem_width, 0.0f, half_thickness), (Vec3f)(-Vec3f::UnitX()));

    data.add_vertex(Vec3f(-half_stem_width, 0.0f, -half_thickness), (Vec3f)(-Vec3f::UnitY()));
    data.add_vertex(Vec3f(half_stem_width, 0.0f, -half_thickness), (Vec3f)(-Vec3f::UnitY()));
    data.add_vertex(Vec3f(-half_stem_width, 0.0f, half_thickness), (Vec3f)(-Vec3f::UnitY()));
    data.add_vertex(Vec3f(half_stem_width, 0.0f, half_thickness), (Vec3f)(-Vec3f::UnitY()));

    // side face triangles
    for (unsigned int i = 0; i < 7; ++i) {
        const unsigned int ii = i * 4;
        data.add_triangle(14 + ii, 15 + ii, 17 + ii);
        data.add_triangle(14 + ii, 17 + ii, 16 + ii);
    }
#else
    // top face vertices
    append_vertex(entity, { half_stem_width, 0.0, half_thickness }, Vec3f::UnitZ());
    append_vertex(entity, { half_stem_width, stem_height, half_thickness }, Vec3f::UnitZ());
    append_vertex(entity, { half_tip_width, stem_height, half_thickness }, Vec3f::UnitZ());
    append_vertex(entity, { 0.0, total_height, half_thickness }, Vec3f::UnitZ());
    append_vertex(entity, { -half_tip_width, stem_height, half_thickness }, Vec3f::UnitZ());
    append_vertex(entity, { -half_stem_width, stem_height, half_thickness }, Vec3f::UnitZ());
    append_vertex(entity, { -half_stem_width, 0.0, half_thickness }, Vec3f::UnitZ());

    // top face triangles
    append_indices(entity, 0, 1, 6);
    append_indices(entity, 6, 1, 5);
    append_indices(entity, 4, 5, 3);
    append_indices(entity, 5, 1, 3);
    append_indices(entity, 1, 2, 3);

    // bottom face vertices
    append_vertex(entity, { half_stem_width, 0.0, -half_thickness }, -Vec3f::UnitZ());
    append_vertex(entity, { half_stem_width, stem_height, -half_thickness }, -Vec3f::UnitZ());
    append_vertex(entity, { half_tip_width, stem_height, -half_thickness }, -Vec3f::UnitZ());
    append_vertex(entity, { 0.0, total_height, -half_thickness }, -Vec3f::UnitZ());
    append_vertex(entity, { -half_tip_width, stem_height, -half_thickness }, -Vec3f::UnitZ());
    append_vertex(entity, { -half_stem_width, stem_height, -half_thickness }, -Vec3f::UnitZ());
    append_vertex(entity, { -half_stem_width, 0.0, -half_thickness }, -Vec3f::UnitZ());

    // bottom face triangles
    append_indices(entity, 7, 13, 8);
    append_indices(entity, 13, 12, 8);
    append_indices(entity, 12, 11, 10);
    append_indices(entity, 8, 12, 10);
    append_indices(entity, 9, 8, 10);

    // side faces vertices
    append_vertex(entity, { half_stem_width, 0.0, -half_thickness }, Vec3f::UnitX());
    append_vertex(entity, { half_stem_width, stem_height, -half_thickness }, Vec3f::UnitX());
    append_vertex(entity, { half_stem_width, 0.0, half_thickness }, Vec3f::UnitX());
    append_vertex(entity, { half_stem_width, stem_height, half_thickness }, Vec3f::UnitX());

    append_vertex(entity, { half_stem_width, stem_height, -half_thickness }, -Vec3f::UnitY());
    append_vertex(entity, { half_tip_width, stem_height, -half_thickness }, -Vec3f::UnitY());
    append_vertex(entity, { half_stem_width, stem_height, half_thickness }, -Vec3f::UnitY());
    append_vertex(entity, { half_tip_width, stem_height, half_thickness }, -Vec3f::UnitY());

    Vec3f normal(tip_height, half_tip_width, 0.0f);
    normal.normalize();
    append_vertex(entity, { half_tip_width, stem_height, -half_thickness }, normal);
    append_vertex(entity, { 0.0, total_height, -half_thickness }, normal);
    append_vertex(entity, { half_tip_width, stem_height, half_thickness }, normal);
    append_vertex(entity, { 0.0, total_height, half_thickness }, normal);

    normal = Vec3f(-tip_height, half_tip_width, 0.0f);
    normal.normalize();
    append_vertex(entity, { 0.0, total_height, -half_thickness }, normal);
    append_vertex(entity, { -half_tip_width, stem_height, -half_thickness }, normal);
    append_vertex(entity, { 0.0, total_height, half_thickness }, normal);
    append_vertex(entity, { -half_tip_width, stem_height, half_thickness }, normal);

    append_vertex(entity, { -half_tip_width, stem_height, -half_thickness }, -Vec3f::UnitY());
    append_vertex(entity, { -half_stem_width, stem_height, -half_thickness }, -Vec3f::UnitY());
    append_vertex(entity, { -half_tip_width, stem_height, half_thickness }, -Vec3f::UnitY());
    append_vertex(entity, { -half_stem_width, stem_height, half_thickness }, -Vec3f::UnitY());

    append_vertex(entity, { -half_stem_width, stem_height, -half_thickness }, -Vec3f::UnitX());
    append_vertex(entity, { -half_stem_width, 0.0, -half_thickness }, -Vec3f::UnitX());
    append_vertex(entity, { -half_stem_width, stem_height, half_thickness }, -Vec3f::UnitX());
    append_vertex(entity, { -half_stem_width, 0.0, half_thickness }, -Vec3f::UnitX());

    append_vertex(entity, { -half_stem_width, 0.0, -half_thickness }, -Vec3f::UnitY());
    append_vertex(entity, { half_stem_width, 0.0, -half_thickness }, -Vec3f::UnitY());
    append_vertex(entity, { -half_stem_width, 0.0, half_thickness }, -Vec3f::UnitY());
    append_vertex(entity, { half_stem_width, 0.0, half_thickness }, -Vec3f::UnitY());

    // side face triangles
    for (int i = 0; i < 7; ++i) {
        const int ii = i * 4;
        append_indices(entity, 14 + ii, 15 + ii, 17 + ii);
        append_indices(entity, 14 + ii, 17 + ii, 16 + ii);
    }

    data.entities.emplace_back(entity);
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

    return data;
}

GLModel::Geometry diamond(unsigned int resolution)
{
    resolution = std::max<unsigned int>(4, resolution);

    GLModel::Geometry data;
#if ENABLE_LEGACY_OPENGL_REMOVAL
    data.format = { GLModel::Geometry::EPrimitiveType::Triangles, GLModel::Geometry::EVertexLayout::P3N3 };
    data.reserve_vertices(resolution + 2);
    data.reserve_indices((2 * (resolution + 1)) * 3);
#else
    GLModel::Geometry::Entity entity;
    entity.type = GLModel::EPrimitiveType::Triangles;
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

    const float step = 2.0f * float(PI) / float(resolution);

#if ENABLE_LEGACY_OPENGL_REMOVAL
    // vertices
    for (unsigned int i = 0; i < resolution; ++i) {
        const float ii = float(i) * step;
        const Vec3f p = { 0.5f * ::cos(ii), 0.5f * ::sin(ii), 0.0f };
        data.add_vertex(p, (Vec3f)p.normalized());
    }
    Vec3f p = { 0.0f, 0.0f, 0.5f };
    data.add_vertex(p, (Vec3f)p.normalized());
    p = { 0.0f, 0.0f, -0.5f };
    data.add_vertex(p, (Vec3f)p.normalized());

    // triangles
    // top
    for (unsigned int i = 0; i < resolution; ++i) {
        data.add_triangle(i + 0, i + 1, resolution);
    }
    data.add_triangle(resolution - 1, 0, resolution);

    // bottom
    for (unsigned int i = 0; i < resolution; ++i) {
        data.add_triangle(i + 0, resolution + 1, i + 1);
    }
    data.add_triangle(resolution - 1, resolution + 1, 0);
#else
    // positions
    for (int i = 0; i < resolution; ++i) {
        float ii = float(i) * step;
        entity.positions.emplace_back(0.5f * ::cos(ii), 0.5f * ::sin(ii), 0.0f);
    }
    entity.positions.emplace_back(0.0f, 0.0f, 0.5f);
    entity.positions.emplace_back(0.0f, 0.0f, -0.5f);

    // normals
    for (const Vec3f& v : entity.positions) {
        entity.normals.emplace_back(v.normalized());
    }

    // triangles
    // top
    for (int i = 0; i < resolution; ++i) {
        entity.indices.push_back(i + 0);
        entity.indices.push_back(i + 1);
        entity.indices.push_back(resolution);
    }
    entity.indices.push_back(resolution - 1);
    entity.indices.push_back(0);
    entity.indices.push_back(resolution);

    // bottom
    for (int i = 0; i < resolution; ++i) {
        entity.indices.push_back(i + 0);
        entity.indices.push_back(resolution + 1);
        entity.indices.push_back(i + 1);
    }
    entity.indices.push_back(resolution - 1);
    entity.indices.push_back(resolution + 1);
    entity.indices.push_back(0);

    data.entities.emplace_back(entity);
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

    return data;
}

#if ENABLE_LEGACY_OPENGL_REMOVAL
#if ENABLE_SHOW_TOOLPATHS_COG
GLModel::Geometry smooth_sphere(unsigned int resolution, float radius)
{
    resolution = std::max<unsigned int>(4, resolution);

    const unsigned int sectorCount = resolution;
    const unsigned int stackCount  = resolution;

    const float sectorStep = float(2.0 * M_PI / sectorCount);
    const float stackStep = float(M_PI / stackCount);

    GLModel::Geometry data;
    data.format = { GLModel::Geometry::EPrimitiveType::Triangles, GLModel::Geometry::EVertexLayout::P3N3 };
    data.reserve_vertices((stackCount - 1) * sectorCount + 2);
    data.reserve_indices((2 * (stackCount - 1) * sectorCount) * 3);

    // vertices
    for (unsigned int i = 0; i <= stackCount; ++i) {
        // from pi/2 to -pi/2
        const double stackAngle = 0.5 * M_PI - stackStep * i;
        const double xy = double(radius) * ::cos(stackAngle);
        const double z = double(radius) * ::sin(stackAngle);
        if (i == 0 || i == stackCount) {
            const Vec3f v(float(xy), 0.0f, float(z));
            data.add_vertex(v, (Vec3f)v.normalized());
        }
        else {
            for (unsigned int j = 0; j < sectorCount; ++j) {
                // from 0 to 2pi
                const double sectorAngle = sectorStep * j;
                const Vec3f v(float(xy * std::cos(sectorAngle)), float(xy * std::sin(sectorAngle)), float(z));
                data.add_vertex(v, (Vec3f)v.normalized());
            }
        }
    }

    // triangles
    for (unsigned int i = 0; i < stackCount; ++i) {
        // Beginning of current stack.
        unsigned int k1 = (i == 0) ? 0 : (1 + (i - 1) * sectorCount);
        const unsigned int k1_first = k1;
        // Beginning of next stack.
        unsigned int k2 = (i == 0) ? 1 : (k1 + sectorCount);
        const unsigned int k2_first = k2;
        for (unsigned int j = 0; j < sectorCount; ++j) {
            // 2 triangles per sector excluding first and last stacks
            unsigned int k1_next = k1;
            unsigned int k2_next = k2;
            if (i != 0) {
                k1_next = (j + 1 == sectorCount) ? k1_first : (k1 + 1);
                data.add_triangle(k1, k2, k1_next);
            }
            if (i + 1 != stackCount) {
                k2_next = (j + 1 == sectorCount) ? k2_first : (k2 + 1);
                data.add_triangle(k1_next, k2, k2_next);
            }
            k1 = k1_next;
            k2 = k2_next;
        }
    }

    return data;
}
#endif // ENABLE_SHOW_TOOLPATHS_COG
#endif // ENABLE_LEGACY_OPENGL_REMOVAL

} // namespace GUI
} // namespace Slic3r

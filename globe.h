#pragma once
// Created: 2021-04-06 23:00

#include <iostream>
#include <iomanip>
#include <fstream>

#include <cmath>
#include <numbers>
#include <limits>
#include <cstdint>
#include <utility>
#include <memory>

#include <map>
#include <array>

#ifndef GLM_ENABLE_EXPERIMENTAL
#    define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtx/norm.hpp>
// #include <glm/gtx/polar_coordinates.hpp>

#include "memmap.h"
#include "mikey_tools.h"

namespace Globe
{

constexpr float pi   = std::numbers::pi_v<float>;
constexpr auto  pi_2 = pi / 2;
constexpr auto  pi_4 = pi / 4;
constexpr auto  pi2  = pi * 2;
//==============
// clang-format off
template <class T, size_t N = 5>
std::enable_if_t<not std::numeric_limits<T>::is_integer, T>
constexpr Tiny = std::numeric_limits<T>::epsilon() * N;

template <class T, size_t N = 5,
    std::enable_if_t<not std::numeric_limits<T>::is_integer, bool> = true>
constexpr bool less_than( T lhs, T rhs )
{   return Tiny<T, N> < ( rhs - lhs );
}
template <class T>
constexpr bool lt(T lhs, T rhs)
{   return less_than<T, 3>(lhs, rhs);
}
template <>
constexpr bool lt<glm::vec3>(glm::vec3 lhs, glm::vec3 rhs)
{
    if ( lt( lhs.x, rhs.x ) )
        return true;
    if ( lt( rhs.x, lhs.x ) )
        return false;

    if ( lt( lhs.y, rhs.y ) )
        return true;
    if ( lt( rhs.y, lhs.y ) )
        return false;

    return lt( lhs.z, rhs.z );
}
template <class T>
constexpr bool eq(T lhs, T rhs)
{   return !lt(lhs, rhs) && !lt(rhs, lhs);
}
template <typename T, T low, T high>
struct wrap_range
{
    const T value_range = high - low;
    constexpr T operator()( T value ) const
    {
        return value;

        if ( lt(value, low) )
            return high - ( low - value );  // % value_range;
        if ( lt(high, value ) )
            return low + ( value - high );  // % value_range;
        return value;
    }
};

template <class T>
class Slice
{
private:
    T& list;
    size_t first, last;
public:
    Slice(T& alist, size_t first, size_t last = 0)
    : list(alist), first(first), last(last ? last : alist.size())
    {}

public:
    auto begin() const
    {   return list.begin() + (first < last ? first : last);
    }
    auto end() const
    {   return last < list.size() ? list.begin() + last : list.end();
    }
    size_t size() const
    {   return last - first;
    }
    auto operator[](size_t idx) const
    {   return *list[first + idx];
    }
    void* data() const
    {   return (void*)(reinterpret_cast<const T::value_type*>(list.data()) + first);
    }
};
template <class T>
auto slice(T& alist, size_t first, size_t last = 0)
{   return Slice<T>(alist, first, last);
}
template <class T, class U, class V>
auto slice(T& alist, std::pair<U,V> bounds)
{   return Slice<T>(alist, bounds.first, bounds.second);
}

// no!!! clang-format on

struct SphericalCoord
{
    glm::vec2 uv = glm::vec2(0.0f);
    glm::vec3 pos = glm::vec3(0.0f);
    float elev = 1.0f;

private:
    constexpr float wrap_lon(float lon) const
    {
        return lon;
        return lon < 0 ? (lon + pi2) : lon;
    }

public:
    //==========
    SphericalCoord(float lat, float lon, float r = 1.0f)
        : uv({lat, wrap_lon(lon)}), pos(euclidean(uv)), elev(r)
    {
    }

    SphericalCoord(const glm::vec3 &p, float r = 1.0f) //
        :  uv(polar(p)), pos(normalize(p)), elev(r)
    {
    }

    SphericalCoord() = default;
    ~SphericalCoord() = default;
    SphericalCoord(const SphericalCoord &other) = default;
    SphericalCoord(SphericalCoord &&other) = default;
    SphericalCoord &operator=(const SphericalCoord &other) = default;
    SphericalCoord &operator=(SphericalCoord &&other) = default;

    //==========
    // 'r' is excluded in these operators
    constexpr bool operator<(const glm::vec3 &rhs) const
    {
        return lt(pos, rhs);
    }

    constexpr bool operator<(const SphericalCoord &rhs) const
    {
        return lt(pos, rhs.pos);
    }

    glm::vec3 operator+(const SphericalCoord &rhs) const
    {
        return pos + rhs.pos;
    }

    glm::vec3 operator-(const SphericalCoord &rhs) const
    {
        return pos - rhs.pos;
    }

    //==========
};

std::ostream &operator<<(std::ostream &os, const glm::vec3 &v)
{
    os << "[" << v.x << ", " << v.y << ", " << v.z << "]";
    return os;
}

std::ostream &operator<<(std::ostream &os, const SphericalCoord &sc)
{
    os << "[" << sc.uv << ", " << sc.pos << "]";
    return os;
}

using index_type = uint32_t;
using Triangle = glm::u32vec3;
using TriangleList = mhy::ListT<Triangle>;

template <class T>
void print(const Slice<T> &triangles, bool details = false)
{
    std::cout << "Triangles: [" << triangles.size() << "]\n";
    if (details && triangles.size() < 100)
    {
        for (auto &t : triangles)
        {
            std::cout << "        " << t << std::endl;
        }
    }
}

template <typename VertexT, typename KeyT = VertexT>
class VertexList
{
public:
    using vertex_type = VertexT;

private:
    std::map<VertexT, index_type> vertex_map;
    mhy::ListT<VertexT> indices;

public:
    VertexList() = default;
    ~VertexList() = default;
    VertexList(const VertexList &other) = default;
    VertexList(VertexList &&other) = default;
    VertexList &operator=(const VertexList &other) = default;
    VertexList &operator=(VertexList &&other) = default;

    //==========
    uint32_t add(const VertexT &vertex)
    {
        auto res = vertex_map.insert(std::make_pair(vertex, (uint32_t)indices.size()));
        if (res.second)
        {
            indices.push_back(vertex);
            return (uint32_t)indices.size() - 1;
        }
        return res.first->second;
    }

    // clang-format off
    Triangle add_triangle( VertexT v1, VertexT v2, VertexT v3 )
    {   return { add( v1 ), add( v2 ), add( v3 ) };
    }

    const auto & operator[]( index_type index ) const
    {   return indices[index];  // WARNING: No bounds checking
    }

    auto & get_indices() const
    {   return indices;
    }

    auto & get_upd_indices()
    {   return indices;
    }

    auto & get_vertices()
    {   return vertex_map;
    }

    // clang-format on

    auto find( const VertexT & vertex ) const
    {
        auto it = vertex_map.find( vertex );
        return std::make_pair( it != vertex_map.end(), *it );
    }

    auto & operator[]( index_type index )
    {
        return indices[index];
    }
};

class GlobeMesh
{
    std::unique_ptr<mhy::MappedBuffer>      gen_file;   // mesh wwas generated into this file
    std::unique_ptr<mhy::MemoryMappedFile>  load_file;  // pre-generated mesh was loaded from this.

    VertexList<SphericalCoord, glm::vec3> vertices;
    TriangleList                          triangles;

    struct SubdivLevel
    {
        size_t offset_begin;
        size_t offset_end;
        size_t vertex_end;

        typedef std::pair<size_t, size_t> Bounds;

        Bounds faces() const
        {
            return { offset_begin, offset_end };
        }

        Bounds vertices() const
        {
            return { 0, vertex_end };
        }
    };

    mhy::ListT<SubdivLevel> subdivs;

  public:
    GlobeMesh()  = default;
    ~GlobeMesh() = default;

    auto & get_vertices() const
    {
        return vertices.get_indices();
    }

    auto & get_upd_vertices()
    {
        return vertices.get_upd_indices();
    }

    auto subdiv_count() const
    {
        return subdivs.size();
    }

    auto get_faces( size_t sub = UINT_MAX ) const
    {
        if ( subdivs.empty() )
        {
            return slice( triangles, 0, 0 );
        }
        if ( sub >= subdivs.size() )
        {
            return slice( triangles, subdivs.back().faces() );
        }
        return slice( triangles, subdivs[sub].faces() );
    }

    void mark_subdiv()
    {
        auto first = !subdivs.empty() ? subdivs.back().offset_end : 0;
        subdivs.push_back( { first, triangles.size(), vertices.get_indices().size() } );
    }

    bool load_from_mesh( const char * fname )
    {  // stubbed for now.
        auto poo = std::make_unique<mhy::MemoryMappedFile>( fname );

        auto & fheader = *poo->cast_to<globe_fileheader>(0);    // get file header at offset 0
        if (fheader.id_word != 0x1234 ||
            fheader.version_id > 0x100)
        {
            std::cout << "File '" << fname << "' is not compatible with this version of Globe.\n";
            return false;
        }

        size_t i_offset = fheader.header_bytes + fheader.data_bytes;
        auto pchunk = poo->cast_to<globe_chunk_header>(i_offset);
        {   // load the subdivs headers
            if (pchunk->chunk_type != eChunkSubdivInfo ||
                pchunk->data_stride != sizeof(SubdivLevel))
            {
                std::cout << "Subdiv info struct size (" << sizeof(SubdivLevel) 
                    << ") does not match its data_stride " << pchunk->data_stride << std::endl;
                return false;
            }
        }
        auto r_subds = mhy::range(poo->cast_to<SubdivLevel>(i_offset + pchunk->header_bytes), pchunk->data_count);

        i_offset += pchunk->header_bytes + pchunk->data_size;
        pchunk = poo->cast_to<globe_chunk_header>(i_offset);
        {   // load the faces
            if (pchunk->chunk_type != eChunkFaces ||
                pchunk->data_stride != sizeof(Triangle))
            {
                std::cout << "Faces struct size (" << sizeof(Triangle) 
                    << ") does not match its data_stride " << pchunk->data_stride << std::endl;
                return false;
            }
        }
        auto r_faces = mhy::range(poo->cast_to<Triangle>(i_offset + pchunk->header_bytes), pchunk->data_count);

        i_offset += pchunk->header_bytes + pchunk->data_size;
        pchunk = poo->cast_to<globe_chunk_header>(i_offset);
        {   // load the faces
            if (pchunk->chunk_type != eChunkVerts ||
                pchunk->data_stride != sizeof(SphericalCoord))
            {
                std::cout << "Verts struct size (" << sizeof(SphericalCoord) 
                    << ") does not match its data_stride " << pchunk->data_stride << std::endl;
                return false;
            }
        }
        auto r_verts = mhy::range(poo->cast_to<SphericalCoord>(i_offset + pchunk->header_bytes), pchunk->data_count);

        subdivs.load_from(r_subds);
        triangles.load_from(r_faces);
        get_upd_vertices().load_from(r_verts);

        load_file.swap(poo);    // assign ownership to `this`

        return true;
    }

    //---- This is all you need to draw a globe. The remainder
    // generates the mesh and writes the data file loaded above.
    //--------------------------------------------------

    template <typename U, typename V, typename W>
    void make_globe( U r_subs, V r_faces, W r_verts )
    {
        subdivs            = r_subs;
        triangles          = r_faces;
        get_upd_vertices() = r_verts;

        // Make some triangles.

        const float n_lat = atan( 0.5f );
        const float s_lat = -n_lat;
        const float wedge = pi * 0.4f;  // 72 degrees
        float       east  = wedge * 0.5f;

        for ( int i = 0; i < 5; ++i, east += wedge )
        {
            auto east2 = east + wedge * 0.5f;
            auto west  = east - wedge;
            auto mid   = east2 - wedge;
            // clockwise vertex order
            // clang-format off
            triangles.push_back(
                vertices.add_triangle(
                    { pi_2, (east + west) * 0.5f },
                    { n_lat, east },
                    { n_lat, west } ) );
            triangles.push_back(
                vertices.add_triangle(
                    { n_lat, east },
                    { s_lat, mid },
                    { n_lat, west } ) );
            triangles.push_back(
                vertices.add_triangle(
                    { n_lat, east },
                    { s_lat, east2 },
                    { s_lat, mid } ) );
            triangles.push_back(
                vertices.add_triangle(
                    { s_lat, mid },
                    { s_lat, east2 },
                    { -pi_2, (mid + east2) * 0.5f } ) );
            // clang-format on
        }
        mark_subdiv();

        print( true );
        Globe::print( get_faces(), true );
    }

    void subdivide( int count = 1 )
    {
        if ( subdivs.empty() )
        {
            return;  // don't have anything to subdivide. make a globe first.
        }
        for ( int i = (int)subdivs.size() - 1; i < count; ++i )
        {
            auto old_triangles = slice( triangles, subdivs.back().faces() );
            for ( auto t : old_triangles )
            {
                auto & v0  = vertices[t[0]];
                auto & v1  = vertices[t[1]];
                auto & v2  = vertices[t[2]];
                auto   v01 = ( v0 + v1 ) / 2;
                auto   v12 = ( v1 + v2 ) / 2;
                auto   v20 = ( v2 + v0 ) / 2;

                // std::cout << "In: {" << v0 << v1 << v2 << "}\n";
                // std::cout << "Out: {" << v01 << v12 << v20 << "}\n";

                auto i01 = vertices.add( v01 );
                auto i12 = vertices.add( v12 );
                auto i20 = vertices.add( v20 );
                triangles.push_back( { t[0], i01, i20 } );
                triangles.push_back( { i01, t[1], i12 } );
                triangles.push_back( { i20, i12, t[2] } );
                triangles.push_back( { i01, i12, i20 } );
            }
            mark_subdiv();
            std::cout << ( i + 1 ) << ' ' << std::flush;
        }
        print( false );
        Globe::print( get_faces(), false );
    }

    void print( bool details = false ) const
    {
        auto & indices = vertices.get_indices();
        std::cout << "Vertices: [" << indices.size() << "], Faces: [" << triangles.size() << "] in "
                  << subdivs.size() << " subdivs:\n";
        for ( auto ss : subdivs )
        {
            auto s = ss.faces();
            std::cout << std::setw( 16 ) << ( s.second - s.first ) << " [" << s.first << ", " << s.second << "]\n";
        }
        if ( details && indices.size() < 100 )
        {
            for ( size_t i = 0; i < indices.size(); ++i )
            {
                const auto & v = indices[i];
                std::cout << std::setw( 8 ) << i << ": " << v << std::endl;
            }
        }
    }

    // uv contains lat, lon in radians.
    // lat is in [-pi/2, pi/2]
    // lon is in [-pi, pi]
    // normalize both to [0, 1]
    static auto map_uv( const glm::vec2 & v )
    {
        return glm::vec2( v.x / pi + 0.5f, v.y / pi2 + 0.5f );
    }

    static size_t index_of( float v, size_t max )
    {
        auto idx = v * max - 1;
        return idx > 0.0f ? static_cast<size_t>( idx ) : 0;
    }

    void map_elevations( const int16_t data[43200][86400] )
    {
        auto & verts = get_upd_vertices();
        int    i     = 0;
        for ( auto & v : verts )
        {
            auto uv   = map_uv( v.uv );
            auto lat  = index_of( uv.x, 43200 );
            auto lon  = index_of( uv.y, 86400 );
            auto elev = static_cast<float>( data[lat][lon] );
            v.elev    = elev;

            if ( ( i < 50 ) || ( 0 == i % 1000 ) )
            {
                std::cout << '.' << std::flush;
                // std::cout << std::setw( 8 ) << i << ": "
                //           << "xy" << v.uv << " uv" << uv << " index[" << lat << ", " << lon << "] -> " << elev
                //           << "\n";
            }
            ++i;
        }
    }

    bool load_from_terrain( const char * dat_name )
    {
        mhy::MemoryMappedFile terrain( dat_name );
        if ( !terrain )
        {
            std::cout << "Error opening terrain data file: " << dat_name << '\n';
            return false;
        }
        typedef int16_t lat_row[86400];
        typedef lat_row grid[43200];
        auto            data  = terrain.cast_to<lat_row>( 0200 );
        const auto      sgrid = sizeof( lat_row ) * 43200;  // sizeof(grid);
        const auto      tsize = terrain.size() - 0200;
        if ( sgrid != tsize )
        {
            std::cout << "Terrain data file mismatch. Expect " << sgrid << " bytes, got " << tsize << std::endl;
            return false;
        }
        map_elevations( data );
        return true;
    }

    bool write_elevations( const char * fname )
    {
        std::ofstream ofs( fname, std::ios::binary | std::ios::out | std::ios::trunc );
        if ( !ofs.is_open() )
        {
            std::cout << "Error writing terrain elevations file: " << fname << std::endl;
            return false;
        }
        auto &     verts = get_vertices();
        const auto vsize = verts.size();

        using ElevList = std::vector<float>;
        ElevList elevs;
        elevs.reserve( verts.size() );

        for ( auto & vert : verts )
        {
            elevs.push_back( vert.elev );
        }
        const auto esize = elevs.size();
        ofs.write( reinterpret_cast<const char *>( elevs.data() ), elevs.size() * sizeof( ElevList::value_type ) );

        ofs.close();
        std::cout << "Wrote " << verts.size() << " elevs to: " << fname << std::endl;
        return true;
    }

    bool load_elevations( const char * fname )
    {
        mhy::MemoryMappedFile elves( fname );
        if ( !elves )
        {
            std::cout << "Error reading terrain elevations file: " << fname << std::endl;
            return false;
        }

        //-- sanity check: file size and globe-mesh must
        // agree on elev data count.
        auto & verts = get_upd_vertices();
        if ( elves.size() != verts.size() * sizeof( float ) )
        {
            std::cout << "load_elevations(): file size mismatch. File size: " << elves.size()
                      << ", GlobeMesh expecting " << ( verts.size() * sizeof( float ) ) << " bytes.\n";
            return false;
        }
        // std::vector<float> elevs;
        // elevs.reserve(verts.size());

        auto elev = elves.cast_to<float>();
        for ( auto & v : verts )
        {
            v.elev = *elev++;
        }
        return true;
    }

    glm::vec3 elev_to_rgb( float elev )
    {
        using glm::vec3;
        using HSV     = vec3;
        auto lerp_rgb = []( float val, const HSV & a, const HSV b, float v1, float v2 ) -> HSV
        {
            float t = ( val - v1 ) / ( v2 - v1 );
            float h = std::lerp( a.r, b.r, t );
            float s = std::lerp( a.g, b.g, t );
            float v = std::lerp( a.b, b.b, t );

            // hsv to rgb
            if ( s == 0.0 )
            {
                const HSV gray{ 0.0f, 0.0f, v };
                return gray;
            }

            {  // scope name conflicts
                h = fmod( h, 360.0f );
                h /= 60.0f;
                int   i = (int)floor( h );
                float f = h - i;
                float p = v * ( 1 - s );
                float q = v * ( 1 - f * s );
                float t = v * ( 1 - ( 1 - f ) * s );

                switch ( i )
                {
                    case 0 : return HSV( v, t, p );  // r = v; g = t; b = p; break;
                    case 1 : return { q, v, p };     // r = q; g = v; b = p; break;
                    case 2 : return { p, v, t };     // r = p; g = v; b = t; break;
                    case 3 : return { p, q, v };     // r = p; g = q; b = v; break;
                    case 4 : return { t, p, v };     // r = t; g = p; b = v; break;
                    default: return { v, p, q };     // r = v; g = p; b = q; break;
                }
            }
        };
        // terrain elevation selects a vertex color as follows:
        //  0..500m     : hsv(117, 92%, 36%) : hsv(88, 28%, 56%) :
        // 500..1200m   :    ... hsv(182, 49%, 94%)
        //   > 1200m    :   hsv(0, 0, 99)
        // deep rich green: hsv(117, 92%, 36%)
        // high altitude faded green: hsv(88, 28%, 56%)
        // h > 500m :  hsv(182, 49%, 94%)
        //  .. 1200m: hsv(206, 13%, 95%)
        const vec3 low_land{ 117.0f, 0.92f, 0.36f };
        const vec3 plains{ 88.0f, 0.28f, 0.56f };
        const vec3 glacier{ 182.0f, 0.49f, 0.94f };
        const vec3 above{ 0.90f, 0.90f, 0.95f };  // in RGB. We return this directly.
        // 0..2000m : hsv(258, 6%, 94%) .. hsv(232, 77%, 48%)
        //  deeper  :     hsv(240, 66%, 17%)
        // beach  0m   : hsv(258, 6%, 94%)
        // deep ocean .. 2000m: hsv(232, 77%, 48%)
        const vec3 beach{ 258.0f, 0.06f, 0.72f };
        const vec3 deep( 232.0f, 0.77f, 0.22f );
        const vec3 too_deep{ 240.0f, 0.66f, 0.17f };
        // ocean:
        const float sealevel = -0;
        if ( elev < sealevel )
        {
            return lerp_rgb( elev, beach, deep, sealevel, -7000 );
        }
        // land:
        if ( elev < 2000 )
        {
            return lerp_rgb( elev, low_land, plains, sealevel, 2000 );
        }
        if ( elev < 5000 )
        {
            return lerp_rgb( elev, plains, glacier, 2000, 5000 );
        }
        return above;
    }

  public:
    struct globe_fileheader
    {
        uint16_t id_word      = 0x1234;
        uint16_t header_bytes = 16;
        uint32_t version_id   = 0x0100;
        uint32_t data_bytes   = 0;
        uint32_t padding      = 0;
    };

    struct globe_chunk_header
    {
        uint16_t chunk_type   = 0;
        uint16_t header_bytes = 16;
        uint32_t data_stride  = 0;
        uint64_t data_count   = 0;
        uint64_t data_size    = 0;
    };

  private:
    auto write_file_header( mhy::MappedBuffer & mbuf )
    {
        static const globe_fileheader header = {
            .id_word      = 0x1234,
            .header_bytes = sizeof( globe_fileheader ),
            .version_id   = 0x0100,
        };
        auto phdr = mbuf.cast_to<globe_fileheader>( 0 );
        *phdr     = header;
        return mhy::RangeT<globe_fileheader>( phdr, 1 );
    }

    enum EChunkType : uint16_t
    {
        eChunkInvalid = 0,
        eChunkSubdivInfo,
        eChunkFaces,
        eChunkVerts,
        eChunkElevs,
        //-----
        eChunkEOF = 0xffff
    };
    std::ostream & print(std::ostream & os, const globe_chunk_header & chunk)
    {
        os << "Chunk Header: \n"
            "    chunk_type   = " << chunk.chunk_type << "\n"
            "    header_bytes = " << chunk.header_bytes << "\n"
            "    data_stride  = " << chunk.data_stride << "\n"
            "    data_count   = " << chunk.data_count << "\n"
            "    data_size    = " << chunk.data_size << "\n"
            ;
        return os;
    }

    template <class T, class U>
    auto next_chunk( mhy::RangeT<U> & prev_chunk, size_t n = 1 )
    {
        return mhy::RangeT<T>( reinterpret_cast<T *>( prev_chunk.end() ), n );
    }

    template <class T, class U>
    auto allocate_data_chunk( mhy::RangeT<U> prev_chunk, EChunkType etype, size_t count )
    {
        auto hdr = next_chunk<globe_chunk_header>( prev_chunk );
        *hdr     = { .chunk_type   = etype,
                     .header_bytes = sizeof( globe_chunk_header ),
                     .data_stride  = sizeof( T ),
                     .data_count   = (uint32_t)count,
                     .data_size    = sizeof( T ) * count };
        return next_chunk<T>( hdr, count );
    }

    template <class U>
    auto write_eof_chunk( mhy::RangeT<U> prev_chunk )
    {
        return allocate_data_chunk<char>( prev_chunk, eChunkEOF, 0 );
    }

    void update_vertex_counts( mhy::MappedBuffer & mbuf )
    {
        auto const verts_count = get_vertices().size();
        auto const faces_count = triangles.size();
        auto const subds_count = subdiv_count();

        //-- get file header
        auto   phdr    = mbuf.cast_to<globe_fileheader>( 0 );
        auto   chunk   = mbuf.cast_to<globe_chunk_header>();
        size_t iOffset = phdr->header_bytes;

        auto get_next_chunk = [&mbuf]( size_t offset )  //
        {                                               //
            return mbuf.cast_to<globe_chunk_header>( offset );
        };

        chunk              = get_next_chunk( iOffset );
        print(std::cout << "Subdivs: ", *chunk);
        auto eType         = chunk->chunk_type;
        bool bValidHeaders = true;
        if ( eChunkSubdivInfo != eType )
        {
            std::cout << "First chunk header is of unexpected type [" << eType
                      << "]. Expecing [1] eChunkSubdivInfo.\n";
            bValidHeaders = false;
        }
        else
        {
            std::cout << "Subdivs count was " << chunk->data_count << ". " "Expecting " << subds_count << std::endl;
            chunk->data_count = subds_count;
        }
        iOffset += chunk->header_bytes + chunk->data_size;
        chunk = get_next_chunk( iOffset );
        print(std::cout << "Faces: ", *chunk);
        eType = chunk->chunk_type;
        if ( eChunkFaces != eType )
        {
            std::cout << "Second chunk header is of unexpected type [" << eType << "]. Expecing [2] eChunkFaces.\n";
            bValidHeaders = false;
        }
        else
        {
            std::cout << "Faces count was " << chunk->data_count << ". " "Expecting " << faces_count << std::endl;
            chunk->data_count = faces_count;
        }
        iOffset += chunk->header_bytes + chunk->data_size;
        chunk = get_next_chunk( iOffset );
        print(std::cout << "Verts: ", *chunk);
        eType = chunk->chunk_type;
        if ( eChunkVerts != eType )
        {
            std::cout << "Third chunk header is of unexpected type [" << eType << "]. Expecing [3] eChunkVerts.\n";
            bValidHeaders = false;
        }
        else
        {
            auto actual_size = chunk->data_count * chunk->data_stride;
            std::cout << "Verts count was " << chunk->data_count << ". " "Expecting " << verts_count
                      << "\n" "   Data stride is " << chunk->data_stride << ". " " Expecting " << sizeof( SphericalCoord )
                      << ".\n" "   Actual verts data size is " << actual_size << " bytes, " " allocated "
                      << chunk->data_size << ".\n";

            chunk->data_count = verts_count;
            bValidHeaders     = !( actual_size < chunk->data_size );
        }

        if ( !bValidHeaders )
        {
            std::cout << "***** Chunk Headers are invalid. Not updating EOF chunk.\n";
            return;
        }
        //-- Fix up verts data size. A mismatch is expected. We allocated extra
        // because vertex count calcs are subject to inexact vertex merging.
        chunk->data_size = chunk->data_count * chunk->data_stride;
        iOffset += chunk->header_bytes + chunk->data_size;

        auto eofChunk = mbuf.cast_to<globe_chunk_header>( iOffset );
        *eofChunk     = {
                .chunk_type   = eChunkEOF,
                .header_bytes = sizeof( globe_chunk_header ),
                .data_stride  = 0,
                .data_count   = 0,
                .data_size    = 0,
        };
        iOffset += eofChunk->header_bytes;
        std::cout << "++++ You may safely truncate this data file to " << iOffset << ".\n";
    }

  public:
    bool generate( const char * fname, const char * fterrain, unsigned nsubdivs )
    {
        std::cout << "Generating Globe with " << nsubdivs << " subdivisions to file " << fname << ".\n";

        //-- calc the space needed for faces.
        // Level 0: 20 faces. Each subdiv splits each into 4.
        // Thus, each subdiv level has 4x more faces.
        size_t nfaces = 20;  // subd 0 has 20 faces.
        size_t prev   = 20;
        for ( size_t i : std::ranges::iota_view{ 0u, nsubdivs } )
        {
            auto sub = prev * 4;
            nfaces += sub;
            prev = sub;

            std::cout << std::setw( 8 ) << ( i + 1 ) << ": +" << sub << " = " << nfaces << std::endl;
        }
        //-- There are exactly half as many vertices as there are faces.
        // (Each face has 3 vertices. Each vertex is shared across 6 faces.
        // Thus, 3 / 6 = 1/2: half as many vertices as there are faces.)
        // Add a few extra for the ones that didn't quite merge correctly.
        //   Allocate for vertices based on the highest subdiv face count,
        // not the total _cumulative_  counts. Add 0.1% and a few more
        // for those that didn't merge right.
        const size_t nverts = prev * 501 / 1000 + 10;

        //-- Allocate space for the file header and 3 chunk headers, for
        // each of the subdivs summary, faces, and vertices chunks.
        // clang_format off
        const size_t flen = sizeof( globe_fileheader ) + sizeof( globe_chunk_header ) * 3 +
                            sizeof( SubdivLevel ) * nsubdivs + sizeof( Triangle ) * nfaces +
                            sizeof( SphericalCoord ) * nverts;
        // clang_format on

        std::cout << "Allocating " << flen << " bytes for " << nfaces << " faces and " << nverts << " vertices.\n";

        //--
        auto poo = std::make_unique<mhy::MappedBuffer>( fname, flen );
        gen_file.swap(poo);
        auto & mbuf = *gen_file;
        if ( !mbuf )
        {
            std::cout << "Error creating globe data file '" << fname << "'.\n";
            return false;
        }
        //--
        try
        {
            auto pHeader  = write_file_header( mbuf );
            auto pSubdivs = allocate_data_chunk<SubdivLevel>( pHeader, eChunkSubdivInfo, nsubdivs + 1 );
            auto pFaces   = allocate_data_chunk<Triangle>( pSubdivs, eChunkFaces, nfaces );
            auto pVerts   = allocate_data_chunk<SphericalCoord>( pFaces, eChunkVerts, nverts );

            make_globe( pSubdivs, pFaces, pVerts );
            subdivide( nsubdivs );
            //-- all done generating. Update counts in
            // the file chunk headers.
            update_vertex_counts( mbuf );

            load_from_terrain( fterrain );
        }
        catch ( const std::exception & ex )
        {
            std::cout << "EXCEPTION: " << ex.what();
        }

        return true;
    }

};

}  // namespace Globe

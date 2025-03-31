# Icosphere geometry

We will model the globe as a subdivided icosphere. Briefly, D&D 20-sided dice are icospheres. 5 faces at each pole form a pentagonal prism (without its base). Each face is an equilateral triangle. A faces "grows" from the base of each of these 5 faces, spanning the equator and interleaving with an identical construct from the other pole.

## icosphere.py

> **Note**: We will not be using icosphere.py at all. Our interest here is to document our early exploration into programmatically generating the geometry.

[Also see this C# implementation](http://blog.andreaskahler.com/2009/06/creating-icosphere-mesh-in-code.html) for discussion, referenced in MatLab's `icosphere.m`.

[Generates and subdivides an icosphere.](https://github.com/vedranaa/icosphere`)

![alt text](https://github.com/vedranaa/icosphere/raw/main/Figure.png)

```python
>>> from icosphere.icosphere import icosphere
>>> verts,faces = icosphere(6)
>>> len(faces)
720
>>> len(verts)
362
>>> with open("outfile", "w") as outfile:
...     outfile.write("\n".join(str(item) for item in verts))

>>> with open("faces", "w") as outfile:
...     outfile.write("\n".join(str(item) for item in faces))

>>> with open("faces.hex", "wb") as outfile:
...     pickle.dump(faces, outfile)
```

```text

01:49:55-> head faces
[ 0 32 12]
[ 32  33 162]
[ 32 162  12]
[ 12 162  13]
[ 33  34 163]
[ 33 163 162]
[162 163 164]
[162 164  13]
[ 13 164  14]
[ 34  35 165]

> od --endian=little -l -j148 -w24 faces.hex|head
0000224                    0                   32                   12
0000254                   32                   33                  162
0000304                   32                  162                   12
0000334                   12                  162                   13
0000364                   33                   34                  163
0000414                   33                  163                  162
0000444                  162                  163                  164
0000474                  162                  164                   13
0000524                   13                  164                   14
0000554                   34                   35                  165
```

Pickle file has a 148 byte header. Values are little-endian int64. For face data, each triangular face has 3 vertex indexes. The `od` command line above prints each face's triplet on its own line.

For processing efficiency, I stripped out 4 bytes [144..147] in faces.hex, to align 64-bit words starting at offset 144 (0x90). This can then be read efficiently with a memory mapped file.

Likewise for verts.hex. Vertices are XYZ triplets of `double`. The aligned vertex data is as below.

```text

01:46:23-> head verts
[0.         0.52573111 0.85065081]
[ 0.         -0.52573111  0.85065081]
[0.52573111 0.85065081 0.        ]
[-0.52573111  0.85065081  0.        ]
[0.85065081 0.         0.52573111]
[-0.85065081  0.          0.52573111]
[-0.         -0.52573111 -0.85065081]
[-0.          0.52573111 -0.85065081]
[-0.52573111 -0.85065081 -0.        ]
[ 0.52573111 -0.85065081 -0.        ]

01:46:08-> od --endian=little -t fD -j144 -w24 verts.hex|head
0000220                        0       0.5257311121191336         0.85065080835204
0000250                        0      -0.5257311121191336         0.85065080835204
0000300       0.5257311121191336         0.85065080835204                        0
0000330      -0.5257311121191336         0.85065080835204                        0
0000360         0.85065080835204                        0       0.5257311121191336
0000410        -0.85065080835204                        0       0.5257311121191336
0000440                       -0      -0.5257311121191336        -0.85065080835204
0000470                       -0       0.5257311121191336        -0.85065080835204
0000520      -0.5257311121191336        -0.85065080835204                       -0
0000550       0.5257311121191336        -0.85065080835204                       -0
```

## Tesselation

The icosphere starts with 20 faces and 12 vertices. Each subdivision splits each edge in half. Thus, each triangular face subdivides to 4 faces and adds 3 vertices. The new vertices are shared with adjacent neighboring faces. So, each face is charged half a vertex for each of the three added, for 1.5 vertices per existing face on each subdivision.

> For subdivision levels beyond the initial, base level, the total count of vertices at that subd level is half the face count for that level. Each vertex is shared across **6** faces. For 3 vertices per triangular face, $ 3 * \dfrac{1}{6} = 1/2 $. I'm uncertain why this doesn't hold true for the base level, which has 12 vertices, not the theoretical half of 20. Likely, this has to do with triangle fans of 5 faces at the poles. The pole vertices will continue to host a fan of 5 even after multiple subdivisions. Think of these as separate, though integrated, "caps" at the top and bottom of the world.

|SubD Level | Faces | Add vertices | Vertex count |
|---: | ---: | ---: | ---: |
|0 | 20 | | 12 |
|1 | 80 | 30 | 42 |
|2 | 320 | 120 | 162 |
|3 | 1,280 | 480 | 642 |
|4 | 5,120 | 1,920 | 2,562 |
|5 | 20,480 | 7,680 | 10,242 |
|6 | 81,920 | 30,720 | 40,962 |
|7 | 327,680 | 122,880 | 163,842 |
|8 | 1,310,720 | 491,520 | 655,362 |
|9 | 5,242,880 | 1,966,080 | 2,621,442 |
|10 | 20,971,520 | 7,864,320 | 10,485,762 |
|11 | 83,886,080 | 31,457,280 | 41,943,042 |
|12 | 335,544,320 | 125,829,120 | 167,772,162 |
|13	| 1,342,177,280	| 503,316,480 | 671,088,642 |
|14 | 5,368,709,120 | 2,013,265,920 | 2,684,354,562 |


## GEBCO Bathymetry data

GEBCO is the UK equivalent of USGS or NOAA. They produce terrain elev. and bathymetry datasets in 1/4 MOA resolution. One particular dataset containing sub-ice terrain height in Greenland and Antarctica is available. This should be interesting to compare against their ice surface datasets. 

The following examines the data grid for sub-ice terrain heights. In brief, _elevation_[lat][lon] data is:

```cpp
int16_t elev[43200][86400];
```

The following probes the data file using netcdf-python. Briefly, retrieve the _elevation_ grid dimensions

```python
>>> from netCDF4 import Dataset
>>> rr = Dataset("/mnt/c/Users/mikew/Downloads/gebco_2024_sub_ice_topo/GEBCO_2024_sub_ice_topo.nc", "r", format="NETCDF4")
>>> dd=rr.dimensions
>>> print(dd)
>>> for a in dd:
...   print(dd[a])
...
<class 'netCDF4._netCDF4.Dimension'>: name = 'lon', size = 86400
<class 'netCDF4._netCDF4.Dimension'>: name = 'lat', size = 43200
>>> for a in dd:
...   print(dd[a].size)
...
86400
43200
>>>
```

Next, we go after the elevation data.

```python
>>> print(rr.variables)

>>> elev=rr.variables["elevation"]
>>> len(elev)
43200
>>> len(elev[0])
86400
>>> elev[43199][:10]
```

Probing about the data rows by latitude, `elev[0]` returns positive values, land elevations above MSL. We presume this is the South Pole, on Antarctica. We further observe that `elev[-1]` returns negative values at the North Pole, sea floor depths in the Arctic Ocean. Probing about random points on the globe, we gather that longitude ranges from West 180 to East 180. We find that near Chicago, N42 W88, is 212 meters, gratifyingly matching our known elevation near 700 ft MSL.

```python
>>> elev[0:5]
masked_array(
  data=[[-18, -18, -18, ..., -18, -18, -18],
        [-11, -11, -11, ..., -11, -11, -11],
        [  0,   0,   0, ...,   0,   0,   0],
        [ 14,  14,  14, ...,  14,  14,  14],
        [ 32,  32,  32, ...,  32,  32,  32]],
  mask=False,
  fill_value=999999,
  dtype=int16)
>>> elev[43199][0]
-4175
>>> elev[-1][:10]
masked_array(data=[-4175, -4177, -4178, -4180, -4182, -4183, -4185, -4186,
                   -4188, -4189],
             mask=False,
       fill_value=999999,
            dtype=int16)
>>> 86400/360   # conversion factor from degrees
240.0
>>> lo=240*92   # 88 west
>>> la=240*132  # 42 north
>>> elev[la][lo:lo+20]
masked_array(data=[212, 212, 215, 218, 216, 214, 212, 210, 209, 208, 208, ...
```

Write the data to a binary data file for further use.

```python
>>> import numpy as np
>>> ndata = np.array(elev[:][:])
>>> ndata.sav
KeyboardInterrupt
>>> np.save("elev.bin", ndata)
>>> import os
>>> os.getcwd()
'/home/mikey/got/globe'
```

Peeking inside, the numpy header is 128 bytes, ending with '\n'. We can see the start of elevation data below, a full row of -18 meters.

```
=722[0]== mikey@MickWhy:~/got/globe (master)
04:09:18-> head -n1 elev.bin.npy
�NUMPYv{'descr': '<i1', 'fortran_order': False, 'shape': (43200, 86400), }

=725[0]== mikey@MickWhy:~/got/globe (master)
05:10:42-> ll elev.bin.npy
7.0G -rw-r--r-- 1 mikey mikey 7.0G Feb 25 04:13 elev.bin.npy

=726[0]== mikey@MickWhy:~/got/globe (master)
05:10:44-> ls -al elev.bin.npy
-rw-r--r-- 1 mikey mikey 7464960128 Feb 25 04:13 elev.bin.npy

=677[0]== mikey@MickWhy:~/got/globe (master)
04:34:32-> od -s elev.bin.npy |head
0000000  20115  19797  22864      1    118  10107  25956  25459
0000020  10098   8250  15399  12905  11303  10016  28518  29810
0000040  24946  24430  29295  25956  10098   8250  24902  29548
0000060  11365  10016  26739  28769  10085   8250  13352  12851
0000100  12336   8236  13880  12340  10544   8236   8317   8224
0000120   8224   8224   8224   8224   8224   8224   8224   8224
0000160   8224   8224   8224   8224   8224   8224   8224   2592
0000200    -18    -18    -18    -18    -18    -18    -18    -18
```

## Next steps

* We have the terrain elevation now.
* We know how to generate a base icosphere in spherical coordinates.
* We have the basic skeleton of a working Vulkan + glfw graphics app.
* We have a Black Marble nighttime ground texture.
* We have a reasonable understanding of vertex and fragment shaders.
* We have a 700 page book on Vulkan programming. (IOW: a whole lot of reading.)

The next steps:

* Generate and render a base icosphere in Vulkan.
* Subdivide the icosphere, merging coincident vertices.

## Spherical coordinates

Generating a globe, or its approximation, in spherical coordinates is both simple and direct. Indeed, Cartesian coordinates come well after noodling together the collection of spherical coordinates. Even after generating the vertex list, spherical coordinates remain useful to directly express _uv_'s and normals.

We will retain spherical coordinates through the render pipeline.

While we think most often in terms of degrees, I'm beginning to weigh the benefits of normalizing to [0..1) for latitude, and [-1 .. 1] for longitude.  There is ample precedent in looking at grad angle measure (100 grads is 90 degrees). The whole trouble is juggling radians and degrees, and now grads. 90 degrees is 100 grads is $pi$/2 radians.

One fifth of a circle, the geometric base of an icosphere, is:

* 72 degrees, 
* is 80 rads,
* is 2/5 $pi$ radians.

Realistically, I think we'll store radians to save on runtime cost. Radians are directly usable to make normals; grads map well to texture coordinates. Conceptually, I might want to start looking at writing in grads, or that normalized range. What stops me is 60 degrees is an important angle for equilateral triangles, of which we will have billions. $Pi$/3 isn't so difficult to hold my head, nor is 1/3 of a half circle.

> Quaternions seem to be an answer to a question that wasn't discussed above. During subdivision, we generate a new vertex at the midpoint of each edge. The algebraic mean of their endpoints, given in latitude and longitude is incorrect (except when that midpoint lies on the plane of the equator). The resulting mesh has strange wedges missing between each face. The error collapses to within the geometry's tolerance for merging close-enough vertices, but is very prominent in eary subdivisions, which form the basis for subsequent subdivisions.
>
> In any case, calculating the midpoint using quaternions will solve the issue, but I'm uncertain if the computation cost is lower. The brute force method currently is to split the edge using euclidean math on their [xyz] coordinate; and taking the polar coordinates back from normalizing the new vertex to a unit vector. More on this later if further research makes this look favorable.

## Storage, and the immense waste of constant recalculation

Low subdivision levels cost next to nothing to regenerate on demand. However, the generation time increases exponentially with subdivision level. 8 subdivs is tolerable, sub-second or near enough. 9 subdivisions is objectionably long, and 12 subdivs is out of reach in memory cost, let alone computation time.

The time has come now to push the mesh into a file for simple retrieval. We had put this off because we didn't then understand gltf and glb file formats. That aside for the moment, the typical loading sequence for gltf meshes (at least in Vulkan examples) still pushes indexes and vertices one at a time into local storage, usually implemented as a vector. That's copying the disk image data FOUR times before pushing it to the GPU. I know a better way.

Memory maps is one option that I understand very well. The key advantages are stable memory cost and fast random access, all managed by the core kernel memory management and disk cache subsystems. If we can count on anything in modern systems, we can count on these being heavily optimized and robust. Memory mapped files is our handle into that system. Why "we" don't do this more routinely has always puzzled me.

### Storage structure

The globe mesh consists of:

* a list of vertices and
* a list indexes into that vertex list.
* Each triangular face is described by its 3 corners, each a vertex in our list and referenced here by their indexes.
* Each subdivison level maintains its own list of faces but share the full list of vertices.
* In addition to the [xyz] coordinates of each vertex, we also store the latitude and longitude of that location. The lat and lon polar coordinates are used to map bathymetry and terrain heights from respective government agencies, and also as _uv_'s for texture maps.
* The terrain elevation from some given data set.
* A base color for that vertex, possibly calculated from its terrain elevation.

Terrain elevations can reasonably be stored separately, since there are multiple terrain data sets. They are not baked into the base mesh, which stores the vertex position of a unit radius sphere (unit vectors). They are, not incidentally, directly usable as vertex normals, if _normal_ is directly up from a presumed spherical surface.

Vertex color is readily calculated, and is completely subjective with respect to the exact application. It's reasonable to adjust this at runtime to, for example, illustrate the effects of rising sea levels. As a practical measure, the recalculation can be pushed aside from the drawing thread, but can will still need the main thread to swap the new buffer into the GPU. (Assuming the buffering can be arranged to accomodate this. This is not the concern or focus at this moment.)

### File structure details

The devil is in the details. Broadly, we need:

* A header describing the file content.
* Data chunks for each of:
  * Subdivision information, containing:
    * Start/end offsets into the index list, or a chunk ID for separate face lists.
    * An end offset in the vertex list. Each subdiv shares the vertices of lower levels, so the start offset is always 0.
  * The vertex list. As a design choice, we can optionally allocate more than the vec3 needed to describe the vertex. The advantage is 
  * Face index list, for each subdivision level. They can each individually be in their own data chunk. Alternatively, they can be combined into a single list for all subdiv levels, distinguished in the subdiv index with start/end offsets. This latter is the current runtime arrangement. It's a single list combining all face indexes, rather than a list of lists of indexes. That's an implementation choice made by default rather than as a conscious, well considered design choice. For what it's worth, the end result is the same, whether it is one chunk or multiple, both from an internal implemeentation detail viewpoint, and to the application code, which will see only beginning and end pointers regardless of file layout.

### Furthermore

Feature size is the distance between vertices. Interestingly, in the below, feature size at subdiv 0 is 6694 km, not very much larger than the sphere's radius.

```text
1 2 3 4 5 6 7 8 9 Vertices: [2621558], Faces: [6990500] in 10 subdivs:
              20 [0, 20]
              80 [20, 100]
             320 [100, 420]
            1280 [420, 1700]
            5120 [1700, 6820]
           20480 [6820, 27300]
           81920 [27300, 109220]
          327680 [109220, 436900]
         1310720 [436900, 1747620]
         5242880 [1747620, 6990500]
Triangles: [5242880]
Subdiv max 9:  Feature size: 13.7663 16.1832 13.7663 km.
Sudivision 9:  Feature size: 13.7663 16.1832 13.7663 km.
Sudivision 8:  Feature size: 27.5325 32.3663 27.5325 km.
Sudivision 0:  Feature size: 6693.82 6693.82 6693.82 km.
 Feature size: 6693.82 6693.82 6693.82 km.
```

Our interest here is to estimate the maximum useful useful subdivision level, given the resolution of terrain data on hand. The GEBCO data sets resolve to 1/4 minute of latitude, or 0.463 km. Each subdivision halves the feature size. The short answer is 14 subdivisions gives 0.41 km feature size. Longer answer follows.

$$ \tag{Earth's radius} 20,000 km / \pi = 6366 km
$$
$$ \tag{Data resolution} 0.25 NM = 0.463 km
$$
$$ \tag{subdivisions} \frac{6694}{0.463} =  2^n
$$
$$ log(14,456) = log(2) * n
$$
$$ n = 13.819
$$
$$ \tag{Feature size} \frac{6693}{2^{14}} = 0.4085 km
$$

### Data size

How much memory does it take to hold 14 subdivisions? Our tabulation above is close enough to exact. (Actually, within 45 ppm of those estimates. Evidently, some vertices didn't merge. We'll revisit those mechanisms directly. The face counts are deterministic and are presumed exact.)

|SubD Level | Faces | Add vertices | Vertex count |
|---: | ---: | ---: | ---: |
|9 | 5,242,880 | 1,966,080 | 2,621,442 |
|10 | 20,971,520 | 7,864,320 | 10,485,762 |
|11 | 83,886,080 | 31,457,280 | 41,943,042 |
|12 | 335,544,320 | 125,829,120 | 167,772,162 |
|13	| 1,342,177,280	| 503,316,480 | 671,088,642 |
|14 | 5,368,709,120 | 2,013,265,920 | 2,684,354,562 |

Each vertex is 24 bytes:

```c++
    glm::vec2 uv = glm::vec2(0.0f);
    glm::vec3 pos = glm::vec3(0.0f);
    float elev = 1.0f;
```
`uv` is actually our latitude and longitude. We'll retain that on disk.

Each face is also 12 byes, 3 `uint_32` indexes.

$$ \tag{Vertices} 2.68 e 9 * 24 = 64.3 GB $$
$$ \tag{Faces} 5.37 e 9 * 12 = 64.3 GB $$
$$ \tag{Total} 128.6 GB $$

That's quite a bit more than device memory on my dGPU (or any conceivable near future graphics device). Subdiv 11 will consume about 2 GB in device buffers. We can build out the data file to subdiv 12, for 12.5 GB on disk. Feature sizes are 3.27 and 1.63 km respectively.

| SubD | Faces (cumulative) | Vertices (shared) | On disk | In memory | Feature Size km |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 9 | 83,886,000.00 | 62,914,608.00 | 146,800,608.00 | 125,829,168.00 | 13.07 |
| 10 | 335,544,240.00 | 251,658,288.00 | 587,202,528.00 | 503,316,528.00 | 6.54 |
| 11 | 1,342,177,200.00 | 1,006,633,008.00 | 2,348,810,208.00 | 2,013,265,968.00 | 3.27 |
| 12 | 5,368,709,040.00 | 4,026,531,888.00 | 9,395,240,928.00 | 8,053,063,728.00 | 1.63 |
| 13 | 21,474,836,400.00 | 16,106,127,408.00 | 37,580,963,808.00 | 32,212,254,768.00 | 0.82 |
| 14 | 85,899,345,840.00 | 64,424,509,488.00 | 150,323,855,328.00 | 128,849,018,928.00 | 0.41 |




```text
$ build/Release/make-globe.exe testdata/globe-mesh-12.dat elev.bin.npy
std::max_align_t: 8
Generating Globe with 12 subdivisions to file testdata/globe-mesh-12.dat.
       1: +80 = 100
       2: +320 = 420
       3: +1280 = 1700
       4: +5120 = 6820
       5: +20480 = 27300
       6: +81920 = 109220
       7: +327680 = 436900
       8: +1310720 = 1747620
       9: +5242880 = 6990500
      10: +20971520 = 27962020
      11: +83886080 = 111848100
      12: +335544320 = 447392420
Allocating 9403294552 bytes for 447392420 faces and 168107714 vertices.
Vertices: [13], Faces: [20] in 1 subdivs:
              20 [0, 20]
       0: [[    1.571,    0.000], [-0, 1, -4.37114e-08]]
       1: [[    0.464,    0.628], [0.525731, 0.447214, 0.723607]]
       2: [[    0.464,   -0.628], [-0.525731, 0.447214, 0.723607]]
       3: [[   -0.464,    0.000], [0, -0.447214, 0.894427]]
       4: [[   -0.464,    1.257], [0.850651, -0.447214, 0.276393]]
       5: [[   -1.571,    0.628], [-2.56929e-08, -1, -3.53633e-08]]
       6: [[    0.464,    1.885], [0.850651, 0.447214, -0.276393]]
       7: [[   -0.464,    2.513], [0.525731, -0.447214, -0.723607]]
       8: [[    0.464,    3.142], [-7.81933e-08, 0.447214, -0.894427]]
       9: [[   -0.464,    3.770], [-0.525731, -0.447214, -0.723607]]
      10: [[    0.464,    4.398], [-0.850651, 0.447214, -0.276393]]
      11: [[   -0.464,    5.027], [-0.850651, -0.447214, 0.276393]]
      12: [[   -0.464,    5.027], [-0.850651, -0.447214, 0.276393]]
Triangles: [20]
        [        0,        1,        2]
        [        1,        3,        2]
        [        1,        4,        3]
        [        3,        4,        5]
        [        0,        6,        1]
        [        6,        4,        1]
        [        6,        7,        4]
        [        4,        7,        5]
        [        0,        8,        6]
        [        8,        7,        6]
        [        8,        9,        7]
        [        7,        9,        5]
        [        0,       10,        8]
        [       10,        9,        8]
        [       10,       11,        9]
        [        9,       11,        5]
        [        0,        2,       10]
        [        2,       12,       10]
        [        2,        3,       12]
        [       12,        3,        5]
1 2 3 4 5 6 7 8 9 10 11 12 Vertices: [167797023], Faces: [447392420] in 13 subdivs:
              20 [0, 20]
              80 [20, 100]
             320 [100, 420]
            1280 [420, 1700]
            5120 [1700, 6820]
           20480 [6820, 27300]
           81920 [27300, 109220]
          327680 [109220, 436900]
         1310720 [436900, 1747620]
         5242880 [1747620, 6990500]
        20971520 [6990500, 27962020]
        83886080 [27962020, 111848100]
       335544320 [111848100, 447392420]
Triangles: [335544320]
..................................
```

There were actually 167797023 (167.8 million) vertices. We allocated room for 168107714, or 310691 too many. (7456584, 7.46 MB, too big.)

```text
$ build/Release/make-globe.exe testdata/globe-mesh-12.dat 
std::max_align_t: 8
File header:
    id_word      4660
    header_bytes 16
    version_id   256
    data_bytes   0
    padding      0
Chunk Header: [16]
    chunk_type   = 1
    header_bytes = 24
    data_stride  = 24
    data_count   = 13
    data_size    = 312
Chunk Header: [352]
    chunk_type   = 2
    header_bytes = 24
    data_stride  = 12
    data_count   = 447392420
    data_size    = 5368709040
Chunk Header: [5368709416]
    chunk_type   = 3
    header_bytes = 24
    data_stride  = 24
    data_count   = 168107714
    data_size    = 4034585136
Actual data size: 9403294576, data file size: 9403294552
   File is -24 bytes too big.
```

Furthermore, there may be problems between Linux and Win32-sourced data files. Linux g++ 14 on Ubuntu 24.04 reports max_align_t as 16 bytes, compared to 8 bytes above for Win32.

### Hex dumps

Dump a sample of the vertices. The console prints the starting offset of the vertex chunk, chunk type 3. Its data block starts immediately past. Below, the chunk header starts at offest 5243056 and is 24 bytes long. The vertex data block thus begins at 5243080. Each vertex is 24 bytes, all floats. The `od` command line below illustrates.

The first vertex is the north pole. Latitude 90 ($\pi/2$), longitude 0. Cartesian xyz follows, [0, 1, 0]. Depth of the Arctic Ocean there is -4223 meters.

Data looks complete and the quality good.

The notation `File is nnnn bytes too big.` is expected. Vertex merging is inexact. We allow extra room to capture the errant few that escape. Here, the vertex data block itself was trimmed and its data_size adjusted in the chunk header. An EOF chunk header was appended to cap the valid data portion of the file. The file can be truncated by other means to not waste the 8k bytes that follow.

```bash
...
Chunk Header: [5243056]
    chunk_type   = 3
    header_bytes = 24
    data_stride  = 24
    data_count   = 163844
    data_size    = 3932256
Actual data size: 9175336, data file size: 9183304
   File is 7968 bytes too big.

$ od -Ad -w24 -f -j5243080 -N4096 testdata/globe-mesh-7.dat |head
5243080  1.5707964            0              -0               1   -4.371139e-08  G   -4233
5243104  0.4636476   0.62831855       0.5257311       0.4472136       0.7236068      -190
5243128  0.4636476  -0.62831855      -0.5257311       0.4472136       0.7236068     -4766
5243152 -0.4636476            0               0      -0.4472136       0.8944272     -5722
5243176 -0.4636476    1.2566371      0.85065085      -0.4472136      0.27639318     -3084
5243200 -1.5707964   0.62831855   -2.569291e-08              -1  -3.5363257e-08       -25
5243224  0.4636476    1.8849556       0.8506508       0.4472136     -0.27639323       839
5243248 -0.4636476    2.5132742        0.525731      -0.4472136      -0.7236068       257
5243272  0.4636476    3.1415927   -7.819331e-08       0.4472136      -0.8944272     -5460
5243296 -0.4636476    3.7699113      -0.5257312      -0.4472136     -0.72360677     -3682
```

## A conversation with Google search:

Query:

> A ring of 6 equal size equilateral triangles on the surface of a unit sphere encircle a common point on the surface of that sphere. What is the length of each side of those triangles, in radians?

------------------

What you're solving for 
The length of each side of the equilateral triangles in radians. 
What's given in the problem 

• There are $6$ equilateral triangles on the surface of a unit sphere. 
• The triangles encircle a common point. 
• The sphere has a radius of $1$. 

Helpful information 

• The sum of the angles around a point is $2\pi$ radians. 
• The angles of an equilateral triangle are all equal to $\frac{\pi}{3}$ radians. 
• The area of a spherical triangle is given by $A = R^2 (\alpha + \beta + \gamma - \pi)$, where $R$ is the radius of the sphere, and $\alpha$, $\beta$, and $\gamma$ are the angles of the triangle in radians. 

How to solve 
Calculate the angle at the common point subtended by each triangle, then use spherical trigonometry to find the side length. 

1. Step 1 Calculate the angle at the common point subtended by each triangle 
	• The total angle around the common point is $2\pi$. 
	• There are $6$ triangles, so each triangle subtends an angle of $\frac{2\pi}{6}$. 
	• The angle is: 
		• $\frac{2\pi}{6} = \frac{\pi}{3}$ 

2. Step 2 Apply the spherical law of cosines 
	• Let $a$ be the side length of the equilateral triangle. 
	• The angles of the triangle are $\alpha = \beta = \gamma = \frac{\pi}{3}$. 
	• The spherical law of cosines states: 
		• $\cos(a) = \cos(b)\cos(c) + \sin(b)\sin(c)\cos(A)$ 

	• In our case, $b = c = a$ and $A = \frac{\pi}{3}$. 
	• So, $\cos(a) = \cos(a)\cos(a) + \sin(a)\sin(a)\cos(\frac{\pi}{3})$. 
	• Since $\cos(\frac{\pi}{3}) = \frac{1}{2}$, we have: 
		• $\cos(a) = \cos^2(a) + \sin^2(a) \cdot \frac{1}{2}$ 

	• Using the identity $\sin^2(a) = 1 - \cos^2(a)$: 
		• $\cos(a) = \cos^2(a) + \frac{1}{2}(1 - \cos^2(a))$ 
		• $\cos(a) = \cos^2(a) + \frac{1}{2} - \frac{1}{2}\cos^2(a)$ 
		• $\cos(a) = \frac{1}{2}\cos^2(a) + \frac{1}{2}$ 
		• $2\cos(a) = \cos^2(a) + 1$ 
		• $\cos^2(a) - 2\cos(a) + 1 = 0$ 
		• $(\cos(a) - 1)^2 = 0$ 
		• $\cos(a) = 1$ 
		• $a = \arccos(1)$ 
		• $a = 0$ 

	• This result is incorrect, we should use the spherical law of cosines for angles instead. 
	• $\cos(A) = -\cos(B)\cos(C) + \sin(B)\sin(C)\cos(a)$ 
	• Since $A = B = C = \frac{\pi}{3}$, we have: 
		• $\cos(\frac{\pi}{3}) = -\cos(\frac{\pi}{3})\cos(\frac{\pi}{3}) + \sin(\frac{\pi}{3})\sin(\frac{\pi}{3})\cos(a)$ 
		• $\frac{1}{2} = -\frac{1}{2} \cdot \frac{1}{2} + \frac{\sqrt{3}}{2} \cdot \frac{\sqrt{3}}{2} \cos(a)$ 
		• $\frac{1}{2} = -\frac{1}{4} + \frac{3}{4}\cos(a)$ 
		• $\frac{3}{4} = \frac{3}{4}\cos(a)$ 
		• $\cos(a) = 1$ 
		• $a = \frac{\pi}{3}$ 

Solution 
The length of each side of the triangles is $\frac{\pi}{3}$ radians. 

Generative AI is experimental.


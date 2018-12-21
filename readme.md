# cadseer

A parametric solid modeler.

## Getting Started

These instructions may help you get cadseer running on your system.

### Prerequisites

* Qt5 REQUIRED COMPONENTS QtCore QtWidgets Qt5OpenGL Qt5X11Extras Qt5Svg
* Boost REQUIRED COMPONENTS system graph timer filesystem
* OpenCasCade REQUIRED
* OpenSceneGraph REQUIRED
* OSGQt REQUIRED
* Eigen3 REQUIRED
* XercesC REQUIRED
* Spnav OPTIONAL
* CGAL COMPONENTS CGAL_Core OPTIONAL
* zlib REQUIRED
* Libzip REQUIRED
* libigl OPTIONAL
* netgen OPTIONAL
* solvespace REQUIRED

### Installing

Git (pun intended) a copy of the repository.

```
git clone --recursive -j8 https://gitlab.com/blobfish/cadseer.git cadseer
```

Setup for build.

```
cd cadseer
mkdir build
cd build
```

Build

```
cmake -DCMAKE_BUILD_TYPE:STRING=Debug ..
make -j4
```

## Built With

* [OpenCasCade](https://www.opencascade.com/) - Solid modeling kernel
* [OpenSceneGraph](http://www.openscenegraph.org/) - OpenGL visualization wrapper
* [Qt](https://www.qt.io/) - GUI Framework library
* [OSGQt](https://github.com/openscenegraph/osgQt) - Bridge between OpenSceneGraph and Qt
* [Boost](http://www.boost.org/) - C++ library
* [Eigen3](http://eigen.tuxfamily.org/index.php?title=Main_Page) - C++ linear algebra library
* [Xerces-c](http://xerces.apache.org/xerces-c/) - C++ xml parsing library
* [XSDCXX](http://www.codesynthesis.com/products/xsd/) - XML Data Binding for C++
* [CGAL](https://www.cgal.org/) - Computational Geometry Algorithms Library
* [libigl](http://libigl.github.io/libigl/) - Simple C++ geometry processing library
* [netgen](https://sourceforge.net/projects/netgen-mesher/) - Automatic 3d tetrahedral mesh generator
* [solvespace](http://solvespace.com/index.pl/) - Geometric constraint solver
* [spnav](http://spacenav.sourceforge.net/) - Connection to the spacenav driver
* [zlib](https://zlib.net/) - A Massively Spiffy Yet Delicately Unobtrusive Compression Library
* [libzip](https://libzip.org/) - a C library for reading, creating, and modifying zip archives.


## License

This project is licensed under the GPLv3 License

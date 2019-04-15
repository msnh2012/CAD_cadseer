#3 is the highest warning level
project('cadseer_project', 'cpp',
  version : '0.1.0',
  license : 'GPL-3+',
  meson_version : '>=0.49.999',
  default_options : ['warning_level=3', 'cpp_std=c++14']
)

#args for cgal. https://github.com/CGAL/cgal/issues/3180. not sure implications for clang.
extra_args = []
comp = meson.get_compiler('cpp')
if (comp.get_id() == 'gcc')
  extra_args += ['-fext-numeric-literals', '-frounding-math']
endif
defines = []

#Just using versions on my computer. Not necessarily accurate.
qt5process = import('qt5')
qt5 = dependency('Qt5', version : '>=5.11.3', modules : ['Core', 'Widgets', 'OpenGL', 'Svg'])

boost = dependency('boost', modules : ['system', 'graph', 'timer', 'filesystem'])

occt = dependency('OpenCASCADE', version : '>=7.3')

osg = dependency('openscenegraph', version : '>=3.6', modules : ['OpenThreads', 'osg',
  'osgDB', 'osgUtil', 'osgViewer', 'osgGA', 'osgText', 'osgManipulator', 'osgFX', 'osgQt5'])
  
osgqt = dependency('openscenegraph-osgQt5', version : '>=3.6')
  
eigen = dependency('eigen3', version : '>=3.3.7')

xerces = dependency('xerces-c', version : '>=3.2.2')

#I had comment out call to 'CGAL_detect_if_Boost_Thread_is_required()' in /usr/lib/x86_64-linux-gnu/cmake/CGAL/CGAL_SetupBoost.cmake:46
cgal = dependency('CGAL', version : '>=4.13')

if(host_machine.system() == 'linux' or host_machine.system() == 'darwin')
  spnav = comp.find_library('spnav', required : false)
  if(spnav.found())
    defines += '-DSPNAV_PRESENT'
    message('6dof device support through spnav')
  else
    message('NO 6dof device support. install spnav')
  endif
endif

gmsh = dependency('', required:false)
if (get_option('gmsh'))
  gmsh = comp.find_library('gmsh', required : false)
  if(gmsh.found())
    defines += '-DGMSH_PRESENT'
  endif
endif

#had to install the application to satisfy netgenconfig.cmake
netgen = dependency('', required:false)
if (get_option('netgen'))
  netgen = dependency('Netgen', version : '>=6.2', required : false, modules : ['nglib'])
  if(netgen.found())
    defines += '-DNETGEN_PRESENT'
  endif
endif

#assuming if this is there, then xsdcxx is installed. No lib to link.
assert(comp.has_header('xsd/cxx/version.hxx'), 'Dependency xsdcxx found: NO')

git2pp_proj = subproject('libgit2pp')
git2pp = git2pp_proj.get_variable('libgit2pp_dep')

solvespace_proj = subproject('solvespace')
solvespace = solvespace_proj.get_variable('libsolvespace_dep')

libzippp_proj = subproject('libzippp')
libzippp = libzippp_proj.get_variable('libzippp_static_dep')

#libigl has some problems. See myBuild.txt in subproject/libigl for description.
#libigl = dependency('libigl', version : '>=3.3.7', modules : ['igl::core'])
libigl_proj = subproject('libigl')
libigl = libigl_proj.get_variable('libigl_dep')
defines += '-DLIBIGL_USE_STATIC_LIBRARY=OFF'
#static igl library needs to link to threads even in header only mode.
threads = dependency('threads')

squash_sources = ['squash/squash.cpp'
  , 'squash/igl.cpp']
  
sketch_sources = ['sketch/solver.cpp'
  , 'sketch/selection.cpp'
  , 'sketch/visual.cpp']
  
modelviz_sources = ['modelviz/base.cpp'
  , 'modelviz/shapegeometry.cpp'
  , 'modelviz/datumplane.cpp'
  , 'modelviz/hiddenlineeffect.cpp'
  , 'modelviz/hiddenlinetechnique.cpp'
  , 'modelviz/datumaxis.cpp'
  , 'modelviz/surfacemesh.cpp']
  
gesture_sources = ['gesture/handler.cpp'
  , 'gesture/node.cpp'
  , 'gesture/animations.cpp']
  
message_sources = ['message/message.cpp'
  , 'message/node.cpp'
  , 'message/sift.cpp']
  
feature_sources = ['feature/inputtype.cpp'
  , 'feature/base.cpp'
  , 'feature/box.cpp'
  , 'feature/boxbuilder.cpp'
  , 'feature/sphere.cpp'
  , 'feature/inert.cpp'
  , 'feature/cone.cpp'
  , 'feature/conebuilder.cpp'
  , 'feature/cylinder.cpp'
  , 'feature/blend.cpp'
  , 'feature/chamfer.cpp'
  , 'feature/draft.cpp'
  , 'feature/cylinderbuilder.cpp'
  , 'feature/union.cpp'
  , 'feature/subtract.cpp'
  , 'feature/intersect.cpp'
  , 'feature/booleanoperation.cpp'
  , 'feature/shapecheck.cpp'
  , 'feature/seershapeinfo.cpp'
  , 'feature/datumplane.cpp'
  , 'feature/message.cpp'
  , 'feature/hollow.cpp'
  , 'feature/pick.cpp'
  , 'feature/oblong.cpp'
  , 'feature/oblongbuilder.cpp'
  , 'feature/shapehistory.cpp'
  , 'feature/extract.cpp'
  , 'feature/squash.cpp'
  , 'feature/strip.cpp'
  , 'feature/nest.cpp'
  , 'feature/dieset.cpp'
  , 'feature/quote.cpp'
  , 'feature/refine.cpp'
  , 'feature/instancelinear.cpp'
  , 'feature/instancemirror.cpp'
  , 'feature/instancepolar.cpp'
  , 'feature/updatepayload.cpp'
  , 'feature/offset.cpp'
  , 'feature/thicken.cpp'
  , 'feature/sew.cpp'
  , 'feature/trim.cpp'
  , 'feature/removefaces.cpp'
  , 'feature/torus.cpp'
  , 'feature/thread.cpp'
  , 'feature/datumaxis.cpp'
  , 'feature/sketch.cpp'
  , 'feature/extrude.cpp'
  , 'feature/revolve.cpp'
  , 'feature/surfacemesh.cpp'
  , 'feature/ftrline.cpp'
  , 'feature/ftrtransitioncurve.cpp'
  , 'feature/ftrruled.cpp'
  , 'feature/ftrimageplane.cpp']
  
parameter_sources = ['parameter/parameter.cpp']

annex_sources = ['annex/base.cpp'
  , 'annex/csysdragger.cpp'
  , 'annex/seershape.cpp'
  , 'annex/intersectionmapper.cpp'
  , 'annex/instancemapper.cpp'
  , 'annex/shapeidhelper.cpp'
  , 'annex/surfacemesh.cpp']
  
expression_sources = ['expressions/abstractnode.cpp'
  , 'expressions/graph.cpp'
  , 'expressions/manager.cpp'
  , 'expressions/widget.cpp'
  , 'expressions/stringtranslator.cpp'
  , 'expressions/stringtranslatorstow.cpp'
  , 'expressions/tablemodel.cpp'
  , 'expressions/tableview.cpp']
  
selection_sources = ['selection/eventhandler.cpp'
  , 'selection/manager.cpp'
  , 'selection/message.cpp'
  , 'selection/container.cpp'
  , 'selection/overlayhandler.cpp'
  , 'selection/interpreter.cpp'
  , 'selection/visitors.cpp'
  , 'selection/definitions.cpp'
  , 'selection/intersection.cpp']
  
tools_sources = ['tools/idtools.cpp'
  , 'tools/infotools.cpp'
  , 'tools/occtools.cpp'
  , 'tools/featuretools.cpp']
  
dialog_sources = ['dialogs/parameter.cpp'
  , 'dialogs/checkgeometry.cpp'
  , 'dialogs/widgetgeometry.cpp'
  , 'dialogs/blend.cpp'
  , 'dialogs/expressionedit.cpp'
  , 'dialogs/strip.cpp'
  , 'dialogs/selectionbutton.cpp'
  , 'dialogs/quote.cpp'
  , 'dialogs/preferences.cpp'
  , 'dialogs/project.cpp'
  , 'dialogs/boolean.cpp'
  , 'dialogs/revision.cpp'
  , 'dialogs/commitwidget.cpp'
  , 'dialogs/tagwidget.cpp'
  , 'dialogs/about.cpp'
  , 'dialogs/splitterdecorated.cpp'
  , 'dialogs/sketch.cpp']
  
preference_sources = ['preferences/preferencesXML.cpp', 'preferences/manager.cpp']

project_sources = ['project/project.cpp'
  , 'project/stow.cpp'
  , 'project/message.cpp'
  , 'project/gitmanager.cpp'
  , 'project/featureload.cpp'
  , 'project/serial/xsdcxxoutput/shapehistory.cpp'
  , 'project/serial/xsdcxxoutput/project.cpp'
  , 'project/serial/xsdcxxoutput/view.cpp'
  , 'project/serial/xsdcxxoutput/featurebase.cpp'
  , 'project/serial/xsdcxxoutput/featurebox.cpp'
  , 'project/serial/xsdcxxoutput/featurecylinder.cpp'
  , 'project/serial/xsdcxxoutput/featuresphere.cpp'
  , 'project/serial/xsdcxxoutput/featurecone.cpp'
  , 'project/serial/xsdcxxoutput/featureunion.cpp'
  , 'project/serial/xsdcxxoutput/featureintersect.cpp'
  , 'project/serial/xsdcxxoutput/featuresubtract.cpp'
  , 'project/serial/xsdcxxoutput/featureinert.cpp'
  , 'project/serial/xsdcxxoutput/featureblend.cpp'
  , 'project/serial/xsdcxxoutput/featurechamfer.cpp'
  , 'project/serial/xsdcxxoutput/featuredraft.cpp'
  , 'project/serial/xsdcxxoutput/featuredatumplane.cpp'
  , 'project/serial/xsdcxxoutput/featurehollow.cpp'
  , 'project/serial/xsdcxxoutput/featureoblong.cpp'
  , 'project/serial/xsdcxxoutput/featureextract.cpp'
  , 'project/serial/xsdcxxoutput/featuresquash.cpp'
  , 'project/serial/xsdcxxoutput/featurenest.cpp'
  , 'project/serial/xsdcxxoutput/featuredieset.cpp'
  , 'project/serial/xsdcxxoutput/featurestrip.cpp'
  , 'project/serial/xsdcxxoutput/featurequote.cpp'
  , 'project/serial/xsdcxxoutput/featurerefine.cpp'
  , 'project/serial/xsdcxxoutput/instancemapper.cpp'
  , 'project/serial/xsdcxxoutput/featureinstancelinear.cpp'
  , 'project/serial/xsdcxxoutput/featureinstancemirror.cpp'
  , 'project/serial/xsdcxxoutput/featureinstancepolar.cpp'
  , 'project/serial/xsdcxxoutput/featureoffset.cpp'
  , 'project/serial/xsdcxxoutput/featurethicken.cpp'
  , 'project/serial/xsdcxxoutput/featuresew.cpp'
  , 'project/serial/xsdcxxoutput/featuretrim.cpp'
  , 'project/serial/xsdcxxoutput/featureremovefaces.cpp'
  , 'project/serial/xsdcxxoutput/featuretorus.cpp'
  , 'project/serial/xsdcxxoutput/featurethread.cpp'
  , 'project/serial/xsdcxxoutput/featuredatumaxis.cpp'
  , 'project/serial/xsdcxxoutput/featureextrude.cpp'
  , 'project/serial/xsdcxxoutput/featurerevolve.cpp'
  , 'project/serial/xsdcxxoutput/featuresketch.cpp'
  , 'project/serial/xsdcxxoutput/featureline.cpp'
  , 'project/serial/xsdcxxoutput/mesh.cpp'
  , 'project/serial/xsdcxxoutput/featuresurfacemesh.cpp'
  , 'project/serial/xsdcxxoutput/featuretransitioncurve.cpp'
  , 'project/serial/xsdcxxoutput/featureruled.cpp'
  , 'project/serial/xsdcxxoutput/featureimageplane.cpp']
  
application_sources = ['application/mainwindow.cpp'
  , 'application/application.cpp'
  , 'application/factory.cpp'
  , 'application/message.cpp'
  , 'application/incrementwidget.cpp'
  , 'application/infowindow.cpp']
  
command_sources = ['command/manager.cpp'
  , 'command/base.cpp'
  , 'command/csysedit.cpp'
  , 'command/featuretosystem.cpp'
  , 'command/systemtofeature.cpp'
  , 'command/featuretodragger.cpp'
  , 'command/draggertofeature.cpp'
  , 'command/checkgeometry.cpp'
  , 'command/editcolor.cpp'
  , 'command/featurerename.cpp'
  , 'command/blend.cpp'
  , 'command/extract.cpp'
  , 'command/featurereposition.cpp'
  , 'command/squash.cpp'
  , 'command/strip.cpp'
  , 'command/nest.cpp'
  , 'command/dieset.cpp'
  , 'command/quote.cpp'
  , 'command/isolate.cpp'
  , 'command/measurelinear.cpp'
  , 'command/refine.cpp'
  , 'command/instancelinear.cpp'
  , 'command/instancemirror.cpp'
  , 'command/instancepolar.cpp'
  , 'command/intersect.cpp'
  , 'command/subtract.cpp'
  , 'command/union.cpp'
  , 'command/systemtoselection.cpp'
  , 'command/offset.cpp'
  , 'command/thicken.cpp'
  , 'command/sew.cpp'
  , 'command/trim.cpp'
  , 'command/revision.cpp'
  , 'command/removefaces.cpp'
  , 'command/thread.cpp'
  , 'command/dissolve.cpp'
  , 'command/datumaxis.cpp'
  , 'command/datumplane.cpp'
  , 'command/sketch.cpp'
  , 'command/extrude.cpp'
  , 'command/revolve.cpp'
  , 'command/cmdline.cpp'
  , 'command/surfacemesh.cpp'
  , 'command/cmdtransitioncurve.cpp'
  , 'command/cmdruled.cpp'
  , 'command/cmdimageplane.cpp']
  
library_sources = ['library/circlebuilder.cpp'
  , 'library/circleprojector.cpp'
  , 'library/conebuilder.cpp'
  , 'library/csysdragger.cpp'
  , 'library/cylinderbuilder.cpp'
  , 'library/geometrylibrary.cpp'
  , 'library/iconbuilder.cpp'
  , 'library/rotatecirculardragger.cpp'
  , 'library/spherebuilder.cpp'
  , 'library/torusbuilder.cpp'
  , 'library/lineardragger.cpp'
  , 'library/lineardimension.cpp'
  , 'library/ipgroup.cpp'
  , 'library/plabel.cpp'
  , 'library/sketchlineardimension.cpp'
  , 'library/angulardimension.cpp'
  , 'library/circlebuilderlod.cpp'
  , 'library/diameterdimension.cpp']
  
viewer_sources = ['viewer/vwrwidget.cpp'
  , 'viewer/spaceballmanipulator.cpp'
  , 'viewer/spaceballqevent.cpp'
  , 'viewer/gleventwidget.cpp'
  , 'viewer/spaceballosgevent.cpp'
  , 'viewer/textcamera.cpp'
  , 'viewer/overlay.cpp'
  , 'viewer/message.cpp']
  
libreoffice_sources = ['libreoffice/odshack.cpp']

lod_sources = ['lod/message.cpp', 'lod/lodmanager.cpp']

mesh_sources = ['mesh/parameters.cpp']

dagview_sources = ['dagview/view.cpp'
  , 'dagview/view.cpp'
  , 'dagview/model.cpp'
  , 'dagview/stow.cpp'
  , 'dagview/rectitem.cpp']

cadseer_sources = ['main.cpp'
  , 'globalutilities.cpp'
  , dagview_sources
  , modelviz_sources
  , gesture_sources
  , message_sources
  , feature_sources
  , parameter_sources
  , annex_sources
  , expression_sources
  , selection_sources
  , tools_sources
  , dialog_sources
  , preference_sources
  , project_sources
  , application_sources
  , library_sources
  , command_sources
  , viewer_sources
  , squash_sources
  , libreoffice_sources
  , lod_sources
  , mesh_sources
  , sketch_sources]

cadseer_uis = ['application/mainwindow.ui'
  , 'dialogs/preferences.ui'
  , 'dialogs/project.ui'
  , 'dialogs/about.ui']

cadseer_moc_hdrs = ['viewer/vwrwidget.h'
  , 'application/mainwindow.h'
  , 'dagview/view.h'
  , 'dagview/model.h'
  , 'application/application.h'
  , 'application/infowindow.h'
  , 'application/incrementwidget.h'
  , 'selection/manager.h'
  , 'dialogs/checkgeometry.h'
  , 'dialogs/widgetgeometry.h'
  , 'dialogs/blend.h'
  , 'dialogs/expressionedit.h'
  , 'dialogs/preferences.h'
  , 'dialogs/parameter.h'
  , 'dialogs/splitterdecorated.h'
  , 'expressions/widget.h'
  , 'expressions/tablemodel.h'
  , 'expressions/tableview.h'
  , 'dialogs/strip.h'
  , 'dialogs/selectionbutton.h'
  , 'dialogs/quote.h'
  , 'dialogs/project.h'
  , 'dialogs/boolean.h'
  , 'dialogs/revision.h'
  , 'dialogs/commitwidget.h'
  , 'lod/lodmanager.h'
  , 'dialogs/about.h'
  , 'dialogs/sketch.h']
  
cadseer_resources = ['resources.qrc']

qt5_processed = qt5process.preprocess(
  moc_headers : cadseer_moc_hdrs
  , ui_files: cadseer_uis
  , qresources : cadseer_resources
)

cadseer_exe = executable('cadseer', [cadseer_sources, qt5_processed]
  , dependencies : [qt5, boost, occt, osg, osgqt, eigen, xerces, cgal, threads, libzippp, spnav, gmsh, netgen, git2pp, solvespace, libigl]
  , cpp_args : [defines, extra_args])
  
lodgenerator_sources = ['lod/main.cpp'
  , 'tools/occtools.cpp'
  , 'tools/idtools.cpp'
  , 'annex/shapeidhelper.cpp'
  , 'modelviz/base.cpp'
  , 'modelviz/hiddenlineeffect.cpp'
  , 'modelviz/hiddenlinetechnique.cpp'
  , 'modelviz/shapegeometry.cpp']

lodgenerator_exe = executable('lodgenerator', lodgenerator_sources
  , dependencies : [boost, osg, occt]
  , cpp_args : [defines, extra_args])

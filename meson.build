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
  'osgDB', 'osgUtil', 'osgViewer', 'osgGA', 'osgText', 'osgManipulator', 'osgFX'])
  
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

#I cannot link both gmsh and netgen at this time or netgen goes all segfaulty
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

osgqt_proj = subproject('osgQt')
osgqt = osgqt_proj.get_variable('libosgqt_dep')

#libigl has some problems. See myBuild.txt in subproject/libigl for description.
#libigl = dependency('libigl', version : '>=3.3.7', modules : ['igl::core'])
libigl_proj = subproject('libigl')
libigl = libigl_proj.get_variable('libigl_dep')
defines += '-DLIBIGL_USE_STATIC_LIBRARY=OFF'
#static igl library needs to link to threads even in header only mode.
threads = dependency('threads')

squash_sources = ['squash/sqssquash.cpp'
  , 'squash/sqsigl.cpp']
  
sketch_sources = ['sketch/sktsolver.cpp'
  , 'sketch/sktselection.cpp'
  , 'sketch/sktvisual.cpp']
  
modelviz_sources = ['modelviz/mdvbase.cpp'
  , 'modelviz/mdvshapegeometry.cpp'
  , 'modelviz/mdvdatumplane.cpp'
  , 'modelviz/mdvhiddenlineeffect.cpp'
  , 'modelviz/mdvhiddenlinetechnique.cpp'
  , 'modelviz/mdvdatumaxis.cpp'
  , 'modelviz/mdvsurfacemesh.cpp'
  , 'modelviz/mdvlawfunction.cpp'
  , 'modelviz/mdvdatumsystem.cpp']
  
gesture_sources = ['gesture/gsnhandler.cpp'
  , 'gesture/gsnnode.cpp'
  , 'gesture/gsnanimations.cpp']
  
message_sources = ['message/msgmessage.cpp'
  , 'message/msgnode.cpp'
  , 'message/msgsift.cpp']
  
feature_sources = ['feature/ftrinputtype.cpp'
  , 'feature/ftrbase.cpp'
  , 'feature/ftrbox.cpp'
  , 'feature/ftrboxbuilder.cpp'
  , 'feature/ftrsphere.cpp'
  , 'feature/ftrinert.cpp'
  , 'feature/ftrcone.cpp'
  , 'feature/ftrconebuilder.cpp'
  , 'feature/ftrcylinder.cpp'
  , 'feature/ftrblend.cpp'
  , 'feature/ftrchamfer.cpp'
  , 'feature/ftrdraft.cpp'
  , 'feature/ftrcylinderbuilder.cpp'
  , 'feature/ftrunion.cpp'
  , 'feature/ftrsubtract.cpp'
  , 'feature/ftrintersect.cpp'
  , 'feature/ftrbooleanoperation.cpp'
  , 'feature/ftrshapecheck.cpp'
  , 'feature/ftrseershapeinfo.cpp'
  , 'feature/ftrdatumplane.cpp'
  , 'feature/ftrmessage.cpp'
  , 'feature/ftrhollow.cpp'
  , 'feature/ftrpick.cpp'
  , 'feature/ftroblong.cpp'
  , 'feature/ftroblongbuilder.cpp'
  , 'feature/ftrshapehistory.cpp'
  , 'feature/ftrextract.cpp'
  , 'feature/ftrsquash.cpp'
  , 'feature/ftrstrip.cpp'
  , 'feature/ftrnest.cpp'
  , 'feature/ftrdieset.cpp'
  , 'feature/ftrquote.cpp'
  , 'feature/ftrrefine.cpp'
  , 'feature/ftrinstancelinear.cpp'
  , 'feature/ftrinstancemirror.cpp'
  , 'feature/ftrinstancepolar.cpp'
  , 'feature/ftrupdatepayload.cpp'
  , 'feature/ftroffset.cpp'
  , 'feature/ftrthicken.cpp'
  , 'feature/ftrsew.cpp'
  , 'feature/ftrtrim.cpp'
  , 'feature/ftrremovefaces.cpp'
  , 'feature/ftrtorus.cpp'
  , 'feature/ftrthread.cpp'
  , 'feature/ftrdatumaxis.cpp'
  , 'feature/ftrsketch.cpp'
  , 'feature/ftrextrude.cpp'
  , 'feature/ftrrevolve.cpp'
  , 'feature/ftrsurfacemesh.cpp'
  , 'feature/ftrline.cpp'
  , 'feature/ftrtransitioncurve.cpp'
  , 'feature/ftrruled.cpp'
  , 'feature/ftrimageplane.cpp'
  , 'feature/ftrsweep.cpp'
  , 'feature/ftrdatumsystem.cpp']
  
parameter_sources = ['parameter/prmparameter.cpp']

annex_sources = ['annex/annbase.cpp'
  , 'annex/anncsysdragger.cpp'
  , 'annex/annseershape.cpp'
  , 'annex/annintersectionmapper.cpp'
  , 'annex/anninstancemapper.cpp'
  , 'annex/annshapeidhelper.cpp'
  , 'annex/annsurfacemesh.cpp'
  , 'annex/annlawfunction.cpp']
  
expression_sources = ['expressions/exprabstractnode.cpp'
  , 'expressions/exprgraph.cpp'
  , 'expressions/exprmanager.cpp'
  , 'expressions/exprwidget.cpp'
  , 'expressions/exprstringtranslator.cpp'
  , 'expressions/exprstringtranslatorstow.cpp'
  , 'expressions/exprtablemodel.cpp'
  , 'expressions/exprtableview.cpp']
  
selection_sources = ['selection/slceventhandler.cpp'
  , 'selection/slcmanager.cpp'
  , 'selection/slcmessage.cpp'
  , 'selection/slccontainer.cpp'
  , 'selection/slcoverlayhandler.cpp'
  , 'selection/slcinterpreter.cpp'
  , 'selection/slcvisitors.cpp'
  , 'selection/slcdefinitions.cpp'
  , 'selection/slcintersection.cpp'
  , 'selection/slcaccrue.cpp']
  
tools_sources = ['tools/idtools.cpp'
  , 'tools/infotools.cpp'
  , 'tools/occtools.cpp'
  , 'tools/featuretools.cpp'
  , 'tools/tlsnameindexer.cpp'
  , 'tools/tlsosgtools.cpp']
  
dialog_sources = ['dialogs/dlgparameter.cpp'
  , 'dialogs/dlgcheckgeometry.cpp'
  , 'dialogs/dlgwidgetgeometry.cpp'
  , 'dialogs/dlgblend.cpp'
  , 'dialogs/dlgexpressionedit.cpp'
  , 'dialogs/dlgstrip.cpp'
  , 'dialogs/dlgselectionbutton.cpp'
  , 'dialogs/dlgquote.cpp'
  , 'dialogs/dlgpreferences.cpp'
  , 'dialogs/dlgproject.cpp'
  , 'dialogs/dlgboolean.cpp'
  , 'dialogs/dlgrevision.cpp'
  , 'dialogs/dlgcommitwidget.cpp'
  , 'dialogs/dlgtagwidget.cpp'
  , 'dialogs/dlgabout.cpp'
  , 'dialogs/dlgsplitterdecorated.cpp'
  , 'dialogs/dlgsketch.cpp'
  , 'dialogs/dlgsweep.cpp'
  , 'dialogs/dlglawfunctionwidget.cpp'
  , 'dialogs/dlgenterfilter.cpp'
  , 'dialogs/dlgselectionlist.cpp'
  , 'dialogs/dlgselectionwidget.cpp'
  , 'dialogs/dlgparameterwidget.cpp'
  , 'dialogs/dlgdraft.cpp'
  , 'dialogs/dlgbase.cpp'
  , 'dialogs/dlgextract.cpp'
  , 'dialogs/dlgchamfer.cpp'
  , 'dialogs/dlgbox.cpp'
  , 'dialogs/dlgoblong.cpp'
  , 'dialogs/dlgtorus.cpp'
  , 'dialogs/dlgcylinder.cpp'
  , 'dialogs/dlgsphere.cpp'
  , 'dialogs/dlgcone.cpp'
  , 'dialogs/dlgremove.cpp'
  , 'dialogs/dlghollow.cpp'
  , 'dialogs/dlgdatumsystem.cpp'
  , 'dialogs/dlgcsyswidget.cpp']
  
preference_sources = ['preferences/preferencesXML.cpp', 'preferences/prfmanager.cpp']

project_sources = ['project/prjproject.cpp'
  , 'project/prjstow.cpp'
  , 'project/prjmessage.cpp'
  , 'project/prjgitmanager.cpp'
  , 'project/prjfeatureload.cpp'
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
  , 'project/serial/xsdcxxoutput/featureimageplane.cpp'
  , 'project/serial/xsdcxxoutput/lawfunction.cpp'
  , 'project/serial/xsdcxxoutput/featuresweep.cpp'
  , 'project/serial/xsdcxxoutput/featuredatumsystem.cpp']
  
application_sources = ['application/appmainwindow.cpp'
  , 'application/appapplication.cpp'
  , 'application/appmessage.cpp'
  , 'application/appincrementwidget.cpp'
  , 'application/appinfowindow.cpp']
  
command_sources = ['command/cmdmanager.cpp'
  , 'command/cmdbase.cpp'
  , 'command/cmdcsysedit.cpp'
  , 'command/cmdfeaturetosystem.cpp'
  , 'command/cmdsystemtofeature.cpp'
  , 'command/cmdfeaturetodragger.cpp'
  , 'command/cmddraggertofeature.cpp'
  , 'command/cmdcheckgeometry.cpp'
  , 'command/cmdeditcolor.cpp'
  , 'command/cmdfeaturerename.cpp'
  , 'command/cmdblend.cpp'
  , 'command/cmdextract.cpp'
  , 'command/cmdfeaturereposition.cpp'
  , 'command/cmdsquash.cpp'
  , 'command/cmdstrip.cpp'
  , 'command/cmdnest.cpp'
  , 'command/cmddieset.cpp'
  , 'command/cmdquote.cpp'
  , 'command/cmdisolate.cpp'
  , 'command/cmdmeasurelinear.cpp'
  , 'command/cmdrefine.cpp'
  , 'command/cmdinstancelinear.cpp'
  , 'command/cmdinstancemirror.cpp'
  , 'command/cmdinstancepolar.cpp'
  , 'command/cmdintersect.cpp'
  , 'command/cmdsubtract.cpp'
  , 'command/cmdunion.cpp'
  , 'command/cmdsystemtoselection.cpp'
  , 'command/cmdoffset.cpp'
  , 'command/cmdthicken.cpp'
  , 'command/cmdsew.cpp'
  , 'command/cmdtrim.cpp'
  , 'command/cmdrevision.cpp'
  , 'command/cmdremovefaces.cpp'
  , 'command/cmdthread.cpp'
  , 'command/cmddissolve.cpp'
  , 'command/cmddatumaxis.cpp'
  , 'command/cmddatumplane.cpp'
  , 'command/cmdsketch.cpp'
  , 'command/cmdextrude.cpp'
  , 'command/cmdrevolve.cpp'
  , 'command/cmdline.cpp'
  , 'command/cmdsurfacemesh.cpp'
  , 'command/cmdtransitioncurve.cpp'
  , 'command/cmdruled.cpp'
  , 'command/cmdimageplane.cpp'
  , 'command/cmdsweep.cpp'
  , 'command/cmdboolean.cpp'
  , 'command/cmddraft.cpp'
  , 'command/cmdchamfer.cpp'
  , 'command/cmdbox.cpp'
  , 'command/cmdoblong.cpp'
  , 'command/cmdtorus.cpp'
  , 'command/cmdcylinder.cpp'
  , 'command/cmdsphere.cpp'
  , 'command/cmdcone.cpp'
  , 'command/cmdimport.cpp'
  , 'command/cmdexport.cpp'
  , 'command/cmdpreferences.cpp'
  , 'command/cmdremove.cpp'
  , 'command/cmdhollow.cpp'
  , 'command/cmdinfo.cpp'
  , 'command/cmddirty.cpp'
  , 'command/cmdfeaturedump.cpp'
  , 'command/cmdshapetrackdump.cpp'
  , 'command/cmdshapegraphdump.cpp'
  , 'command/cmdtest.cpp'
  , 'command/cmddatumsystem.cpp']
  
law_sources = ['law/lwfcue.cpp']

library_sources = ['library/lbrcirclebuilder.cpp'
  , 'library/lbrcircleprojector.cpp'
  , 'library/lbrconebuilder.cpp'
  , 'library/lbrcsysdragger.cpp'
  , 'library/lbrcylinderbuilder.cpp'
  , 'library/lbrgeometrylibrary.cpp'
  , 'library/lbriconbuilder.cpp'
  , 'library/lbrrotatecirculardragger.cpp'
  , 'library/lbrspherebuilder.cpp'
  , 'library/lbrtorusbuilder.cpp'
  , 'library/lbrlineardragger.cpp'
  , 'library/lbrlineardimension.cpp'
  , 'library/lbripgroup.cpp'
  , 'library/lbrplabel.cpp'
  , 'library/lbrsketchlineardimension.cpp'
  , 'library/lbrangulardimension.cpp'
  , 'library/lbrcirclebuilderlod.cpp'
  , 'library/lbrdiameterdimension.cpp']
  
viewer_sources = ['viewer/vwrwidget.cpp'
  , 'viewer/vwrspaceballmanipulator.cpp'
  , 'viewer/vwrspaceballqevent.cpp'
  , 'viewer/vwrspaceballosgevent.cpp'
  , 'viewer/vwrtextcamera.cpp'
  , 'viewer/vwroverlay.cpp'
  , 'viewer/vwrmessage.cpp']
  
libreoffice_sources = ['libreoffice/lboodshack.cpp']

lod_sources = ['lod/lodmessage.cpp', 'lod/lodmanager.cpp']

menu_sources = ['menu/mnuserial.cpp', 'menu/mnumanager.cpp']

mesh_sources = ['mesh/mshparameters.cpp']

dagview_sources = ['dagview/dagview.cpp'
  , 'dagview/dagview.cpp'
  , 'dagview/dagmodel.cpp'
  , 'dagview/dagstow.cpp'
  , 'dagview/dagrectitem.cpp']

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
  , law_sources
  , library_sources
  , command_sources
  , viewer_sources
  , squash_sources
  , libreoffice_sources
  , lod_sources
  , mesh_sources
  , sketch_sources
  , menu_sources]

cadseer_uis = ['dialogs/dlgpreferences.ui'
  , 'dialogs/dlgproject.ui'
  , 'dialogs/dlgabout.ui']

cadseer_moc_hdrs = ['viewer/vwrwidget.h'
  , 'application/appmainwindow.h'
  , 'dagview/dagview.h'
  , 'dagview/dagmodel.h'
  , 'application/appapplication.h'
  , 'application/appinfowindow.h'
  , 'application/appincrementwidget.h'
  , 'selection/slcmanager.h'
  , 'dialogs/dlgcheckgeometry.h'
  , 'dialogs/dlgwidgetgeometry.h'
  , 'dialogs/dlgblend.h'
  , 'dialogs/dlgexpressionedit.h'
  , 'dialogs/dlgpreferences.h'
  , 'dialogs/dlgparameter.h'
  , 'dialogs/dlgsplitterdecorated.h'
  , 'expressions/exprwidget.h'
  , 'expressions/exprtablemodel.h'
  , 'expressions/exprtableview.h'
  , 'dialogs/dlgstrip.h'
  , 'dialogs/dlgselectionbutton.h'
  , 'dialogs/dlgquote.h'
  , 'dialogs/dlgproject.h'
  , 'dialogs/dlgboolean.h'
  , 'dialogs/dlgrevision.h'
  , 'dialogs/dlgcommitwidget.h'
  , 'lod/lodmanager.h'
  , 'dialogs/dlgabout.h'
  , 'dialogs/dlgsketch.h'
  , 'dialogs/dlgsweep.h'
  , 'dialogs/dlglawfunctionwidget.h'
  , 'dialogs/dlgenterfilter.h'
  , 'dialogs/dlgselectionlist.h'
  , 'dialogs/dlgselectionwidget.h'
  , 'dialogs/dlgparameterwidget.h'
  , 'dialogs/dlgdraft.h'
  , 'dialogs/dlgbase.h'
  , 'dialogs/dlgextract.h'
  , 'dialogs/dlgchamfer.h'
  , 'dialogs/dlgbox.h'
  , 'dialogs/dlgoblong.h'
  , 'dialogs/dlgtorus.h'
  , 'dialogs/dlgcylinder.h'
  , 'dialogs/dlgsphere.h'
  , 'dialogs/dlgcone.h'
  , 'dialogs/dlgremove.h'
  , 'dialogs/dlghollow.h'
  , 'dialogs/dlgdatumsystem.h'
  , 'dialogs/dlgcsyswidget.h'
  , 'dialogs/dlgvizfilter.h']
  
cadseer_resources = ['resources.qrc']

qt5_processed = qt5process.preprocess(
  moc_headers : cadseer_moc_hdrs
  , ui_files: cadseer_uis
  , qresources : cadseer_resources
)

cadseer_exe = executable('cadseer', [cadseer_sources, qt5_processed]
  , dependencies : [qt5, boost, occt, osg, osgqt, eigen, xerces, cgal, threads, libzippp, spnav, gmsh, netgen, git2pp, solvespace, libigl]
  , cpp_args : [defines, extra_args]
  , install : true)
  
lodgenerator_sources = ['lod/lodmain.cpp'
  , 'tools/occtools.cpp'
  , 'tools/idtools.cpp'
  , 'annex/annshapeidhelper.cpp'
  , 'modelviz/mdvbase.cpp'
  , 'modelviz/mdvhiddenlineeffect.cpp'
  , 'modelviz/mdvhiddenlinetechnique.cpp'
  , 'modelviz/mdvshapegeometry.cpp']

lodgenerator_exe = executable('lodgenerator', lodgenerator_sources
  , dependencies : [boost, osg, occt]
  , cpp_args : [defines, extra_args]
  , install : true)

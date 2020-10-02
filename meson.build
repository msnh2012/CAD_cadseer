#3 is the highest warning level
project('cadseer_project', 'cpp',
  version : '0.1.0',
  license : 'GPL-3+',
  meson_version : '>=0.52.0',
  default_options : ['warning_level=3', 'cpp_std=c++17']
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
qt5 = dependency('Qt5', version : '>=5.11.3', modules : ['Core', 'Widgets', 'OpenGL', 'Svg'], include_type : 'system')

boost = dependency('boost', modules : ['system', 'graph', 'timer', 'filesystem'])

occt = dependency('OpenCASCADE', method : 'cmake', version : '>=7.4'
  , modules : [
    'TKernel'
    , 'TKMath'
    , 'TKG2d'
    , 'TKG3d'
    , 'TKGeomBase'
    , 'TKBRep'
    , 'TKGeomAlgo'
    , 'TKTopAlgo'
    , 'TKPrim'
    , 'TKBO'
    , 'TKShHealing'
    , 'TKBool'
    , 'TKHLR'
    , 'TKFillet'
    , 'TKOffset'
    , 'TKFeat'
    , 'TKMesh'
    , 'TKXMesh'
    , 'TKSTEP'
    , 'TKIGES'
  ]
  , include_type : 'system'
)

osg = dependency('openscenegraph', version : '>=3.6', modules : ['OpenThreads', 'osg', 'osgDB', 'osgUtil'
, 'osgViewer', 'osgGA', 'osgText', 'osgManipulator', 'osgFX'], include_type : 'system')
  
eigen = dependency('eigen3', version : '>=3.3.7', include_type : 'system')

xerces = dependency('xerces-c', version : '>=3.2.2', include_type : 'system')

#I had comment out call to 'CGAL_detect_if_Boost_Thread_is_required()' in /usr/lib/x86_64-linux-gnu/cmake/CGAL/CGAL_SetupBoost.cmake:46
cgal = dependency('CGAL', version : '>=4.13', include_type : 'system')

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

pmp_proj = subproject('pmp-library')
pmp = pmp_proj.get_variable('libpmp_dep')

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
  , 'feature/ftrdatumsystem.cpp'
  , 'feature/ftrsurfaceremesh.cpp'
  , 'feature/ftrsurfacemeshfill.cpp'
  , 'feature/ftrmappcurve.cpp'
  , 'feature/ftruntrim.cpp'
  , 'feature/ftrface.cpp'
  , 'feature/ftrfill.cpp'
  ]
  
parameter_sources = ['parameter/prmparameter.cpp', 'parameter/prmexpressionlink.cpp']

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
  , 'tools/tlsosgtools.cpp'
  , 'tools/tlsshapeid.cpp'
  ]
  
dialog_sources = ['dialogs/dlgparameter.cpp'
  , 'dialogs/dlgwidgetgeometry.cpp'
  , 'dialogs/dlgexpressionedit.cpp'
  , 'dialogs/dlgselectionbutton.cpp'
  , 'dialogs/dlgpreferences.cpp'
  , 'dialogs/dlgproject.cpp'
  , 'dialogs/dlgcommitwidget.cpp'
  , 'dialogs/dlgtagwidget.cpp'
  , 'dialogs/dlgabout.cpp'
  , 'dialogs/dlgsplitterdecorated.cpp'
  , 'dialogs/dlgenterfilter.cpp'
  , 'dialogs/dlgselectionlist.cpp'
  , 'dialogs/dlgselectionwidget.cpp'
  , 'dialogs/dlgbase.cpp'
  , 'dialogs/dlgnetgenparameters.cpp'
  , 'dialogs/dlgocctparameters.cpp']
  
preference_sources = ['preferences/preferencesXML.cpp', 'preferences/prfmanager.cpp']

project_sources = ['project/prjproject.cpp'
  , 'project/prjstow.cpp'
  , 'project/prjmessage.cpp'
  , 'project/prjgitmanager.cpp'
  , 'project/prjfeatureload.cpp']

project_serial_sources = [
  'project/serial/generated/prjsrlsptcolor.cpp'
  , 'project/serial/generated/prjsrlsptvectormath.cpp'
  , 'project/serial/generated/prjsrlsptparameter.cpp'
  , 'project/serial/generated/prjsrlsptshapehistory.cpp'
  , 'project/serial/generated/prjsrlsptpick.cpp'
  , 'project/serial/generated/prjsrlsptintersectionmapping.cpp'
  , 'project/serial/generated/prjsrlsptinstancemapping.cpp'
  , 'project/serial/generated/prjsrlsptseershape.cpp'
  , 'project/serial/generated/prjsrlsptoverlay.cpp'
  , 'project/serial/generated/prjsrlsptbase.cpp'
  , 'project/serial/generated/prjsrlbxsbox.cpp'
  , 'project/serial/generated/prjsrlblnsblend.cpp'
  , 'project/serial/generated/prjsrlchmschamfer.cpp'
  , 'project/serial/generated/prjsrlcnscone.cpp'
  , 'project/serial/generated/prjsrlcylscylinder.cpp'
  , 'project/serial/generated/prjsrldtasdatumaxis.cpp'
  , 'project/serial/generated/prjsrldtpsdatumplane.cpp'
  , 'project/serial/generated/prjsrldtmsdatumsystem.cpp'
  , 'project/serial/generated/prjsrldstsdieset.cpp'
  , 'project/serial/generated/prjsrldrfsdraft.cpp'
  , 'project/serial/generated/prjsrlextsextract.cpp'
  , 'project/serial/generated/prjsrlexrsextrude.cpp'
  , 'project/serial/generated/prjsrlhllshollow.cpp'
  , 'project/serial/generated/prjsrlimpsimageplane.cpp'
  , 'project/serial/generated/prjsrlintsinert.cpp'
  , 'project/serial/generated/prjsrlinlsinstancelinear.cpp'
  , 'project/serial/generated/prjsrlinmsinstancemirror.cpp'
  , 'project/serial/generated/prjsrlinpsinstancepolar.cpp'
  , 'project/serial/generated/prjsrlinssintersect.cpp'
  , 'project/serial/generated/prjsrllnsline.cpp'
  , 'project/serial/generated/prjsrlnstsnest.cpp'
  , 'project/serial/generated/prjsrloblsoblong.cpp'
  , 'project/serial/generated/prjsrloffsoffset.cpp'
  , 'project/serial/generated/prjsrlqtsquote.cpp'
  , 'project/serial/generated/prjsrlrfnsrefine.cpp'
  , 'project/serial/generated/prjsrlrmfsremovefaces.cpp'
  , 'project/serial/generated/prjsrlrvlsrevolve.cpp'
  , 'project/serial/generated/prjsrlrldsruled.cpp'
  , 'project/serial/generated/prjsrlswssew.cpp'
  , 'project/serial/generated/prjsrlsktssketch.cpp'
  , 'project/serial/generated/prjsrlsprssphere.cpp'
  , 'project/serial/generated/prjsrlsqsssquash.cpp'
  , 'project/serial/generated/prjsrlstpsstrip.cpp'
  , 'project/serial/generated/prjsrlsbtssubtract.cpp'
  , 'project/serial/generated/prjsrlmshsmesh.cpp'
  , 'project/serial/generated/prjsrlsfmssurfacemesh.cpp'
  , 'project/serial/generated/prjsrlsmfssurfacemeshfill.cpp'
  , 'project/serial/generated/prjsrlsrmssurfaceremesh.cpp'
  , 'project/serial/generated/prjsrllwfslawfunction.cpp'
  , 'project/serial/generated/prjsrlswpssweep.cpp'
  , 'project/serial/generated/prjsrlthksthicken.cpp'
  , 'project/serial/generated/prjsrlthdsthread.cpp'
  , 'project/serial/generated/prjsrltrsstorus.cpp'
  , 'project/serial/generated/prjsrltscstransitioncurve.cpp'
  , 'project/serial/generated/prjsrltrmstrim.cpp'
  , 'project/serial/generated/prjsrlunnsunion.cpp'
  , 'project/serial/generated/prjsrlprjsproject.cpp'
  , 'project/serial/generated/prjsrlvwsview.cpp'
  , 'project/serial/generated/prjsrlmpcmappcurve.cpp'
  , 'project/serial/generated/prjsrlutruntrim.cpp'
  , 'project/serial/generated/prjsrlfceface.cpp'
  , 'project/serial/generated/prjsrlflsfill.cpp'
]
  
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
  , 'command/cmdsystemtoselection.cpp'
  , 'command/cmdoffset.cpp'
  , 'command/cmdthicken.cpp'
  , 'command/cmdsew.cpp'
  , 'command/cmdtrim.cpp'
  , 'command/cmdrevision.cpp'
  , 'command/cmdremovefaces.cpp'
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
  , 'command/cmddatumsystem.cpp'
  , 'command/cmdsurfaceremesh.cpp'
  , 'command/cmdsurfacemeshfill.cpp'
  , 'command/cmdleafmanager.cpp'
  , 'command/cmdprimitive.cpp'
  , 'command/cmdmappcurve.cpp'
  , 'command/cmduntrim.cpp'
  , 'command/cmdface.cpp'
  , 'command/cmdfill.cpp'
]

commandview_sources = [
  'commandview/cmvpane.cpp'
  , 'commandview/cmvbase.cpp'
  , 'commandview/cmvmessage.cpp'
  , 'commandview/cmvmeasurelinear.cpp'
  , 'commandview/cmvdatumaxis.cpp'
  , 'commandview/cmvtrafficsignal.cpp'
  , 'commandview/cmvparameterwidgets.cpp'
  , 'commandview/cmvdatumplane.cpp'
  , 'commandview/cmvextrude.cpp'
  , 'commandview/cmvcsyswidget.cpp'
  , 'commandview/cmvrevolve.cpp'
  , 'commandview/cmvtrim.cpp'
  , 'commandview/cmvoffset.cpp'
  , 'commandview/cmvremovefaces.cpp'
  , 'commandview/cmvruled.cpp'
  , 'commandview/cmvsew.cpp'
  , 'commandview/cmvthicken.cpp'
  , 'commandview/cmvtransitioncurve.cpp'
  , 'commandview/cmvprimitive.cpp'
  , 'commandview/cmvsketch.cpp'
  , 'commandview/cmvsweep.cpp'
  , 'commandview/cmvlawfunctionwidget.cpp'
  , 'commandview/cmvmappcurve.cpp'
  , 'commandview/cmvsurfacemesh.cpp'
  , 'commandview/cmvboolean.cpp'
  , 'commandview/cmvstrip.cpp'
  , 'commandview/cmvrevision.cpp'
  , 'commandview/cmvremove.cpp'
  , 'commandview/cmvquote.cpp'
  , 'commandview/cmvhollow.cpp'
  , 'commandview/cmvextract.cpp'
  , 'commandview/cmvdraft.cpp'
  , 'commandview/cmvdatumsystem.cpp'
  , 'commandview/cmvcheckgeometry.cpp'
  , 'commandview/cmvchamfer.cpp'
  , 'commandview/cmvblend.cpp'
  , 'commandview/cmvuntrim.cpp'
  , 'commandview/cmvface.cpp'
  , 'commandview/cmvfill.cpp'
]
  
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

mesh_sources = ['mesh/mshparameters.cpp'
  , 'mesh/mshconvert.cpp'
  , 'mesh/mshocct.cpp'
  , 'mesh/mshfillholescgal.cpp'
  , 'mesh/mshfillholespmp.cpp'
]

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
  , project_serial_sources
  , application_sources
  , law_sources
  , library_sources
  , command_sources
  , commandview_sources
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
  , 'dialogs/dlgwidgetgeometry.h'
  , 'dialogs/dlgexpressionedit.h'
  , 'dialogs/dlgpreferences.h'
  , 'dialogs/dlgparameter.h'
  , 'dialogs/dlgsplitterdecorated.h'
  , 'expressions/exprwidget.h'
  , 'expressions/exprtablemodel.h'
  , 'expressions/exprtableview.h'
  , 'dialogs/dlgselectionbutton.h'
  , 'dialogs/dlgproject.h'
  , 'dialogs/dlgcommitwidget.h'
  , 'lod/lodmanager.h'
  , 'dialogs/dlgabout.h'
  , 'dialogs/dlgenterfilter.h'
  , 'dialogs/dlgselectionlist.h'
  , 'dialogs/dlgselectionwidget.h'
  , 'dialogs/dlgbase.h'
  , 'dialogs/dlgvizfilter.h'
  , 'dialogs/dlgnetgenparameters.h'
  , 'dialogs/dlgocctparameters.h'
  , 'commandview/cmvpane.h'
  , 'commandview/cmvbase.h'
  , 'commandview/cmvmeasurelinear.h'
  , 'commandview/cmvdatumaxis.h'
  , 'commandview/cmvtrafficsignal.h'
  , 'commandview/cmvparameterwidgets.h'
  , 'commandview/cmvdatumplane.h'
  , 'commandview/cmvextrude.h'
  , 'commandview/cmvcsyswidget.h'
  , 'commandview/cmvrevolve.h'
  , 'commandview/cmvtrim.h'
  , 'commandview/cmvoffset.h'
  , 'commandview/cmvremovefaces.h'
  , 'commandview/cmvruled.h'
  , 'commandview/cmvsew.h'
  , 'commandview/cmvthicken.h'
  , 'commandview/cmvtransitioncurve.h'
  , 'commandview/cmvprimitive.h'
  , 'commandview/cmvsketch.h'
  , 'commandview/cmvsweep.h'
  , 'commandview/cmvlawfunctionwidget.h'
  , 'commandview/cmvmappcurve.h'
  , 'commandview/cmvsurfacemesh.h'
  , 'commandview/cmvboolean.h'
  , 'commandview/cmvstrip.h'
  , 'commandview/cmvrevision.h'
  , 'commandview/cmvremove.h'
  , 'commandview/cmvquote.h'
  , 'commandview/cmvhollow.h'
  , 'commandview/cmvextract.h'
  , 'commandview/cmvdraft.h'
  , 'commandview/cmvdatumsystem.h'
  , 'commandview/cmvcheckgeometry.h'
  , 'commandview/cmvchamfer.h'
  , 'commandview/cmvblend.h'
  , 'commandview/cmvuntrim.h'
  , 'commandview/cmvface.h'
  , 'commandview/cmvfill.h'
]
  
cadseer_resources = ['resources.qrc']

qt5_processed = qt5process.preprocess(
  moc_headers : cadseer_moc_hdrs
  , ui_files: cadseer_uis
  , qresources : cadseer_resources
)

cadseer_exe = executable('cadseer', [cadseer_sources, qt5_processed]
  , dependencies : [qt5, boost, occt, osg, osgqt, eigen, xerces, cgal, threads, libzippp, spnav, gmsh, netgen, git2pp, solvespace, libigl, pmp]
  , include_directories : include_directories(occt.get_variable(cmake : 'OpenCASCADE_INCLUDE_DIR'))
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
  , include_directories : include_directories(occt.get_variable(cmake : 'OpenCASCADE_INCLUDE_DIR'))
  , cpp_args : [defines, extra_args]
  , install : true)

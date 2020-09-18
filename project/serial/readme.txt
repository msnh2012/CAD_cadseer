xml, xsd validation website.
http://www.utilities-online.info/xsdvalidation/#.Vf2u9ZOVvts

getting started with xsdcxx
http://www.codesynthesis.com/projects/xsd/documentation/cxx/tree/guide/

command line switches for xsdcxx
http://www.codesynthesis.com/projects/xsd/documentation/xsd.xhtml

general note: I had better luck running commands from output directory and 'relative pathing' to input files.
opposed to trying to use the --output-dir option. This makes the includes for both input and output work.

generate parsing files.
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --output-dir xsdcxx projectXML.xsd

//this is in the context of the preferences schema
ok this is screwy. the parser, by default, validates the xml so it
needs the xsd at runtime. We have the xsd and a default xml in
the qt controlled resources. setup() creates these files in the temp
directory if they don't already exist.


//first one should be static so only run it if the file is missing.
cd */cadseer/source
xsdcxx cxx-tree --std c++11 --generate-serialization --generate-doxygen --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --generate-xml-schema xmlbase.xsd

/*
refactor of serial:
context: breaking up featurebase.xsd. It had too many special types for a general include.
  I defined the vectormath types in a separate file with 'noNamespace' and included it where needed.
  This worked, but we have to be careful because, the generated cpp parsing code for vectormath
  was duplicated for any xsd that included vectormath.xsd. Also, I am including to separate
  files with the same namespace, so I am getting "error: redefinition of ‘class *". It is cool
  how the xml 'noNamespace' works with include, but I don't think it is very useful for cadseer.
  Gave up on the 'noNamespace'.
*/

//these will need to be ran when the respective xsd file has been changed.
cd */cadseer/source/project/serial/generated
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_SPT --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlsptcolor.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_SPT --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlsptvectormath.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_SPT --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlsptparameter.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_SPT --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlsptshapehistory.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_SPT --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlsptpick.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_SPT --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlsptintersectionmapping.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_SPT --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlsptinstancemapping.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_SPT --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlsptseershape.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_SPT --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlsptoverlay.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_SPT --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlsptbase.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_BXS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlbxsbox.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_BLNS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlblnsblend.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_CHMS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlchmschamfer.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_CNS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlcnscone.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_CYLS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlcylscylinder.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_DTAS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrldtasdatumaxis.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_DTPS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrldtpsdatumplane.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_DTMS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrldtmsdatumsystem.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_DSTS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrldstsdieset.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_DRFS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrldrfsdraft.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_EXTS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlextsextract.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_EXRS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlexrsextrude.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_HLLS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlhllshollow.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_IMPS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlimpsimageplane.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_INTS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlintsinert.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_INLS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlinlsinstancelinear.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_INMS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlinmsinstancemirror.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_INPS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlinpsinstancepolar.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_INSS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlinssintersect.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_LNS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrllnsline.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_NSTS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlnstsnest.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_OBLS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrloblsoblong.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_OFFS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrloffsoffset.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_QTS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlqtsquote.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_RFNS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlrfnsrefine.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_RMFS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlrmfsremovefaces.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_RVLS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlrvlsrevolve.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_RLDS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlrldsruled.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_SWS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlswssew.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_SKTS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlsktssketch.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_SPRS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlsprssphere.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_SQSS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlsqsssquash.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_STPS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlstpsstrip.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_SBTS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlsbtssubtract.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_MSHS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlmshsmesh.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_SFMS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlsfmssurfacemesh.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_SMFS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlsmfssurfacemeshfill.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_SRMS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlsrmssurfaceremesh.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_LWFS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrllwfslawfunction.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_SWPS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlswpssweep.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_THKS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlthksthicken.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_THDS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlthdsthread.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_TRSS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrltrsstorus.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_TSCS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrltscstransitioncurve.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_TRMS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrltrmstrim.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_UNNS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlunnsunion.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_PRJS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlprjsproject.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_VWS --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlvwsview.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_MPC --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlmpcmappcurve.xsd
xsdcxx cxx-tree --std c++11 --generate-serialization --type-naming ucc --hxx-suffix .h --cxx-suffix .cpp --guard-prefix PRJ_SRL_UTR --extern-xml-schema ../../../xmlbase.xsd ../schema/prjsrlutruntrim.xsd

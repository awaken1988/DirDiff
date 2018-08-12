QT += gui widgets

SOURCES =   src/main.cpp \     
            src/fsdiff.cpp \    
           	src/treemodel.cpp \
           	src/sortfilterproxy.cpp \
           	src/maingui.cpp \
           	src/detailgui.cpp \
           	src/opengui.cpp \
           	src/duplicatemodel.cpp \
           	src/filter.cpp \
           	src/logger.cpp \
			src/findapp.cpp

HEADERS = 	src/treemodel.h \
			src/sortfilterproxy.h \
			src/maingui.h \
			src/detailgui.h \
			src/opengui.h \
			src/duplicatemodel.h \
			src/filter.h \
			src/logger.h \
			src/findapp.h

CONFIG += debug

unix:LIBS += -lboost_system -lboost_filesystem 

QMAKE_CXXFLAGS +=	-std=c++1z \
					-Wno-unused-variable \
					-Wno-unused-parameter 

DESTDIR = build
OBJECTS_DIR = build
MOC_DIR = build
RCC_DIR = build
UI_DIR = build
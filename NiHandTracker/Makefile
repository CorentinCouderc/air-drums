include ../Build/Common/CommonDefs.mak

BIN_DIR = ../Bin

INC_DIRS = \
	../../Include =/usr/include/ni \
    ./Libs/fmodstudioapi11010linux/api/lowlevel/inc \
	./

SRC_FILES = \
	./*.cpp \
	./lib/*.cpp \
	./lib/format/*.cpp \
	./lib/sound/*.cpp 

USED_LIBS += OpenNI pthread fmod glut GL GLEW GLU dl

EXE_NAME = air-drums

include ../Build/Common/CommonCppMakefile

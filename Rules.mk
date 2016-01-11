#############################################################################
#                                                                           #
#                        Makefile 的配置文件                                # 
#                                                                           #
#############################################################################

CC=gcc
MAKE=make

#主文件夹 和 各个服务的文件夹
WTDIR=$(CURDIR)
PUBDIR=$(WTDIR)/pub
GUIDEDIR=$(WTDIR)/guide
AUTHENTICATEDIR=$(WTDIR)/authenticate
MANAGEMENTDIR=$(WTDIR)/management
ACACDIR=$(WTDIR)/acac
PROBEDIR=$(WTDIR)/probe
STMPDIR=$(WTDIR)/stmp
BOSSDIR=$(WTDIR)/boss
GACDIR=$(WTDIR)/gac
SGUIDEDIR=$(WTDIR)/sguide
SAUTHENTICATEDIR=$(WTDIR)/sauthenticate
TESTDIR=$(WTDIR)/test

#编译选项
INCLUDE=-I$(PUBDIR)
PTHREADLIB=-lpthread -lm
ODBCLIB=-lodbc
PUBLIC_DEPEND=$(PUBDIR)/*.o

#引用程序目录
BIN_DIR=/mx

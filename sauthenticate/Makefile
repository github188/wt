include $(WTDIR)/Rules.mk

#认证
SAUTHENTICATE_SOURCE=main.c router.c guide_list.c server_mutual.c
SAUTHENTICATE_DEPEND=main.o router.o guide_list.o server_mutual.o
SAUTHENTICATE_TARGET=sauthenticate.out

all:clean out mv
	@echo "\(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ )"

out:
	@$(CC) -c $(SAUTHENTICATE_SOURCE) $(INCLUDE)
	@$(CC) $(SAUTHENTICATE_DEPEND) $(PUBLIC_DEPEND) -o $(SAUTHENTICATE_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

mv:
	-@mv $(BIN_DIR)/sauthenticate/$(SAUTHENTICATE_TARGET) $(BIN_DIR)/sauthenticate/backup.out 
	@mv $(SAUTHENTICATE_TARGET) $(BIN_DIR)/sauthenticate/
	@cp $(WTDIR)/../boss.ini $(BIN_DIR)/sauthenticate/config.ini

GDB:clean gdb_out gdb_mv
	@echo "\(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ )"

gdb_out:
	@$(CC) -g -c $(SAUTHENTICATE_SOURCE) $(INCLUDE)
	@$(CC) -g $(SAUTHENTICATE_DEPEND) $(PUBLIC_DEPEND) -o $(SAUTHENTICATE_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

gdb_mv:mv

clean:
	-@rm -f *.out *.o

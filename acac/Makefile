include $(WTDIR)/Rules.mk

#集中管理
ACAC_SOURCE=main.c router.c server_mutual.c
ACAC_DEPEND=main.o router.o server_mutual.o
ACAC_TARGET=acac.out

all:clean out mv
	@echo "\(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ )"

out:
	@$(CC) -c $(ACAC_SOURCE) $(INCLUDE)
	@$(CC) $(ACAC_DEPEND) $(PUBLIC_DEPEND) -o $(ACAC_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

mv:
	-@mv $(BIN_DIR)/acac/$(ACAC_TARGET) $(BIN_DIR)/acac/backup.out 
	@mv $(ACAC_TARGET) $(BIN_DIR)/acac/
	@cp $(WTDIR)/../config.ini $(BIN_DIR)/acac/

GDB:clean gdb_out gdb_mv
	@echo "\(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ )"

gdb_out:
	@$(CC) -g -c $(ACAC_SOURCE) $(INCLUDE)
	@$(CC) -g $(ACAC_DEPEND) $(PUBLIC_DEPEND) -o $(ACAC_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

gdb_mv:mv

clean:
	-@rm -f *.out *.o

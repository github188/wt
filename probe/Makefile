include $(WTDIR)/Rules.mk

#探针
PROBE_SOURCE=probe.c
PROBE_DEPEND=probe.o
PROBE_TARGET=probe.out

all:clean out mv
	@echo "\(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ )"

out:
	@$(CC) -c $(PROBE_SOURCE) $(INCLUDE)
	@$(CC) $(PROBE_DEPEND) $(PUBLIC_DEPEND) -o $(PROBE_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

mv:
	-@mv $(BIN_DIR)/probe/$(PROBE_TARGET) $(BIN_DIR)/probe/backup.out 
	@mv $(PROBE_TARGET) $(BIN_DIR)/probe/
	@cp $(WTDIR)/../config.ini $(BIN_DIR)/probe/

GDB:clean gdb_out gdb_mv
	@echo "\(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ )"

gdb_out:
	@$(CC) -g -c $(PROBE_SOURCE) $(INCLUDE)
	@$(CC) -g $(PROBE_DEPEND) $(PUBLIC_DEPEND) -o $(PROBE_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

gdb_mv:mv

clean:
	-@rm -f *.out *.o

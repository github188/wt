include $(WTDIR)/Rules.mk

#认证
AUTHENTICATE_SOURCE=main.c router.c utils.c exec_sql.c order.c packets_io.c server_mutual.c time_task.c
AUTHENTICATE_DEPEND=main.o router.o utils.o exec_sql.o order.o packets_io.o server_mutual.o time_task.o
AUTHENTICATE_TARGET=authenticate.out

all:clean out mv
	@echo "\(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ )"

out:
	@$(CC) -c $(AUTHENTICATE_SOURCE) $(INCLUDE)
	@$(CC) $(AUTHENTICATE_DEPEND) $(PUBLIC_DEPEND) -o $(AUTHENTICATE_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

mv:
	-@mv $(BIN_DIR)/authenticate/$(AUTHENTICATE_TARGET) $(BIN_DIR)/authenticate/backup.out 
	@mv $(AUTHENTICATE_TARGET) $(BIN_DIR)/authenticate/
	@cp $(WTDIR)/../config.ini $(BIN_DIR)/authenticate/

GDB:clean gdb_out gdb_mv
	@echo "\(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ ) \(≧ ▽ ≦ )"

gdb_out:
	@$(CC) -g -c $(AUTHENTICATE_SOURCE) $(INCLUDE)
	@$(CC) -g $(AUTHENTICATE_DEPEND) $(PUBLIC_DEPEND) -o $(AUTHENTICATE_TARGET) $(INCLUDE) $(PTHREADLIB) $(ODBCLIB)
	-@rm -f *.o

gdb_mv:mv

clean:
	-@rm -f *.out *.o

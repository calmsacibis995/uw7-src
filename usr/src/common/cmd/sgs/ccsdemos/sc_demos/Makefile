#ident	"@(#)ccsdemos:sc_demos/Makefile	1.3"

CC=CC

DEMOS=calendar_sc Args_demo Array_set_demo Graph_demo List_demo Map_demo

all: $(DEMOS)

calendar_sc: calendar_sc.C calendar_sc.h appoint.h
	$(CC) -o calendar_sc calendar_sc.C -l++
	@echo Executing calendar_sc
	./calendar_sc -n < appts

Args_demo: Args_demo.C
	$(CC) -o Args_demo Args_demo.C -l++
	@echo Executing Args_demo
	./Args_demo -o blech -O -o -O -DFOO=BAR,BAZ foo.c -dryrun

Array_set_demo: Array_set.C Array_set_demo.C Array_set.h
	$(CC) -o Array_set_demo Array_set.C Array_set_demo.C -l++
	@echo Executing Array_set_demo
	./Array_set_demo

Graph_demo: Graph_demo.C
	$(CC) -w -o Graph_demo Graph_demo.C -l++
	@echo Executing Graph_demo
	./Graph_demo

List_demo: List_demo.C List_demo.h
	$(CC) -o List_demo List_demo.C -l++
	@echo Executing List_demo
	./List_demo

Map_demo: Map_demo.C
	$(CC) -o Map_demo Map_demo.C -l++
	@echo Executing Map_demo
	./Map_demo < Map_demo.C

clean:
	rm -f *.o *.ii

clobber: clean
	rm -f $(DEMOS)

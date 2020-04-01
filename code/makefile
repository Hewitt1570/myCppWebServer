src = ${wildcard ./*.cpp}
obj = ${patsubst %.cpp, %.o, ${src}}
headfile = ${wildcard ./*.h}
target = server

CC = g++ -std=c++11 -g
RM = rm -rf
LIB = -lpthread

${target} : ${obj} 
	${CC} -o $@ $^ ${LIB}

${obj} : %.o : %.cpp ${headfile}
	${CC} -o $@ -c $<

.PHONY: clean cleanall
cleanall:
	${RM} ${target} ${obj}

clean:
	${RM} ${obj}



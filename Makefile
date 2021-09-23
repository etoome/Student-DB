FLAGS=-pthread -std=c11 -Wall -Werror -Wpedantic

student-db: src/main.c src/utils/parsing.o src/student/student.o src/db/db.o
	gcc $^ ${FLAGS} -o bin/$@

src/%/%.o: src/%/%.c
	gcc -c $< ${FLAGS}

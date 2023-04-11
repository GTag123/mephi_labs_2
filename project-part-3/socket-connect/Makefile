all: compile run

compile:
	cmake -S . -B cmake-build
	cd cmake-build && make

run: compile
	./cmake-build/socket-connect

clean:
	rm -rf cmake-build